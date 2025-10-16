# AES67 CLI Tools Documentation

## Overview
Command-line utilities for managing, testing, and debugging AES67 streams.

---

## aes67-list

**Purpose**: Discover and list all AES67 streams on the network via SAP/SDP.

### Usage
```bash
./tools/build/aes67-list [options]
```

### Options
- `-i, --interface <name>` - Network interface (default: en0)
- `-d, --duration <sec>` - Listen duration in seconds (default: 10)
- `-v, --verbose` - Show detailed stream information
- `-h, --help` - Display help message

### Examples
```bash
# List streams for 10 seconds
./tools/build/aes67-list

# Listen on specific interface for 30 seconds
./tools/build/aes67-list -i en1 -d 30

# Verbose output with full SDP details
./tools/build/aes67-list -v
```

### Output Format
```
AES67 Stream Discovery
======================
Interface: en0
Duration:  10 seconds

Waiting for SAP announcements...

Discovered Streams:
-------------------

Stream: "Mac Studio Audio"
  Address:     239.69.1.1:5004
  Channels:    8
  Sample Rate: 48000 Hz
  PTP Clock:   00:1b:21:ab:cd:ef:00:00

Stream: "Console Mix"
  Address:     239.69.1.2:5004
  Channels:    2
  Sample Rate: 48000 Hz
  PTP Clock:   00:1b:21:ab:cd:ef:00:00
```

---

## aes67-subscribe

**Purpose**: Subscribe to a specific AES67 stream and monitor reception.

### Usage
```bash
./tools/build/aes67-subscribe [options] <multicast_address> <port>
```

### Arguments
- `<multicast_address>` - Multicast IP address (e.g., 239.69.1.1)
- `<port>` - UDP port number (e.g., 5004)

### Options
- `-i, --interface <name>` - Network interface (default: en0)
- `-c, --channels <num>` - Number of channels (default: 8)
- `-d, --duration <sec>` - Run duration (default: infinite)
- `-s, --stats` - Print detailed statistics every second
- `-v, --verbose` - Verbose output
- `-h, --help` - Display help message

### Examples
```bash
# Subscribe to stream on default interface
./tools/build/aes67-subscribe 239.69.1.1 5004

# Monitor specific interface with stats
./tools/build/aes67-subscribe -i en1 -c 2 -s 239.69.1.2 5004

# Run for 30 seconds with verbose output
./tools/build/aes67-subscribe -d 30 -v 239.69.1.1 5004
```

### Output Format
```
AES67 Stream Subscriber
=======================
Multicast: 239.69.1.1:5004
Interface: en0
Channels:  8

Starting network engine...
Waiting for PTP synchronization...
PTP locked! Offset: -125.34 µs

Subscribing to stream on 239.69.1.1:5004...
Subscription activated (stream index: 0)

Monitoring stream (Ctrl+C to stop)...

=== Statistics at 18:45:23 ===
PTP Status:
  Locked:      Yes
  Offset:      -125.34 µs
  Rate Scalar: 1.000000012

Stream Statistics:
  Stream Index: 0
  Status:       Subscribed
  
Ring Buffer:
  Status:      Available
```

### Use Cases
1. **Stream Validation**: Verify a stream is receivable
2. **Network Debugging**: Monitor packet reception and loss
3. **PTP Monitoring**: Check synchronization status
4. **Latency Testing**: Combined with transmission tools

---

## aes67-stream

**Purpose**: Transmit audio files as AES67 streams over the network.

### Usage
```bash
./tools/build/aes67-stream [options] <audio_file>
```

### Arguments
- `<audio_file>` - RAW audio file (32-bit int, interleaved)

### Options
- `-i, --interface <name>` - Network interface (default: en0)
- `-a, --address <addr>` - Multicast address (default: 239.69.1.1)
- `-p, --port <port>` - UDP port (default: 5004)
- `-c, --channels <num>` - Number of channels (default: 2)
- `-r, --rate <hz>` - Sample rate (default: 48000)
- `-n, --name <name>` - Stream name for SAP (default: filename)
- `-l, --loop` - Loop playback continuously
- `-s, --stats` - Print statistics every second
- `-v, --verbose` - Verbose output
- `-h, --help` - Display help message

