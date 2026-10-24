#!/usr/bin/env bash
set -euo pipefail

RUNTIME_DIR="${XDG_RUNTIME_DIR:-/tmp/runtime-qgc}"

DIRECTORIES=(
    /workspace-build
    /ccache
    /home/qgc
    /home/qgc/.config
    /home/qgc/.cache
    /home/qgc/.local/share
    /home/qgc/Documents
    "${RUNTIME_DIR}"
)

mkdir -p "${DIRECTORIES[@]}"
chown qgc:qgc "${DIRECTORIES[@]}"
chmod 700 "${RUNTIME_DIR}"

exec gosu qgc "$@"
