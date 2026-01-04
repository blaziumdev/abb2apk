#pragma once

#include <string>
#include <filesystem>
#include <optional>

namespace aab2apk {

namespace fs = std::filesystem;

class FileUtils {
public:
    static bool file_exists(const fs::path& path);
    static bool is_regular_file(const fs::path& path);
    static bool is_directory(const fs::path& path);
    static bool create_directories(const fs::path& path);
    static std::optional<fs::path> find_bundletool();
    static std::optional<fs::path> find_java_executable();
    static std::string get_absolute_path(const fs::path& path);
    static bool validate_aab_file(const fs::path& aab_path);
    static bool validate_keystore_file(const fs::path& keystore_path);
    static std::string get_temp_directory();
    static fs::path create_temp_directory();
    static void remove_temp_directory(const fs::path& path);
};

} // namespace aab2apk

