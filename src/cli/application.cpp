#include "cli/application.hpp"

#include <iostream>
#include <stdexcept>

#include "cli/command_parser.hpp"
#include "report/printer.hpp"
#include "runner/command_runner.hpp"
#include "stats/stats_engine.hpp"
#include "storage/run_repository.hpp"

namespace profilex::cli {

int Application::run(const std::vector<std::string>& args) const {
    CommandParser parser;
    const ParsedCommand command = parser.parse(args);

    if (command.type == CommandType::help) {
        std::cout << usage();
        return 0;
    }
    if (command.type == CommandType::version) {
        std::cout << "profilex " << PROFILEX_VERSION << '\n';
        return 0;
    }

    switch (command.type) {
        case CommandType::help:
            std::cout << usage();
            return 0;
        case CommandType::version:
            std::cout << "profilex " << PROFILEX_VERSION << '\n';
            return 0;
        case CommandType::run:
            return run_command(command);
        case CommandType::list:
            return list_command(command);
        case CommandType::show:
            return show_command(command);
        case CommandType::compare:
            return compare_command(command);
        case CommandType::export_run:
            return export_command(command);
        case CommandType::delete_run:
            return delete_command(command);
    }

    throw std::runtime_error("unreachable command state");
}

std::string Application::usage() {
    return
        "ProfileX\n"
        "  profilex --version\n"
        "  profilex run --name <name> [--runs N] [--warmup N] [--tag value] [--notes text] [--overwrite] -- <command>\n"
        "  profilex list\n"
        "  profilex show <name>\n"
        "  profilex compare <baseline> <candidate>\n"
        "  profilex export <name> [--format json]\n"
        "  profilex delete <name>\n";
}

int Application::run_command(const ParsedCommand& command) const {
    runner::CommandRunner runner;
    storage::RunRepository repository;
    stats::StatsEngine stats_engine;
    report::Printer printer;

    const auto record = runner.run(command.run_options);
    repository.save(record, command.overwrite);
    std::cout << printer.format_run_summary(record, stats_engine.summarize(record.samples)) << '\n';
    return 0;
}

int Application::list_command(const ParsedCommand&) const {
    storage::RunRepository repository;
    report::Printer printer;
    std::cout << printer.format_run_list(repository.list());
    return 0;
}

int Application::show_command(const ParsedCommand& command) const {
    storage::RunRepository repository;
    stats::StatsEngine stats_engine;
    report::Printer printer;

    const auto record = repository.load(command.run_name);
    std::cout << printer.format_run_summary(record, stats_engine.summarize(record.samples)) << '\n';
    return 0;
}

int Application::compare_command(const ParsedCommand& command) const {
    storage::RunRepository repository;
    stats::StatsEngine stats_engine;
    report::Printer printer;

    const auto baseline = repository.load(command.baseline_name);
    const auto candidate = repository.load(command.candidate_name);
    const auto comparison = stats_engine.compare(baseline.samples, candidate.samples);

    std::cout << printer.format_comparison(command.baseline_name, command.candidate_name, comparison)
              << '\n';
    return 0;
}

int Application::export_command(const ParsedCommand& command) const {
    storage::RunRepository repository;
    std::cout << repository.export_json(command.run_name);
    return 0;
}

int Application::delete_command(const ParsedCommand& command) const {
    storage::RunRepository repository;
    repository.remove(command.run_name);
    std::cout << "Deleted run '" << command.run_name << "'.\n";
    return 0;
}

}  // namespace profilex::cli
