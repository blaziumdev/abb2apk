#include "config.h"
#include "file_utils.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <cstring>

namespace aab2apk {

namespace {
    constexpr const char* VERSION = "1.0.0";
    constexpr const char* USAGE_TEMPLATE = R"(
Usage: %s [OPTIONS]

Convert Android App Bundle (.aab) to APK files.

Required:
  -i, --input <path>          Input .aab file path

Optional:
  -o, --output <path>         Output directory (default: ./dist)
  -m, --mode <mode>           Output mode: universal or split (default: universal)
  --keystore <path>           Keystore file path for signing
  --ks-pass <password>        Keystore password (or env:VAR_NAME)
  --key-alias <alias>         Key alias
  --key-pass <password>       Key password (or env:VAR_NAME)
  --bundletool <path>         Path to bundletool.jar (auto-detected if not specified)
  --java <path>               Path to java executable (auto-detected if not specified)
  -v, --verbose               Verbose output
  -q, --quiet                 Quiet mode (errors only)
  -h, --help                  Show this help message
  --version                   Show version information

Examples:
  %s -i app.aab -o ./dist --mode universal
  %s -i app.aab --keystore release.jks --ks-pass env:KS_PASS --key-alias release
)";
}

std::string ConfigParser::resolve_env_var(const std::string& value) {
    if (value.size() > 5 && value.substr(0, 5) == "env:") {
        std::string var_name = value.substr(5);
        const char* env_value = std::getenv(var_name.c_str());
        if (env_value == nullptr) {
            throw std::runtime_error("Environment variable '" + var_name + "' not set");
        }
        return std::string(env_value);
    }
    return value;
}

bool ConfigParser::validate_config(const Config& config) {
    if (config.input_aab.empty()) {
        std::cerr << "Error: Input AAB file is required\n";
        return false;
    }

    if (!FileUtils::file_exists(config.input_aab)) {
        std::cerr << "Error: Input AAB file does not exist: " << config.input_aab << "\n";
        return false;
    }

    if (!FileUtils::validate_aab_file(config.input_aab)) {
        std::cerr << "Error: Invalid AAB file: " << config.input_aab << "\n";
        return false;
    }

    if (config.signing.has_value()) {
        const auto& sig = config.signing.value();
        if (!FileUtils::file_exists(sig.keystore_path)) {
            std::cerr << "Error: Keystore file does not exist: " << sig.keystore_path << "\n";
            return false;
        }

        if (sig.key_alias.empty()) {
            std::cerr << "Error: Key alias is required when signing\n";
            return false;
        }
    }

    return true;
}

void ConfigParser::print_usage(const char* program_name) {
    std::printf(USAGE_TEMPLATE, program_name, program_name, program_name);
}

void ConfigParser::print_version() {
    std::cout << "aab2apk version " << VERSION << "\n";
}

Config ConfigParser::parse(int argc, char* argv[]) {
    Config config;

    if (argc < 2) {
        print_usage(argv[0]);
        std::exit(1);
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        }

        if (arg == "--version") {
            print_version();
            std::exit(0);
        }

        if (arg == "-i" || arg == "--input") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --input requires a file path\n";
                std::exit(1);
            }
            config.input_aab = argv[++i];
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --output requires a directory path\n";
                std::exit(1);
            }
            config.output_dir = argv[++i];
        }
        else if (arg == "-m" || arg == "--mode") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --mode requires 'universal' or 'split'\n";
                std::exit(1);
            }
            std::string mode = argv[++i];
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
            if (mode == "universal") {
                config.mode = OutputMode::Universal;
            } else if (mode == "split") {
                config.mode = OutputMode::Split;
            } else {
                std::cerr << "Error: Invalid mode. Must be 'universal' or 'split'\n";
                std::exit(1);
            }
        }
        else if (arg == "--keystore") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --keystore requires a file path\n";
                std::exit(1);
            }
            if (!config.signing.has_value()) {
                config.signing = SigningConfig{};
            }
            config.signing->keystore_path = argv[++i];
        }
        else if (arg == "--ks-pass") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --ks-pass requires a password or env:VAR_NAME\n";
                std::exit(1);
            }
            if (!config.signing.has_value()) {
                config.signing = SigningConfig{};
            }
            try {
                config.signing->keystore_password = resolve_env_var(argv[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
                std::exit(1);
            }
        }
        else if (arg == "--key-alias") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --key-alias requires an alias name\n";
                std::exit(1);
            }
            if (!config.signing.has_value()) {
                config.signing = SigningConfig{};
            }
            config.signing->key_alias = argv[++i];
        }
        else if (arg == "--key-pass") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --key-pass requires a password or env:VAR_NAME\n";
                std::exit(1);
            }
            if (!config.signing.has_value()) {
                config.signing = SigningConfig{};
            }
            try {
                config.signing->key_password = resolve_env_var(argv[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
                std::exit(1);
            }
        }
        else if (arg == "--bundletool") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --bundletool requires a file path\n";
                std::exit(1);
            }
            config.bundletool_path = argv[++i];
        }
        else if (arg == "--java") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --java requires an executable path\n";
                std::exit(1);
            }
            config.java_path = argv[++i];
        }
        else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        }
        else if (arg == "-q" || arg == "--quiet") {
            config.quiet = true;
        }
        else {
            std::cerr << "Error: Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            std::exit(1);
        }
    }

    // Set defaults
    if (config.output_dir.empty()) {
        config.output_dir = "./dist";
    }

    // Auto-detect bundletool and java if not provided
    if (config.bundletool_path.empty()) {
        auto bundletool = FileUtils::find_bundletool();
        if (!bundletool.has_value()) {
            std::cerr << "Error: bundletool.jar not found. Please specify --bundletool or place bundletool.jar in current directory or PATH\n";
            std::exit(1);
        }
        config.bundletool_path = bundletool->string();
    }

    if (config.java_path.empty()) {
        auto java = FileUtils::find_java_executable();
        if (!java.has_value()) {
            std::cerr << "Error: Java executable not found. Please install Java or specify --java\n";
            std::exit(1);
        }
        config.java_path = java->string();
    }

    if (!validate_config(config)) {
        std::exit(1);
    }

    return config;
}

} // namespace aab2apk

