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

WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/profilex-corrupt.XXXXXX")

cleanup() {
    rm -rf "$WORK_DIR"
}

trap cleanup EXIT INT TERM

mkdir -p "$WORK_DIR/.perf_runs"
printf '{\n' > "$WORK_DIR/.perf_runs/bad.json"
printf '  "name": "bad",\n' >> "$WORK_DIR/.perf_runs/bad.json"
printf '  "samples": [\n' >> "$WORK_DIR/.perf_runs/bad.json"

cd "$WORK_DIR"

printf 'Workspace: %s\n' "$WORK_DIR"

printf '[1/4] show bad\n'
if "$PROFILEX_BINARY" show bad >"$WORK_DIR/show.out" 2>"$WORK_DIR/show.err"; then
    echo "expected show to fail" >&2
    exit 1
fi
grep -q "malformed stored JSON" "$WORK_DIR/show.err"

printf '[2/4] list\n'
if "$PROFILEX_BINARY" list >"$WORK_DIR/list.out" 2>"$WORK_DIR/list.err"; then
    echo "expected list to fail" >&2
    exit 1
fi
grep -q "malformed stored JSON" "$WORK_DIR/list.err"

printf '[3/4] export bad\n'
if "$PROFILEX_BINARY" export bad --format json >"$WORK_DIR/export.out" 2>"$WORK_DIR/export.err"; then
    echo "expected export to fail" >&2
    exit 1
fi
grep -q "malformed stored JSON" "$WORK_DIR/export.err"

printf '[4/4] create a valid run to prove recovery after corruption cleanup\n'
rm -f "$WORK_DIR/.perf_runs/bad.json"
"$PROFILEX_BINARY" run --name good --runs 3 --warmup 0 -- /usr/bin/true >/dev/null
"$PROFILEX_BINARY" show good >/dev/null

printf 'Storage corruption demo completed successfully.\n'
