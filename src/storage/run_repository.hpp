#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "model/types.hpp"

namespace profilex::storage {

class RunRepository {
  public:
    explicit RunRepository(std::filesystem::path root = ".perf_runs");

    void save(const model::RunRecord& record, bool overwrite);
    model::RunRecord load(const std::string& name) const;
    std::vector<model::RunRecord> list() const;
    void remove(const std::string& name) const;
    std::string export_json(const std::string& name) const;

  private:
    std::filesystem::path path_for(const std::string& name) const;
    void ensure_root_exists() const;

    std::filesystem::path root_;
};

}  // namespace profilex::storage
