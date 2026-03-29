#pragma once

#include <string>
#include <vector>

namespace profilex::cli {

struct ParsedCommand;

class Application {
  public:
    int run(const std::vector<std::string>& args) const;

  private:
    int run_command(const ParsedCommand& command) const;
    int list_command(const ParsedCommand& command) const;
    int show_command(const ParsedCommand& command) const;
    int compare_command(const ParsedCommand& command) const;
    int export_command(const ParsedCommand& command) const;
    int delete_command(const ParsedCommand& command) const;
    static std::string usage();
};

}  // namespace profilex::cli
