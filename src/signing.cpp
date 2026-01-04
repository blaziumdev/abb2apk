#include "signing.h"
#include "file_utils.h"
#include "process_runner.h"
#include <filesystem>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace aab2apk {

std::string SigningManager::find_apksigner() const {
    // apksigner is part of Android SDK Build Tools
    // Check common locations

#ifdef _WIN32
    std::string exe_name = "apksigner.bat";
#else
    std::string exe_name = "apksigner";
#endif

    // Check ANDROID_HOME
    const char* android_home = std::getenv("ANDROID_HOME");
    if (android_home) {
        std::filesystem::path build_tools_base = std::filesystem::path(android_home) / "build-tools";
        if (std::filesystem::exists(build_tools_base)) {
            // Find latest build-tools version
            std::filesystem::path latest_version;
            for (const auto& entry : std::filesystem::directory_iterator(build_tools_base)) {
                if (entry.is_directory()) {
                    std::filesystem::path apksigner_path = entry.path() / "lib" / exe_name;
                    if (std::filesystem::exists(apksigner_path)) {
                        if (latest_version.empty() || entry.path() > latest_version) {
                            latest_version = entry.path();
                        }
                    }
                }
            }
            if (!latest_version.empty()) {
                return (latest_version / "lib" / exe_name).string();
            }
        }
    }

    // Check PATH
    const char* path_env = std::getenv("PATH");
    if (path_env) {
        std::string path_str(path_env);
        std::string delimiter;

#ifdef _WIN32
        delimiter = ";";
#else
        delimiter = ":";
#endif

        size_t pos = 0;
        while ((pos = path_str.find(delimiter)) != std::string::npos) {
            std::string dir = path_str.substr(0, pos);
            std::filesystem::path apksigner_path = std::filesystem::path(dir) / exe_name;
            if (FileUtils::file_exists(apksigner_path)) {
                return apksigner_path.string();
            }
            path_str.erase(0, pos + delimiter.length());
        }
        if (!path_str.empty()) {
            std::filesystem::path apksigner_path = std::filesystem::path(path_str) / exe_name;
            if (FileUtils::file_exists(apksigner_path)) {
                return apksigner_path.string();
            }
        }
    }

    return "";
}

bool SigningManager::validate_signing_result(const ProcessResult& result) const {
    if (!result.success()) {
        return false;
    }

    // Check for common error patterns in output
    std::string lower_stderr = result.stderr_output;
    std::transform(lower_stderr.begin(), lower_stderr.end(), lower_stderr.begin(), ::tolower);

    // Success indicators
    if (lower_stderr.find("signed") != std::string::npos ||
        lower_stderr.find("verified") != std::string::npos) {
        return true;
    }

    // Error indicators
    if (lower_stderr.find("error") != std::string::npos ||
        lower_stderr.find("failed") != std::string::npos ||
        lower_stderr.find("exception") != std::string::npos) {
        return false;
    }

    // If exit code is 0 and no obvious errors, assume success
    return result.success();
}

bool SigningManager::sign_apk(
    const std::filesystem::path& apk_path,
    const SigningConfig& config
) const {
    std::string apksigner = find_apksigner();
    if (apksigner.empty()) {
        std::cerr << "Error: apksigner not found. Please install Android SDK Build Tools or add it to PATH\n";
        return false;
    }

    if (!FileUtils::file_exists(apk_path)) {
        std::cerr << "Error: APK file does not exist: " << apk_path << "\n";
        return false;
    }

    std::vector<std::string> args;
    args.push_back("sign");
    args.push_back("--ks");
    args.push_back(config.keystore_path);
    args.push_back("--ks-pass");
    args.push_back("pass:" + config.keystore_password);
    args.push_back("--key-pass");
    args.push_back("pass:" + config.key_password);
    args.push_back("--ks-key-alias");
    args.push_back(config.key_alias);
    args.push_back(apk_path.string());

    ProcessResult result = runner_.run(apksigner, args);

    if (!validate_signing_result(result)) {
        std::cerr << "Error: APK signing failed\n";
        if (!result.stderr_output.empty()) {
            std::cerr << result.stderr_output << "\n";
        }
        return false;
    }

    return true;
}

bool SigningManager::sign_apks(
    const std::filesystem::path& apks_path,
    const SigningConfig& config
) const {
    // For .apks files, we need to extract, sign each APK, then repackage
    // This is more complex - for now, we'll sign the universal APK if it exists
    // or sign all APKs in the directory

    if (apks_path.extension() == ".apks") {
        // .apks is a ZIP file containing multiple APKs
        // We need to extract, sign each, then repackage
        // For simplicity, we'll focus on signing individual APKs
        std::cerr << "Warning: Signing .apks files requires extraction. Please sign individual APKs.\n";
        return false;
    }

    // If it's a directory, sign all APKs in it
    if (std::filesystem::is_directory(apks_path)) {
        bool all_signed = true;
        for (const auto& entry : std::filesystem::directory_iterator(apks_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".apk") {
                if (!sign_apk(entry.path(), config)) {
                    all_signed = false;
                }
            }
        }
        return all_signed;
    }

    return sign_apk(apks_path, config);
}

} // namespace aab2apk

