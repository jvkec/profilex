#include "runner/command_runner.hpp"

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <ctime>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char** environ;

namespace profilex::runner {
namespace {

std::atomic<pid_t> g_active_child_pgid {0};

void forwarding_signal_handler(const int signal_number) {
    const pid_t pgid = g_active_child_pgid.load();
    if (pgid > 0) {
        kill(-pgid, signal_number);
    }
}

class SignalForwardingScope {
  public:
    explicit SignalForwardingScope(const pid_t child_pgid) : child_pgid_(child_pgid) {
        g_active_child_pgid.store(child_pgid_);

        install_handler(SIGINT, 0U);
        install_handler(SIGTERM, 1U);
        install_handler(SIGHUP, 2U);
        install_handler(SIGQUIT, 3U);
    }

    SignalForwardingScope(const SignalForwardingScope&) = delete;
    SignalForwardingScope& operator=(const SignalForwardingScope&) = delete;

    ~SignalForwardingScope() {
        restore_handler(SIGINT, 0U);
        restore_handler(SIGTERM, 1U);
        restore_handler(SIGHUP, 2U);
        restore_handler(SIGQUIT, 3U);
        g_active_child_pgid.store(0);
    }

  private:
    void install_handler(const int signal_number, const std::size_t slot) {
        struct sigaction action {};
        action.sa_handler = forwarding_signal_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;

        if (sigaction(signal_number, &action, &previous_actions_[slot]) != 0) {
            throw std::runtime_error("failed to install signal forwarding handler");
        }
    }

    void restore_handler(const int signal_number, const std::size_t slot) noexcept {
        sigaction(signal_number, &previous_actions_[slot], nullptr);
    }

    pid_t child_pgid_ {};
    std::array<struct sigaction, 4U> previous_actions_ {};
};

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
    posix_spawnattr_t attributes;
    if (posix_spawnattr_init(&attributes) != 0) {
        throw std::runtime_error("failed to initialize spawn attributes");
    }

    if (posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP) != 0) {
        posix_spawnattr_destroy(&attributes);
        throw std::runtime_error("failed to configure spawn flags");
    }
    if (posix_spawnattr_setpgroup(&attributes, 0) != 0) {
        posix_spawnattr_destroy(&attributes);
        throw std::runtime_error("failed to configure child process group");
    }

    const int spawn_result =
        posix_spawn(&process_id, shell_path, nullptr, &attributes, argv, environ);
    posix_spawnattr_destroy(&attributes);
    if (spawn_result != 0) {
        throw std::runtime_error("failed to spawn command");
    }

    int status {};
    {
        SignalForwardingScope forwarding_scope(process_id);
        while (waitpid(process_id, &status, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("failed to wait for command completion");
        }
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
