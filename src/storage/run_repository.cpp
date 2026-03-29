#include "storage/run_repository.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "storage/json_codec.hpp"

namespace profilex::storage {

RunRepository::RunRepository(std::filesystem::path root) : root_(std::move(root)) {}

void RunRepository::save(const model::RunRecord& record, const bool overwrite) {
    if (record.name.empty()) {
        throw std::invalid_argument("run name must not be empty");
    }

    ensure_root_exists();
    const auto path = path_for(record.name);
    if (std::filesystem::exists(path) && !overwrite) {
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
            throw std::runtime_error("failed while writing run record");
        }
    }

    std::filesystem::rename(temp_path, path);
}

model::RunRecord RunRepository::load(const std::string& name) const {
    const auto path = path_for(name);
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("saved run not found: " + name);
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
    if (!std::filesystem::exists(root_)) {
        return runs;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root_)) {
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
    const auto path = path_for(name);
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("saved run not found: " + name);
    }

    std::filesystem::remove(path);
}

std::string RunRepository::export_json(const std::string& name) const {
    return serialize_run_record(load(name));
}

std::filesystem::path RunRepository::path_for(const std::string& name) const {
    return root_ / (name + ".json");
}

void RunRepository::ensure_root_exists() const {
    std::error_code error;
    std::filesystem::create_directories(root_, error);
    if (error) {
        throw std::runtime_error("failed to create storage directory: " + error.message());
    }
}

}  // namespace profilex::storage
