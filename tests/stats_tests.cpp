#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "model/types.hpp"
#include "stats/stats_engine.hpp"

namespace {

bool nearly_equal(const double left, const double right, const double epsilon = 1e-6) {
    return std::fabs(left - right) < epsilon;
}

void require(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::vector<profilex::model::Sample> samples(std::initializer_list<double> values) {
    std::vector<profilex::model::Sample> output;
    output.reserve(values.size());
    for (const double value : values) {
        output.push_back(profilex::model::Sample{.duration_ms = value, .exit_code = 0});
    }
    return output;
}

void test_summary_stats() {
    profilex::stats::StatsEngine engine;
    const auto summary = engine.summarize(samples({10.0, 11.0, 12.0, 13.0, 14.0}));

    require(summary.count == 5U, "count should match sample size");
    require(nearly_equal(summary.min_ms, 10.0), "min should match");
    require(nearly_equal(summary.max_ms, 14.0), "max should match");
    require(nearly_equal(summary.mean_ms, 12.0), "mean should match");
    require(nearly_equal(summary.median_ms, 12.0), "median should match");
    require(nearly_equal(summary.stddev_ms, std::sqrt(2.5)), "stddev should match sample stddev");
    require(nearly_equal(summary.p95_ms, 13.8), "p95 should use linear interpolation");
}

void test_likely_improvement() {
    profilex::stats::StatsEngine engine;
    const auto result = engine.compare(samples({100.0, 101.0, 99.0, 100.0, 100.0}),
                                       samples({90.0, 91.0, 89.0, 90.0, 90.0}));

    require(result.verdict == profilex::model::Verdict::likely_improvement,
            "faster candidate should be an improvement");
    require(result.notes == std::nullopt, "clear improvement should not include notes");
}

void test_inconclusive_for_small_delta() {
    profilex::stats::StatsEngine engine;
    const auto result = engine.compare(samples({100.0, 100.0, 100.0, 100.0, 100.0}),
                                       samples({101.0, 101.0, 101.0, 101.0, 101.0}));

    require(result.verdict == profilex::model::Verdict::inconclusive,
            "small deltas should be inconclusive");
    require(result.notes.has_value(), "inconclusive result should explain why");
}

void test_inconclusive_for_high_variance() {
    profilex::stats::StatsEngine engine;
    const auto result = engine.compare(samples({100.0, 99.0, 101.0, 100.0, 100.0}),
                                       samples({92.0, 107.0, 88.0, 110.0, 90.0}));

    require(result.verdict == profilex::model::Verdict::inconclusive,
            "high variance should be inconclusive");
    require(result.notes.has_value(), "variance-based inconclusive result should explain why");
}

}  // namespace

int main() {
    try {
        test_summary_stats();
        test_likely_improvement();
        test_inconclusive_for_small_delta();
        test_inconclusive_for_high_variance();
        std::cout << "All stats tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failure: " << ex.what() << '\n';
        return 1;
    }
}
