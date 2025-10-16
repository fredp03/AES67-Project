# AES67 Virtual Soundcard for macOS - Project Instructions

## Project Overview

**Goal:** Build a production-grade macOS virtual soundcard implementing AES67 audio-over-IP standards for professional studio networking.

**Target Performance:**
- 64×64 channels full-duplex @ 48 kHz, 24-bit
- < 4 ms round-trip latency (Mac1 → Mac2 → Mac1)
- Sample-accurate PTP synchronization
- Zero kernel extensions (userland only)

## Architecture Components

### 1. Core Audio HAL Driver (`driver/`)
- **Type:** AudioServerPlugIn bundle
- **Location:** `/Library/Audio/Plug-Ins/HAL/AES67VSC.driver`
- **Language:** C++20
- **Responsibilities:**
  - Expose virtual audio device to macOS
  - Handle I/O callbacks from DAWs/audio apps
  - Manage zero-copy ring buffers to network engine
  - PTP-disciplined device clock

### 2. Network Engine (`engine/`)
- **Type:** Static library (linked into driver + UI)
- **Language:** C++20
- **Responsibilities:**
  - RTP L24 packetization/depacketization (RFC 3190)
  - PTP client (IEEE 1588-2008) with PI servo
  - Adaptive jitter buffer (2-4 packets)
  - SAP/SDP announcements (multicast discovery)
  - QoS/DSCP marking for traffic prioritization
  - 8 streams × 8 channels per direction

### 3. Menu Bar UI (`ui/`)
- **Type:** SwiftUI application
- **Framework:** macOS 12+ SwiftUI
- **Responsibilities:**
  - Live PTP status (offset, jitter, lock state)
  - Per-stream level meters
  - Configuration (packet time, interface, profiles)
  - Latency/xrun monitoring

### 4. CLI Tools (`tools/`)
- **Utilities:**
  - `aes67-list` - Discover streams via SAP
  - `aes67-subscribe` - Subscribe to specific stream
  - `aes67-publish` - Publish test stream
  - `aes67-stats` - Real-time PTP/RTP statistics

## Implementation Phases

### ✅ Phase 1: Scaffolding (COMPLETE)
- [x] Repository structure
- [x] Driver skeleton (compiles, loads)
- [x] Engine headers (complete API)
- [x] Build scripts
- [x] Documentation

### Phase 2: Core Functionality (NEXT)
- [ ] RTP Packetizer (L24 encoding/decoding)
- [ ] PTP Client (sync, servo, affine mapping)
- [ ] Jitter Buffer (adaptive playout)
- [ ] Wire driver ↔ engine data path
- [ ] SAP/SDP announcements

**Deliverable:** Audio flows between two Macs

### Phase 3: User Interface
- [ ] SwiftUI menu bar app
- [ ] Live status dashboard
- [ ] Configuration panels
- [ ] CLI tools

**Deliverable:** User can configure and monitor streams

### Phase 4: Optimization
- [ ] Low-latency profile (250 µs packet time)
- [ ] Measure round-trip latency
- [ ] Tune jitter buffer depth
- [ ] CPU optimization

**Deliverable:** < 4 ms round-trip verified

### Phase 5: Polish & Interop
- [ ] Full AES67 SDP attributes
- [ ] Interop profile (1 ms packet time)
- [ ] Error recovery (network loss)
- [ ] Integration tests
- [ ] Third-party tool verification

**Deliverable:** Production-ready release

## Technical Requirements

### Format Specifications
- **Sample Rate:** 48 kHz (fixed, AES67 mandatory)
- **Bit Depth:** 24-bit in 32-bit containers
- **Format Flags:** `kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked`
- **Network Payload:** L24 (3 bytes per sample, big-endian)

### Latency Budget (Low-Latency Profile)
```
Component                  Target
─────────────────────────────────────
Core Audio I/O buffer      0.67 ms   (32 frames)
RTP packet time            0.25 ms   (250 µs, 12 frames)
Jitter buffer (2-3 pkts)   0.50 ms
Network + switch           0.50 ms   (per hop)
Processing overhead        0.20 ms
─────────────────────────────────────
One-way total             ~2.1 ms
Round-trip                <4.0 ms   ✓ TARGET
```

### Stream Configuration
- **Total Channels:** 64 input + 64 output
- **Stream Count:** 8 TX + 8 RX
- **Channels per Stream:** 8
- **Multicast Base:** 239.69.1.x (TX), 239.69.2.x (RX)
- **Port:** 5004 (standard RTP audio)
- **MTU Constraint:** Packets < 1200 bytes

### PTP Requirements
- **Domain:** Selectable (default 0)
- **Lock Threshold:** < 500 ns offset, < 100 ns jitter
- **Sync Interval:** 1/sec or faster
- **Servo:** PI controller with adaptive gain
- **Time Mapping:** Affine `host_time = a * ptp_time + b`

## Development Guidelines

### Code Standards
- **C++ Version:** C++20 minimum
- **Compiler:** Clang (Xcode)
- **Warnings:** `-Wall -Wextra -Wpedantic`
- **Optimization:** `-O3 -march=native` for release
- **Style:** LLVM (use `clang-format`)

