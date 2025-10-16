#!/bin/bash
# run-ptp.sh - Start PTP daemon (master or slave)
# SPDX-License-Identifier: MIT

set -e

MODE="${1:-slave}"  # master or slave
INTERFACE="${2:-en0}"
DOMAIN="${3:-0}"

echo "==================================================================="
echo "AES67 PTP Daemon"
echo "==================================================================="
echo "Mode:      $MODE"
echo "Interface: $INTERFACE"
echo "Domain:    $DOMAIN"
echo ""

# Check if we have a PTP implementation
# NOTE: macOS doesn't have native PTPd; we'll use the engine's built-in PTP
# This script is a placeholder for launching the PTP component

TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/tools/build"

if [ "$MODE" == "master" ]; then
    echo "Starting PTP master (Grandmaster)..."
    # TODO: Launch engine in GM mode
    echo "⚠ PTP master mode not yet implemented"
    echo "   Engine will run as boundary clock by default"
elif [ "$MODE" == "slave" ]; then
    echo "Starting PTP slave..."
    # TODO: Launch engine in slave mode
    echo "⚠ PTP slave mode not yet implemented"
    echo "   Engine will auto-detect and sync to best master"
else
    echo "ERROR: Mode must be 'master' or 'slave'"
    exit 1
fi

echo ""
echo "PTP will run automatically when the audio device starts I/O"
echo "Monitor status with: $TOOLS_DIR/aes67-stats --ptp"
echo ""
