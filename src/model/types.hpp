#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace profilex::model {

enum class Verdict {
    likely_improvement,
    likely_regression,
    inconclusive,
};

struct Sample {
    double duration_ms {};
    int exit_code {};
};

struct RunRecord {
    std::string name;
    std::string command;
    std::int64_t created_at_unix {};
    int requested_runs {};
    int warmup_runs {};
    std::vector<std::string> tags;
    std::optional<std::string> notes;
    std::vector<Sample> samples;
};

struct SummaryStats {
    std::size_t count {};
    double min_ms {};
    double max_ms {};
    double mean_ms {};
    double median_ms {};
    double stddev_ms {};
    double p95_ms {};
};

struct ComparisonResult {
    SummaryStats baseline;
    SummaryStats candidate;
    double mean_delta_percent {};
    double median_delta_percent {};
    Verdict verdict {Verdict::inconclusive};
    std::optional<std::string> notes;
};

std::string to_string(Verdict verdict);

}  // namespace profilex::model
