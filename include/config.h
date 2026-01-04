#pragma once

#include <string>
#include <optional>
#include <vector>

namespace aab2apk {

enum class OutputMode {
    Universal,
    Split
};

struct SigningConfig {
    std::string keystore_path;
    std::string keystore_password;
    std::string key_alias;
    std::string key_password;
};

struct Config {
    std::string input_aab;
    std::string output_dir;
    OutputMode mode = OutputMode::Universal;
    std::optional<SigningConfig> signing;
    bool verbose = false;
    bool quiet = false;
    bool list_tools = false;
    bool show_timing = false;
    bool check_only = false;
    bool json_output = false;
    std::string bundletool_path;
    std::string java_path;
};

class ConfigParser {
public:
    static Config parse(int argc, char* argv[]);
    static void print_usage(const char* program_name);
    static void print_version();

private:
    static std::string resolve_env_var(const std::string& value);
    static bool validate_config(const Config& config);
};

} // namespace aab2apk

