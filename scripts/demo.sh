#!/bin/sh

set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 <path-to-profilex-binary>" >&2
    exit 1
fi

BINARY=$1
BASELINE_RUN=demo-baseline
CANDIDATE_RUN=demo-candidate

cleanup() {
    "$BINARY" delete "$BASELINE_RUN" >/dev/null 2>&1 || true
    "$BINARY" delete "$CANDIDATE_RUN" >/dev/null 2>&1 || true
}

trap cleanup EXIT INT TERM

printf '\n[1/6] Running baseline benchmark\n'
"$BINARY" run --name "$BASELINE_RUN" --runs 5 --warmup 1 -- /usr/bin/true

printf '\n[2/6] Running slower candidate benchmark\n'
"$BINARY" run --name "$CANDIDATE_RUN" --runs 5 --warmup 0 -- /bin/sh -c "sleep 0.01"

printf '\n[3/6] Listing saved runs\n'
"$BINARY" list

printf '\n[4/6] Showing baseline summary\n'
"$BINARY" show "$BASELINE_RUN"

printf '\n[5/6] Comparing candidate against baseline\n'
"$BINARY" compare "$BASELINE_RUN" "$CANDIDATE_RUN"

printf '\n[6/6] Exporting baseline as JSON\n'
"$BINARY" export "$BASELINE_RUN" --format json

printf '\nDemo complete. Temporary runs will be deleted automatically.\n'
