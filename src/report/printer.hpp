#pragma once

#include <string>

#include "model/types.hpp"

namespace profilex::report {

class Printer {
  public:
    std::string format_run_summary(const model::RunRecord& run,
                                   const model::SummaryStats& stats) const;
    std::string format_run_list(const std::vector<model::RunRecord>& runs) const;
    std::string format_comparison(const std::string& baseline_name,
                                  const std::string& candidate_name,
                                  const model::ComparisonResult& comparison) const;

  private:
    static std::string format_timestamp(std::int64_t unix_seconds);
};

}  // namespace profilex::report
