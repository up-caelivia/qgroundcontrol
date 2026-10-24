#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="${QGC_SOURCE_DIR:-/workspaces/qgroundcontrol}"
BUILD_DIR="${QGC_BUILD_DIR:-/workspace-build}"
STAGING_DIR="${BUILD_DIR}/staging"

if [[ -n "${QGC_BINARY_NAME:-}" ]]; then
    BINARY_NAME="${QGC_BINARY_NAME}"
elif [[ -f "${SOURCE_DIR}/custom/custom.pri" ]]; then
    if [[ -e "${SOURCE_DIR}/custom/create_ABLUO" ]]; then
        BINARY_NAME="QGroundControlABLUO"
    else
        BINARY_NAME="QGroundControlUP"
    fi
else
    BINARY_NAME="QGroundControl"
fi

BINARY="${STAGING_DIR}/${BINARY_NAME}"

if [[ ! -x "${BINARY}" ]]; then
    echo "No executable found at ${BINARY}. Run qgc-build.sh first." >&2
    exit 1
fi

# Use the Qt libraries and plugins staged by this checkout's qmake build.
export LD_LIBRARY_PATH="${STAGING_DIR}/usr/lib/x86_64-linux-gnu:${STAGING_DIR}/Qt/libs${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export QML2_IMPORT_PATH="${STAGING_DIR}/Qt/qml${QML2_IMPORT_PATH:+:${QML2_IMPORT_PATH}}"
export QT_PLUGIN_PATH="${STAGING_DIR}/Qt/plugins"
export QT_IM_MODULE="${QT_IM_MODULE:-qtvirtualkeyboard}"

exec "${BINARY}" "$@"
