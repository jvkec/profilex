#include "runner/command_runner.hpp"

#include <chrono>
#include <csignal>
#include <ctime>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <vector>

extern char** environ;

namespace profilex::runner {
namespace {

std::string shell_quote(std::string_view token) {
    std::string quoted;
    quoted.reserve(token.size() + 2U);
    quoted.push_back('\'');
    for (const char ch : token) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted.push_back('\'');
    return quoted;
}

std::string join_command(const std::vector<std::string>& tokens) {
    std::string command;
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (index != 0U) {
            command.push_back(' ');
        }
        command += shell_quote(tokens[index]);
    }
    return command;
}

int execute_shell_command(const std::string& command) {
    std::vector<char> mutable_command(command.begin(), command.end());
    mutable_command.push_back('\0');

    char shell_path[] = "/bin/sh";
    char shell_name[] = "sh";
    char flag[] = "-lc";

    char* argv[] = {shell_name, flag, mutable_command.data(), nullptr};
    pid_t process_id {};

    const int spawn_result =
        posix_spawn(&process_id, shell_path, nullptr, nullptr, argv, environ);
    if (spawn_result != 0) {
        throw std::runtime_error("failed to spawn command");
    }

    int status {};
    if (waitpid(process_id, &status, 0) == -1) {
        throw std::runtime_error("failed to wait for command completion");
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return status;
}

model::Sample measure_once(const std::string& command) {
    const auto start = std::chrono::steady_clock::now();
    const int exit_code = execute_shell_command(command);
    const auto end = std::chrono::steady_clock::now();

    const std::chrono::duration<double, std::milli> duration = end - start;
    return model::Sample{.duration_ms = duration.count(), .exit_code = exit_code};
}

}  // namespace

model::RunRecord CommandRunner::run(const RunOptions& options) const {
    if (options.name.empty()) {
        throw std::invalid_argument("run name must not be empty");
    }
    if (options.command_tokens.empty()) {
        throw std::invalid_argument("missing command after --");
    }
    if (options.runs <= 0) {
        throw std::invalid_argument("--runs must be greater than zero");
    }
    if (options.warmup < 0) {
        throw std::invalid_argument("--warmup must be zero or greater");
    }

    const std::string command = join_command(options.command_tokens);

    for (int iteration = 0; iteration < options.warmup; ++iteration) {
        static_cast<void>(measure_once(command));
    }

    model::RunRecord record{
        .name = options.name,
        .command = command,
        .created_at_unix = static_cast<std::int64_t>(std::time(nullptr)),
        .requested_runs = options.runs,
        .warmup_runs = options.warmup,
        .tags = options.tags,
        .notes = options.notes.empty() ? std::optional<std::string>{} : options.notes,
        .samples = {},
    };
    record.samples.reserve(static_cast<std::size_t>(options.runs));

    for (int iteration = 0; iteration < options.runs; ++iteration) {
        record.samples.push_back(measure_once(command));
    }

    return record;
}

}  // namespace profilex::runner
