#!/bin/bash
# Install AES67 Virtual Soundcard driver
# This script must be run with sudo

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DRIVER_PATH="$SCRIPT_DIR/build/AES67VSC.driver"
INSTALL_DIR="/Library/Audio/Plug-Ins/HAL"

echo "=================================================="
echo "AES67 Virtual Soundcard - Driver Installation"
echo "=================================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "❌ Error: This script must be run with sudo"
    echo ""
    echo "Usage: sudo ./install.sh"
    exit 1
fi

# Check if driver exists
if [ ! -d "$DRIVER_PATH" ]; then
    echo "❌ Error: Driver not found at $DRIVER_PATH"
    echo ""
    echo "Please build the project first:"
    echo "  cd ../.. && ./scripts/build.sh"
    exit 1
fi

echo "Driver found: $DRIVER_PATH"
echo "Install location: $INSTALL_DIR"
echo ""

# Create HAL directory if it doesn't exist
if [ ! -d "$INSTALL_DIR" ]; then
    echo "Creating HAL plugin directory..."
    mkdir -p "$INSTALL_DIR"
fi

# Remove old installation if exists
if [ -d "$INSTALL_DIR/AES67VSC.driver" ]; then
    echo "Removing old installation..."
    rm -rf "$INSTALL_DIR/AES67VSC.driver"
fi

# Copy driver
echo "Installing driver..."
cp -R "$DRIVER_PATH" "$INSTALL_DIR/"

# Set permissions
echo "Setting permissions..."
chown -R root:wheel "$INSTALL_DIR/AES67VSC.driver"
chmod -R 755 "$INSTALL_DIR/AES67VSC.driver"

echo ""
echo "✅ Driver installed successfully!"
echo ""
echo "=================================================="
echo "Next Steps"
echo "=================================================="
echo ""
echo "1. Restart Core Audio:"
echo "   sudo killall coreaudiod"
echo ""
echo "2. Wait a few seconds for Core Audio to restart"
echo ""
echo "3. Check if driver is loaded:"
echo "   system_profiler SPAudioDataType | grep -i aes67"
echo ""
echo "4. If you see the driver listed, you're ready to go!"
echo "   Open your DAW and select 'AES67 Virtual Soundcard'"
echo ""
echo "Note: On macOS Ventura and later, you may need to:"
echo "  - Approve the driver in System Settings > Privacy & Security"
echo "  - Restart your computer if the driver doesn't appear"
echo ""
echo "=================================================="
echo ""

# Offer to restart coreaudiod
read -p "Restart Core Audio now? (recommended) [Y/n] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]] || [[ -z $REPLY ]]; then
    echo ""
    echo "Restarting Core Audio..."
    killall coreaudiod 2>/dev/null || true
    sleep 2
    echo ""
    echo "Checking driver status..."
    sleep 2
    if system_profiler SPAudioDataType 2>/dev/null | grep -qi "aes67"; then
        echo "✅ SUCCESS! AES67 Virtual Soundcard is now available!"
    else
        echo "⚠️  Driver installed but not yet detected."
        echo "   Try restarting your computer or check System Settings > Privacy & Security"
    fi
fi

echo ""
echo "Installation complete!"
