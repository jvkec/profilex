#include "storage/json_codec.hpp"

#include <stdexcept>

#include <nlohmann/json.hpp>

namespace profilex::storage {
namespace {

model::Sample decode_sample(const nlohmann::json& json) {
    return model::Sample{
        .duration_ms = json.at("duration_ms").get<double>(),
        .exit_code = json.at("exit_code").get<int>(),
    };
}

std::vector<model::Sample> decode_samples(const nlohmann::json& json) {
    std::vector<model::Sample> samples;
    samples.reserve(json.size());
    for (const auto& item : json) {
        samples.push_back(decode_sample(item));
    }
    return samples;
}

nlohmann::json encode_sample(const model::Sample& sample) {
    return nlohmann::json{
        {"duration_ms", sample.duration_ms},
        {"exit_code", sample.exit_code},
    };
}

model::RunRecord decode_run_record(const nlohmann::json& json) {
    model::RunRecord record;
    record.name = json.at("name").get<std::string>();
    record.command = json.at("command").get<std::string>();
    record.created_at_unix = json.at("created_at_unix").get<std::int64_t>();
    record.requested_runs = json.at("requested_runs").get<int>();
    record.warmup_runs = json.at("warmup_runs").get<int>();
    record.tags = json.at("tags").get<std::vector<std::string>>();
    if (json.at("notes").is_null()) {
        record.notes.reset();
    } else {
        record.notes = json.at("notes").get<std::string>();
    }
    record.samples = decode_samples(json.at("samples"));
    return record;
}

nlohmann::json encode_run_record(const model::RunRecord& record) {
    nlohmann::json samples = nlohmann::json::array();
    for (const auto& sample : record.samples) {
        samples.push_back(encode_sample(sample));
    }

    return nlohmann::json{
        {"name", record.name},
        {"command", record.command},
        {"created_at_unix", record.created_at_unix},
        {"requested_runs", record.requested_runs},
        {"warmup_runs", record.warmup_runs},
        {"tags", record.tags},
        {"notes", record.notes.has_value() ? nlohmann::json(*record.notes) : nlohmann::json(nullptr)},
        {"samples", std::move(samples)},
    };
}

}  // namespace

void validate_run_record(const model::RunRecord& record) {
    if (record.name.empty()) {
        throw std::runtime_error("run record is missing a name");
    }
    if (record.command.empty()) {
        throw std::runtime_error("run record is missing a command");
    }
    if (record.requested_runs <= 0) {
        throw std::runtime_error("requested_runs must be greater than zero");
    }
    if (record.warmup_runs < 0) {
        throw std::runtime_error("warmup_runs must be zero or greater");
    }
    if (record.samples.empty()) {
        throw std::runtime_error("run record must contain at least one sample");
    }
    if (static_cast<int>(record.samples.size()) != record.requested_runs) {
        throw std::runtime_error("sample count does not match requested_runs");
    }
    for (const auto& sample : record.samples) {
        if (sample.duration_ms < 0.0) {
            throw std::runtime_error("sample duration_ms must be non-negative");
        }
    }
}

std::string serialize_run_record(const model::RunRecord& record) {
    validate_run_record(record);
    return encode_run_record(record).dump(2) + '\n';
}

model::RunRecord deserialize_run_record(const std::string& json_text) {
    try {
        auto json = nlohmann::json::parse(json_text);
        auto record = decode_run_record(json);
        validate_run_record(record);
        return record;
    } catch (const nlohmann::json::exception& ex) {
        throw std::runtime_error(ex.what());
    }
}

}  // namespace profilex::storage
