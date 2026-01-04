#include "aab_converter.h"
#include "file_utils.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace aab2apk {

namespace fs = std::filesystem;

bool AabConverter::convert(const Config& config) const {
    // Create output directory
    fs::path output_path(config.output_dir);
    if (!FileUtils::create_directories(output_path)) {
        std::cerr << "Error: Failed to create output directory: " << config.output_dir << "\n";
        return false;
    }

    // Create temporary directory for intermediate files
    fs::path temp_dir;
    try {
        temp_dir = FileUtils::create_temp_directory();
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to create temporary directory: " << e.what() << "\n";
        return false;
    }

    // Ensure cleanup
    struct TempDirGuard {
        fs::path path;
        ~TempDirGuard() { FileUtils::remove_temp_directory(path); }
    } guard{temp_dir};

    bool success = false;
    if (config.mode == OutputMode::Universal) {
        success = convert_to_universal(config, temp_dir);
    } else {
        success = convert_to_split(config, temp_dir);
    }

    if (!success) {
        return false;
    }

    // Sign APKs if signing config is provided
    if (config.signing.has_value()) {
        if (config.mode == OutputMode::Universal) {
            fs::path apk_path = output_path / (fs::path(config.input_aab).stem().string() + ".apk");
            if (!signer_.sign_apk(apk_path, config.signing.value())) {
                return false;
            }
        } else {
            // Sign all APKs in output directory
            if (!signer_.sign_apks(output_path, config.signing.value())) {
                return false;
            }
        }
    }

    if (!config.quiet) {
        std::cout << "Successfully converted AAB to APK(s) in: " << config.output_dir << "\n";
    }

    return true;
}