### File Format
The audio file must be **raw PCM data**:
- **Format**: 32-bit signed integer (little-endian)
- **Layout**: Interleaved (L, R, L, R, ...)
- **No header**: Raw samples only

### Converting Audio Files
Use ffmpeg to convert WAV/AIFF/etc to raw format:

```bash
# Stereo, 48kHz
ffmpeg -i input.wav -f s32le -ar 48000 -ac 2 output.raw

# 8-channel, 48kHz
ffmpeg -i multichannel.wav -f s32le -ar 48000 -ac 8 output.raw

# From AIFF
ffmpeg -i input.aiff -f s32le -ar 48000 -ac 2 output.raw
```

### Examples
```bash
# Stream stereo file to default address
./tools/build/aes67-stream -c 2 audio.raw

# Stream 8-channel file, loop forever
./tools/build/aes67-stream -c 8 -l multichannel.raw

# Stream to specific address with name
./tools/build/aes67-stream -a 239.69.1.10 -n "Console Mix" audio.raw

# Stream with statistics display
./tools/build/aes67-stream -c 2 -s -v audio.raw
```

### Output Format
```
AES67 Stream Transmitter
========================
File:       audio.raw
Stream:     audio
Multicast:  239.69.1.1:5004
Interface:  en0
Channels:   2
Rate:       48000 Hz
Loop:       No

Loading audio file...
Loaded 480000 frames (10.00 seconds)

Starting network engine...
Waiting for PTP synchronization...
PTP locked! Offset: -125.34 µs

Streaming (Ctrl+C to stop)...

=== Statistics at 18:50:15 ===
Playback:
  Position:     96000 / 480000 frames
  Time:         2.0 / 10.0 seconds (20.0%)

PTP Status:
  Locked:      Yes
  Offset:      -125.34 µs
  Rate Scalar: 1.000000012
```

### Use Cases
1. **Stream Testing**: Transmit known audio for validation
2. **Audio Playout**: Stream recorded content to receivers
3. **Latency Measurement**: Use with receiver tools
4. **Network Testing**: Verify multicast routing and QoS

---

## Common Workflows

### 1. Discover Available Streams
```bash
# List all streams on the network
./tools/build/aes67-list -d 30

# Find streams on specific interface
./tools/build/aes67-list -i en1 -v
```

### 2. Subscribe and Monitor
```bash
# Subscribe to discovered stream
./tools/build/aes67-subscribe 239.69.1.1 5004 -s

# Monitor with detailed stats
./tools/build/aes67-subscribe 239.69.1.1 5004 -s -v
```

### 3. Transmit Test Audio
```bash
# Prepare test file (1kHz tone, stereo, 10 seconds)
ffmpeg -f lavfi -i "sine=frequency=1000:duration=10" \
       -f s32le -ar 48000 -ac 2 test_tone.raw

# Stream the file
./tools/build/aes67-stream -c 2 -n "Test Tone" test_tone.raw

# Loop continuously for testing
./tools/build/aes67-stream -c 2 -l test_tone.raw
```

### 4. Two-Machine Audio Path Test
```bash
# On Mac 1 (transmitter):
./tools/build/aes67-stream -c 2 -a 239.69.1.100 audio.raw

# On Mac 2 (receiver):
./tools/build/aes67-subscribe 239.69.1.100 5004 -c 2 -s
```

### 5. Latency Measurement
```bash
# Generate impulse test signal
ffmpeg -f lavfi -i "sine=frequency=1000:duration=0.01" \
       -af "apad=pad_len=48000" \
       -f s32le -ar 48000 -ac 2 impulse.raw

# Transmit and measure round-trip
# (Use external audio loopback or second machine)
./tools/build/aes67-stream -c 2 impulse.raw
```

---

## Network Configuration

### Multicast Routing
Ensure multicast is enabled on your network interface:

```bash
# macOS
sudo route add -net 239.0.0.0/8 -interface en0

# Check multicast routes
netstat -rn | grep 239
```

### Firewall Configuration
Allow UDP traffic on AES67 ports:
- **RTP Audio**: UDP 5004 (or configured port)
- **PTP Event**: UDP 319
- **PTP General**: UDP 320
- **SAP**: UDP 9875

```bash
# macOS firewall (if enabled)
# Add rules in System Preferences > Security & Privacy > Firewall > Options
```

### Network Monitoring
Monitor AES67 traffic:

