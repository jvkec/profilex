#pragma once

#include <string>
#include <vector>

namespace profilex::cli {

class Application {
  public:
    int run(const std::vector<std::string>& args) const;

  private:
    int run_command(const std::vector<std::string>& args) const;
    int list_command() const;
    int show_command(const std::vector<std::string>& args) const;
    int compare_command(const std::vector<std::string>& args) const;
    int export_command(const std::vector<std::string>& args) const;
    int delete_command(const std::vector<std::string>& args) const;
    static std::string usage();
};

}  // namespace profilex::cli