### Real-Time Constraints
- **Audio Thread:** NEVER allocate, lock, or block
- **Memory:** Pre-allocate all buffers
- **Synchronization:** Lock-free queues only
- **Logging:** Ring buffer, flush async

### Testing Strategy
1. **Unit Tests:** Ring buffers, RTP encode/decode, timestamp math
2. **Loopback Test:** Same machine, verify bit-true round-trip
3. **Two-Machine Test:** Measure actual latency
4. **Interop Test:** Third-party AES67 tools (Merging, Dante, Ravenna)
5. **Stress Test:** 60 min full-duplex, no xruns

## Build & Install

### Prerequisites
```bash
# macOS 12.0+ (Monterey or later)
# Xcode 14.0+ with Command Line Tools
xcode-select --install

# CMake 3.20+
brew install cmake
```

### Build Commands
```bash
cd "/Users/fredparsons/Documents/Side Projects/AES67 Project"

# Build all components
./scripts/build.sh

# Sign and install driver (requires sudo)
./scripts/dev-sign.sh

# Verify installation
system_profiler SPAudioDataType | grep -A 5 "AES67"
```

### Development Workflow
```bash
# Rebuild after changes
cd driver/build && make -j$(sysctl -n hw.ncpu)

# Reinstall driver
sudo cp -R AES67VSC.driver /Library/Audio/Plug-Ins/HAL/
sudo launchctl kickstart -k system/com.apple.audio.coreaudiod

# Check logs
log show --predicate 'subsystem == "com.apple.audio"' --last 5m | grep -i aes67
```

## Key Files

### Driver Entry Points
- `driver/src/AES67_PlugIn.cpp` - HAL dispatch table, plugin factory
- `driver/src/AES67_Device.cpp` - Device lifecycle, properties
- `driver/src/AES67_Stream.cpp` - I/O operations, ring buffer interface

### Engine Core
- `engine/src/NetworkEngine.cpp` - Main engine, thread management
- `engine/src/RTPPacketizer.cpp` - L24 encoding/decoding **[IMPLEMENT NEXT]**
- `engine/src/PTPClient.cpp` - PTP protocol, servo **[IMPLEMENT NEXT]**
- `engine/src/JitterBuffer.cpp` - Playout scheduling **[IMPLEMENT NEXT]**

### Configuration
- `configs/default.sdp` - 8 stream templates
- `configs/qos.json` - Latency profiles (low-latency, interop, broadcast)

### Documentation
- `README.md` - User documentation
- `MILESTONE-1.md` - Phase 1 completion notes
- `NEXT-STEPS.md` - Quick reference for resuming work

## Known Issues & TODOs

### Critical (Blocks Audio Flow)
- ❌ RTP packetizer not implemented (stub only)
- ❌ PTP client not implemented (stub only)
- ❌ Jitter buffer not implemented (stub only)
- ❌ Network threads not started

### Medium Priority
- ⚠️ No UI application yet
- ⚠️ No CLI tools yet
- ⚠️ No integration tests
- ⚠️ No logging system

### Low Priority
- ⚠️ Config loaded from hardcoded defaults
- ⚠️ No runtime reconfiguration
- ⚠️ No error recovery for network loss

## Success Criteria

### Milestone 1 ✅
- [x] Driver loads and appears in Audio MIDI Setup
- [x] Accepts connections at 48 kHz
- [x] Build system works end-to-end

### Milestone 2 (Target)
- [ ] Audio flows between two Macs
- [ ] PTP locked (< 1 µs offset)
- [ ] No dropouts for 5 minutes

### Milestone 3 (Target)
- [ ] UI shows live status
- [ ] User can configure streams
- [ ] CLI tools work

### Milestone 4 (Target)
- [ ] Round-trip < 4 ms measured
- [ ] Bit-true loopback test passes
- [ ] No xruns under load

### Milestone 5 (Target)
- [ ] Interop with third-party AES67 tools
- [ ] Full test suite passes
- [ ] Production-ready release

## Resources

### Standards
- **AES67-2018:** Audio-over-IP interoperability
- **IEEE 1588-2008:** Precision Time Protocol v2
- **RFC 3550:** RTP: A Transport Protocol for Real-Time Applications
- **RFC 3190:** RTP Payload Format for 12/20/24-bit Audio

### Apple Documentation
- Core Audio Programming Guide
- AudioServerPlugIn.h reference
- Technical Note TN2091: Device Drivers

### Reference Implementations
- BlackHole (virtual audio driver)
- Dante Virtual Soundcard (commercial AES67)
- Ravenna ALSA driver (Linux AES67)

## Contact & Support

- **Repository:** github.com/fredp03/AES67-Project
- **Issues:** Use GitHub Issues for bugs/features
- **Documentation:** See `README.md` and Wiki

---

**Last Updated:** 2025-10-16  
**Current Phase:** 2 (Core Functionality)  
**Next Action:** Implement RTP Packetizer in `engine/src/RTPPacketizer.cpp`
