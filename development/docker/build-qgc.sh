#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="${QGC_SOURCE_DIR:-/workspaces/qgroundcontrol}"
BUILD_DIR="${QGC_BUILD_DIR:-/workspace-build}"
JOBS="${QGC_BUILD_JOBS:-4}"

if [[ ! -f "${SOURCE_DIR}/qgroundcontrol.pro" ]]; then
    echo "QGroundControl source is not mounted at ${SOURCE_DIR}" >&2
    exit 1
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

qmake -r "${SOURCE_DIR}/qgroundcontrol.pro" CONFIG+=debug CONFIG-=release
bear -a -o "${BUILD_DIR}/compile_commands.json" make -j"${JOBS}"
