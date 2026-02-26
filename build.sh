#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ -f "$SCRIPT_DIR/build.env" ]; then
    set -a
    source "$SCRIPT_DIR/build.env"
    set +a
fi

BUILD_TYPE="${1:-debug}"

# Auto-detect QTDIR if not set
if [ -z "${QTDIR:-}" ]; then
    if [[ "$(uname)" == "Darwin" ]]; then
        # macOS: Homebrew Qt6
        if [ -d "/opt/homebrew/opt/qt@6" ]; then
            QTDIR="/opt/homebrew/opt/qt@6"
        elif [ -d "/usr/local/opt/qt@6" ]; then
            QTDIR="/usr/local/opt/qt@6"
        fi
    else
        # Linux: Qt installer default location
        for dir in "$HOME"/Qt/6.*/gcc_64; do
            [ -d "$dir" ] && QTDIR="$dir"
        done
    fi

    if [ -z "${QTDIR:-}" ]; then
        echo "ERROR: Could not detect QTDIR. Set it manually:"
        echo "  export QTDIR=/path/to/qt6"
        exit 1
    fi
fi

export QTDIR
echo "Using QTDIR=$QTDIR"

# Determine platform and preset
if [[ "$(uname)" == "Darwin" ]]; then
    PRESET="macos-${BUILD_TYPE}"
elif [[ "$(uname)" == "Linux" ]]; then
    PRESET="linux-${BUILD_TYPE}"
else
    echo "ERROR: Unsupported platform $(uname)"
    exit 1
fi

BUILD_DIR="out/build/${PRESET}"

echo "=== CONFIGURING ($PRESET) ==="
cmake --preset "$PRESET"

echo "=== BUILDING ==="
cmake --build "$BUILD_DIR"

echo "=== BUILD SUCCEEDED ==="
