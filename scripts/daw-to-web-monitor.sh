#!/bin/bash
# Complete DAW to Web Monitor Pipeline using BlackHole as intermediate
# 
# Audio Flow:
#   Ableton → BlackHole 64ch → aes67-stream → Network → aes67-monitor → Web Browser
#
# Usage: ./scripts/daw-to-web-monitor.sh [channels]
#   channels: Number of channels to stream (default: 8)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
CHANNELS=${1:-8}
SAMPLE_RATE=48000
STREAM_NAME="Ableton_Output"
MULTICAST_IP="239.69.83.1"
PORT=5004
INTERFACE="en0"  # Change if needed

# Directories
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/tools/build"

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  AES67 DAW to Web Monitor Pipeline${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Check if BlackHole is available
echo -e "${YELLOW}Checking for BlackHole...${NC}"
if ! system_profiler SPAudioDataType | grep -q "BlackHole 64ch"; then
    echo -e "${RED}✗ BlackHole 64ch not found!${NC}"
    echo ""
    echo "Please install BlackHole from: https://existential.audio/blackhole/"
    echo "Or use: brew install blackhole-64ch"
    exit 1
fi
echo -e "${GREEN}✓ BlackHole 64ch found${NC}"
echo ""

# Check if tools are built
echo -e "${YELLOW}Checking build status...${NC}"
if [ ! -f "$BUILD_DIR/aes67-stream" ] || [ ! -f "$BUILD_DIR/aes67-monitor" ]; then
    echo -e "${RED}✗ Tools not built!${NC}"
    echo ""
    echo "Building tools..."
    
    # Build engine first
    cd "$PROJECT_ROOT/engine/build"
    cmake .. && make -j$(sysctl -n hw.ncpu)
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Engine build failed${NC}"
        exit 1
    fi
    
    # Build tools
    cd "$PROJECT_ROOT/tools/build"
    cmake .. && make -j$(sysctl -n hw.ncpu)
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Tools build failed${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ Build complete${NC}"
else
    echo -e "${GREEN}✓ Tools already built${NC}"
fi
echo ""

# Kill any existing processes
echo -e "${YELLOW}Cleaning up existing processes...${NC}"
pkill -f "aes67-stream" || true
pkill -f "aes67-monitor" || true
sleep 1
echo -e "${GREEN}✓ Cleanup complete${NC}"
echo ""

# Print setup instructions
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  SETUP INSTRUCTIONS${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${GREEN}Step 1: Configure Ableton Live${NC}"
echo "  1. Open Ableton Live"
echo "  2. Go to Preferences → Audio"
echo "  3. Set Audio Output Device to: ${GREEN}BlackHole 64ch${NC}"
echo "  4. Set Sample Rate to: ${GREEN}48000 Hz${NC}"
echo "  5. Click on the Output Channel Configuration"
echo "  6. Enable channels 1-$CHANNELS (or as many as you need)"
echo ""
echo -e "${YELLOW}Press Enter when Ableton is configured...${NC}"
read

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  STARTING PIPELINE${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Start the streamer (captures from BlackHole and sends to network)
echo -e "${YELLOW}Starting aes67-stream (BlackHole → Network)...${NC}"
echo "  Device: BlackHole 64ch"
echo "  Channels: $CHANNELS"
echo "  Sample Rate: $SAMPLE_RATE Hz"
echo "  Multicast: $MULTICAST_IP:$PORT"
echo ""

"$BUILD_DIR/aes67-stream" \
    --device "BlackHole 64ch" \
    --channels $CHANNELS \
    --sample-rate $SAMPLE_RATE \
    --stream-name "$STREAM_NAME" \
    --multicast-ip "$MULTICAST_IP" \
    --port $PORT \
    --interface $INTERFACE \
    > /tmp/aes67-stream.log 2>&1 &

STREAM_PID=$!
sleep 2

# Check if streamer started successfully
if ! ps -p $STREAM_PID > /dev/null; then
    echo -e "${RED}✗ Failed to start aes67-stream${NC}"
    echo ""
    echo "Log output:"
    cat /tmp/aes67-stream.log
    exit 1
fi
echo -e "${GREEN}✓ aes67-stream started (PID: $STREAM_PID)${NC}"
echo ""

# Start the web monitor (receives from network and displays)
echo -e "${YELLOW}Starting aes67-monitor (Network → Web)...${NC}"
echo "  HTTP Server: http://localhost:8080"
echo "  Listening on: $MULTICAST_IP:$PORT"
echo ""

"$BUILD_DIR/aes67-monitor" \
    --multicast-ip "$MULTICAST_IP" \
    --port $PORT \
    --interface $INTERFACE \
    --http-port 8080 \
    > /tmp/aes67-monitor.log 2>&1 &

MONITOR_PID=$!
sleep 2

# Check if monitor started successfully
if ! ps -p $MONITOR_PID > /dev/null; then
    echo -e "${RED}✗ Failed to start aes67-monitor${NC}"
    echo ""
    echo "Log output:"
    cat /tmp/aes67-monitor.log
    kill $STREAM_PID 2>/dev/null || true
    exit 1
fi
echo -e "${GREEN}✓ aes67-monitor started (PID: $MONITOR_PID)${NC}"
echo ""

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  PIPELINE RUNNING${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${GREEN}✓ Complete audio chain is running!${NC}"
echo ""
echo -e "${YELLOW}Audio Flow:${NC}"
echo "  Ableton → BlackHole 64ch → aes67-stream → Network → aes67-monitor → Web"
echo ""
echo -e "${GREEN}Open in your browser:${NC}"
echo "  http://localhost:8080"
echo ""
echo -e "${YELLOW}What to do now:${NC}"
echo "  1. Open http://localhost:8080 in your web browser"
echo "  2. Play audio in Ableton Live"
echo "  3. Watch the level meters in your browser update in real-time!"
echo ""
echo -e "${YELLOW}Logs:${NC}"
echo "  Stream log: tail -f /tmp/aes67-stream.log"
echo "  Monitor log: tail -f /tmp/aes67-monitor.log"
echo ""
echo -e "${RED}Press Ctrl+C to stop the pipeline${NC}"
echo ""

# Function to cleanup on exit
cleanup() {
    echo ""
    echo -e "${YELLOW}Stopping pipeline...${NC}"
    kill $STREAM_PID 2>/dev/null || true
    kill $MONITOR_PID 2>/dev/null || true
    echo -e "${GREEN}✓ Pipeline stopped${NC}"
    exit 0
}

trap cleanup INT TERM

# Keep script running and show live stats
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  LIVE STATUS (updates every 5 seconds)${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

while true; do
    sleep 5
    
    # Check if processes are still running
    if ! ps -p $STREAM_PID > /dev/null; then
        echo -e "${RED}✗ aes67-stream died unexpectedly${NC}"
        echo "Check log: /tmp/aes67-stream.log"
        kill $MONITOR_PID 2>/dev/null || true
        exit 1
    fi
    
    if ! ps -p $MONITOR_PID > /dev/null; then
        echo -e "${RED}✗ aes67-monitor died unexpectedly${NC}"
        echo "Check log: /tmp/aes67-monitor.log"
        kill $STREAM_PID 2>/dev/null || true
        exit 1
    fi
    
    echo -e "${GREEN}✓ Pipeline running... (Stream PID: $STREAM_PID, Monitor PID: $MONITOR_PID)${NC}"
done
