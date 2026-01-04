#pragma once

#include "config.h"
#include "process_runner.h"
#include "signing.h"
#include <string>
#include <filesystem>
#include <vector>

namespace aab2apk {

class AabConverter {
public:
    AabConverter(
        const ProcessRunner& runner,
        const SigningManager& signer
    ) : runner_(runner), signer_(signer) {}

    bool convert(const Config& config) const;

private:
    const ProcessRunner& runner_;
    const SigningManager& signer_;

    bool convert_to_universal(
        const Config& config,
        const std::filesystem::path& temp_dir
    ) const;

    bool convert_to_split(
        const Config& config,
        const std::filesystem::path& temp_dir
    ) const;
};

} // namespace aab2apk

