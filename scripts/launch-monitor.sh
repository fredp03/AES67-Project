#!/bin/bash
# Quick launch script for web monitor
# Usage: ./launch-monitor.sh [channels]

CHANNELS=${1:-8}
PORT=8080

cd "$(dirname "$0")/../tools/build"

echo "🎵 Launching AES67 Web Monitor"
echo "   Channels: $CHANNELS"
echo "   URL: http://localhost:$PORT"
echo ""

# Check if already running
if lsof -Pi :$PORT -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    echo "⚠️  Port $PORT already in use!"
    echo "   Kill existing monitor or use different port"
    exit 1
fi

# Launch monitor
./aes67-monitor --channels "$CHANNELS" --port "$PORT" &
MONITOR_PID=$!

# Wait for server
sleep 1

# Check if running
if kill -0 $MONITOR_PID 2>/dev/null; then
    echo "✓ Monitor running (PID: $MONITOR_PID)"
    echo ""
    echo "Opening browser..."
    
    # Try to open browser
    if command -v open >/dev/null 2>&1; then
        open "http://localhost:$PORT"
    elif command -v xdg-open >/dev/null 2>&1; then
        xdg-open "http://localhost:$PORT"
    else
        echo "Please open: http://localhost:$PORT"
    fi
    
    echo ""
    echo "Press Ctrl+C in the terminal where monitor is running to stop"
else
    echo "❌ Failed to start monitor"
    exit 1
fi
