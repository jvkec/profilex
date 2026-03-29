#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>

#include "runner/command_runner.hpp"

namespace {

profilex::runner::RunOptions make_options() {
    return profilex::runner::RunOptions{
        .name = "runner-test",
        .command_tokens = {"/usr/bin/true"},
        .runs = 3,
        .warmup = 0,
    };
}

}  // namespace

TEST_CASE("CommandRunner captures zero exit codes") {
    profilex::runner::CommandRunner runner;
    auto options = make_options();

    const auto record = runner.run(options);

    REQUIRE(record.samples.size() == 3U);
    for (const auto& sample : record.samples) {
        CHECK(sample.exit_code == 0);
        CHECK(sample.duration_ms >= 0.0);
    }
}

TEST_CASE("CommandRunner captures non-zero exit codes") {
    profilex::runner::CommandRunner runner;
    auto options = make_options();
    options.command_tokens = {"/bin/sh", "-c", "exit 7"};
    options.runs = 2;

    const auto record = runner.run(options);

    REQUIRE(record.samples.size() == 2U);
    for (const auto& sample : record.samples) {
        CHECK(sample.exit_code == 7);
    }
}

TEST_CASE("CommandRunner maps signal termination to shell-style exit status") {
    profilex::runner::CommandRunner runner;
    auto options = make_options();
    options.command_tokens = {"/bin/sh", "-c", "kill -TERM $$"};
    options.runs = 1;

    const auto record = runner.run(options);

    REQUIRE(record.samples.size() == 1U);
    CHECK(record.samples.front().exit_code == 143);
}

TEST_CASE("CommandRunner forwards SIGTERM to the active child process group") {
    if (std::getenv("PROFILEX_ENABLE_SIGNAL_FORWARDING_TEST") == nullptr) {
        MESSAGE("Skipping signal forwarding integration test; set PROFILEX_ENABLE_SIGNAL_FORWARDING_TEST=1 to enable it.");
        return;
    }

    const auto ready_file =
        std::filesystem::temp_directory_path() / "profilex_runner_sigint_ready";
    const auto result_file =
        std::filesystem::temp_directory_path() / "profilex_runner_sigint_result";
    std::filesystem::remove(ready_file);
    std::filesystem::remove(result_file);

    const pid_t child = fork();
    REQUIRE(child >= 0);

    if (child == 0) {
        profilex::runner::CommandRunner runner;
        auto options = make_options();
        options.command_tokens = {"/bin/sh",
                                  "-c",
                                  "trap 'exit 143' TERM; printf ready > \"$1\"; while :; do sleep 0.1; done",
                                  "sh",
                                  ready_file.string()};
        options.runs = 1;

        try {
            const auto record = runner.run(options);
            std::ofstream out(result_file, std::ios::trunc);
            out << record.samples.front().exit_code;
            _exit(record.samples.front().exit_code == 143 ? 0 : 1);
        } catch (...) {
            _exit(2);
        }
    }

    for (int attempt = 0; attempt < 50; ++attempt) {
        if (std::filesystem::exists(ready_file)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    REQUIRE(std::filesystem::exists(ready_file));
    kill(child, SIGTERM);

    int status = 0;
    REQUIRE(waitpid(child, &status, 0) == child);
    REQUIRE(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
    CHECK(std::filesystem::exists(result_file));

    std::ifstream in(result_file);
    int observed_exit_code = 0;
    in >> observed_exit_code;
    CHECK(observed_exit_code == 143);

    std::filesystem::remove(ready_file);
    std::filesystem::remove(result_file);
}

TEST_CASE("CommandRunner rejects invalid inputs") {
    profilex::runner::CommandRunner runner;
    auto options = make_options();

    options.name.clear();
    CHECK_THROWS_AS(runner.run(options), std::invalid_argument);

    options = make_options();
    options.command_tokens.clear();
    CHECK_THROWS_AS(runner.run(options), std::invalid_argument);

    options = make_options();
    options.runs = 0;
    CHECK_THROWS_AS(runner.run(options), std::invalid_argument);

    options = make_options();
    options.warmup = -1;
    CHECK_THROWS_AS(runner.run(options), std::invalid_argument);
}
