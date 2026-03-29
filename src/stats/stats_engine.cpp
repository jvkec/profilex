#include "stats/stats_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace profilex::stats {
namespace {

std::vector<double> extract_durations(const std::vector<model::Sample>& samples) {
    std::vector<double> values;
    values.reserve(samples.size());
    for (const auto& sample : samples) {
        values.push_back(sample.duration_ms);
    }
    return values;
}

double compute_mean(const std::vector<double>& values) {
    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / static_cast<double>(values.size());
}

double compute_percentile(const std::vector<double>& sorted_values, const double percentile) {
    if (sorted_values.empty()) {
        throw std::invalid_argument("cannot compute percentile of empty sample set");
    }

    if (sorted_values.size() == 1U) {
        return sorted_values.front();
    }

    const double rank = percentile * static_cast<double>(sorted_values.size() - 1U);
    const auto lower_index = static_cast<std::size_t>(std::floor(rank));
    const auto upper_index = static_cast<std::size_t>(std::ceil(rank));
    const double fraction = rank - static_cast<double>(lower_index);

    return sorted_values[lower_index] +
           ((sorted_values[upper_index] - sorted_values[lower_index]) * fraction);
}

double compute_stddev(const std::vector<double>& values, const double mean) {
    if (values.size() < 2U) {
        return 0.0;
    }

    double accum = 0.0;
    for (const double value : values) {
        const double delta = value - mean;
        accum += delta * delta;
    }

    return std::sqrt(accum / static_cast<double>(values.size() - 1U));
}

double compute_delta_percent(const double baseline_value, const double candidate_value) {
    if (baseline_value == 0.0) {
        throw std::invalid_argument("baseline metric must be non-zero");
    }

    return ((candidate_value - baseline_value) / baseline_value) * 100.0;
}

}  // namespace

model::SummaryStats StatsEngine::summarize(const std::vector<model::Sample>& samples) const {
    if (samples.empty()) {
        throw std::invalid_argument("cannot summarize an empty run");
    }

    std::vector<double> values = extract_durations(samples);
    std::sort(values.begin(), values.end());

    const double mean = compute_mean(values);
    const std::size_t midpoint = values.size() / 2U;
    const double median = values.size() % 2U == 0U
                              ? (values[midpoint - 1U] + values[midpoint]) / 2.0
                              : values[midpoint];

    return model::SummaryStats{
        .count = samples.size(),
        .min_ms = values.front(),
        .max_ms = values.back(),
        .mean_ms = mean,
        .median_ms = median,
        .stddev_ms = compute_stddev(values, mean),
        .p95_ms = compute_percentile(values, 0.95),
    };
}

model::ComparisonResult StatsEngine::compare(const std::vector<model::Sample>& baseline,
                                             const std::vector<model::Sample>& candidate) const {
    const auto baseline_stats = summarize(baseline);
    const auto candidate_stats = summarize(candidate);

    const double mean_delta = compute_delta_percent(baseline_stats.mean_ms, candidate_stats.mean_ms);
    const double median_delta =
        compute_delta_percent(baseline_stats.median_ms, candidate_stats.median_ms);

    model::Verdict verdict = model::Verdict::inconclusive;
    std::optional<std::string> notes;

    if (baseline_stats.count < kMinimumSamples || candidate_stats.count < kMinimumSamples) {
        notes = "At least 5 samples per run are required for a verdict.";
    } else if (std::abs(mean_delta) < kMinimumDeltaPercent &&
               std::abs(median_delta) < kMinimumDeltaPercent) {
        notes = "Observed deltas are below the 3% confidence threshold.";
    } else {
        const double absolute_mean_delta_ms =
            std::abs(candidate_stats.mean_ms - baseline_stats.mean_ms);
        const double max_stddev = std::max(baseline_stats.stddev_ms, candidate_stats.stddev_ms);

        if (absolute_mean_delta_ms == 0.0) {
            notes = "Mean delta is zero.";
        } else if (max_stddev > (absolute_mean_delta_ms * 0.5)) {
            notes = "Variance is large relative to the observed mean delta.";
        } else if (mean_delta < 0.0 && median_delta < 0.0) {
            verdict = model::Verdict::likely_improvement;
        } else if (mean_delta > 0.0 && median_delta > 0.0) {
            verdict = model::Verdict::likely_regression;
        } else {
            notes = "Mean and median deltas disagree on direction.";
        }
    }

    return model::ComparisonResult{
        .baseline = baseline_stats,
        .candidate = candidate_stats,
        .mean_delta_percent = mean_delta,
        .median_delta_percent = median_delta,
        .verdict = verdict,
        .notes = notes,
    };
}

}  // namespace profilex::stats
