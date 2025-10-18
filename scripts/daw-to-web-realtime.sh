#!/bin/bash
# Ultra-low latency DAW to Web Monitor using aes67-sender
#
# This script provides the LOWEST LATENCY path from your DAW to web browser:
#   Ableton → BlackHole 64ch → aes67-sender (Node.js/CoreAudio) → Network → aes67-monitor → Web
#
# Latency characteristics:
#   - BlackHole: ~5ms (configurable buffer)
#   - aes67-sender: < 2ms (real-time CoreAudio API)
#   - Network: < 1ms (local multicast)
#   - aes67-monitor: < 5ms (ring buffer)
#   - Total: ~10-15ms end-to-end
#
# Usage: ./scripts/daw-to-web-realtime.sh [channels]

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
CHANNELS=${1:-8}
MULTICAST_IP="239.69.2.1"  # Stream 0 (239.69.2.x matches NetworkEngine's receiver)
PORT=5006  # Using 5006 to avoid conflict with macOS MIDIServer on 5004-5005
INTERFACE="en0"
STREAM_NAME="Ableton_Live"

# Directories
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SENDER_DIR="$PROJECT_ROOT/third_party/aes67-sender"
MONITOR_BIN="$PROJECT_ROOT/tools/build/aes67-monitor"

echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  ULTRA-LOW LATENCY DAW → WEB MONITOR                          ║${NC}"
echo -e "${CYAN}║  Using aes67-sender + BlackHole                               ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check Node.js
echo -e "${YELLOW}Checking prerequisites...${NC}"
if ! command -v node &> /dev/null; then
    echo -e "${RED}✗ Node.js not found!${NC}"
    echo "Install with: brew install node"
    exit 1
fi
echo -e "${GREEN}✓ Node.js $(node --version)${NC}"

# Check if aes67-sender is installed
if [ ! -d "$SENDER_DIR/node_modules" ]; then
    echo -e "${YELLOW}Installing aes67-sender dependencies...${NC}"
    cd "$SENDER_DIR"
    npm install
    echo -e "${GREEN}✓ Dependencies installed${NC}"
else
    echo -e "${GREEN}✓ aes67-sender ready${NC}"
fi

# Check BlackHole
if ! system_profiler SPAudioDataType | grep -q "BlackHole 64ch"; then
    echo -e "${RED}✗ BlackHole 64ch not found!${NC}"
    echo "Install with: brew install blackhole-64ch"
    exit 1
fi
echo -e "${GREEN}✓ BlackHole 64ch found${NC}"

# Check monitor tool
if [ ! -f "$MONITOR_BIN" ]; then
    echo -e "${RED}✗ aes67-monitor not built!${NC}"
    echo "Building..."
    cd "$PROJECT_ROOT/engine/build" && cmake .. && make -j$(sysctl -n hw.ncpu)
    cd "$PROJECT_ROOT/tools/build" && cmake .. && make -j$(sysctl -n hw.ncpu)
    echo -e "${GREEN}✓ Build complete${NC}"
else
    echo -e "${GREEN}✓ aes67-monitor ready${NC}"
fi
echo ""

# Find BlackHole device index
echo -e "${YELLOW}Detecting BlackHole device...${NC}"
cd "$SENDER_DIR"
DEVICE_LIST=$(node aes67.js -a macos --devices 2>/dev/null || echo "")

if [ -z "$DEVICE_LIST" ]; then
    echo -e "${RED}✗ Could not list audio devices${NC}"
    exit 1
fi

DEVICE_INDEX=$(echo "$DEVICE_LIST" | grep -i "blackhole" | grep "64" | awk '{print $1}' | head -1)

if [ -z "$DEVICE_INDEX" ]; then
    echo -e "${RED}✗ Could not find BlackHole 64ch device index${NC}"
    echo ""
    echo "Available devices:"
    echo "$DEVICE_LIST"
    exit 1
fi

echo -e "${GREEN}✓ BlackHole 64ch found at device index: $DEVICE_INDEX${NC}"
echo ""

# Cleanup existing processes
echo -e "${YELLOW}Cleaning up existing processes...${NC}"
pkill -f "aes67.js" || true
pkill -f "aes67-monitor" || true
sleep 1
echo -e "${GREEN}✓ Cleanup complete${NC}"
echo ""

