#include "process_runner.h"
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#endif

namespace aab2apk {

std::string ProcessRunner::escape_argument(const std::string& arg) const {
    // Basic escaping for Windows and Unix
    std::string escaped;
    escaped.reserve(arg.size() + 2);

#ifdef _WIN32
    // Windows command line escaping
    bool needs_quoting = arg.empty() || arg.find_first_of(" \t\n\v\"") != std::string::npos;
    if (needs_quoting) {
        escaped += '"';
        for (char c : arg) {
            if (c == '"') {
                escaped += "\\\"";
            } else if (c == '\\') {
                escaped += "\\\\";
            } else {
                escaped += c;
            }
        }
        escaped += '"';
    } else {
        escaped = arg;
    }
#else
    // Unix shell escaping
    bool needs_quoting = arg.empty() || arg.find_first_of(" \t\n\"'$`\\*?[]") != std::string::npos;
    if (needs_quoting) {
        escaped += '\'';
        for (char c : arg) {
            if (c == '\'') {
                escaped += "'\\''";
            } else {
                escaped += c;
            }
        }
        escaped += '\'';
    } else {
        escaped = arg;
    }
#endif

    return escaped;
}

std::string ProcessRunner::join_args(const std::vector<std::string>& args) const {
    if (args.empty()) {
        return "";
    }

    std::ostringstream oss;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
            oss << " ";
        }
        oss << escape_argument(args[i]);
    }
    return oss.str();
}

ProcessResult ProcessRunner::run(
    const std::string& command,
    const std::vector<std::string>& args,
    const std::optional<std::string>& working_dir,
    const std::optional<std::vector<std::pair<std::string, std::string>>>& env
) const {
    std::vector<std::string> full_args;
    full_args.push_back(command);
    full_args.insert(full_args.end(), args.begin(), args.end());

    std::string full_command = join_args(full_args);

#ifdef _WIN32
    HANDLE h_stdout_read = nullptr;
    HANDLE h_stdout_write = nullptr;
    HANDLE h_stderr_read = nullptr;
    HANDLE h_stderr_write = nullptr;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    if (!CreatePipe(&h_stdout_read, &h_stdout_write, &sa, 0) ||
        !CreatePipe(&h_stderr_read, &h_stderr_write, &sa, 0)) {
        return ProcessResult{-1, "", "Failed to create pipes"};
    }

    SetHandleInformation(h_stdout_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(h_stderr_read, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = h_stdout_write;
    si.hStdError = h_stderr_write;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    DWORD exit_code = 1;
    std::string stdout_output;
    std::string stderr_output;

    // CreateProcessA requires a mutable buffer
    std::vector<char> cmd_line_buf(full_command.begin(), full_command.end());
    cmd_line_buf.push_back('\0');
    
    if (CreateProcessA(
            nullptr,
            cmd_line_buf.data(),
            nullptr,
            nullptr,
            TRUE,
            0,
            nullptr,
            working_dir.has_value() ? working_dir->c_str() : nullptr,
            &si,
            &pi)) {
        CloseHandle(h_stdout_write);
        CloseHandle(h_stderr_write);

        // Read output
        char buffer[4096];
        DWORD bytes_read;
        while (ReadFile(h_stdout_read, buffer, sizeof(buffer) - 1, &bytes_read, nullptr) && bytes_read > 0) {
            buffer[bytes_read] = '\0';
            stdout_output += buffer;
        }
        while (ReadFile(h_stderr_read, buffer, sizeof(buffer) - 1, &bytes_read, nullptr) && bytes_read > 0) {
            buffer[bytes_read] = '\0';
            stderr_output += buffer;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        stderr_output = "Failed to create process";
    }

    CloseHandle(h_stdout_read);
    CloseHandle(h_stderr_read);

    return ProcessResult{
        static_cast<int>(exit_code),
        stdout_output,
        stderr_output
    };

#else
    // Unix implementation
    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        return ProcessResult{-1, "", "Failed to create pipes"};
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        return ProcessResult{-1, "", "Failed to fork process"};
    }

    if (pid == 0) {
        // Child process
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        if (working_dir.has_value()) {
            chdir(working_dir->c_str());
        }

        // Prepare arguments for execvp
        std::vector<char*> exec_args;
        exec_args.reserve(full_args.size() + 1);
        for (const auto& arg : full_args) {
            exec_args.push_back(const_cast<char*>(arg.c_str()));
        }
        exec_args.push_back(nullptr);

        execvp(command.c_str(), exec_args.data());
        _exit(127);
    } else {
        // Parent process
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        std::string stdout_output;
        std::string stderr_output;

        char buffer[4096];
        ssize_t bytes_read;

        // Read stdout
        while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            stdout_output += buffer;
        }

        // Read stderr
        while ((bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            stderr_output += buffer;
        }

        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        int status;
        waitpid(pid, &status, 0);

        int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

        return ProcessResult{
            exit_code,
            stdout_output,
            stderr_output
        };
    }
#endif
}

ProcessResult ProcessRunner::run_java(
    const std::string& java_path,
    const std::string& jar_path,
    const std::vector<std::string>& java_args,
    const std::optional<std::string>& working_dir
) const {
    std::vector<std::string> args;
    args.push_back("-jar");
    args.push_back(jar_path);
    args.insert(args.end(), java_args.begin(), java_args.end());

    return run(java_path, args, working_dir);
}

} // namespace aab2apk

