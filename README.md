# ProfileX

![Version](https://img.shields.io/badge/version-0.1.0-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B20-00599C)
![Status](https://img.shields.io/badge/status-MVP%20usable-orange)
![License](https://img.shields.io/badge/license-MIT-green)

ProfileX is a local, compare-first CLI for investigating performance changes in C++ workflows.

Its job is narrow on purpose:

- run a benchmarkable command repeatedly
- store raw samples and metadata locally
- compare a baseline against a candidate
- output a conservative verdict:
  `likely improvement`, `likely regression`, or `inconclusive`

This repository is currently in an MVP-but-usable state:

- usable for local, real-project benchmarking now
- tested against synthetic workflows and at least one real external C++ project
- installable from source today
- packageable into a release archive today
- prepared to publish tagged GitHub release artifacts
- not yet polished enough to call a broadly distributed finished product

The product scope is anchored to [AGENT/PRD.md](AGENT/PRD.md). The implementation history so far is documented in [AGENT/PROGRESSION.md](AGENT/PROGRESSION.md).
Release history lives in [CHANGELOG.md](CHANGELOG.md).

## Current State

What works now:

- `profilex run`
- `profilex list`
- `profilex show`
- `profilex compare`
- `profilex export`
- `profilex delete`
- project-scoped run storage in `.perf_runs/`
- summary stats and conservative comparison verdicts
- corruption handling for malformed saved runs
- stress-tested save/load/overwrite/delete flows
- install and package generation with CMake

What is still rough:

- distribution is still source-first unless you build a release archive
- CLI ergonomics are functional, not final
- there is no package-manager integration yet
- the project needs more real-world usage to smooth the UX

## Why This Exists

Many performance checks are still too informal:

- run a command once or twice
- eyeball the result
- forget the raw data
- lose track of exact command lines
- overreact to noise

ProfileX is meant to turn that into a repeatable local workflow.

## Requirements

- CMake 3.24+
- a C++20-capable compiler
- macOS or Linux is the intended early target

On this machine, the project was verified with Apple Clang and C++20.

## Quick Start

The lowest-friction source install path is:

```sh
git clone <your-profilex-repo-url>
cd profilex
./scripts/install_from_source.sh
```

If `~/.local/bin` is on your `PATH`, you can then run:

```sh
profilex --version
profilex help
```

If you prefer manual steps:

```sh
cmake -S . -B .build
cmake --build .build
cmake --install .build --prefix "$HOME/.local"
```

## Installation Options

### 1. Install From Source

One-command install:

```sh
./scripts/install_from_source.sh
```

Custom prefix:

```sh
./scripts/install_from_source.sh "$HOME/tools/profilex"
```

### 2. Build A Release Archive

Generate a package:

```sh
./scripts/build_release_artifact.sh
```

This produces a `.tgz` archive in `.build/`.

If you tag a release in GitHub, the workflow in `.github/workflows/release.yml` is set up to build and publish release artifacts for macOS and Linux.
Until you publish those tagged releases, users still need either the source tree or a locally built archive.

### 3. Install From A Release Archive

If you already have a packaged artifact:

```sh
./scripts/install_from_package.sh .build/profilex-0.1.0-Darwin-arm64.tar.gz
```

Custom prefix:

```sh
./scripts/install_from_package.sh .build/profilex-0.1.0-Darwin-arm64.tar.gz "$HOME/.local"
```

### 4. Clean Release Validation

To validate a clean source tree build, test, install, and package workflow:

```sh
./scripts/clean_release_validate.sh
```

More detail is in [docs/release-install.md](docs/release-install.md).

## Commands

### `run`

Runs a command repeatedly and saves the result.

```sh
profilex run --name baseline --runs 10 --warmup 2 -- ./bench_parser input.json
```

Supported options:

- `--name <run-name>` required
- `--runs <count>` default `10`
- `--warmup <count>` default `2`
- `--tag <value>` repeatable
- `--notes <text>`
- `--overwrite`

Global flags:

- `--help`
- `--version`

### `list`

Lists saved runs in the current project:

```sh
profilex list
```

### `show`

Shows metadata and summary stats for a saved run:

```sh
profilex show baseline
```

### `compare`

Compares a baseline and a candidate:

```sh
profilex compare baseline candidate
```

### `export`

Exports a saved run as JSON to stdout:

```sh
profilex export baseline --format json
```

### `delete`

Deletes a saved run:

```sh
profilex delete baseline
```

## Storage Model

ProfileX stores results relative to the directory where you execute it.

Example:

```sh
cd /path/to/my-project
profilex run --name baseline --runs 10 --warmup 2 -- ./build/my_bench
```

That creates:

```sh
/path/to/my-project/.perf_runs/
```

This is intentional. It keeps benchmark history project-scoped instead of scattering it globally.

## Real Usage Pattern

Build or install ProfileX once, then use it from inside another project.

Example:

```sh
cd /path/to/my-project
profilex run --name baseline --runs 10 --warmup 2 -- ./build/bench_parser input.json
profilex run --name candidate --runs 10 --warmup 2 -- ./build/bench_parser input.json
profilex compare baseline candidate
```

The command after `--` is the thing being benchmarked. ProfileX itself is not the workload.

## Example Project

A tiny stand-in target project is included under `examples/sample_target/`.

Run the example:

```sh
./scripts/example_target_demo.sh ./.build/profilex
```

Or:

```sh
cmake --build .build --target profilex_demo
```

## Real External Validation

ProfileX was also exercised against a real external C++ project (`simdjson`) outside this repo.

That validated the intended workflow:

- clone/build a separate project
- run `profilex` from inside that project
- store run history in that project’s `.perf_runs/`
- compare two real executable variants

In that test, ProfileX correctly reported an `inconclusive` result when the observed delta was too small to clear the heuristic threshold.

## Testing

Run all tests:

```sh
ctest --test-dir .build --output-on-failure
```

Coverage currently includes:

- stats calculations and verdict logic
- JSON serialization and validation
- malformed and unreadable storage handling
- CLI command parsing
- runner exit-code capture
- signal-forwarding behavior
- finalize-write failure cleanup

## Stress And Robustness Workflows

Basic stress workflow:

```sh
./scripts/stress_test.sh ./.build/profilex
cmake --build .build --target profilex_stress
```

Deeper stress workflow:

```sh
./scripts/deep_stress_test.sh ./.build/profilex
cmake --build .build --target profilex_deep_stress
```

Storage corruption workflow:

```sh
./scripts/storage_corruption_demo.sh ./.build/profilex
cmake --build .build --target profilex_corruption_demo
```

## Release Automation

A release workflow scaffold is included at:

```text
.github/workflows/release.yml
```

It currently:

- builds on macOS and Linux
- runs tests
- generates package artifacts
- uploads release artifacts to the GitHub Actions run

This reduces the gap between “clone and build” and “download a packaged binary,” but it still depends on you wiring the repository to GitHub and using tagged releases.

## Current Verdict Heuristic

The current MVP heuristic is intentionally conservative:

- minimum samples: `5`
- minimum delta magnitude: `3%`
- inconclusive if variance is too large relative to the mean delta

That means ProfileX will often choose `inconclusive` instead of overclaiming.

## Known Limitations

- no profiler integration
- no SQLite backend
- no package-manager distribution yet
- no advanced filtering/grouping yet
- no cross-machine comparison
- no CI benchmarking platform
- no GUI

These are outside the MVP.

## Project Structure

```text
src/
  cli/
  runner/
  storage/
  stats/
  report/
  model/
tests/
examples/
scripts/
AGENT/
```

## Development Notes

The implementation intentionally favors:

- modular boundaries over a monolithic CLI file
- conservative benchmarking conclusions
- explicit failure handling
- straightforward local inspection of stored run data

The current stack includes:

- C++20
- CMake
- `nlohmann/json`
- `doctest`

## License

[MIT](LICENSE)
