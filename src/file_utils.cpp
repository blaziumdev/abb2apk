#include "file_utils.h"
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

namespace aab2apk {

namespace fs = std::filesystem;

bool FileUtils::file_exists(const fs::path& path) {
    return fs::exists(path);
}

bool FileUtils::is_regular_file(const fs::path& path) {
    return fs::is_regular_file(path);
}

bool FileUtils::is_directory(const fs::path& path) {
    return fs::is_directory(path);
}

bool FileUtils::create_directories(const fs::path& path) {
    try {
        // If directory already exists, return true
        if (fs::exists(path) && fs::is_directory(path)) {
            return true;
        }
        // Otherwise, create it (returns true if created, false if already existed)
        return fs::create_directories(path);
    } catch (const std::exception&) {
        return false;
    }
}

std::optional<fs::path> FileUtils::find_bundletool() {
    // Check current directory
    fs::path current_dir = fs::current_path();
    fs::path local_bundletool = current_dir / "bundletool.jar";
    if (file_exists(local_bundletool)) {
        return local_bundletool;
    }

    // Check bundletool directory
    fs::path bundletool_dir = current_dir / "bundletool" / "bundletool.jar";
    if (file_exists(bundletool_dir)) {
        return bundletool_dir;
    }

    // Check common installation locations
    std::vector<fs::path> search_paths;

#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    const char* localappdata = std::getenv("LOCALAPPDATA");
    if (localappdata) {
        search_paths.push_back(fs::path(localappdata) / "bundletool" / "bundletool.jar");
    }
    if (appdata) {
        search_paths.push_back(fs::path(appdata) / "bundletool" / "bundletool.jar");
    }
#else
    const char* home = std::getenv("HOME");
    if (home) {
        search_paths.push_back(fs::path(home) / ".local" / "bin" / "bundletool.jar");
        search_paths.push_back(fs::path(home) / ".bundletool" / "bundletool.jar");
    }
    search_paths.push_back(fs::path("/usr/local/bin/bundletool.jar"));
    search_paths.push_back(fs::path("/opt/bundletool/bundletool.jar"));
#endif

    for (const auto& path : search_paths) {
        if (file_exists(path)) {
            return path;
        }
    }

    // Check PATH environment variable
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
            fs::path bundletool_path = fs::path(dir) / "bundletool.jar";
            if (file_exists(bundletool_path)) {
                return bundletool_path;
            }
            path_str.erase(0, pos + delimiter.length());
        }
        // Check last directory
        if (!path_str.empty()) {
            fs::path bundletool_path = fs::path(path_str) / "bundletool.jar";
            if (file_exists(bundletool_path)) {
                return bundletool_path;
            }
        }
    }

    return std::nullopt;
}

std::optional<fs::path> FileUtils::find_java_executable() {
    // Check JAVA_HOME
    const char* java_home = std::getenv("JAVA_HOME");
    if (java_home) {
        fs::path java_path = fs::path(java_home) / "bin" / "java";
#ifdef _WIN32
        java_path.replace_extension(".exe");
#endif
        if (file_exists(java_path)) {
            return java_path;
        }
    }

    // Check PATH
    const char* path_env = std::getenv("PATH");
    if (path_env) {
        std::string path_str(path_env);
        std::string delimiter;

#ifdef _WIN32
        delimiter = ";";
        std::string exe_name = "java.exe";
#else
        delimiter = ":";
        std::string exe_name = "java";
#endif

        size_t pos = 0;
        while ((pos = path_str.find(delimiter)) != std::string::npos) {
            std::string dir = path_str.substr(0, pos);
            fs::path java_path = fs::path(dir) / exe_name;
            if (file_exists(java_path)) {
                return java_path;
            }
            path_str.erase(0, pos + delimiter.length());
        }
        // Check last directory
        if (!path_str.empty()) {
            fs::path java_path = fs::path(path_str) / exe_name;
            if (file_exists(java_path)) {
                return java_path;
            }
        }
    }

    return std::nullopt;
}

std::string FileUtils::get_absolute_path(const fs::path& path) {
    try {
        return fs::absolute(path).string();
    } catch (const std::exception&) {
        return path.string();
    }
}

bool FileUtils::validate_aab_file(const fs::path& aab_path) {
    if (!is_regular_file(aab_path)) {
        return false;
    }

    // Check extension
    if (aab_path.extension() != ".aab") {
        return false;
    }

    // Check if file is readable and has minimum size (AAB files are ZIP archives)
    std::ifstream file(aab_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Check ZIP signature (PK header)
    char header[2];
    file.read(header, 2);
    if (file.gcount() != 2 || header[0] != 'P' || header[1] != 'K') {
        return false;
    }

    return true;
}

bool FileUtils::validate_keystore_file(const fs::path& keystore_path) {
    if (!is_regular_file(keystore_path)) {
        return false;
    }

    // Check if file is readable
    std::ifstream file(keystore_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Basic validation - keystore files should have some content
    file.seekg(0, std::ios::end);
    std::streampos size = file.tellg();
    return size > 0;
}

std::string FileUtils::get_temp_directory() {
#ifdef _WIN32
    char temp_path[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_path) != 0) {
        return std::string(temp_path);
    }
    return std::string(std::getenv("TEMP") ? std::getenv("TEMP") : "C:\\Temp");
#else
    const char* tmpdir = std::getenv("TMPDIR");
    if (tmpdir) {
        return std::string(tmpdir);
    }
    tmpdir = std::getenv("TMP");
    if (tmpdir) {
        return std::string(tmpdir);
    }
    return "/tmp";
#endif
}

fs::path FileUtils::create_temp_directory() {
    std::string base_temp = get_temp_directory();
    fs::path temp_base(base_temp);
    
    // Create unique directory name
    std::string dir_name = "aab2apk_";
#ifdef _WIN32
    dir_name += std::to_string(GetCurrentProcessId());
#else
    dir_name += std::to_string(getpid());
#endif
    dir_name += "_" + std::to_string(std::time(nullptr));

    fs::path temp_dir = temp_base / dir_name;
    
    if (create_directories(temp_dir)) {
        return temp_dir;
    }

    throw std::runtime_error("Failed to create temporary directory");
}

void FileUtils::remove_temp_directory(const fs::path& path) {
    try {
        if (fs::exists(path)) {
            fs::remove_all(path);
        }
    } catch (const std::exception&) {
        // Ignore errors during cleanup
    }
}

} // namespace aab2apk

