# AES67 Virtual Soundcard for macOS

**Production-grade AES67/PTP audio-over-IP driver with < 4ms round-trip latency**

A fully-featured, low-latency virtual soundcard implementing AES67 standards for professional audio networking on macOS. Provides 64×64 channels at 48 kHz/24-bit with PTPv2 synchronization.

## Features

- **64×64 full-duplex channels** @ 48 kHz, 24-bit (32-bit containers)
- **Sub-4ms round-trip latency** (Mac → Mac → Mac over gigabit Ethernet)
- **AES67 compliant**: RTP L24 payload, SDP/SAP discovery, PTPv2 clock sync
- **Dual packet-time modes**: 250 µs (low-latency) and 1 ms (interop)
- **Zero kernel extensions**: Pure userland AudioServerPlugIn + network engine
- **Menu bar control app**: Live PTP status, latency metrics, per-stream metering
- **QoS/DSCP marking**: Priority traffic shaping for RTP and PTP
- **Multicast & unicast**: Full IGMP support, SAP announcements
- **CLI tools**: Publish, subscribe, list streams, monitor stats

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  DAW / Audio App (Logic, Ableton, etc.)                      │
└────────────────┬────────────────────────────────────────────┘
                 │ Core Audio API
┌────────────────▼────────────────────────────────────────────┐
│  AES67 HAL Driver (AudioServerPlugIn)                        │
│  • 64×64 @ 48kHz fixed format                                │
│  • PTP-disciplined device clock                              │
│  • 32-64 frame I/O buffers                                   │
└────────────────┬────────────────────────────────────────────┘
                 │ Lock-free SPSC rings
┌────────────────▼────────────────────────────────────────────┐
│  Network Engine (C++)                                        │
│  • RTP L24 packetizer (8 streams × 8ch each)                 │
│  • Adaptive jitter buffer (2-4 packets)                      │
│  • PTP grandmaster (IEEE-1588v2) broadcasting Sync/Announce  │
│  • SAP/SDP announcements                                     │
│  • QoS/DSCP marking                                          │
└────────────────┬────────────────────────────────────────────┘
                 │ UDP/IP, Cat6 Ethernet
┌────────────────▼────────────────────────────────────────────┐
│  Gigabit Switch (PTP-aware recommended)                      │
└─────────────────────────────────────────────────────────────┘
```

## Latency Budget (Low-Latency Profile)

| Stage                    | Target       | Notes                          |
|--------------------------|--------------|--------------------------------|
| Core Audio I/O buffer    | 0.67-1.33 ms | 32-64 frames @ 48kHz           |
| RTP packet time          | 0.25 ms      | 250 µs, 12 frames              |
| Jitter buffer            | 0.5-0.75 ms  | 2-3 packets                    |
| NIC + stack + switch     | 0.4-1.0 ms   | Per-hop overhead               |
| **Total one-way**        | **1.8-3.3 ms** |                              |
| **Round-trip target**    | **< 4.0 ms** | Mac1 → Mac2 → Mac1            |

## Requirements

- macOS 12.0+ (Monterey or later)
- Apple Silicon or Intel x86_64
- Xcode 14.0+ with Command Line Tools
- CMake 3.20+
- Gigabit Ethernet adapter (Cat6 cable)
- Managed switch with IGMP snooping (PTP-aware preferred)

## Quick Start

### 1. Build & Install

```bash
# Clone repository
git clone https://github.com/yourusername/aes67-vsc.git
cd aes67-vsc

# Build all components
./scripts/build.sh

# Sign and install HAL driver (requires sudo for /Library/Audio/Plug-Ins/HAL/)
./scripts/dev-sign.sh
```

### 2. Configure Network

On both Macs:
```bash
# Disable Energy Efficient Ethernet (adds jitter)
sudo sysctl -w kern.ipc.maxsockbuf=8388608

# Set interface (replace en0 with your Ethernet adapter)
networksetup -setmtu en0 1500
```

On your switch:
- Enable IGMP snooping
- Enable PTP forwarding (if available)
- Disable flow control / pause frames
- Set all ports to fixed 1000 Mbps, full-duplex

### 3. Run

```bash
# On Mac1 (PTP master + receiver)
./scripts/run-ptp.sh master

# On Mac2 (PTP slave + sender)
./scripts/run-ptp.sh slave

# Launch menu bar app (on both)
open ui/build/Release/Aes67VSC.app
```

### 4. Verify

```bash
# Check device is visible
system_profiler SPAudioDataType | grep -A 10 "AES67"

# List discovered streams
./tools/build/aes67-list

