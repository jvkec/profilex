#!/bin/sh

set -eu

if [ "$#" -gt 1 ]; then
    echo "usage: $0 [source-dir]" >&2
    exit 1
fi

SOURCE_DIR=${1:-$(pwd)}
if [ ! -d "$SOURCE_DIR" ]; then
    echo "source directory does not exist: $SOURCE_DIR" >&2
    exit 1
fi

WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/profilex-release.XXXXXX")
SOURCE_COPY="$WORK_DIR/source"
BUILD_DIR="$WORK_DIR/build"
INSTALL_DIR="$WORK_DIR/install"

cleanup() {
    rm -rf "$WORK_DIR"
}

trap cleanup EXIT INT TERM

mkdir -p "$SOURCE_COPY"
rsync -a \
    --exclude '.git' \
    --exclude '.build' \
    --exclude '.perf_runs' \
    --exclude 'cmake-build-*' \
    --exclude 'install' \
    --exclude 'dist' \
    "$SOURCE_DIR"/ "$SOURCE_COPY"/

printf '[1/4] Configuring clean workspace\n'
cmake -S "$SOURCE_COPY" -B "$BUILD_DIR"

printf '[2/4] Building and testing\n'
cmake --build "$BUILD_DIR"
ctest --test-dir "$BUILD_DIR" --output-on-failure

printf '[3/4] Installing package\n'
cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR"

printf '[4/4] Running installed binary and packaging check\n'
"$INSTALL_DIR/bin/profilex" help >/dev/null
cmake --build "$BUILD_DIR" --target package >/dev/null

printf 'Release/install validation completed successfully.\n'
