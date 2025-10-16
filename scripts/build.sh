#!/bin/bash
# build.sh - Build all AES67 VSC components
# SPDX-License-Identifier: MIT

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NCPUS=$(sysctl -n hw.ncpu)

echo "==================================================================="
echo "AES67 Virtual Soundcard - Build Script"
echo "==================================================================="
echo "Project root: $PROJECT_ROOT"
echo "CPU cores: $NCPUS"
echo ""

# Check dependencies
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found. Install with: brew install cmake"
    exit 1
fi

if ! command -v xcodebuild &> /dev/null; then
    echo "ERROR: Xcode not found. Install Xcode from App Store"
    exit 1
fi

# Build driver
echo "-------------------------------------------------------------------"
echo "[1/4] Building Core Audio Driver"
echo "-------------------------------------------------------------------"
mkdir -p "$PROJECT_ROOT/driver/build"
cd "$PROJECT_ROOT/driver/build"
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j"$NCPUS"
echo "✓ Driver built successfully"
echo ""

# Build engine
echo "-------------------------------------------------------------------"
echo "[2/4] Building Network Engine"
echo "-------------------------------------------------------------------"
mkdir -p "$PROJECT_ROOT/engine/build"
cd "$PROJECT_ROOT/engine/build"
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j"$NCPUS"
echo "✓ Engine built successfully"
echo ""

# Build tools
echo "-------------------------------------------------------------------"
echo "[3/4] Building CLI Tools"
echo "-------------------------------------------------------------------"
mkdir -p "$PROJECT_ROOT/tools/build"
cd "$PROJECT_ROOT/tools/build"
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j"$NCPUS"
echo "✓ Tools built successfully"
echo ""

# Build UI
echo "-------------------------------------------------------------------"
echo "[4/4] Building UI (Menu Bar App)"
echo "-------------------------------------------------------------------"
cd "$PROJECT_ROOT/ui"
if [ -f "Aes67VSC.xcodeproj/project.pbxproj" ]; then
    xcodebuild -project Aes67VSC.xcodeproj \
               -scheme Aes67VSC \
               -configuration Release \
               -derivedDataPath build \
               ONLY_ACTIVE_ARCH=NO
    echo "✓ UI built successfully"
else
    echo "⚠ UI project not yet created (will be generated in next step)"
fi
echo ""

echo "==================================================================="
echo "Build Complete!"
echo "==================================================================="
echo ""
echo "Next steps:"
echo "  1. Sign and install driver:  ./scripts/dev-sign.sh"
echo "  2. Verify installation:      system_profiler SPAudioDataType | grep AES67"
echo "  3. Run tests:                ./tests/integration/loopback-test.sh"
echo ""
