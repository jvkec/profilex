# Changelog

All notable changes to this project should be documented in this file.

The format is intentionally simple and release-oriented.

## [Unreleased]

### Added

- Placeholder for upcoming release notes.

## [0.1.0] - 2026-03-28

### Added

- Initial C++20 scaffold for the ProfileX MVP.
- CLI commands for `run`, `list`, `show`, `compare`, `export`, and `delete`.
- Project-scoped JSON-backed run persistence in `.perf_runs/`.
- Summary statistics and conservative comparison verdicts.
- Sample target project and demo workflows.
- Stress, deep-stress, and corruption demo scripts.
- Source install, package build, and package install helper scripts.
- Clean release validation workflow and package generation support.
- Tests for stats, storage, CLI parsing, and runner behavior.
- Progression documentation in `AGENT/PROGRESSION.md`.

### Changed

- Replaced the custom JSON parser with `nlohmann/json`.
- Replaced the ad hoc test harness with `doctest`.
- Split CLI parsing into a dedicated parser module.
- Improved storage error handling for malformed and unreadable saved runs.
- Added process-group signal forwarding for active benchmark commands.

### Notes

- This release is MVP-usable, but still source-first in overall distribution maturity.
- The verdict heuristic intentionally prefers `inconclusive` over overclaiming.
