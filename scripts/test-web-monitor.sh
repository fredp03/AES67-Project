#!/bin/bash
# Test script for AES67 Web Monitor with DAW
# Usage: ./test-web-monitor.sh [channels] [port]

set -e

CHANNELS=${1:-2}
PORT=${2:-8080}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_DIR="$SCRIPT_DIR/../tools/build"
MONITOR_BIN="$TOOLS_DIR/aes67-monitor"

echo "=================================================="
echo "AES67 Web Monitor - DAW Test"
echo "=================================================="
echo ""

# Check if monitor exists
if [ ! -f "$MONITOR_BIN" ]; then
    echo "❌ Error: aes67-monitor not found at $MONITOR_BIN"
    echo "   Please build the project first: ./scripts/build.sh"
    exit 1
fi

# Check if port is in use
if lsof -Pi :$PORT -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    echo "⚠️  Warning: Port $PORT is already in use"
    echo "   Use a different port: $0 $CHANNELS <other_port>"
    exit 1
fi

# Check if driver is loaded
echo "Checking AES67 driver..."
if ! system_profiler SPAudioDataType 2>/dev/null | grep -q "AES67" ; then
    echo "⚠️  Warning: AES67 Virtual Soundcard not detected"
    echo "   The driver may not be installed or loaded."
    echo "   Install with: cd driver/build && sudo ./install.sh"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
else
    echo "✓ AES67 Virtual Soundcard detected"
fi

echo ""
echo "Configuration:"
echo "  Channels: $CHANNELS"
echo "  Port: $PORT"
echo "  Interface: en0 (default)"
echo ""

echo "=================================================="
echo "Starting Web Monitor"
echo "=================================================="
echo ""

# Trap Ctrl+C to cleanup
trap 'echo ""; echo "Stopping monitor..."; exit 0' INT TERM

# Start the monitor
"$MONITOR_BIN" --channels "$CHANNELS" --port "$PORT" --verbose &
MONITOR_PID=$!

# Wait a moment for server to start
sleep 2

# Check if monitor is still running
if ! kill -0 $MONITOR_PID 2>/dev/null; then
    echo "❌ Error: Monitor failed to start"
    exit 1
fi

echo ""
echo "=================================================="
echo "Monitor Status"
echo "=================================================="
echo ""
echo "✓ Web interface: http://localhost:$PORT"
echo "✓ Monitoring $CHANNELS channels"
echo "✓ PID: $MONITOR_PID"
echo ""
echo "=================================================="
echo "Testing Instructions"
echo "=================================================="
echo ""
echo "1. Open your browser to: http://localhost:$PORT"
echo ""
echo "2. Configure your DAW:"
echo "   • Set output device to 'AES67 Virtual Soundcard'"
echo "   • Enable $CHANNELS output channels"
echo "   • Create some tracks with audio"
echo ""
echo "3. Play audio in your DAW:"
echo "   • You should see level meters moving in the browser"
echo "   • Check that PTP status shows 'Locked' (green)"
echo "   • Verify all channels are working"
echo ""
echo "4. Tests to perform:"
echo "   • Play a 1kHz tone at -20 dBFS (meter should show ~-20 dB)"
echo "   • Pan mono track left/right (verify channel separation)"
echo "   • Mute/unmute tracks (verify immediate response)"
echo "   • Play for 5+ minutes (verify stability)"
echo ""
echo "=================================================="
echo "Quick Tests"
echo "=================================================="
echo ""

# Function to test if server is responding
test_server() {
    if curl -s "http://localhost:$PORT/status" >/dev/null 2>&1; then
        echo "✓ HTTP server responding on port $PORT"
        return 0
    else
        echo "❌ HTTP server not responding"
        return 1
    fi
}

# Function to check PTP status
check_ptp() {
    local status=$(curl -s "http://localhost:$PORT/status" 2>/dev/null | grep -o '"ptpLocked":[^,]*' | cut -d: -f2)
    if [ "$status" = "true" ]; then
        echo "✓ PTP is locked"
        return 0
    elif [ "$status" = "false" ]; then
        echo "⚠️  PTP not yet locked (may take a few seconds)"
        return 1
    else
        echo "❓ Could not determine PTP status"
        return 1
    fi
}

# Wait for server to be ready
echo "Testing server..."
for i in {1..5}; do
    if test_server; then
        break
    fi
    if [ $i -eq 5 ]; then
        echo "❌ Server failed to become ready"
        kill $MONITOR_PID 2>/dev/null
        exit 1
    fi
    sleep 1
done

echo ""
echo "Checking PTP synchronization..."
sleep 2
check_ptp || echo "   (This is normal if no PTP master is available)"

echo ""
echo "=================================================="
echo "Monitoring..."
echo "=================================================="
echo ""
echo "Press Ctrl+C to stop the monitor"
echo ""

# Keep script running and monitor process
while kill -0 $MONITOR_PID 2>/dev/null; do
    sleep 1
done

echo ""
echo "Monitor stopped."
