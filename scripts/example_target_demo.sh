#!/bin/sh

set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 <path-to-profilex-binary>" >&2
    exit 1
fi

PROFILEX_BINARY=$1
case "$PROFILEX_BINARY" in
    /*) ;;
    *) PROFILEX_BINARY=$(CDPATH= cd -- "$(dirname -- "$PROFILEX_BINARY")" && pwd)/$(basename -- "$PROFILEX_BINARY") ;;
esac
EXAMPLE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../examples/sample_target" && pwd)
EXAMPLE_BUILD_DIR="$EXAMPLE_DIR/.build"
BASELINE_RUN=example-baseline
CANDIDATE_RUN=example-candidate

cleanup() {
    cd "$EXAMPLE_DIR"
    "$PROFILEX_BINARY" delete "$BASELINE_RUN" >/dev/null 2>&1 || true
    "$PROFILEX_BINARY" delete "$CANDIDATE_RUN" >/dev/null 2>&1 || true
}

trap cleanup EXIT INT TERM

printf '\n[1/7] Building the sample target project\n'
cmake -S "$EXAMPLE_DIR" -B "$EXAMPLE_BUILD_DIR"
cmake --build "$EXAMPLE_BUILD_DIR"

cd "$EXAMPLE_DIR"

printf '\n[2/7] Running the sample target directly\n'
"$EXAMPLE_BUILD_DIR/sample_bench" 400000

printf '\n[3/7] Measuring a baseline run with ProfileX\n'
"$PROFILEX_BINARY" run --name "$BASELINE_RUN" --runs 6 --warmup 2 -- \
    "$EXAMPLE_BUILD_DIR/sample_bench" 400000

printf '\n[4/7] Measuring a slower candidate with more work\n'
"$PROFILEX_BINARY" run --name "$CANDIDATE_RUN" --runs 6 --warmup 2 -- \
    "$EXAMPLE_BUILD_DIR/sample_bench" 700000

printf '\n[5/7] Listing saved runs in the sample target project\n'
"$PROFILEX_BINARY" list

printf '\n[6/7] Comparing the candidate to the baseline\n'
"$PROFILEX_BINARY" compare "$BASELINE_RUN" "$CANDIDATE_RUN"

printf '\n[7/7] Exporting the baseline JSON\n'
"$PROFILEX_BINARY" export "$BASELINE_RUN" --format json

printf '\nExample complete. Runs are stored under %s/.perf_runs while the script is running.\n' "$EXAMPLE_DIR"