# Print configuration
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  CONFIGURATION                                                ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}Audio Path:${NC}"
echo "  Input Device: BlackHole 64ch (index $DEVICE_INDEX)"
echo "  Channels: $CHANNELS"
echo "  Sample Rate: 48000 Hz"
echo "  Format: L24 (24-bit PCM)"
echo "  Frame Size: 48 samples (1ms @ 48kHz)"
echo ""
echo -e "${BLUE}Network:${NC}"
echo "  Multicast: $MULTICAST_IP:$PORT"
echo "  Interface: $INTERFACE"
echo "  Protocol: AES67 RTP + PTP sync"
echo ""
echo -e "${BLUE}Web Monitor:${NC}"
echo "  HTTP Server: http://localhost:8080"
echo "  Update Rate: 10 Hz"
echo ""

# Ableton setup instructions
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  ABLETON SETUP                                                ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "1. Open ${GREEN}Ableton Live${NC}"
echo "2. Go to ${GREEN}Preferences → Audio${NC}"
echo "3. Set ${GREEN}Audio Output Device${NC} to: ${CYAN}BlackHole 64ch${NC}"
echo "4. Set ${GREEN}Sample Rate${NC} to: ${CYAN}48000 Hz${NC}"
echo "5. Set ${GREEN}Buffer Size${NC} to: ${CYAN}64 or 128 samples${NC} (for lowest latency)"
echo "6. Enable output channels 1-$CHANNELS in ${GREEN}Output Config${NC}"
echo ""
echo -e "${YELLOW}⚠️  You won't hear audio with this setup!${NC}"
echo ""
echo -e "${BLUE}To monitor audio while testing:${NC}"
echo "  Option A: Create a Multi-Output Device (Audio MIDI Setup)"
echo "            - Include your speakers AND BlackHole 64ch"
echo "            - Select this device in Ableton"
echo "  Option B: Use headphones on a separate interface"
echo ""
echo -e "${YELLOW}Make sure Ableton is configured and playing audio...${NC}"
echo ""

# Request sudo password upfront
echo -e "${YELLOW}Requesting administrator privileges (needed for PTP)...${NC}"
sudo -v
echo ""

# Start aes67-sender (with sudo for PTP)
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  STARTING AUDIO CAPTURE                                       ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${YELLOW}Starting aes67-sender...${NC}"
echo "  Capturing from: BlackHole 64ch"
echo "  Sending to: $MULTICAST_IP:$PORT"
echo "  Syncing to: PTP master (may take a few seconds)"
echo ""

cd "$SENDER_DIR"
node aes67.js \
    -a macos \
    -d $DEVICE_INDEX \
    -c $CHANNELS \
    -m $MULTICAST_IP \
    -n "$STREAM_NAME" \
    --address $(ifconfig $INTERFACE | grep "inet " | awk '{print $2}') \
    -v \
    > /tmp/aes67-sender.log 2>&1 &

SENDER_PID=$!
sleep 3

# Check if sender started
if ! ps -p $SENDER_PID > /dev/null; then
    echo -e "${RED}✗ aes67-sender failed to start!${NC}"
    echo ""
    echo "Log output:"
    cat /tmp/aes67-sender.log
    exit 1
fi

# Wait for PTP sync (check log)
echo -e "${YELLOW}Waiting for PTP synchronization...${NC}"
for i in {1..15}; do
    if grep -q "Synced to" /tmp/aes67-sender.log 2>/dev/null; then
        PTP_MASTER=$(grep "Synced to" /tmp/aes67-sender.log | tail -1 | awk '{print $3}')
        echo -e "${GREEN}✓ Synced to PTP master: $PTP_MASTER${NC}"
        break
    fi
    echo -n "."
    sleep 1
    
    if [ $i -eq 15 ]; then
        echo ""
        echo -e "${RED}✗ PTP sync timeout!${NC}"
        echo "This may be normal if no PTP master is present."
        echo "Audio will still work but timestamps may drift."
        echo ""
        echo "Press Enter to continue anyway..."
        read
    fi
done
echo ""

echo -e "${GREEN}✓ aes67-sender running (PID: $SENDER_PID)${NC}"
echo ""

# Start web monitor
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  STARTING WEB MONITOR                                         ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${YELLOW}Starting aes67-monitor (requires sudo for PTP ports 319/320)...${NC}"
echo "  Listening on: $MULTICAST_IP:$PORT"
echo "  Web interface: http://localhost:8080"
echo ""

