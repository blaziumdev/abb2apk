#include "config.h"
#include "aab_converter.h"
#include "process_runner.h"
#include "signing.h"
#include "file_utils.h"
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {

// Simple JSON string escaping
std::string json_escape(const std::string& str) {
    std::ostringstream o;
    for (char c : str) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else if (static_cast<unsigned char>(c) < 0x20) {
            o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
            o << c;
        }
    }
    return o.str();
}

// Output JSON result
void output_json(const std::string& status, const std::string& error_message = "", 
                 double execution_time = -1.0, const std::string& output_dir = "") {
    std::cout << "{\n";
    std::cout << "  \"status\": \"" << status << "\"";
    
    if (!error_message.empty()) {
        std::cout << ",\n  \"error\": \"" << json_escape(error_message) << "\"";
    }
    
    if (execution_time >= 0.0) {
        std::cout << ",\n  \"execution_time\": " << std::fixed << std::setprecision(3) << execution_time;
    }
    
    if (!output_dir.empty()) {
        std::cout << ",\n  \"output_dir\": \"" << json_escape(output_dir) << "\"";
    }
    
    std::cout << "\n}\n";
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    try {
        // Parse configuration
        aab2apk::Config config = aab2apk::ConfigParser::parse(argc, argv);

        // Enable quiet mode when JSON output is requested to suppress human-readable output
        if (config.json_output) {
            config.quiet = true;
        }

        // Handle --list-tools flag
        if (config.list_tools) {
            if (config.json_output) {
                auto java_path = aab2apk::FileUtils::find_java_executable();
                auto bundletool_path = aab2apk::FileUtils::find_bundletool();
                
                std::cout << "{\n";
                std::cout << "  \"status\": \"success\",\n";
                std::cout << "  \"tools\": {\n";
                std::cout << "    \"java\": " << (java_path.has_value() ? "\"" + json_escape(java_path->string()) + "\"" : "null") << ",\n";
                std::cout << "    \"bundletool\": " << (bundletool_path.has_value() ? "\"" + json_escape(bundletool_path->string()) + "\"" : "null") << "\n";
                std::cout << "  }\n";
                std::cout << "}\n";
            } else {
                std::cout << "Detected tools:\n\n";

                // Find and display Java
                auto java_path = aab2apk::FileUtils::find_java_executable();
                if (java_path.has_value()) {
                    std::cout << "Java: " << java_path->string() << "\n";
                } else {
                    std::cout << "Java: Not found\n";
                }

                // Find and display bundletool
                auto bundletool_path = aab2apk::FileUtils::find_bundletool();
                if (bundletool_path.has_value()) {
                    std::cout << "bundletool: " << bundletool_path->string() << "\n";
                } else {
                    std::cout << "bundletool: Not found\n";
                }
            }

            return 0;
        }

        // Handle --check / --validate flag
        if (config.check_only) {
            // Validation has already been performed in ConfigParser::parse()
            // At this point, we know:
            // - Input AAB file exists and is valid
            // - Java and bundletool are available
            // - Signing config (if provided) is valid
            
            if (config.json_output) {
                std::cout << "{\n";
                std::cout << "  \"status\": \"success\",\n";
                std::cout << "  \"validation\": {\n";
                std::cout << "    \"input_aab\": \"" << json_escape(config.input_aab) << "\",\n";
                std::cout << "    \"java\": \"" << json_escape(config.java_path) << "\",\n";
                std::cout << "    \"bundletool\": \"" << json_escape(config.bundletool_path) << "\",\n";
                std::cout << "    \"signing_enabled\": " << (config.signing.has_value() ? "true" : "false");
                if (config.signing.has_value()) {
                    std::cout << ",\n    \"keystore\": \"" << json_escape(config.signing->keystore_path) << "\"";
                }
                std::cout << "\n  }\n";
                std::cout << "}\n";
            } else {
                std::cout << "Validation successful:\n";
                std::cout << "  Input AAB: " << config.input_aab << "\n";
                std::cout << "  Java: " << config.java_path << "\n";
                std::cout << "  bundletool: " << config.bundletool_path << "\n";
                if (config.signing.has_value()) {
                    std::cout << "  Signing: Enabled (keystore: " << config.signing->keystore_path << ")\n";
                } else {
                    std::cout << "  Signing: Disabled\n";
                }
                std::cout << "\nAll checks passed. Ready for conversion.\n";
            }
            return 0;
        }

        // Initialize components
        aab2apk::ProcessRunner runner;
        aab2apk::SigningManager signer(runner);
        aab2apk::AabConverter converter(runner, signer);

        // Record start time for JSON output or timing
        auto start_time = std::chrono::steady_clock::now();

        // Perform conversion
        bool success = converter.convert(config);

        // Calculate elapsed time
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        double seconds = elapsed.count() / 1000.0;

        if (config.json_output) {
            if (success) {
                output_json("success", "", seconds, config.output_dir);
            } else {
                output_json("failure", "Conversion failed", seconds);
            }
        } else {
            // Calculate and display elapsed time if timing is enabled
            if (config.show_timing) {
                std::cout << std::fixed << std::setprecision(3);
                std::cout << "\nConversion completed in " << seconds << " seconds\n";
            }

            if (!success) {
                std::cerr << "Conversion failed\n";
                return 1;
            }
        }

        return success ? 0 : 1;
    }
    catch (const std::exception& e) {
        // Try to get config to check json_output, but if parsing failed, use default
        // Since we're in exception handler, assume no json_output if we can't determine
        // For simplicity, we'll output to stderr as before - JSON output only works if parsing succeeds
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    catch (...) {
        std::cerr << "Fatal error: Unknown exception\n";
        return 1;
    }
}

