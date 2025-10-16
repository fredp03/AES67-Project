# AES67 Virtual Soundcard - SwiftUI Menu Bar App

## Overview
The Aes67VSC menu bar application provides a macOS native interface for monitoring and controlling the AES67 Virtual Soundcard driver and network engine.

## Features

### Menu Bar Integration
- **Status Bar Icon**: Shows waveform icon with popover on click
- **Always Available**: Runs as background app (LSUIElement = YES)
- **Native macOS**: Built with SwiftUI for modern macOS 12.0+

### User Interface

#### 1. Streams Tab
- **Discovered Streams List**
  - Real-time display of SAP-discovered AES67 streams
  - Shows stream name, multicast address, channel count, sample rate
  - Visual indicator for active streams
  - Context menu for stream subscription
  - Copy multicast address to clipboard
  
- **Subscribed Streams**
  - List of currently subscribed streams
  - Quick unsubscribe button
  - Compact display at bottom of streams view

#### 2. Status Tab
- **PTP Synchronization**
  - Lock status (Locked/Unlocked)
  - Offset in microseconds
  - Rate ratio (frequency correction)
  
- **Network Statistics**
  - Active stream count
  - RX/TX packet counts
  - Packet loss detection
  
- **Audio Performance**
  - Sample rate (48000 Hz fixed)
  - Buffer underrun count
  - Buffer overrun count

#### 3. Settings Tab
- **Network Interface Selection**
  - Choose which interface to use for AES67 traffic
  - Lists available network interfaces (en0, en1, en2)
  
- **Engine Control**
  - Start/Stop engine button
  - Visual indication of engine state
  
- **About Section**
  - Version information
  - Configuration summary

### Header Section
- Shows PTP lock status with colored icon
  - Green clock (filled) when locked
  - Orange clock (outline) when unlocked
- Real-time offset display
- Engine running indicator (green/red dot)

## Architecture

### Swift Files

#### `Aes67VSCApp.swift`
- Main app entry point with `@main` attribute
- AppDelegate for menu bar setup
- Creates StatusBarController on launch

#### `StatusBarView.swift`
- `StatusBarController` manages NSStatusItem
- Creates menu bar icon with popover
- Handles popover show/hide
- Auto-starts engine on launch

#### `ContentView.swift`
- Main container view with tabs
- `HeaderView`: Status display at top
- `StatusView`: System metrics and performance
- `SettingsView`: Configuration and controls
- `FooterView`: Quit button

#### `StreamListView.swift`
- `StreamListView`: Main streams interface
- `StreamRow`: Individual stream display with metadata
- `SubscribedStreamRow`: Compact subscribed stream entry
- Empty state when no streams discovered

#### `EngineInterface.swift`
- Bridge between Swift UI and C++ engine
- `@Published` properties for reactive updates
- Timer-based polling (1 Hz) for status updates
- Simulated data for development (TODO: C++ bridge)
- `DiscoveredStream` model struct

### Data Flow
```
C++ NetworkEngine
       ↓
EngineInterface (Swift wrapper)
       ↓
@Published properties
       ↓
SwiftUI Views (auto-update)
```

## TODO: C++ Bridge Integration

Currently, `EngineInterface.swift` uses simulated data. To integrate with the real C++ engine:

### Option 1: XPC Service (Recommended)
```swift
// Create XPC service wrapper around NetworkEngine
// Communicate via Mach IPC
let connection = NSXPCConnection(serviceName: "com.aes67.engine")
```

### Option 2: Embedded C++ (Complex)
```swift
// Create Objective-C++ wrapper
// Bridge C++ NetworkEngine to Swift
@objc class NetworkEngineBridge: NSObject {
    private var engine: UnsafeMutablePointer<AES67::NetworkEngine>
    // ... bridging methods
}
```

### Option 3: Unix Domain Socket
```swift
// Engine runs as daemon
// Swift communicates via socket
let socket = Socket.create(family: .unix, type: .stream)
```

## Key Integration Points

Methods to bridge from C++ to Swift:

1. **Start/Stop Engine**
   ```cpp
   NetworkEngine::Start(const char* interfaceName)
   NetworkEngine::Stop()
   ```

2. **Stream Discovery**
   ```cpp
   std::vector<std::string> NetworkEngine::GetDiscoveredStreamNames()
   StreamInfo NetworkEngine::GetDiscoveredStream(const std::string& name)
   ```

3. **Status Monitoring**
   ```cpp
   bool PTPClient::IsLocked()
   double PTPClient::GetOffsetNs()
   double PTPClient::GetRateRatio()
   ```

4. **Statistics**
   ```cpp
   // Add to NetworkEngine:
   uint64_t GetRxPacketCount()
   uint64_t GetTxPacketCount()
   uint64_t GetPacketLossCount()
   ```

## Building

The app is built automatically by `./scripts/build.sh`:

```bash
cd ui/Aes67VSC
xcodebuild -project Aes67VSC.xcodeproj \
           -scheme Aes67VSC \
           -configuration Release \
           -derivedDataPath build
```

Output: `ui/Aes67VSC/build/Build/Products/Release/Aes67VSC.app`

## Running

```bash
# From build script
open ui/Aes67VSC/build/Build/Products/Release/Aes67VSC.app

# Or install to Applications
cp -r ui/Aes67VSC/build/Build/Products/Release/Aes67VSC.app /Applications/
```

The app will:
1. Create menu bar icon (waveform)
2. Auto-start the network engine
3. Begin discovering streams
4. Update status every second

## Code Statistics

- **Total Lines**: ~650 LOC Swift
- **Files**: 5 Swift source files + Xcode project
- **Size**: ~200 KB app bundle
- **Dependencies**: SwiftUI, AppKit (system frameworks only)

## Future Enhancements

- [ ] Real C++ engine integration
- [ ] Stream audio level meters
- [ ] Latency graph/histogram
- [ ] Export stream configuration
- [ ] Manual stream entry (non-SAP)
- [ ] Notification center integration
- [ ] Launch at login option
- [ ] Preferences window
- [ ] Log viewer
