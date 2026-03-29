#pragma once

#include <string>
#include <vector>

#include "model/types.hpp"

namespace profilex::runner {

struct RunOptions {
    std::string name;
    std::vector<std::string> command_tokens;
    int runs {10};
    int warmup {2};
    std::vector<std::string> tags;
    std::string notes;
};

class CommandRunner {
  public:
    model::RunRecord run(const RunOptions& options) const;
};

}  // namespace profilex::runner