```bash
# Monitor RTP packets
sudo tcpdump -i en0 'multicast and udp port 5004'

# Monitor SAP announcements
sudo tcpdump -i en0 'dst 239.255.255.255 and udp port 9875'

# Monitor PTP packets
sudo tcpdump -i en0 'udp port 319 or udp port 320'
```

---

## Troubleshooting

### No Streams Discovered (aes67-list)
1. Check network interface is correct: `ifconfig`
2. Verify multicast routing: `netstat -rn | grep 239`
3. Check for SAP announcements: `sudo tcpdump -i en0 'dst 239.255.255.255'`
4. Ensure firewall allows UDP 9875

### Cannot Subscribe (aes67-subscribe)
1. Verify stream address is reachable: `ping <address>`
2. Check multicast membership: Use `netstat -g`
3. Verify no packet loss: Use `-s` option for statistics
4. Ensure firewall allows UDP 5004 (or specified port)

### PTP Not Locking
1. Verify PTP master is present on network
2. Check PTP traffic: `sudo tcpdump -i en0 'udp port 319'`
3. Use `-v` flag to see detailed PTP status
4. Try different network interface

### Audio File Issues (aes67-stream)
1. **File format**: Must be raw 32-bit integer PCM
2. **Conversion**: Use ffmpeg as shown in examples
3. **Channel count**: File size must be multiple of (channels × 4 bytes)
4. **Sample rate**: Use 48000 or 96000 for AES67 compliance

### Poor Audio Quality
1. Check PTP lock status with `-v` flag
2. Monitor packet loss with `-s` statistics
3. Verify QoS/DSCP marking: `sudo tcpdump -vvv`
4. Check network congestion: `netstat -i`

---

## Building the Tools

The CLI tools are built automatically with the main project:

```bash
cd /path/to/AES67\ Project
./scripts/build.sh
```

Or build tools independently:

```bash
cd tools
mkdir -p build && cd build
cmake ..
make -j
```

The executables will be in `tools/build/`:
- `aes67-list` (~181 KB)
- `aes67-subscribe` (~200 KB)
- `aes67-stream` (~218 KB)

---

## Technical Details

### Dependencies
- **Engine Library**: libaes67_engine.a
- **System Frameworks**: CoreFoundation, SystemConfiguration
- **Standards**: C++20

### Performance
- **CPU Usage**: <1% per tool (idle)
- **Memory**: ~5 MB per tool
- **Latency**: Minimal overhead, network-bound

### Limitations
1. **Config Path**: Tools use `../configs/engine.json` (relative path)
2. **Stream Management**: Manual subscription API not fully implemented
3. **Statistics**: Some metrics require additional NetworkEngine API
4. **File Format**: Only raw 32-bit PCM supported (use ffmpeg for conversion)

---

## aes67-monitor

**Purpose**: Web-based real-time audio level monitoring and stream visualization.

### Usage
```bash
./tools/build/aes67-monitor [options]
```

### Options
- `-p, --port <num>` - HTTP server port (default: 8080)
- `-c, --channels <num>` - Number of channels to monitor (default: 2)
- `-i, --interface <name>` - Network interface (default: en0)
- `-v, --verbose` - Verbose output
- `-h, --help` - Display help message

### Examples
```bash
# Monitor 2 channels on default port
./tools/build/aes67-monitor

# Monitor 8 channels with custom port
./tools/build/aes67-monitor --channels 8 --port 3000

# Use specific interface
./tools/build/aes67-monitor -i en1 -c 16

# Verbose logging
./tools/build/aes67-monitor --verbose
```

### Quick Start
```bash
# Using helper script (auto-opens browser)
./scripts/launch-monitor.sh 8

# Or manual start
cd tools/build
./aes67-monitor --channels 8
# Then open http://localhost:8080 in your browser
```

### Browser Interface

The web interface provides:

1. **PTP Status Panel** (Top)
   - Lock status (Green = Locked, Red = Unlocked)
   - PTP offset in microseconds
   - Rate scalar (frequency correction)

2. **Real-Time Level Meters**
   - One meter per channel
   - RMS levels in dBFS
   - Color-coded zones:
     - Green: -∞ to -20 dBFS (safe)
     - Yellow: -20 to -6 dBFS (moderate)
     - Red: -6 to 0 dBFS (hot)
   - Updates 10 times per second (100ms)

