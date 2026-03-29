#!/bin/sh

set -eu

PREFIX=${1:-"$HOME/.local"}
BUILD_DIR=${BUILD_DIR:-".build"}

printf '[1/3] Configuring build in %s\n' "$BUILD_DIR"
cmake -S . -B "$BUILD_DIR"

printf '[2/3] Building profilex\n'
cmake --build "$BUILD_DIR"

printf '[3/3] Installing to %s\n' "$PREFIX"
cmake --install "$BUILD_DIR" --prefix "$PREFIX"

printf '\nInstalled binary:\n  %s/bin/profilex\n' "$PREFIX"
printf 'If that directory is on PATH, run:\n  profilex help\n'
