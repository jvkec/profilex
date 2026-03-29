#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <vector>

#include "cli/command_parser.hpp"

namespace {

std::vector<std::string> make_args(std::initializer_list<const char*> args) {
    std::vector<std::string> values;
    values.reserve(args.size());
    for (const char* arg : args) {
        values.emplace_back(arg);
    }
    return values;
}

}  // namespace

TEST_CASE("CommandParser returns help by default") {
    profilex::cli::CommandParser parser;
    const auto parsed = parser.parse(make_args({"profilex"}));

    CHECK(parsed.type == profilex::cli::CommandType::help);
}

TEST_CASE("CommandParser parses version flags") {
    profilex::cli::CommandParser parser;

    CHECK(parser.parse(make_args({"profilex", "--version"})).type ==
          profilex::cli::CommandType::version);
    CHECK(parser.parse(make_args({"profilex", "-V"})).type ==
          profilex::cli::CommandType::version);
}

TEST_CASE("CommandParser parses run with defaults and command separator") {
    profilex::cli::CommandParser parser;
    const auto parsed = parser.parse(
        make_args({"profilex", "run", "--name", "baseline", "--", "./bench", "input.json"}));

    CHECK(parsed.type == profilex::cli::CommandType::run);
    CHECK(parsed.run_options.name == "baseline");
    CHECK(parsed.run_options.runs == 10);
    CHECK(parsed.run_options.warmup == 2);
    REQUIRE(parsed.run_options.command_tokens.size() == 2);
    CHECK(parsed.run_options.command_tokens[0] == "./bench");
    CHECK(parsed.run_options.command_tokens[1] == "input.json");
}

TEST_CASE("CommandParser parses full run options") {
    profilex::cli::CommandParser parser;
    const auto parsed = parser.parse(make_args({"profilex",
                                                "run",
                                                "--name",
                                                "candidate",
                                                "--runs",
                                                "25",
                                                "--warmup",
                                                "3",
                                                "--tag",
                                                "opt",
                                                "--tag",
                                                "lto",
                                                "--notes",
                                                "release build",
                                                "--overwrite",
                                                "--",
                                                "./bench"}));

    CHECK(parsed.type == profilex::cli::CommandType::run);
    CHECK(parsed.overwrite);
    CHECK(parsed.run_options.runs == 25);
    CHECK(parsed.run_options.warmup == 3);
    CHECK(parsed.run_options.notes == "release build");
    REQUIRE(parsed.run_options.tags.size() == 2);
    CHECK(parsed.run_options.tags[0] == "opt");
    CHECK(parsed.run_options.tags[1] == "lto");
}

TEST_CASE("CommandParser rejects missing run command") {
    profilex::cli::CommandParser parser;
    CHECK_THROWS_AS(
        parser.parse(make_args({"profilex", "run", "--name", "baseline"})), std::invalid_argument);
}

TEST_CASE("CommandParser rejects invalid export format") {
    profilex::cli::CommandParser parser;
    CHECK_THROWS_AS(parser.parse(make_args({"profilex", "export", "baseline", "--format", "yaml"})),
                    std::invalid_argument);
}

TEST_CASE("CommandParser parses compare and delete") {
    profilex::cli::CommandParser parser;

    const auto compare =
        parser.parse(make_args({"profilex", "compare", "baseline", "candidate"}));
    CHECK(compare.type == profilex::cli::CommandType::compare);
    CHECK(compare.baseline_name == "baseline");
    CHECK(compare.candidate_name == "candidate");

    const auto remove = parser.parse(make_args({"profilex", "delete", "candidate"}));
    CHECK(remove.type == profilex::cli::CommandType::delete_run);
    CHECK(remove.run_name == "candidate");
}