3. **Discovered Streams**
   - List of all AES67 streams on network
   - Shows name, multicast address, and port
   - Auto-updates as streams appear/disappear

### Features
- 🎵 Real-time audio visualization
- 📊 PTP synchronization monitoring
- 🔍 Automatic stream discovery
- 🌐 Access from any device on network
- ⚡ Low latency (< 1ms added)
- 📱 Responsive design for mobile/tablet

### Integration with DAWs

#### Setup Steps
1. Start the monitor: `./aes67-monitor --channels 8`
2. Open browser: `http://localhost:8080`
3. Configure DAW to use **AES67 Virtual Soundcard** as output
4. Play audio in DAW
5. Watch meters respond in real-time

#### Supported DAWs
- Logic Pro X
- Ableton Live
- Pro Tools
- Reaper
- Studio One
- Cubase/Nuendo
- FL Studio
- Any CoreAudio-compatible DAW

### API Endpoint

The monitor exposes a JSON status endpoint for automation:

```bash
# Get current status
curl http://localhost:8080/status

# Example response
{
  "timestamp": 1634323200000,
  "ptpLocked": true,
  "ptpOffset": 0.23,
  "rateScalar": 1.000001,
  "channels": [
    {"channel": 0, "level": -18.5, "peak": -18.5},
    {"channel": 1, "level": -20.1, "peak": -20.1}
  ],
  "streams": [
    {"name": "Console Mix", "address": "239.69.1.1", "port": 5004}
  ]
}
```

### Scripting Examples

```bash
# Monitor PTP offset continuously
watch -n 1 'curl -s http://localhost:8080/status | jq .ptpOffset'

# Alert if any channel is clipping
while true; do
  curl -s http://localhost:8080/status | \
    jq -e '.channels[] | select(.level > -1)' && \
    osascript -e 'display notification "Audio clipping detected!" with title "AES67 Monitor"'
  sleep 1
done

# Log levels to CSV
echo "timestamp,ch0,ch1" > levels.csv
while true; do
  curl -s http://localhost:8080/status | \
    jq -r '[.timestamp, .channels[0].level, .channels[1].level] | @csv' >> levels.csv
  sleep 1
done
```

### Remote Access

To access from other devices on your network:

1. Find your Mac's IP: `ipconfig getifaddr en0`
2. Start monitor: `./aes67-monitor --channels 8`
3. Access from remote device: `http://YOUR_MAC_IP:8080`
4. Configure firewall if needed

### Troubleshooting

**No audio visible:**
- Verify DAW is using AES67 Virtual Soundcard
- Check channel count matches DAW output
- Confirm audio is actually playing

**PTP not locking:**
- Check network interface (`-i` option)
- Verify PTP master is running
- Check firewall (UDP ports 319/320)

**Browser won't connect:**
- Verify monitor is running
- Try different port: `--port 8081`
- Check firewall settings
- Try `http://127.0.0.1:8080` instead

**High CPU usage:**
- Close other browser tabs
- Use modern browser (Chrome/Safari)
- Reduce number of channels

### Performance
- **Latency**: < 1ms (only reads ring buffer)
- **CPU**: < 2% on M1/M2 Mac
- **Network**: ~1 KB per 100ms (JSON updates)
- **Memory**: ~10 MB

### Browser Compatibility
- ✅ Chrome/Edge (Excellent)
- ✅ Safari (Excellent)
- ✅ Firefox (Good)
- ✅ Opera (Good)

---

## Future Enhancements

- [x] Real-time audio level meters (✅ Added in aes67-monitor)
- [x] Web-based monitoring interface (✅ Added in aes67-monitor)
- [ ] WAV/AIFF file support (eliminate ffmpeg dependency)
- [ ] Packet capture/replay for debugging
- [ ] Automatic stream discovery and subscription
- [ ] JSON output format for scripting (✅ Partial - monitor has API)
- [ ] Peak hold meters
- [ ] Spectrum analyzer visualization
- [ ] Phase correlation meter
- [ ] Historical level plotting

---

**Last Updated**: October 16, 2025  
**Version**: 1.1  
**See Also**: `docs/ARCHITECTURE.md`, `docs/WEB_MONITOR_GUIDE.md`, `PROJECT_STATUS.md`
