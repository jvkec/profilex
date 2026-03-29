#pragma once

#include <string>

#include "model/types.hpp"

namespace profilex::storage {

std::string serialize_run_record(const model::RunRecord& record);
model::RunRecord deserialize_run_record(const std::string& json_text);

}  // namespace profilex::storage
