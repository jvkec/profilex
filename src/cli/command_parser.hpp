#pragma once

#include <optional>
#include <string>
#include <vector>

#include "runner/command_runner.hpp"

namespace profilex::cli {

enum class CommandType {
    help,
    run,
    list,
    show,
    compare,
    export_run,
    delete_run,
};

struct ParsedCommand {
    CommandType type {CommandType::help};
    runner::RunOptions run_options;
    bool overwrite {};
    std::string run_name;
    std::string baseline_name;
    std::string candidate_name;
};

class CommandParser {
  public:
    ParsedCommand parse(const std::vector<std::string>& args) const;

  private:
    static int parse_positive_int(const std::string& value, const std::string& flag_name);
    static int parse_non_negative_int(const std::string& value, const std::string& flag_name);
    static std::vector<std::string> slice(const std::vector<std::string>& values, std::size_t start);
    static ParsedCommand parse_run(const std::vector<std::string>& args);
    static ParsedCommand parse_show(const std::vector<std::string>& args);
    static ParsedCommand parse_compare(const std::vector<std::string>& args);
    static ParsedCommand parse_export(const std::vector<std::string>& args);
    static ParsedCommand parse_delete(const std::vector<std::string>& args);
};

}  // namespace profilex::cli
