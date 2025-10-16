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

# Build Tests
echo "-------------------------------------------------------------------"
echo "[4/5] Building Unit Tests"
echo "-------------------------------------------------------------------"
cd "$PROJECT_ROOT/tests/unit"
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
echo "✓ Tests built successfully"
echo ""

# Run Tests
echo "-------------------------------------------------------------------"
echo "[5/6] Running Unit Tests"
echo "-------------------------------------------------------------------"
if ./aes67_tests; then
    echo "✓ All tests passed"
else
    echo "✗ Some tests failed"
    exit 1
fi
echo ""

# Build UI
echo "-------------------------------------------------------------------"
echo "[6/6] Building UI (Menu Bar App)"
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
