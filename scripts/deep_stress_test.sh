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

WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/profilex-deep-stress.XXXXXX")
HISTORY_RUNS=60
SAMPLES=30

cleanup() {
    rm -rf "$WORK_DIR"
}

trap cleanup EXIT INT TERM

printf 'Deep stress workspace: %s\n' "$WORK_DIR"
cd "$WORK_DIR"

printf '[1/7] Creating a larger run history (%s runs, %s samples each)\n' "$HISTORY_RUNS" "$SAMPLES"
i=1
while [ "$i" -le "$HISTORY_RUNS" ]; do
    "$PROFILEX_BINARY" run --name "bulk-$i" --runs "$SAMPLES" --warmup 2 -- /usr/bin/true >/dev/null
    i=$((i + 1))
done

printf '[2/7] Verifying list/show/export on the larger history\n'
"$PROFILEX_BINARY" list >/dev/null
"$PROFILEX_BINARY" show bulk-1 >/dev/null
"$PROFILEX_BINARY" export bulk-1 --format json >/dev/null

printf '[3/7] Capturing a run with non-zero exit codes\n'
"$PROFILEX_BINARY" run --name failing --runs 6 --warmup 1 -- /bin/sh -c "exit 7" >/dev/null
"$PROFILEX_BINARY" export failing --format json > failing.json
grep -q '"exit_code": 7' failing.json

printf '[4/7] Capturing mixed-duration shell commands\n'
"$PROFILEX_BINARY" run --name sleepy --runs 8 --warmup 1 -- /bin/sh -c "sleep 0.002" >/dev/null
"$PROFILEX_BINARY" compare bulk-1 sleepy >/dev/null || true

printf '[5/7] Injecting malformed and unreadable stored JSON and verifying failure\n'
printf '{broken json\n' > .perf_runs/corrupt.json
if "$PROFILEX_BINARY" show corrupt >/dev/null 2> corrupt.stderr; then
    echo "expected malformed JSON lookup to fail" >&2
    exit 1
fi
grep -q "malformed stored JSON for run 'corrupt'" corrupt.stderr
printf '{"name":"sealed","command":"cmd","created_at_unix":1,"requested_runs":1,"warmup_runs":0,"tags":[],"notes":null,"samples":[{"duration_ms":1.0,"exit_code":0}]}\n' > .perf_runs/sealed.json
chmod 000 .perf_runs/sealed.json
if "$PROFILEX_BINARY" show sealed >/dev/null 2> sealed.stderr; then
    echo "expected unreadable file lookup to fail" >&2
    exit 1
fi
grep -q "failed to open saved run for reading: sealed" sealed.stderr
chmod 600 .perf_runs/sealed.json
rm .perf_runs/corrupt.json .perf_runs/sealed.json corrupt.stderr sealed.stderr

printf '[6/7] Repeated overwrite churn on one run\n'
i=1
while [ "$i" -le 20 ]; do
    "$PROFILEX_BINARY" run --name bulk-1 --runs 12 --warmup 1 --overwrite -- /usr/bin/true >/dev/null
    i=$((i + 1))
done

printf '[7/7] Cleaning up\n'
i=1
while [ "$i" -le "$HISTORY_RUNS" ]; do
    "$PROFILEX_BINARY" delete "bulk-$i" >/dev/null
    i=$((i + 1))
done
"$PROFILEX_BINARY" delete failing >/dev/null
"$PROFILEX_BINARY" delete sleepy >/dev/null

printf 'Deep stress test completed successfully.\n'