# Run latency test (from Mac1, after starting streams on both)
./scripts/latency-test.sh
```

## Configuration Profiles

### Low-Latency (Mac-to-Mac)
- **I/O Buffer**: 32 frames (0.67 ms)
- **Packet Time**: 250 µs (12 samples)
- **Jitter Buffer**: 2-4 packets
- **Use Case**: Studio playback/recording, live monitoring
- **Round-trip**: 2.5-3.8 ms typical

### Interop (AES67 Standard)
- **I/O Buffer**: 64 frames (1.33 ms)
- **Packet Time**: 1 ms (48 samples)
- **Jitter Buffer**: 1-2 packets
- **Use Case**: Multi-vendor AES67 networks, broadcast
- **Round-trip**: 4-6 ms typical

Switch profiles in the menu bar app or via CLI:
```bash
./tools/build/aes67-configure --profile low-latency
```

## PTP Synchronization

AES67 requires PTP (IEEE 1588-2008) for sample-accurate timing:

- **Built-in Grandmaster**: Starting the network engine now turns the Mac into a PTPv2 master, multicasting Sync and Announce traffic on the selected interface.
- **Privileges**: Binding to the well-known PTP ports (319/320) demands administrator rights. Run `aes67-monitor`, `aes67-stream`, or any tool that starts the engine with `sudo` (the helper scripts prompt automatically).
- **Domain**: Configurable (default: 0)
- **Announce Interval**: 1/sec (AES67 default)
- **Sync Interval**: 8/sec (125 ms)
- **Servo**: Master uses system clock; slave mode still available for interop work
- **Lock Threshold**: < 500 ns offset, < 100 ns jitter

Monitor PTP status:
```bash
./tools/build/aes67-stats --ptp
```

Expected output:
```
PTP State: SLAVE
Grand Master: 00:1b:21:ab:cd:ef (priority 128)
Offset from Master: +127 ns (mean), ±45 ns (jitter)
Path Delay: 342 ns
Lock Quality: EXCELLENT
```

## Stream Configuration

Channels are organized into 8 streams (Tx and Rx) to optimize MTU and IRQ distribution:

| Stream | Channels | Multicast (default)  | DSCP |
|--------|----------|----------------------|------|
| TX 1   | 1-8      | 239.69.1.1:5004      | EF   |
| TX 2   | 9-16     | 239.69.1.2:5004      | EF   |
| TX 3   | 17-24    | 239.69.1.3:5004      | EF   |
| TX 4   | 25-32    | 239.69.1.4:5004      | EF   |
| TX 5   | 33-40    | 239.69.1.5:5004      | EF   |
| TX 6   | 41-48    | 239.69.1.6:5004      | EF   |
| TX 7   | 49-56    | 239.69.1.7:5004      | EF   |
| TX 8   | 57-64    | 239.69.1.8:5004      | EF   |

RX streams mirror this layout on .2.x range. Edit `configs/default.sdp` to customize.

## Troubleshooting

### Device Not Appearing in Audio MIDI Setup

```bash
# Check if driver is installed
ls -la /Library/Audio/Plug-Ins/HAL/AES67VSC.driver

# Verify code signature
codesign -vvv /Library/Audio/Plug-Ins/HAL/AES67VSC.driver

# Restart Core Audio daemon
sudo launchctl kickstart -k system/com.apple.audio.coreaudiod

# Check system logs
log show --predicate 'subsystem == "com.apple.audio"' --last 5m | grep -i aes67
```

### High Latency / Dropouts

1. **Check PTP lock**: Must show < 1 µs offset
2. **Verify network path**: Use `ping -i 0.001` to check for jitter
3. **Disable WiFi**: Ensure wired Ethernet is active
4. **Check CPU load**: Menu bar app shows %; stay < 50%
5. **Increase jitter buffer**: Try 4-6 packets in UI
6. **Disable EEE**: `ethtool -s en0 eee off` (if ethtool available)

### No PTP Lock

```bash
# Verify PTP packets are flowing (requires tcpdump)
sudo tcpdump -i en0 -n ether proto 0x88f7

# Check firewall rules
sudo pfctl -sr | grep -i ptp

# Force PTP domain/interface in UI or:
./tools/build/aes67-configure --ptp-domain 0 --interface en0
```

### Streams Not Discovered

```bash
# Check SAP multicast (239.255.255.255:9875)
./tools/build/aes67-list --verbose

# Manually subscribe with SDP file
./tools/build/aes67-subscribe --sdp configs/default.sdp

# Verify IGMP join
netstat -g | grep 239.69
```

## Performance Tuning

### macOS System Settings

```bash
# Increase socket buffer sizes
sudo sysctl -w kern.ipc.maxsockbuf=8388608
sudo sysctl -w net.inet.udp.recvspace=2097152

# Disable App Nap for menu bar app
defaults write com.aes67.vsc NSAppSleepDisabled -bool YES

