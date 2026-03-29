#include "storage/run_repository.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "storage/json_codec.hpp"

namespace profilex::storage {
namespace {

bool simulate_finalize_failure() {
    const char* value = std::getenv("PROFILEX_SIMULATE_FINALIZE_FAILURE");
    return value != nullptr && std::string_view(value) == "1";
}

}  // namespace

RunRepository::RunRepository(std::filesystem::path root) : root_(std::move(root)) {}

void RunRepository::save(const model::RunRecord& record, const bool overwrite) {
    validate_run_name(record.name);
    validate_run_record(record);

    ensure_root_exists();
    const auto path = path_for(record.name);
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        throw std::runtime_error("failed to inspect existing run file: " + error.message());
    }
    if (exists && !overwrite) {
        throw std::runtime_error("run already exists; rerun with --overwrite to replace it");
    }

    const auto temp_path = path.string() + ".tmp";
    {
        std::ofstream out(temp_path, std::ios::trunc);
        if (!out) {
            throw std::runtime_error("failed to open temp file for writing");
        }
        out << serialize_run_record(record);
        if (!out.good()) {
            out.close();
            std::remove(temp_path.c_str());
            throw std::runtime_error("failed while writing run record");
        }
    }

    if (simulate_finalize_failure()) {
        std::remove(temp_path.c_str());
        throw std::runtime_error("simulated failure while finalizing run write");
    }

    std::filesystem::rename(temp_path, path, error);
    if (error) {
        std::remove(temp_path.c_str());
        throw std::runtime_error("failed to finalize run write: " + error.message());
    }
}

model::RunRecord RunRepository::load(const std::string& name) const {
    validate_run_name(name);
    const auto path = path_for(name);
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        throw std::runtime_error("failed to inspect saved run: " + error.message());
    }
    if (!exists) {
        throw std::runtime_error("saved run not found: " + name);
    }

    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open saved run for reading: " + name);
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    if (!in.good() && !in.eof()) {
        throw std::runtime_error("failed while reading saved run: " + name);
    }

    try {
        return deserialize_run_record(buffer.str());
    } catch (const std::exception& ex) {
        throw std::runtime_error("malformed stored JSON for run '" + name + "': " + ex.what());
    }
}

std::vector<model::RunRecord> RunRepository::list() const {
    std::vector<model::RunRecord> runs;
    std::error_code error;
    if (!std::filesystem::exists(root_, error)) {
        if (error) {
            throw std::runtime_error("failed to inspect storage directory: " + error.message());
        }
        return runs;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root_, error)) {
        if (error) {
            throw std::runtime_error("failed to iterate storage directory: " + error.message());
        }
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        runs.push_back(load(entry.path().stem().string()));
    }

    std::sort(runs.begin(), runs.end(), [](const auto& left, const auto& right) {
        return left.created_at_unix > right.created_at_unix;
    });

    return runs;
}

void RunRepository::remove(const std::string& name) const {
    validate_run_name(name);
    const auto path = path_for(name);
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        if (error) {
            throw std::runtime_error("failed to inspect saved run: " + error.message());
        }
        throw std::runtime_error("saved run not found: " + name);
    }

    std::filesystem::remove(path, error);
    if (error) {
        throw std::runtime_error("failed to delete saved run: " + error.message());
    }
}

std::string RunRepository::export_json(const std::string& name) const {
    return serialize_run_record(load(name));
}

std::filesystem::path RunRepository::path_for(const std::string& name) const {
    validate_run_name(name);
    return root_ / (name + ".json");
}

void RunRepository::ensure_root_exists() const {
    std::error_code error;
    std::filesystem::create_directories(root_, error);
    if (error) {
        throw std::runtime_error("failed to create storage directory: " + error.message());
    }
}

void RunRepository::validate_run_name(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("run name must not be empty");
    }

    for (const char ch : name) {
        const bool allowed = std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '-' ||
                             ch == '_' || ch == '.';
        if (!allowed) {
            throw std::invalid_argument(
                "run name may only contain letters, numbers, '.', '-', and '_'");
        }
    }
}

}  // namespace profilex::storage
