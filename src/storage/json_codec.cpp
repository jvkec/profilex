#include "storage/json_codec.hpp"

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>

namespace profilex::storage {
namespace {

using JsonValue = std::variant<std::nullptr_t,
                               bool,
                               double,
                               std::string,
                               std::vector<std::string>,
                               std::vector<model::Sample>,
                               std::map<std::string, std::variant<std::nullptr_t,
                                                                  bool,
                                                                  double,
                                                                  std::string,
                                                                  std::vector<std::string>,
                                                                  std::vector<model::Sample>,
                                                                  std::map<std::string, std::variant<std::nullptr_t,
                                                                                                     bool,
                                                                                                     double,
                                                                                                     std::string,
                                                                                                     std::vector<std::string>,
                                                                                                     std::vector<model::Sample>>>>>>;

std::string escape_json(std::string_view input) {
    std::string out;
    for (const char ch : input) {
        switch (ch) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out.push_back(ch);
                break;
        }
    }
    return out;
}

class Parser {
  public:
    explicit Parser(std::string_view input) : input_(input) {}

    model::RunRecord parse_run_record() {
        expect('{');
        model::RunRecord record;
        bool first = true;

        while (true) {
            skip_ws();
            if (consume('}')) {
                break;
            }
            if (!first) {
                expect(',');
            }
            first = false;

            const std::string key = parse_string();
            expect(':');

            if (key == "name") {
                record.name = parse_string();
            } else if (key == "command") {
                record.command = parse_string();
            } else if (key == "created_at_unix") {
                record.created_at_unix = static_cast<std::int64_t>(parse_number());
            } else if (key == "requested_runs") {
                record.requested_runs = static_cast<int>(parse_number());
            } else if (key == "warmup_runs") {
                record.warmup_runs = static_cast<int>(parse_number());
            } else if (key == "tags") {
                record.tags = parse_string_array();
            } else if (key == "notes") {
                if (peek() == 'n') {
                    expect_literal("null");
                    record.notes.reset();
                } else {
                    record.notes = parse_string();
                }
            } else if (key == "samples") {
                record.samples = parse_samples();
            } else {
                throw std::runtime_error("unexpected key in run record JSON: " + key);
            }
        }

        return record;
    }

  private:
    char peek() {
        skip_ws();
        if (position_ >= input_.size()) {
            throw std::runtime_error("unexpected end of JSON");
        }
        return input_[position_];
    }

    void skip_ws() {
        while (position_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position_])) != 0) {
            ++position_;
        }
    }

    bool consume(const char expected) {
        skip_ws();
        if (position_ < input_.size() && input_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    void expect(const char expected) {
        if (!consume(expected)) {
            throw std::runtime_error(std::string("expected '") + expected + "'");
        }
    }

    void expect_literal(std::string_view literal) {
        skip_ws();
        if (input_.substr(position_, literal.size()) != literal) {
            throw std::runtime_error("unexpected token in JSON");
        }
        position_ += literal.size();
    }

    std::string parse_string() {
        skip_ws();
        expect('"');

        std::string value;
        while (position_ < input_.size()) {
            const char ch = input_[position_++];
            if (ch == '"') {
                return value;
            }
            if (ch == '\\') {
                if (position_ >= input_.size()) {
                    throw std::runtime_error("incomplete escape sequence");
                }
                const char escaped = input_[position_++];
                switch (escaped) {
                    case '\\':
                    case '"':
                    case '/':
                        value.push_back(escaped);
                        break;
                    case 'n':
                        value.push_back('\n');
                        break;
                    case 'r':
                        value.push_back('\r');
                        break;
                    case 't':
                        value.push_back('\t');
                        break;
                    default:
                        throw std::runtime_error("unsupported escape sequence");
                }
                continue;
            }
            value.push_back(ch);
        }

        throw std::runtime_error("unterminated JSON string");
    }

    double parse_number() {
        skip_ws();
        const std::size_t start = position_;
        while (position_ < input_.size()) {
            const char ch = input_[position_];
            if (std::isdigit(static_cast<unsigned char>(ch)) != 0 || ch == '-' || ch == '+' ||
                ch == '.' || ch == 'e' || ch == 'E') {
                ++position_;
                continue;
            }
            break;
        }

        const std::string_view token = input_.substr(start, position_ - start);
        if (token.empty()) {
            throw std::runtime_error("expected JSON number");
        }

        char* end = nullptr;
        const double value = std::strtod(token.data(), &end);
        if (end == token.data()) {
            throw std::runtime_error("invalid JSON number");
        }
        return value;
    }

    std::vector<std::string> parse_string_array() {
        expect('[');
        std::vector<std::string> values;
        bool first = true;
        while (true) {
            skip_ws();
            if (consume(']')) {
                break;
            }
            if (!first) {
                expect(',');
            }
            first = false;
            values.push_back(parse_string());
        }
        return values;
    }

    std::vector<model::Sample> parse_samples() {
        expect('[');
        std::vector<model::Sample> values;
        bool first = true;

        while (true) {
            skip_ws();
            if (consume(']')) {
                break;
            }
            if (!first) {
                expect(',');
            }
            first = false;

            expect('{');
            model::Sample sample;
            bool object_first = true;
            while (true) {
                skip_ws();
                if (consume('}')) {
                    break;
                }
                if (!object_first) {
                    expect(',');
                }
                object_first = false;

                const std::string key = parse_string();
                expect(':');

                if (key == "duration_ms") {
                    sample.duration_ms = parse_number();
                } else if (key == "exit_code") {
                    sample.exit_code = static_cast<int>(parse_number());
                } else {
                    throw std::runtime_error("unexpected key in sample JSON: " + key);
                }
            }
            values.push_back(sample);
        }

        return values;
    }

    std::string_view input_;
    std::size_t position_ {};
};

}  // namespace

std::string serialize_run_record(const model::RunRecord& record) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"name\": \"" << escape_json(record.name) << "\",\n";
    out << "  \"command\": \"" << escape_json(record.command) << "\",\n";
    out << "  \"created_at_unix\": " << record.created_at_unix << ",\n";
    out << "  \"requested_runs\": " << record.requested_runs << ",\n";
    out << "  \"warmup_runs\": " << record.warmup_runs << ",\n";
    out << "  \"tags\": [";
    for (std::size_t index = 0; index < record.tags.size(); ++index) {
        if (index != 0U) {
            out << ", ";
        }
        out << "\"" << escape_json(record.tags[index]) << "\"";
    }
    out << "],\n";
    out << "  \"notes\": ";
    if (record.notes.has_value()) {
        out << "\"" << escape_json(*record.notes) << "\"";
    } else {
        out << "null";
    }
    out << ",\n";
    out << "  \"samples\": [\n";
    for (std::size_t index = 0; index < record.samples.size(); ++index) {
        const auto& sample = record.samples[index];
        out << "    {\"duration_ms\": " << sample.duration_ms << ", \"exit_code\": "
            << sample.exit_code << "}";
        if (index + 1U != record.samples.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

model::RunRecord deserialize_run_record(const std::string& json_text) {
    Parser parser(json_text);
    return parser.parse_run_record();
}

}  // namespace profilex::storage