# Set network interface to full-duplex
# (Use System Preferences > Network > Advanced > Hardware)
```

### Recommended Switch Settings

- **Port Speed**: Fixed 1000 Mbps, full-duplex (disable auto-negotiation)
- **Flow Control**: OFF
- **Jumbo Frames**: Optional (MTU 9000) for future expansion
- **IGMP Snooping**: ON (v2 minimum, v3 preferred)
- **PTP**: Enable TC (Transparent Clock) if available
- **QoS**: Trust DSCP markings, strict priority for EF
- **Storm Control**: Disable for PTP/RTP ports

### Real-Time Thread Priorities

Engine sets threads with these QoS classes (automatic):
- **Audio I/O**: `QOS_CLASS_USER_INTERACTIVE` + real-time constraints
- **PTP Servo**: `QOS_CLASS_USER_INITIATED`
- **SAP Announcements**: `QOS_CLASS_UTILITY`

## Development

### Building from Source

```bash
# Install dependencies (if using Homebrew)
brew install cmake

# Generate build files
mkdir -p driver/build engine/build tools/build
cd driver/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(sysctl -n hw.ncpu)
cd ../../engine/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(sysctl -n hw.ncpu)
cd ../../tools/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(sysctl -n hw.ncpu)

# Build UI (Xcode)
cd ../../ui
xcodebuild -project Aes67VSC.xcodeproj -scheme Aes67VSC -configuration Release
```

### Running Tests

```bash
# Unit tests
cd tests/unit/build && ctest --output-on-failure

# Integration test (requires two machines or loopback)
./tests/integration/loopback-test.sh

# Latency measurement
./scripts/latency-test.sh --samples 1000 --report latency-report.json
```

### Debugging

```bash
# Enable verbose logging (written to ~/Library/Logs/aes67-vsc/)
./tools/build/aes67-configure --log-level debug

# Attach debugger to coreaudiod (driver)
sudo lldb -p $(pgrep coreaudiod)

# Monitor real-time stats
watch -n 0.5 './tools/build/aes67-stats --json'
```

## Architecture Details

### Driver (AudioServerPlugIn)

- **Location**: `/Library/Audio/Plug-Ins/HAL/AES67VSC.driver`
- **Language**: C++20
- **Format**: 48 kHz, 24-bit in 32-bit containers (LPCM)
- **Channels**: 64 input, 64 output
- **Clock**: PTP-disciplined, exposes affine host→PTP mapping
- **I/O**: Zero-copy via lock-free SPSC rings to engine
- **Latency**: 32-frame default (configurable to 64/128)

### Engine (Network Layer)

- **Location**: Embedded in menu bar app + CLI tools
- **Language**: C++20 (potential Rust FFI for PTP)
- **RTP**: L24 payload type, 96 kHz timestamp clock
- **Packetization**: 8 streams × 8 channels, 250 µs or 1 ms ptime
- **PTP**: IEEE 1588-2008 v2, domains 0-127, BMC algorithm
- **Jitter Buffer**: Time-aligned playout, adaptive depth (2-6 packets)
- **Discovery**: SAP/SDP (multicast 239.255.255.255:9875)
- **QoS**: DSCP EF (46) for RTP, CS7 (56) for PTP

### UI (Control Panel)

- **Framework**: SwiftUI (macOS 12+)
- **Type**: Menu bar agent (LSUIElement)
- **Features**:
  - Live PTP offset/jitter graph
  - Per-stream level meters (dBFS)
  - Xrun/dropout counters
  - Latency histogram
  - Stream selector (enable/disable per 8-ch block)
  - Profile switcher (Low-Latency / Interop)
  - Network interface picker
  - SDP editor

## License

MIT License - see [LICENSE](LICENSE) file.

## Contributing

Contributions welcome! Please:
1. Follow C++ Core Guidelines
2. Run `clang-format` (LLVM style) before committing
3. Add unit tests for new features
4. Update documentation

## References

- [AES67-2018 Standard](https://www.aes.org/publications/standards/search.cfm?docID=96) - Audio-over-IP interoperability
- [IEEE 1588-2008](https://standards.ieee.org/standard/1588-2008.html) - Precision Time Protocol v2
- [RFC 3550](https://tools.ietf.org/html/rfc3550) - RTP: A Transport Protocol for Real-Time Applications
- [RFC 3190](https://tools.ietf.org/html/rfc3190) - RTP Payload Format for 12/20/24-bit Audio
- [Core Audio Documentation](https://developer.apple.com/documentation/coreaudio) - macOS Audio HAL

## Support

- **Issues**: https://github.com/yourusername/aes67-vsc/issues
- **Wiki**: https://github.com/yourusername/aes67-vsc/wiki
- **Discussions**: https://github.com/yourusername/aes67-vsc/discussions

---

**Built for professional audio engineers who demand sample-accurate, low-latency networking.**
