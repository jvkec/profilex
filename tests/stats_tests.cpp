#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "model/types.hpp"
#include "stats/stats_engine.hpp"

namespace {

std::vector<profilex::model::Sample> samples(std::initializer_list<double> values) {
    std::vector<profilex::model::Sample> output;
    output.reserve(values.size());
    for (const double value : values) {
        output.push_back(profilex::model::Sample{.duration_ms = value, .exit_code = 0});
    }
    return output;
}

TEST_CASE("StatsEngine summarizes a run") {
    profilex::stats::StatsEngine engine;
    const auto summary = engine.summarize(samples({10.0, 11.0, 12.0, 13.0, 14.0}));

    CHECK(summary.count == 5U);
    CHECK(doctest::Approx(summary.min_ms) == 10.0);
    CHECK(doctest::Approx(summary.max_ms) == 14.0);
    CHECK(doctest::Approx(summary.mean_ms) == 12.0);
    CHECK(doctest::Approx(summary.median_ms) == 12.0);
    CHECK(doctest::Approx(summary.stddev_ms) == std::sqrt(2.5));
    CHECK(doctest::Approx(summary.p95_ms) == 13.8);
}

TEST_CASE("StatsEngine reports likely improvement") {
    profilex::stats::StatsEngine engine;
    const auto result = engine.compare(samples({100.0, 101.0, 99.0, 100.0, 100.0}),
                                       samples({90.0, 91.0, 89.0, 90.0, 90.0}));

    CHECK(result.verdict == profilex::model::Verdict::likely_improvement);
    CHECK_FALSE(result.notes.has_value());
}

TEST_CASE("StatsEngine marks small deltas as inconclusive") {
    profilex::stats::StatsEngine engine;
    const auto result = engine.compare(samples({100.0, 100.0, 100.0, 100.0, 100.0}),
                                       samples({101.0, 101.0, 101.0, 101.0, 101.0}));

    CHECK(result.verdict == profilex::model::Verdict::inconclusive);
    CHECK(result.notes.has_value());
}

TEST_CASE("StatsEngine marks high variance as inconclusive") {
    profilex::stats::StatsEngine engine;
    const auto result = engine.compare(samples({100.0, 99.0, 101.0, 100.0, 100.0}),
                                       samples({92.0, 107.0, 88.0, 110.0, 90.0}));

    CHECK(result.verdict == profilex::model::Verdict::inconclusive);
    CHECK(result.notes.has_value());
}

}  // namespace
