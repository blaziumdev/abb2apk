#pragma once

#include "config.h"
#include "process_runner.h"
#include <string>
#include <filesystem>

namespace aab2apk {

class SigningManager {
public:
    explicit SigningManager(const ProcessRunner& runner) : runner_(runner) {}

    bool sign_apk(
        const std::filesystem::path& apk_path,
        const SigningConfig& config
    ) const;

    bool sign_apks(
        const std::filesystem::path& apks_path,
        const SigningConfig& config
    ) const;

private:
    const ProcessRunner& runner_;
    std::string find_apksigner() const;
    bool validate_signing_result(const ProcessResult& result) const;
};

} // namespace aab2apk