bool AabConverter::convert_to_universal(
    const Config& config,
    const fs::path& temp_dir
) const {
    fs::path bundletool_path(config.bundletool_path);
    if (!FileUtils::file_exists(bundletool_path)) {
        std::cerr << "Error: bundletool.jar not found: " << bundletool_path << "\n";
        return false;
    }

    fs::path output_path(config.output_dir);
    fs::path output_apk = output_path / (fs::path(config.input_aab).stem().string() + ".apk");

    std::vector<std::string> args;
    args.push_back("build-apks");
    args.push_back("--bundle=" + FileUtils::get_absolute_path(config.input_aab));
    args.push_back("--output=" + (temp_dir / "output.apks").string());
    args.push_back("--mode=universal");

    if (!config.quiet) {
        std::cout << "Converting AAB to universal APK...\n";
    }

    ProcessResult result = runner_.run_java(
        config.java_path,
        bundletool_path.string(),
        args,
        temp_dir.string()
    );

    if (!result.success()) {
        std::cerr << "Error: bundletool execution failed\n";
        if (!result.stderr_output.empty()) {
            std::cerr << result.stderr_output << "\n";
        }
        return false;
    }

    // Extract universal APK from .apks file
    fs::path apks_file = temp_dir / "output.apks";
    if (!FileUtils::file_exists(apks_file)) {
        std::cerr << "Error: bundletool did not generate output file\n";
        return false;
    }

    // Extract the universal APK from the .apks ZIP file
    // The .apks file is a ZIP containing the APK(s)
    // For universal mode, it contains a single APK (usually named "universal.apk" or "base.apk")
    
    // Extract .apks ZIP file to get the APK
    // .apks is a ZIP file, so we extract it directly
    fs::path extract_dir = temp_dir / "extracted";
    try {
        fs::create_directories(extract_dir);
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to create extraction directory: " << e.what() << "\n";
        return false;
    }

#ifdef _WIN32
    // On Windows, use PowerShell Expand-Archive
    // PowerShell requires .zip extension, so copy file temporarily
    fs::path zip_copy = temp_dir / "output.zip";
    try {
        fs::copy_file(apks_file, zip_copy, fs::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to copy .apks file: " << e.what() << "\n";
        return false;
    }
    
    std::vector<std::string> extract_args;
    extract_args.push_back("-Command");
    extract_args.push_back("Expand-Archive -Path \"" + zip_copy.string() + "\" -DestinationPath \"" + extract_dir.string() + "\" -Force");
    
    ProcessResult extract_result = runner_.run(
        "powershell.exe",
        extract_args,
        temp_dir.string()
    );
    
    // Clean up temporary zip file
    try {
        fs::remove(zip_copy);
    } catch (...) {
        // Ignore cleanup errors
    }
#else
    // On Unix, use unzip
    std::vector<std::string> extract_args;
    extract_args.push_back("-q");
    extract_args.push_back(apks_file.string());
    extract_args.push_back("-d");
    extract_args.push_back(extract_dir.string());
    
    ProcessResult extract_result = runner_.run(
        "unzip",
        extract_args,
        temp_dir.string()
    );
#endif

    if (!extract_result.success()) {
        std::cerr << "Error: Failed to extract APK from .apks file\n";
        if (!extract_result.stderr_output.empty()) {
            std::cerr << extract_result.stderr_output << "\n";
        }
        return false;
    }

    // Find the extracted APK
    fs::path extracted_apk;
    for (const auto& entry : fs::recursive_directory_iterator(extract_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".apk") {
            extracted_apk = entry.path();
            break;
        }
    }

    if (extracted_apk.empty()) {
        std::cerr << "Error: No APK found in extracted files\n";
        return false;
    }

    // Move APK to output directory
    try {
        fs::rename(extracted_apk, output_apk);
    } catch (const std::exception&) {
        // Try copy if rename fails (cross-filesystem)
        try {
            fs::copy_file(extracted_apk, output_apk, fs::copy_options::overwrite_existing);
            fs::remove(extracted_apk);
        } catch (const std::exception& e2) {
            std::cerr << "Error: Failed to move APK to output directory: " << e2.what() << "\n";
            return false;
        }
    }

    return true;
}

bool AabConverter::convert_to_split(
    const Config& config,
    const fs::path& temp_dir
) const {
    fs::path bundletool_path(config.bundletool_path);
    if (!FileUtils::file_exists(bundletool_path)) {
        std::cerr << "Error: bundletool.jar not found: " << bundletool_path << "\n";
        return false;
    }

    fs::path output_path(config.output_dir);

    std::vector<std::string> args;
    args.push_back("build-apks");
    args.push_back("--bundle=" + FileUtils::get_absolute_path(config.input_aab));
    args.push_back("--output=" + (temp_dir / "output.apks").string());
    args.push_back("--mode=default");

    if (!config.quiet) {
        std::cout << "Converting AAB to split APKs...\n";
    }

    ProcessResult result = runner_.run_java(
        config.java_path,
        bundletool_path.string(),
        args,
        temp_dir.string()
    );

    if (!result.success()) {
        std::cerr << "Error: bundletool execution failed\n";
        if (!result.stderr_output.empty()) {
            std::cerr << result.stderr_output << "\n";
        }
        return false;
    }

    // Extract split APKs from .apks file
    fs::path apks_file = temp_dir / "output.apks";
    if (!FileUtils::file_exists(apks_file)) {
        std::cerr << "Error: bundletool did not generate output file\n";
        return false;
    }

    // Extract .apks ZIP file to get all split APKs
    // .apks is a ZIP file containing all split APKs
    fs::path extract_dir = temp_dir / "extracted";
    try {
        fs::create_directories(extract_dir);
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to create extraction directory: " << e.what() << "\n";
        return false;
    }

#ifdef _WIN32
    // On Windows, use PowerShell Expand-Archive
    // PowerShell requires .zip extension, so copy file temporarily
    fs::path zip_copy = temp_dir / "output.zip";
    try {
        fs::copy_file(apks_file, zip_copy, fs::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to copy .apks file: " << e.what() << "\n";
        return false;
    }
    
    std::vector<std::string> extract_args;
    extract_args.push_back("-Command");
    extract_args.push_back("Expand-Archive -Path \"" + zip_copy.string() + "\" -DestinationPath \"" + extract_dir.string() + "\" -Force");
    
    ProcessResult extract_result = runner_.run(
        "powershell.exe",
        extract_args,
        temp_dir.string()
    );
    
    // Clean up temporary zip file
    try {
        fs::remove(zip_copy);
    } catch (...) {
        // Ignore cleanup errors
    }
#else
    // On Unix, use unzip
    std::vector<std::string> extract_args;
    extract_args.push_back("-q");
    extract_args.push_back(apks_file.string());
    extract_args.push_back("-d");
    extract_args.push_back(extract_dir.string());
    
    ProcessResult extract_result = runner_.run(
        "unzip",
        extract_args,
        temp_dir.string()
    );
#endif

    if (!extract_result.success()) {
        std::cerr << "Error: Failed to extract split APKs from .apks file\n";
        if (!extract_result.stderr_output.empty()) {
            std::cerr << extract_result.stderr_output << "\n";
        }
        return false;
    }

    // Copy all extracted APKs to output directory
    bool found_apk = false;
    for (const auto& entry : fs::recursive_directory_iterator(extract_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".apk") {
            fs::path dest_apk = output_path / entry.path().filename();
            try {
                fs::copy_file(entry.path(), dest_apk, fs::copy_options::overwrite_existing);
                found_apk = true;
            } catch (const std::exception& e) {
                std::cerr << "Error: Failed to copy APK " << entry.path().filename() << ": " << e.what() << "\n";
                return false;
            }
        }
    }

    if (!found_apk) {
        std::cerr << "Error: No APK files found in extracted .apks file\n";
        return false;
    }

    return true;
}

} // namespace aab2apk

