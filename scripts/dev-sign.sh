#!/bin/bash
# dev-sign.sh - Code sign and install HAL driver
# SPDX-License-Identifier: MIT

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DRIVER_BUNDLE="$PROJECT_ROOT/driver/build/AES67VSC.driver"
INSTALL_PATH="/Library/Audio/Plug-Ins/HAL/AES67VSC.driver"

echo "==================================================================="
echo "AES67 Virtual Soundcard - Driver Signing & Installation"
echo "==================================================================="
echo ""

# Check if driver bundle exists
if [ ! -d "$DRIVER_BUNDLE" ]; then
    echo "ERROR: Driver bundle not found at: $DRIVER_BUNDLE"
    echo "Run ./scripts/build.sh first"
    exit 1
fi

# Check if we have a signing identity
SIGNING_IDENTITY="-"  # Ad-hoc signing (development only)

echo "Signing driver with ad-hoc signature..."
codesign --force --deep --sign "$SIGNING_IDENTITY" \
         --entitlements "$PROJECT_ROOT/driver/Signing.entitlements" \
         "$DRIVER_BUNDLE"

# Verify signature
if ! codesign -vvv "$DRIVER_BUNDLE" 2>&1; then
    echo "ERROR: Signature verification failed"
    exit 1
fi

echo "✓ Driver signed successfully"
echo ""

# Install (requires sudo)
echo "Installing driver to: $INSTALL_PATH"
echo "(This requires administrator privileges)"
echo ""

sudo rm -rf "$INSTALL_PATH"
sudo cp -R "$DRIVER_BUNDLE" "$INSTALL_PATH"
sudo chown -R root:wheel "$INSTALL_PATH"

echo "✓ Driver installed successfully"
echo ""

# Restart coreaudiod
echo "Restarting coreaudiod..."
sudo launchctl kickstart -k system/com.apple.audio.coreaudiod
sleep 2

echo "✓ coreaudiod restarted"
echo ""

# Verify device appears
echo "-------------------------------------------------------------------"
echo "Verifying installation..."
echo "-------------------------------------------------------------------"

if system_profiler SPAudioDataType 2>/dev/null | grep -q "AES67"; then
    echo "✓ AES67 device detected!"
    echo ""
    system_profiler SPAudioDataType | grep -A 10 "AES67"
else
    echo "⚠ AES67 device NOT detected"
    echo ""
    echo "Troubleshooting:"
    echo "  1. Check system log: log show --predicate 'subsystem == \"com.apple.audio\"' --last 5m"
    echo "  2. Verify bundle: ls -la $INSTALL_PATH"
    echo "  3. Check signature: codesign -dvvv $INSTALL_PATH"
    exit 1
fi

echo ""
echo "==================================================================="
echo "Installation Complete!"
echo "==================================================================="
echo ""
echo "The device should now appear in:"
echo "  - Audio MIDI Setup.app"
echo "  - System Preferences > Sound"
echo "  - DAW audio device lists"
echo ""
