#!/bin/bash
# rebuild-all.sh - Clean rebuild of engine, tools, tests, UI, and Node sender deps
# SPDX-License-Identifier: MIT

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREFLIGHT_ONLY="${1:-}"

echo "==================================================================="
echo "AES67 Virtual Soundcard - Clean Rebuild"
echo "==================================================================="
echo "Project root: $PROJECT_ROOT"
echo ""

if [[ "${PREFLIGHT_ONLY}" == "--help" || "${PREFLIGHT_ONLY}" == "-h" ]]; then
    cat <<'USAGE'
Usage: scripts/rebuild-all.sh
       scripts/rebuild-all.sh --preflight   # only check dependencies

Performs a clean rebuild of:
  • Core Audio driver
  • Network engine static library
  • CLI tools (monitor/stream/etc.)
  • Unit tests
  • macOS menu bar UI
  • Node AES67 sender dependencies

All existing CMake build directories are removed before configuration to
ensure the build reflects the latest source after a git pull.
USAGE
    exit 0
fi

function check_cmd() {
    local cmd="$1"
    local hint="$2"
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "ERROR: '$cmd' not found. Install via: $hint"
        exit 1
    fi
}

echo "[*] Checking build prerequisites..."
check_cmd cmake "brew install cmake"
check_cmd make "Xcode Command Line Tools (xcode-select --install)"
check_cmd xcodebuild "Install Xcode from the App Store"
check_cmd npm "brew install node"
echo "✓ All required tooling found"
echo ""

if [[ "${PREFLIGHT_ONLY}" == "--preflight" ]]; then
    echo "Preflight checks completed. No build performed."
    exit 0
fi

echo "[*] Removing previous build directories..."
rm -rf \
    "$PROJECT_ROOT/driver/build" \
    "$PROJECT_ROOT/engine/build" \
    "$PROJECT_ROOT/tools/build" \
    "$PROJECT_ROOT/tests/unit/build" \
    "$PROJECT_ROOT/ui/Aes67VSC/build"
echo "✓ Clean slate ready"
echo ""

echo "[*] Installing Node sender dependencies..."
npm install --prefix "$PROJECT_ROOT/third_party/aes67-sender"
echo "✓ Node dependencies installed"
echo ""

echo "[*] Running master build script..."
(
    cd "$PROJECT_ROOT"
    ./scripts/build.sh
)
echo ""

echo "==================================================================="
echo "Clean rebuild completed successfully."
echo "==================================================================="
echo "Artifacts:"
echo "  • Engine/library:      engine/build"
echo "  • CLI tools:           tools/build"
echo "  • Unit tests:          tests/unit/build"
echo "  • UI app bundle:       ui/Aes67VSC/build/Build/Products/Release"
echo "  • Node sender modules: third_party/aes67-sender/node_modules"
echo ""
