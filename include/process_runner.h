#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace aab2apk {

struct ProcessResult {
    int exit_code;
    std::string stdout_output;
    std::string stderr_output;
    bool success() const { return exit_code == 0; }
};

class ProcessRunner {
public:
    ProcessRunner() = default;
    ~ProcessRunner() = default;

    // Non-copyable, movable
    ProcessRunner(const ProcessRunner&) = delete;
    ProcessRunner& operator=(const ProcessRunner&) = delete;
    ProcessRunner(ProcessRunner&&) = default;
    ProcessRunner& operator=(ProcessRunner&&) = default;

    ProcessResult run(
        const std::string& command,
        const std::vector<std::string>& args,
        const std::optional<std::string>& working_dir = std::nullopt,
        const std::optional<std::vector<std::pair<std::string, std::string>>>& env = std::nullopt
    ) const;

    ProcessResult run_java(
        const std::string& java_path,
        const std::string& jar_path,
        const std::vector<std::string>& java_args,
        const std::optional<std::string>& working_dir = std::nullopt
    ) const;

private:
    std::string join_args(const std::vector<std::string>& args) const;
    std::string escape_argument(const std::string& arg) const;
};

} // namespace aab2apk

