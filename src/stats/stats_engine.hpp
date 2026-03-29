#pragma once

#include <vector>

#include "model/types.hpp"

namespace profilex::stats {

class StatsEngine {
  public:
    model::SummaryStats summarize(const std::vector<model::Sample>& samples) const;
    model::ComparisonResult compare(const std::vector<model::Sample>& baseline,
                                    const std::vector<model::Sample>& candidate) const;

  private:
    static constexpr std::size_t kMinimumSamples = 5;
    static constexpr double kMinimumDeltaPercent = 3.0;
};

}  // namespace profilex::stats
