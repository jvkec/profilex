#include <filesystem>
#include <fstream>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "model/types.hpp"
#include "storage/json_codec.hpp"
#include "storage/run_repository.hpp"

namespace {

profilex::model::RunRecord make_record() {
    return profilex::model::RunRecord{
        .name = "baseline-run",
        .command = "./bench_parser input.json",
        .created_at_unix = 1774700000,
        .requested_runs = 3,
        .warmup_runs = 1,
        .tags = {"parser", "json"},
        .notes = std::string("note with \"quotes\" and\nnewline"),
        .samples = {
            profilex::model::Sample{.duration_ms = 12.5, .exit_code = 0},
            profilex::model::Sample{.duration_ms = 12.0, .exit_code = 0},
            profilex::model::Sample{.duration_ms = 12.2, .exit_code = 0},
        },
    };
}

void write_text_file(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream out(path, std::ios::trunc);
    REQUIRE(out.good());
    out << contents;
    REQUIRE(out.good());
}

TEST_CASE("RunRecord JSON round-trips") {
    const auto original = make_record();
    const std::string serialized = profilex::storage::serialize_run_record(original);
    const auto parsed = profilex::storage::deserialize_run_record(serialized);

    CHECK(parsed.name == original.name);
    CHECK(parsed.command == original.command);
    CHECK(parsed.created_at_unix == original.created_at_unix);
    CHECK(parsed.requested_runs == original.requested_runs);
    CHECK(parsed.warmup_runs == original.warmup_runs);
    CHECK(parsed.tags == original.tags);
    CHECK(parsed.notes == original.notes);
    REQUIRE(parsed.samples.size() == original.samples.size());
    CHECK(doctest::Approx(parsed.samples.front().duration_ms) == 12.5);
}

TEST_CASE("Invalid run records are rejected during decode") {
    const std::string invalid_json =
        R"({"name":"bad","command":"cmd","created_at_unix":1,"requested_runs":1,"warmup_runs":0,"tags":[],"notes":null,"samples":[]})";

    CHECK_THROWS_AS(profilex::storage::deserialize_run_record(invalid_json), std::runtime_error);
}

TEST_CASE("Repository save, load, and overwrite work") {
    const auto root =
        std::filesystem::temp_directory_path() / "profilex_storage_tests_save_load";
    std::filesystem::remove_all(root);

    profilex::storage::RunRepository repository(root);
    auto record = make_record();
    repository.save(record, false);

    const auto loaded = repository.load(record.name);
    CHECK(loaded.name == record.name);

    CHECK_THROWS_AS(repository.save(record, false), std::runtime_error);

    record.notes = "updated";
    repository.save(record, true);
    const auto overwritten = repository.load(record.name);
    CHECK(overwritten.notes == record.notes);

    std::filesystem::remove_all(root);
}

TEST_CASE("Repository rejects unsafe run names") {
    const auto root =
        std::filesystem::temp_directory_path() / "profilex_storage_tests_invalid_names";
    std::filesystem::remove_all(root);

    profilex::storage::RunRepository repository(root);
    auto record = make_record();
    record.name = "../escape";

    CHECK_THROWS_AS(repository.save(record, false), std::invalid_argument);

    std::filesystem::remove_all(root);
}

TEST_CASE("Repository delete removes a run") {
    const auto root =
        std::filesystem::temp_directory_path() / "profilex_storage_tests_delete";
    std::filesystem::remove_all(root);

    profilex::storage::RunRepository repository(root);
    const auto record = make_record();
    repository.save(record, false);
    repository.remove(record.name);

    CHECK_THROWS_AS(repository.load(record.name), std::runtime_error);

    std::filesystem::remove_all(root);
}

TEST_CASE("Repository reports unreadable files clearly") {
    const auto root =
        std::filesystem::temp_directory_path() / "profilex_storage_tests_unreadable";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    write_text_file(root / "sealed.json",
                    R"({"name":"sealed","command":"cmd","created_at_unix":1,"requested_runs":1,"warmup_runs":0,"tags":[],"notes":null,"samples":[{"duration_ms":1.0,"exit_code":0}]})");
    std::filesystem::permissions(root / "sealed.json", std::filesystem::perms::none);

    profilex::storage::RunRepository repository(root);
    try {
        CHECK_THROWS_WITH_AS(repository.load("sealed"),
                             doctest::Contains("failed to open saved run for reading: sealed"),
                             std::runtime_error);
    } catch (...) {
        std::filesystem::permissions(root / "sealed.json",
                                     std::filesystem::perms::owner_read |
                                         std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::replace);
        std::filesystem::remove_all(root);
        throw;
    }

    std::filesystem::permissions(root / "sealed.json",
                                 std::filesystem::perms::owner_read |
                                     std::filesystem::perms::owner_write,
                                 std::filesystem::perm_options::replace);
    std::filesystem::remove_all(root);
}

TEST_CASE("Repository reports malformed stored JSON clearly") {
    const auto root =
        std::filesystem::temp_directory_path() / "profilex_storage_tests_malformed";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto path = root / "broken.json";
    {
        std::ofstream out(path);
        out << R"({"name":"broken","command":"cmd","created_at_unix":1,"requested_runs":2,"warmup_runs":0,"tags":[],"notes":null,"samples":[{"duration_ms":1.0,"exit_code":0}]})";
    }

    profilex::storage::RunRepository repository(root);

    CHECK_THROWS_WITH_AS(repository.load("broken"),
                         doctest::Contains("malformed stored JSON for run 'broken'"),
                         std::runtime_error);

    std::filesystem::remove_all(root);
}

TEST_CASE("Repository list surfaces malformed entries") {
    const auto root =
        std::filesystem::temp_directory_path() / "profilex_storage_tests_list_malformed";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto good_record = make_record();
    profilex::storage::RunRepository repository(root);
    repository.save(good_record, false);

    {
        std::ofstream out(root / "bad.json");
        out << "{not valid json";
    }

    CHECK_THROWS_WITH_AS(repository.list(),
                         doctest::Contains("malformed stored JSON for run 'bad'"),
                         std::runtime_error);

    std::filesystem::remove_all(root);
}

TEST_CASE("Repository cleans up temp files on simulated finalize failure") {
    const auto root =
        std::filesystem::temp_directory_path() / "profilex_storage_tests_finalize_failure";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    profilex::storage::RunRepository repository(root);
    auto record = make_record();
    repository.save(record, false);

    record.notes = "new notes";
    setenv("PROFILEX_SIMULATE_FINALIZE_FAILURE", "1", 1);
    CHECK_THROWS_WITH_AS(repository.save(record, true),
                         doctest::Contains("simulated failure while finalizing run write"),
                         std::runtime_error);
    unsetenv("PROFILEX_SIMULATE_FINALIZE_FAILURE");

    CHECK_FALSE(std::filesystem::exists(root / "baseline-run.json.tmp"));

    const auto persisted = repository.load(record.name);
    CHECK(persisted.notes == make_record().notes);

    std::filesystem::remove_all(root);
}

}  // namespace
