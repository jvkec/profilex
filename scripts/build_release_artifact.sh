#!/bin/sh

set -eu

BUILD_DIR=${1:-".build"}

printf '[1/3] Configuring build in %s\n' "$BUILD_DIR"
cmake -S . -B "$BUILD_DIR"

printf '[2/3] Building profilex and tests\n'
cmake --build "$BUILD_DIR"

printf '[3/3] Generating release archive\n'
cmake --build "$BUILD_DIR" --target package

printf '\nRelease artifacts in %s:\n' "$BUILD_DIR"
find "$BUILD_DIR" -maxdepth 1 \( -name '*.tar.gz' -o -name '*.tgz' \) -print
