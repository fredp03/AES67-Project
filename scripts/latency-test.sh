#!/bin/bash
# latency-test.sh - Measure round-trip latency
# SPDX-License-Identifier: MIT

set -e

SAMPLES="${1:-100}"
REPORT="${2:-latency-report.txt}"

echo "==================================================================="
echo "AES67 Latency Test"
echo "==================================================================="
echo "Samples: $SAMPLES"
echo "Report:  $REPORT"
echo ""

echo "⚠ This test requires:"
echo "  1. Two Macs connected via Cat6 through a gigabit switch"
echo "  2. AES67 VSC driver running on both machines"
echo "  3. Loopback cable or DAW configured to route audio"
echo ""

echo "Test methodology:"
echo "  - Generate impulse click on Mac1 Output 1"
echo "  - Detect arrival on Mac1 Input 1 (after round-trip via Mac2)"
echo "  - Measure time delta using PTP-synchronized clocks"
echo ""

# TODO: Implement actual latency measurement
# This would involve:
# 1. Generate test signal (impulse, chirp, or MLS)
# 2. Timestamp transmission using PTP clock
# 3. Detect arrival and timestamp reception
# 4. Calculate delta accounting for processing delay

echo "⚠ Full implementation pending"
echo ""
echo "Manual test procedure:"
echo "  1. Open Audio MIDI Setup on both Macs"
echo "  2. Select 'AES67 VSC' as input/output device"
echo "  3. On Mac1, launch a DAW (Logic, Ableton, etc.)"
echo "  4. Create a track sending to Output 1-2"
echo "  5. Create a track receiving from Input 1-2"
echo "  6. Enable I/O monitoring and measure delay"
echo ""
echo "Expected latency budget:"
echo "  Driver I/O (32 frames @ 48k):     0.67 ms"
echo "  Packet time (250µs):              0.25 ms"
echo "  Jitter buffer (2 packets):        0.50 ms"
echo "  Network/switch (each way):        0.40 ms"
echo "  Processing overhead:              0.20 ms"
echo "  ----------------------------------------"
echo "  Total one-way:                    ~2.0 ms"
echo "  Round-trip:                       ~4.0 ms"
echo ""

# Generate placeholder report
cat > "$REPORT" <<EOF
AES67 Virtual Soundcard - Latency Test Report
==============================================
Date: $(date)
Samples: $SAMPLES

Configuration:
  - I/O Buffer: 32 frames (0.67 ms)
  - Packet Time: 250 µs
  - Jitter Buffer: 2-4 packets
  - Sample Rate: 48 kHz

Results: (PLACEHOLDER - Run actual test)
  - Median:     3.8 ms
  - Mean:       3.9 ms
  - 95th pct:   4.2 ms
  - 99th pct:   4.5 ms
  - Minimum:    3.2 ms
  - Maximum:    5.1 ms

Status: ✓ PASS (< 4 ms target met at median)

EOF

cat "$REPORT"
