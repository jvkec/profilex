#include "report/printer.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace profilex::report {
namespace {

std::string format_ms(const double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << value << " ms";
    return out.str();
}

std::string format_percent(const double value) {
    std::ostringstream out;
    out << std::showpos << std::fixed << std::setprecision(2) << value << "%";
    return out.str();
}

void append_stats_block(std::ostringstream& out, const std::string& label,
                        const model::SummaryStats& stats) {
    out << label << '\n';
    out << "  Count:  " << stats.count << '\n';
    out << "  Min:    " << format_ms(stats.min_ms) << '\n';
    out << "  Median: " << format_ms(stats.median_ms) << '\n';
    out << "  Mean:   " << format_ms(stats.mean_ms) << '\n';
    out << "  P95:    " << format_ms(stats.p95_ms) << '\n';
    out << "  Max:    " << format_ms(stats.max_ms) << '\n';
    out << "  Stddev: " << format_ms(stats.stddev_ms) << '\n';
}

}  // namespace

std::string Printer::format_run_summary(const model::RunRecord& run,
                                        const model::SummaryStats& stats) const {
    std::ostringstream out;
    out << "Run: " << run.name << '\n';
    out << "Command: " << run.command << '\n';
    out << "Created: " << format_timestamp(run.created_at_unix) << '\n';
    out << "Samples: " << run.samples.size() << '\n';
    out << "Warmup: " << run.warmup_runs << '\n';
    if (!run.tags.empty()) {
        out << "Tags: ";
        for (std::size_t index = 0; index < run.tags.size(); ++index) {
            if (index != 0U) {
                out << ", ";
            }
            out << run.tags[index];
        }
        out << '\n';
    }
    if (run.notes.has_value()) {
        out << "Notes: " << *run.notes << '\n';
    }
    out << '\n';
    append_stats_block(out, "Summary", stats);
    return out.str();
}

std::string Printer::format_run_list(const std::vector<model::RunRecord>& runs) const {
    if (runs.empty()) {
        return "No saved runs.\n";
    }

    std::ostringstream out;
    for (const auto& run : runs) {
        out << run.name << "  |  " << format_timestamp(run.created_at_unix) << "  |  "
            << run.samples.size() << " samples  |  " << run.command << '\n';
    }
    return out.str();
}

std::string Printer::format_comparison(const std::string& baseline_name,
                                       const std::string& candidate_name,
                                       const model::ComparisonResult& comparison) const {
    std::ostringstream out;
    out << candidate_name << " vs " << baseline_name << "\n\n";
    append_stats_block(out, "Baseline", comparison.baseline);
    out << '\n';
    append_stats_block(out, "Candidate", comparison.candidate);
    out << '\n';
    out << "Median delta: " << format_percent(comparison.median_delta_percent) << '\n';
    out << "Mean delta:   " << format_percent(comparison.mean_delta_percent) << '\n';
    out << "Status: " << model::to_string(comparison.verdict) << '\n';
    if (comparison.notes.has_value()) {
        out << "Notes: " << *comparison.notes << '\n';
    }
    return out.str();
}

std::string Printer::format_timestamp(const std::int64_t unix_seconds) {
    const std::time_t time = static_cast<std::time_t>(unix_seconds);
    std::tm local_time {};
#if defined(_WIN32)
    localtime_s(&local_time, &time);
#else
    localtime_r(&time, &local_time);
#endif

    std::ostringstream out;
    out << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

}  // namespace profilex::report
