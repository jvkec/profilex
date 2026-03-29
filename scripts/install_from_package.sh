#!/bin/sh

set -eu

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "usage: $0 <package.tgz> [prefix]" >&2
    exit 1
fi

PACKAGE_PATH=$1
PREFIX=${2:-"$HOME/.local"}
WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/profilex-package.XXXXXX")

cleanup() {
    rm -rf "$WORK_DIR"
}

trap cleanup EXIT INT TERM

tar -xzf "$PACKAGE_PATH" -C "$WORK_DIR"

BIN_PATH=$(find "$WORK_DIR" -type f -name profilex | head -n 1)
DOC_DIR=$(find "$WORK_DIR" -type d -path '*/share/doc/profilex' | head -n 1 || true)

if [ -z "$BIN_PATH" ]; then
    echo "could not find profilex binary in package: $PACKAGE_PATH" >&2
    exit 1
fi

mkdir -p "$PREFIX/bin"
cp "$BIN_PATH" "$PREFIX/bin/profilex"

if [ -n "$DOC_DIR" ]; then
    mkdir -p "$PREFIX/share/doc/profilex"
    cp -R "$DOC_DIR"/. "$PREFIX/share/doc/profilex/"
fi

printf 'Installed profilex to %s/bin/profilex\n' "$PREFIX"
