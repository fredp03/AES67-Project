#!/bin/bash
# Sign and install AES67 driver with self-signed certificate
# Usage: ./sign-and-install.sh [certificate-name]

set -e

CERT_NAME="${1:-AES67 Audio Driver}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/.."
DRIVER_PATH="$PROJECT_ROOT/driver/build/AES67VSC.driver"
INSTALL_PATH="/Library/Audio/Plug-Ins/HAL/AES67VSC.driver"

echo "=================================================="
echo "AES67 Driver - Sign and Install"
echo "=================================================="
echo ""
echo "Certificate: $CERT_NAME"
echo "Driver:      $DRIVER_PATH"
echo "Install to:  $INSTALL_PATH"
echo ""

# Check if driver exists
if [ ! -d "$DRIVER_PATH" ]; then
    echo "❌ Error: Driver not found at $DRIVER_PATH"
    echo "   Build it first: ./scripts/build.sh"
    exit 1
fi

# Check if certificate exists
echo "Checking for certificate '$CERT_NAME'..."
if ! security find-identity -v -p codesigning | grep -q "$CERT_NAME"; then
    echo ""
    echo "❌ Certificate '$CERT_NAME' not found!"
    echo ""
    echo "Please create a self-signed certificate first:"
    echo ""
    echo "1. Open Keychain Access (Cmd+Space, type 'Keychain Access')"
    echo "2. Menu: Keychain Access → Certificate Assistant → Create a Certificate"
    echo "3. Settings:"
    echo "   - Name: $CERT_NAME"
    echo "   - Identity Type: Self Signed Root"
    echo "   - Certificate Type: Code Signing"
    echo "   - ☑ Let me override defaults"
    echo "4. Follow the wizard, accept defaults"
    echo "5. After creating, double-click the certificate and set Trust to 'Always Trust'"
    echo ""
    echo "Then run this script again."
    echo ""
    echo "Available code signing identities:"
    security find-identity -v -p codesigning
    echo ""
    exit 1
fi

echo "✓ Certificate found"
echo ""

# Sign the driver
echo "Signing driver..."
echo ""

# Try with hardened runtime first
if codesign --force --deep --sign "$CERT_NAME" \
    --timestamp --options runtime \
    "$DRIVER_PATH" 2>/dev/null; then
    echo "✓ Signed with hardened runtime"
else
    echo "⚠️  Hardened runtime failed, trying without..."
    if codesign --force --deep --sign "$CERT_NAME" \
        "$DRIVER_PATH" 2>/dev/null; then
        echo "✓ Signed (without hardened runtime)"
    else
        echo "❌ Signing failed!"
        echo ""
        echo "Debug info:"
        codesign --force --deep --sign "$CERT_NAME" "$DRIVER_PATH" 2>&1
        exit 1
    fi
fi

echo ""

# Verify signature
echo "Verifying signature..."
if codesign -dvvv "$DRIVER_PATH" 2>&1 | grep -q "Authority=$CERT_NAME"; then
    echo "✓ Signature verified"
else
    echo "⚠️  Signature verification unclear"
    echo ""
    echo "Signature details:"
    codesign -dvvv "$DRIVER_PATH" 2>&1 | grep -E "Authority|Identifier|Signature"
fi

echo ""

# Remove old driver
echo "Removing old driver (requires sudo)..."
if [ -d "$INSTALL_PATH" ]; then
    sudo rm -rf "$INSTALL_PATH"
    echo "✓ Removed old driver"
else
    echo "✓ No old driver found"
fi

echo ""

# Install new driver
echo "Installing signed driver (requires sudo)..."
sudo cp -R "$DRIVER_PATH" "$INSTALL_PATH"

# Set correct ownership and permissions
sudo chown -R root:wheel "$INSTALL_PATH"
sudo chmod -R 755 "$INSTALL_PATH"

echo "✓ Driver installed"
echo ""

# Restart CoreAudio
echo "Restarting CoreAudio daemon..."
sudo killall coreaudiod 2>/dev/null || true

echo "✓ CoreAudio restarted"
echo ""

# Wait for CoreAudio to restart
echo "Waiting for CoreAudio to reload drivers..."
sleep 3

# Check if driver is loaded
echo ""
echo "=================================================="
echo "Checking if driver loaded..."
echo "=================================================="
echo ""

if system_profiler SPAudioDataType 2>/dev/null | grep -qi "aes67"; then
    echo "✅ SUCCESS! AES67 driver is loaded!"
    echo ""
    system_profiler SPAudioDataType 2>/dev/null | grep -i "aes67" -A 10
    echo ""
    echo "=================================================="
    echo "Next Steps"
    echo "=================================================="
    echo ""
    echo "1. Open your DAW (Ableton, Logic, etc.)"
    echo "2. Go to Audio Preferences"
    echo "3. Select 'AES67 Virtual Soundcard' as output device"
    echo "4. Test with: ./scripts/launch-monitor.sh 8"
    echo ""
else
    echo "❌ Driver not detected by CoreAudio"
    echo ""
    echo "Self-signing may not be sufficient on your macOS version."
    echo ""
    echo "Options:"
    echo ""
    echo "1. Check Console logs:"
    echo "   Applications → Utilities → Console"
    echo "   Filter for 'coreaudiod' and look for errors"
    echo ""
    echo "2. Try without hardened runtime:"
    echo "   codesign --force --deep --sign '$CERT_NAME' '$DRIVER_PATH'"
    echo "   sudo rm -rf '$INSTALL_PATH'"
    echo "   sudo cp -R '$DRIVER_PATH' '$INSTALL_PATH'"
    echo "   sudo killall coreaudiod"
    echo ""
    echo "3. Disable SIP (see docs/TROUBLESHOOTING_DRIVER.md)"
    echo ""
    echo "4. Get Apple Developer certificate (\$99/year)"
    echo ""
    echo "5. Test without driver using CLI tools:"
    echo "   ./scripts/test-without-driver.sh"
    echo ""
fi

echo ""
echo "Installation complete."
echo ""
