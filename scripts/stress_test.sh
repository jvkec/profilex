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

WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/profilex-stress.XXXXXX")
RUNS=40
SAMPLES=30

cleanup() {
    rm -rf "$WORK_DIR"
}

trap cleanup EXIT INT TERM

printf 'Stress workspace: %s\n' "$WORK_DIR"
cd "$WORK_DIR"

printf '[1/5] Measuring %s independent runs with %s samples each\n' "$RUNS" "$SAMPLES"
i=1
while [ "$i" -le "$RUNS" ]; do
    "$PROFILEX_BINARY" run --name "run-$i" --runs "$SAMPLES" --warmup 2 -- /usr/bin/true >/dev/null
    i=$((i + 1))
done

printf '[2/5] Listing runs\n'
"$PROFILEX_BINARY" list >/dev/null

printf '[3/5] Exporting and showing representative runs\n'
"$PROFILEX_BINARY" show run-1 >/dev/null
"$PROFILEX_BINARY" export run-1 --format json >/dev/null
"$PROFILEX_BINARY" compare run-1 run-2 >/dev/null || true

printf '[4/5] Overwriting one run repeatedly and mixing exit codes\n'
i=1
while [ "$i" -le 10 ]; do
    "$PROFILEX_BINARY" run --name run-1 --runs 8 --warmup 1 --overwrite -- /usr/bin/true >/dev/null
    i=$((i + 1))
done

cat > "$WORK_DIR/alternating_exit.sh" <<'EOF'
#!/bin/sh
COUNTER_FILE="$1"
COUNT=0
if [ -f "$COUNTER_FILE" ]; then
    COUNT=$(cat "$COUNTER_FILE")
fi
COUNT=$((COUNT + 1))
printf '%s\n' "$COUNT" > "$COUNTER_FILE"
if [ $((COUNT % 3)) -eq 0 ]; then
    exit 7
fi
exit 0
EOF
chmod +x "$WORK_DIR/alternating_exit.sh"
rm -f "$WORK_DIR/counter.txt"
"$PROFILEX_BINARY" run --name alternating-exit --runs 12 --warmup 0 --overwrite -- \
    "$WORK_DIR/alternating_exit.sh" "$WORK_DIR/counter.txt" >/dev/null || true

printf '[5/5] Deleting all stress runs\n'
i=1
while [ "$i" -le 10 ]; do
    "$PROFILEX_BINARY" delete "run-$i" >/dev/null
    i=$((i + 1))
done
"$PROFILEX_BINARY" delete alternating-exit >/dev/null
while [ "$i" -le "$RUNS" ]; do
    "$PROFILEX_BINARY" delete "run-$i" >/dev/null
    i=$((i + 1))
done

printf 'Stress test completed successfully.\n'
