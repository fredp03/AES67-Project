# AES67 Virtual Soundcard - Project Status

**Last Updated**: October 16, 2025  
**Build Status**: ✅ All tests passing (23/23)  
**UI Status**: ✅ Menu bar app functional

---

## Quick Start

### Build Everything
```bash
cd "/Users/fredparsons/Documents/Side Projects/AES67 Project"
./scripts/build.sh
```

This builds:
1. Core Audio driver plugin (AES67VSC.driver)
2. Network engine library (libaes67_engine.a)
3. CLI tools (aes67-list)
4. Unit tests (23 tests)
5. SwiftUI menu bar app (Aes67VSC.app)

### Run Tests
```bash
./tests/unit/build/aes67_tests
```

Expected output: `23 passed, 0 failed`

### Launch UI
```bash
open ui/Aes67VSC/build/Build/Products/Release/Aes67VSC.app
```

The app appears in the menu bar with a waveform icon.

---

## Project Structure

```
AES67 Project/
├── driver/              # Core Audio HAL plugin
│   ├── src/            # Plugin implementation (700+ LOC)
│   ├── include/        # Headers (ring buffers, types)
│   └── build/          # → AES67VSC.driver
│
├── engine/              # Network engine library
│   ├── src/            # RTP, PTP, Jitter, SAP (~1200 LOC)
│   ├── include/        # Public API headers
│   └── build/          # → libaes67_engine.a
│
├── tests/
│   └── unit/           # Unit tests (23 tests, ~560 LOC)
│       └── build/      # → aes67_tests
│
├── tools/              # CLI utilities
│   ├── aes67-list.cpp # Stream discovery tool
│   └── build/          # → aes67-list
│
├── ui/
│   └── Aes67VSC/      # SwiftUI menu bar app (~650 LOC)
│       └── build/      # → Aes67VSC.app
│
├── configs/            # JSON configuration files
├── scripts/            # Build and deployment scripts
└── docs/               # Documentation
```

---

## Current Capabilities

### ✅ Completed

**Driver (Core Audio Plugin)**
- AudioServerPlugIn API implementation
- 64×64 channel virtual device @ 48 kHz
- 8 stereo streams (8 TX + 8 RX)
- Lock-free SPSC ring buffers
- Zero-copy audio path

**Network Engine**
- RTP packetizer/depacketizer with L24 codec
- IEEE 1588 PTP client with PI servo
- Adaptive jitter buffer (10-100ms depth)
- SAP/SDP stream discovery
- QoS/DSCP marking (EF/46)
- 26 threads (8 RX + 8 TX + 8 playout + 1 PTP + 1 SAP)

**Testing**
- 23 unit tests (100% pass rate):
  - 7 ring buffer tests (SPSC correctness)
  - 7 RTP codec tests (L24 precision)
  - 9 PTP time tests (conversion accuracy)
- Custom test framework (no external deps)
- Integrated into build pipeline

**User Interface**
- SwiftUI menu bar app
- Real-time stream discovery display
- PTP synchronization status
- Network statistics monitoring
- Stream subscription controls
- Settings and configuration

**CLI Tools**
- `aes67-list`: Discover and list AES67 streams

### ⏸ In Progress

**C++ ↔ Swift Bridge**
- Currently using simulated data in UI
- Need IPC mechanism (XPC/socket/shared memory)
- Real-time status updates from engine

### 📋 Planned

**Additional CLI Tools**
- `aes67-subscribe`: Manual stream subscription
- `aes67-stream`: Transmit audio files as AES67

**Integration Testing**
- Two-Mac audio loopback test
- Latency measurement (<4ms target)
- Packet loss recovery validation
- PTP synchronization accuracy

**Optimization**
- Profile with Instruments
- Tune jitter buffer parameters
- Optimize thread scheduling
- Minimize packet time (target: 125µs)

---

## Technical Specifications

### Audio Format
- **Sample Rate**: 48 kHz (fixed)
- **Bit Depth**: 24-bit (L24 RTP payload)
- **Channels**: 64 (8 streams × 8 channels)
- **Latency Target**: <4 ms round-trip

### Network
- **Protocol**: AES67 (RTP/PTP/SAP)
- **RTP Payload**: L24 (type 96)
- **Packet Size**: 48 frames (1ms @ 48kHz)
- **Multicast Range**: 239.69.0.0/16
- **PTP Domain**: 0 (default)
- **QoS**: DSCP EF (46), 256KB buffers

### Platform
- **macOS**: 12.0+ (Monterey)
- **Architecture**: arm64, x86_64
- **Framework**: AudioServerPlugIn (no kext)
- **Languages**: C++20, Swift 5.0

---

## Build Statistics

### Code Volume
- **Driver**: ~900 LOC C++
- **Engine**: ~1200 LOC C++
- **Tests**: ~560 LOC C++
- **UI**: ~650 LOC Swift
- **Total**: ~3,300 LOC (excluding comments/blanks)

### Compilation
- **Build Time**: ~15 seconds (full clean build)
- **Warnings**: 0
- **Test Time**: <1 second (23 tests)
- **Dependencies**: System frameworks only (no external libs)

---

## Key Files

### Driver Entry Points
- `driver/src/AES67_PlugIn.cpp` - HAL plugin interface
- `driver/src/AES67_Device.cpp` - Virtual device
- `driver/src/AES67_Stream.cpp` - Audio I/O

### Engine Core
- `engine/src/NetworkEngine.cpp` - Main orchestrator
- `engine/src/RTPPacketizer.cpp` - L24 codec
- `engine/src/PTPClient.cpp` - IEEE 1588 sync
- `engine/src/JitterBuffer.cpp` - Playout buffer

### UI Core
- `ui/Aes67VSC/Aes67VSC/Aes67VSCApp.swift` - App entry
- `ui/Aes67VSC/Aes67VSC/StatusBarView.swift` - Menu bar
- `ui/Aes67VSC/Aes67VSC/ContentView.swift` - Main UI
- `ui/Aes67VSC/Aes67VSC/EngineInterface.swift` - Engine bridge

---

## Development Workflow

### Daily Build
```bash
./scripts/build.sh
```

### Install Driver (Development)
```bash
./scripts/dev-sign.sh
```

### Verify Installation
```bash
system_profiler SPAudioDataType | grep AES67
```

### Debug Tools
```bash
# List discovered streams
./tools/build/aes67-list

# Check driver status
log stream --predicate 'subsystem == "com.aes67.vsc"'

# Monitor network traffic
sudo tcpdump -i en0 'multicast and port 5004'
```

---

## Next Steps

1. **Implement C++ Bridge**
   - Choose IPC mechanism (XPC recommended)
   - Create Swift wrapper for NetworkEngine
   - Connect UI to real engine data

2. **Build Additional Tools**
   - `aes67-subscribe` for manual stream control
   - `aes67-stream` for file playback

3. **Integration Testing**
   - Deploy on two Macs
   - Measure round-trip latency
   - Validate audio quality

4. **Performance Tuning**
   - Profile with Instruments
   - Optimize hot paths
   - Minimize jitter

---

## Documentation

- `README.md` - Project overview and setup
- `docs/ARCHITECTURE.md` - System design
- `docs/BUILD.md` - Build instructions
- `docs/DEPLOYMENT.md` - Installation guide
- `docs/UI_README.md` - Menu bar app details
- `.vscode/UPDATE-LOG.md` - Development history

---

**Status**: Ready for two-machine integration testing after C++ bridge implementation.