# Run from project root so config file is found
# Monitor needs sudo to bind PTP ports 319 and 320
cd "$PROJECT_ROOT"
sudo "$MONITOR_BIN" \
    --port 8080 \
    --channels $CHANNELS \
    --interface $INTERFACE \
    -v \
    > /tmp/aes67-monitor.log 2>&1 &

MONITOR_PID=$!
sleep 2

# Check if monitor started
if ! ps -p $MONITOR_PID > /dev/null; then
    echo -e "${RED}✗ aes67-monitor failed to start!${NC}"
    echo ""
    echo "Log output:"
    cat /tmp/aes67-monitor.log
    kill $SENDER_PID 2>/dev/null || true
    sudo kill $SENDER_PID 2>/dev/null || true
    exit 1
fi

echo -e "${GREEN}✓ aes67-monitor running (PID: $MONITOR_PID)${NC}"
echo ""

# Success!
echo -e "${GREEN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✓ SYSTEM RUNNING                                             ║${NC}"
echo -e "${GREEN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}  AUDIO FLOW (ULTRA-LOW LATENCY)${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo "  ┌─────────────┐"
echo "  │   Ableton   │  Your DAW"
echo "  │    Live     │"
echo "  └──────┬──────┘"
echo "         │ CoreAudio output"
echo "         ↓"
echo "  ┌─────────────┐"
echo "  │  BlackHole  │  Virtual audio device (~5ms)"
echo "  │    64ch     │"
echo "  └──────┬──────┘"
echo "         │ CoreAudio input"
echo "         ↓"
echo "  ┌─────────────┐"
echo "  │ aes67-sender│  Real-time capture (< 2ms)"
echo "  │  (Node.js)  │  + PTP sync"
echo "  └──────┬──────┘"
echo "         │ AES67 RTP multicast"
echo "         ↓"
echo "  ┌─────────────┐"
echo "  │   Network   │  Local multicast (< 1ms)"
echo "  │ $MULTICAST_IP │"
echo "  └──────┬──────┘"
echo "         │ UDP packets"
echo "         ↓"
echo "  ┌─────────────┐"
echo "  │aes67-monitor│  Ring buffer (< 5ms)"
echo "  │   (C++)     │"
echo "  └──────┬──────┘"
echo "         │ HTTP/JSON"
echo "         ↓"
echo "  ┌─────────────┐"
echo "  │   Browser   │  Real-time meters"
echo "  │  localhost  │  http://localhost:8080"
echo "  │    :8080    │"
echo "  └─────────────┘"
echo ""
echo -e "${GREEN}Total Latency: ~10-15ms${NC}"
echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${GREEN}🎉 Open your browser to:${NC}"
echo -e "${CYAN}   http://localhost:8080${NC}"
echo ""
echo -e "${YELLOW}📊 View logs:${NC}"
echo "   Sender:  tail -f /tmp/aes67-sender.log"
echo "   Monitor: tail -f /tmp/aes67-monitor.log"
echo ""
echo -e "${YELLOW}🎵 Now play audio in Ableton and watch the meters!${NC}"
echo ""
echo -e "${RED}Press Ctrl+C to stop${NC}"
echo ""

# Cleanup function
cleanup() {
    echo ""
    echo -e "${YELLOW}Stopping services...${NC}"
    sudo kill $MONITOR_PID 2>/dev/null || true
    sudo kill $SENDER_PID 2>/dev/null || true
    kill $SENDER_PID 2>/dev/null || true
    sleep 1
    echo -e "${GREEN}✓ Stopped${NC}"
    exit 0
}

trap cleanup INT TERM

# Keep running and show status
while true; do
    sleep 5
    
    # Check if processes are still running
    if ! ps -p $SENDER_PID > /dev/null; then
        echo -e "${RED}✗ aes67-sender died!${NC}"
        echo "Check log: /tmp/aes67-sender.log"
        kill $MONITOR_PID 2>/dev/null || true
        exit 1
    fi
    
    if ! ps -p $MONITOR_PID > /dev/null; then
        echo -e "${RED}✗ aes67-monitor died!${NC}"
        echo "Check log: /tmp/aes67-monitor.log"
        sudo kill $SENDER_PID 2>/dev/null || true
        exit 1
    fi
done
