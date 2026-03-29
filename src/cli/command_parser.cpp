#include "cli/command_parser.hpp"

#include <stdexcept>

namespace profilex::cli {

ParsedCommand CommandParser::parse(const std::vector<std::string>& args) const {
    if (args.size() < 2U) {
        return ParsedCommand{.type = CommandType::help};
    }

    const std::string& command = args[1];
    if (command == "help" || command == "--help" || command == "-h") {
        return ParsedCommand{.type = CommandType::help};
    }
    if (command == "run") {
        return parse_run(slice(args, 2U));
    }
    if (command == "list") {
        return ParsedCommand{.type = CommandType::list};
    }
    if (command == "show") {
        return parse_show(slice(args, 2U));
    }
    if (command == "compare") {
        return parse_compare(slice(args, 2U));
    }
    if (command == "export") {
        return parse_export(slice(args, 2U));
    }
    if (command == "delete") {
        return parse_delete(slice(args, 2U));
    }

    throw std::invalid_argument("unknown command: " + command);
}

int CommandParser::parse_positive_int(const std::string& value, const std::string& flag_name) {
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

int CommandParser::parse_non_negative_int(const std::string& value, const std::string& flag_name) {
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

std::vector<std::string> CommandParser::slice(const std::vector<std::string>& values,
                                              const std::size_t start) {
    return {values.begin() + static_cast<std::ptrdiff_t>(start), values.end()};
}

ParsedCommand CommandParser::parse_run(const std::vector<std::string>& args) {
    ParsedCommand parsed{.type = CommandType::run};

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
            parsed.run_options.name = args[index];
            continue;
        }
        if (arg == "--runs") {
            if (++index >= args.size()) {
                throw std::invalid_argument("--runs requires a value");
            }
            parsed.run_options.runs = parse_positive_int(args[index], "--runs");
            continue;
        }
        if (arg == "--warmup") {
            if (++index >= args.size()) {
                throw std::invalid_argument("--warmup requires a value");
            }
            parsed.run_options.warmup = parse_non_negative_int(args[index], "--warmup");
            continue;
        }
        if (arg == "--tag") {
            if (++index >= args.size()) {
                throw std::invalid_argument("--tag requires a value");
            }
            parsed.run_options.tags.push_back(args[index]);
            continue;
        }
        if (arg == "--notes") {
            if (++index >= args.size()) {
                throw std::invalid_argument("--notes requires a value");
            }
            parsed.run_options.notes = args[index];
            continue;
        }
        if (arg == "--overwrite") {
            parsed.overwrite = true;
            continue;
        }

        throw std::invalid_argument("unknown run option: " + arg);
    }

    if (parsed.run_options.name.empty()) {
        throw std::invalid_argument("run requires --name");
    }
    if (index >= args.size()) {
        throw std::invalid_argument("run requires a command after --");
    }

    parsed.run_options.command_tokens = slice(args, index);
    return parsed;
}

ParsedCommand CommandParser::parse_show(const std::vector<std::string>& args) {
    if (args.size() != 1U) {
        throw std::invalid_argument("show requires a run name");
    }

    return ParsedCommand{
        .type = CommandType::show,
        .run_name = args[0],
    };
}

ParsedCommand CommandParser::parse_compare(const std::vector<std::string>& args) {
    if (args.size() != 2U) {
        throw std::invalid_argument("compare requires baseline and candidate names");
    }

    return ParsedCommand{
        .type = CommandType::compare,
        .baseline_name = args[0],
        .candidate_name = args[1],
    };
}

ParsedCommand CommandParser::parse_export(const std::vector<std::string>& args) {
    if (args.empty()) {
        throw std::invalid_argument("export requires a run name");
    }
    if (args.size() != 1U && args.size() != 3U) {
        throw std::invalid_argument("export accepts at most a run name and --format json");
    }
    if (args.size() == 3U && !(args[1] == "--format" && args[2] == "json")) {
        throw std::invalid_argument("only --format json is supported");
    }

    return ParsedCommand{
        .type = CommandType::export_run,
        .run_name = args[0],
    };
}

ParsedCommand CommandParser::parse_delete(const std::vector<std::string>& args) {
    if (args.size() != 1U) {
        throw std::invalid_argument("delete requires a run name");
    }

    return ParsedCommand{
        .type = CommandType::delete_run,
        .run_name = args[0],
    };
}

}  // namespace profilex::cli
