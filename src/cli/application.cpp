#include "cli/application.hpp"

#include <iostream>
#include <stdexcept>

#include "report/printer.hpp"
#include "runner/command_runner.hpp"
#include "stats/stats_engine.hpp"
#include "storage/run_repository.hpp"

namespace profilex::cli {
namespace {

int parse_positive_int(const std::string& value, const std::string& flag_name) {
    try {
        const int parsed = std::stoi(value);
        if (parsed <= 0) {
            throw std::invalid_argument("non-positive");
        }
        return parsed;
    } catch (...) {
        throw std::invalid_argument(flag_name + " must be a positive integer");
    }
}

int parse_non_negative_int(const std::string& value, const std::string& flag_name) {
    try {
        const int parsed = std::stoi(value);
        if (parsed < 0) {
            throw std::invalid_argument("negative");
        }
        return parsed;
    } catch (...) {
        throw std::invalid_argument(flag_name + " must be zero or a positive integer");
    }
}

std::vector<std::string> slice(const std::vector<std::string>& values, const std::size_t start) {
    return {values.begin() + static_cast<std::ptrdiff_t>(start), values.end()};
}

}  // namespace

int Application::run(const std::vector<std::string>& args) const {
    if (args.size() < 2U) {
        std::cout << usage();
        return 0;
    }

    const std::string& command = args[1];
    if (command == "run") {
        return run_command(slice(args, 2U));
    }
    if (command == "list") {
        return list_command();
    }
    if (command == "show") {
        return show_command(slice(args, 2U));
    }
    if (command == "compare") {
        return compare_command(slice(args, 2U));
    }
    if (command == "export") {
        return export_command(slice(args, 2U));
    }
    if (command == "delete") {
        return delete_command(slice(args, 2U));
    }

    throw std::invalid_argument("unknown command: " + command);
}

int Application::run_command(const std::vector<std::string>& args) const {
    runner::RunOptions options;
    bool overwrite = false;

    std::size_t index = 0U;
    for (; index < args.size(); ++index) {
        const auto& arg = args[index];
        if (arg == "--") {
            ++index;
            break;
        }
        if (arg == "--name") {
            if (++index >= args.size()) {
                throw std::invalid_argument("--name requires a value");
            }
            options.name = args[index];
            continue;
        }
        if (arg == "--runs") {
            if (++index >= args.size()) {
                throw std::invalid_argument("--runs requires a value");
            }
            options.runs = parse_positive_int(args[index], "--runs");
            continue;
        }
        if (arg == "--warmup") {
            if (++index >= args.size()) {
                throw std::invalid_argument("--warmup requires a value");
            }
            options.warmup = parse_non_negative_int(args[index], "--warmup");
            continue;
        }
        if (arg == "--tag") {
            if (++index >= args.size()) {
                throw std::invalid_argument("--tag requires a value");
            }
            options.tags.push_back(args[index]);
            continue;
        }
        if (arg == "--notes") {
            if (++index >= args.size()) {
                throw std::invalid_argument("--notes requires a value");
            }
            options.notes = args[index];
            continue;
        }
        if (arg == "--overwrite") {
            overwrite = true;
            continue;
        }

        throw std::invalid_argument("unknown run option: " + arg);
    }

    if (options.name.empty()) {
        throw std::invalid_argument("run requires --name");
    }
    if (index >= args.size()) {
        throw std::invalid_argument("run requires a command after --");
    }

    options.command_tokens = slice(args, index);

    runner::CommandRunner runner;
    storage::RunRepository repository;
    stats::StatsEngine stats_engine;
    report::Printer printer;

    const auto record = runner.run(options);
    repository.save(record, overwrite);
    std::cout << printer.format_run_summary(record, stats_engine.summarize(record.samples)) << '\n';
    return 0;
}

int Application::list_command() const {
    storage::RunRepository repository;
    report::Printer printer;
    std::cout << printer.format_run_list(repository.list());
    return 0;
}

int Application::show_command(const std::vector<std::string>& args) const {
    if (args.size() != 1U) {
        throw std::invalid_argument("show requires a run name");
    }

    storage::RunRepository repository;
    stats::StatsEngine stats_engine;
    report::Printer printer;

    const auto record = repository.load(args[0]);
    std::cout << printer.format_run_summary(record, stats_engine.summarize(record.samples)) << '\n';
    return 0;
}

int Application::compare_command(const std::vector<std::string>& args) const {
    if (args.size() != 2U) {
        throw std::invalid_argument("compare requires baseline and candidate names");
    }

    storage::RunRepository repository;
    stats::StatsEngine stats_engine;
    report::Printer printer;

    const auto baseline = repository.load(args[0]);
    const auto candidate = repository.load(args[1]);
    const auto comparison = stats_engine.compare(baseline.samples, candidate.samples);

    std::cout << printer.format_comparison(args[0], args[1], comparison) << '\n';
    return 0;
}

int Application::export_command(const std::vector<std::string>& args) const {
    if (args.empty()) {
        throw std::invalid_argument("export requires a run name");
    }
    if (args.size() != 1U && args.size() != 3U) {
        throw std::invalid_argument("export accepts at most a run name and --format json");
    }
    if (args.size() == 3U && !(args[1] == "--format" && args[2] == "json")) {
        throw std::invalid_argument("only --format json is supported");
    }

    storage::RunRepository repository;
    std::cout << repository.export_json(args[0]);
    return 0;
}

int Application::delete_command(const std::vector<std::string>& args) const {
    if (args.size() != 1U) {
        throw std::invalid_argument("delete requires a run name");
    }

    storage::RunRepository repository;
    repository.remove(args[0]);
    std::cout << "Deleted run '" << args[0] << "'.\n";
    return 0;
}

std::string Application::usage() {
    return
        "ProfileX\n"
        "  profilex run --name <name> [--runs N] [--warmup N] [--tag value] [--notes text] [--overwrite] -- <command>\n"
        "  profilex list\n"
        "  profilex show <name>\n"
        "  profilex compare <baseline> <candidate>\n"
        "  profilex export <name> [--format json]\n"
        "  profilex delete <name>\n";
}

}  // namespace profilex::cli
