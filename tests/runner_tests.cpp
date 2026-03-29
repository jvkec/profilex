#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <chrono>
#include <csignal>
#include <filesystem>
#include <future>
#include <thread>

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

TEST_CASE("CommandRunner forwards SIGINT to the active child process group") {
    profilex::runner::CommandRunner runner;
    auto options = make_options();
    const auto ready_file =
        std::filesystem::temp_directory_path() / "profilex_runner_sigint_ready";
    std::filesystem::remove(ready_file);

    options.command_tokens = {"/bin/sh",
                              "-c",
                              "trap 'exit 42' INT; printf ready > \"$1\"; while :; do sleep 0.1; done",
                              "sh",
                              ready_file.string()};
    options.runs = 1;

    auto future = std::async(std::launch::async, [&runner, options]() { return runner.run(options); });

    for (int attempt = 0; attempt < 50; ++attempt) {
        if (std::filesystem::exists(ready_file)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    REQUIRE(std::filesystem::exists(ready_file));
    std::raise(SIGINT);

    const auto record = future.get();
    REQUIRE(record.samples.size() == 1U);
    CHECK(record.samples.front().exit_code == 42);

    std::filesystem::remove(ready_file);
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
