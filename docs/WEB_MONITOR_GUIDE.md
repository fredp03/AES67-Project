# AES67 Web Monitor - Quick Start Guide

## Overview

The **aes67-monitor** tool provides a real-time web-based visualization of AES67 audio streams passing through your system. Perfect for monitoring audio from your DAW!

## Features

- 🎵 **Real-time Level Meters** - Visual feedback for all channels
- 📊 **PTP Synchronization Status** - Clock accuracy monitoring
- 🔍 **Stream Discovery** - See all AES67 streams on your network
- 🌐 **Browser-Based** - No additional software needed
- ⚡ **Low Latency** - 10 Hz updates (100ms refresh)

---

## Quick Start

### 1. Start the Monitor

```bash
cd tools/build
./aes67-monitor --channels 8 --port 8080
```

You should see:
```
AES67 Web-Based Audio Monitor
==============================

Starting network engine...
Waiting for PTP lock...
PTP locked! Offset: 0.23 µs

✓ Server running on http://localhost:8080
  Monitoring 8 channels
  Network interface: en0

Open the URL in your browser to view real-time audio levels.
Press Ctrl+C to stop.
```

### 2. Open in Browser

Navigate to: **http://localhost:8080**

You'll see:
- **PTP Status Panel** - Shows synchronization state, offset, and rate scalar
- **Level Meters** - One per channel with real-time dBFS readings
- **Discovered Streams** - List of all AES67 streams on your network

### 3. Configure Your DAW

#### Logic Pro X
1. Go to **Preferences → Audio**
2. Select **AES67 Virtual Soundcard** as output device
3. Set up a multi-channel output bus
4. Route tracks to the AES67 output

#### Ableton Live
1. Go to **Preferences → Audio**
2. Set **Audio Output Device** to **AES67 Virtual Soundcard**
3. Enable the channels you need in Audio preferences
4. Route tracks to the output channels

#### Pro Tools
1. Go to **Setup → Playback Engine**
2. Select **AES67 Virtual Soundcard**
3. Set up I/O configuration for multi-channel output
4. Route tracks to AES67 outputs

#### Reaper
1. Go to **Preferences → Audio → Device**
2. Set **Audio System** to **Core Audio**
3. Select **AES67 Virtual Soundcard** as output device
4. Configure channel routing in routing matrix

### 4. Play Audio

- Hit play in your DAW
- Watch the level meters respond in your browser
- Verify all channels are active
- Check PTP synchronization status

---

## Command-Line Options

```bash
./aes67-monitor [options]

Options:
  -p, --port PORT      HTTP server port (default: 8080)
  -c, --channels NUM   Number of channels to monitor (default: 2)
  -i, --interface IF   Network interface (default: en0)
  -v, --verbose        Verbose output
  -h, --help           Show this help message
```

### Examples

**Monitor 2 stereo channels:**
```bash
./aes67-monitor
```

**Monitor 8 channels on custom port:**
```bash
./aes67-monitor --channels 8 --port 3000
```

**Use specific network interface:**
```bash
./aes67-monitor --interface en1 --channels 16
```

**Verbose logging:**
```bash
./aes67-monitor --verbose --channels 2
```

---

## Browser Interface

### Status Panel (Top)

| Indicator | Meaning |
|-----------|---------|
| **PTP Locked** (Green) | Clock is synchronized |
| **PTP Unlocked** (Red) | Clock synchronization lost |
| **PTP Offset** | Time difference from master clock (µs) |
| **Rate Scalar** | Clock frequency correction factor |

### Level Meters

- **Green zone**: -∞ to -20 dBFS (safe levels)
- **Yellow zone**: -20 to -6 dBFS (moderate levels)
- **Red zone**: -6 to 0 dBFS (hot levels, watch for clipping!)

Each meter shows:
- Real-time RMS level in dBFS
- Visual bar graph
- Numeric readout

### Discovered Streams

Lists all AES67 streams found on the network:
- Stream name
- Multicast address
- Port number

---

## Troubleshooting

### No Audio Visible

**Problem**: Meters stay at -∞ dB

**Solutions**:
1. Verify DAW is set to use AES67 Virtual Soundcard
2. Check that audio is actually playing in your DAW
3. Verify correct number of channels (`--channels` parameter)
4. Check that the driver is loaded:
   ```bash
   system_profiler SPAudioDataType | grep AES67
   ```

### PTP Not Locking

**Problem**: Shows "PTP Unlocked"

**Solutions**:
1. Check that PTP master is running on network
2. Verify network interface is correct (`-i` option)
3. Check firewall allows PTP (UDP ports 319/320)
4. Ensure multicast is enabled on network switch
5. Run with `--verbose` to see PTP messages

### Browser Won't Connect

**Problem**: Can't access http://localhost:8080

**Solutions**:
1. Verify monitor is running (check terminal output)
2. Try a different port: `--port 8081`
3. Check firewall settings
4. Try accessing from same machine first
5. Check that port is not already in use:
   ```bash
   lsof -i :8080
   ```

### High CPU Usage

**Problem**: Browser or monitor using too much CPU

**Solutions**:
1. Close other browser tabs
2. Reduce number of channels being monitored
3. Use a modern browser (Chrome, Safari, Firefox)
4. Increase update interval (requires code modification)

### Levels Seem Wrong

**Problem**: Meters don't match DAW levels

**Solutions**:
1. RMS vs Peak: Monitor shows RMS, DAW may show peak
2. Check sample format in DAW (should be 32-bit)
3. Verify channel routing in DAW
4. Check for DC offset or phase issues

---

## Integration Testing Workflow

### Test 1: Single DAW Output

1. Start monitor: `./aes67-monitor --channels 2`
2. Open browser: http://localhost:8080
3. Play 1 kHz tone in DAW at -20 dBFS
4. Verify meters show approximately -20 dB
5. Pan tone left/right to test individual channels

### Test 2: Multi-Channel Output

1. Start monitor: `./aes67-monitor --channels 8`
2. Create 8 tracks in DAW, each with different tone
3. Route each track to separate AES67 channel
4. Play all tracks simultaneously
5. Verify all 8 meters show activity
6. Verify channels are independent (mute tracks individually)

### Test 3: Latency Check

1. Start monitor with statistics
2. Play audio in DAW
3. Check PTP offset in browser (should be < 1 µs)
4. Monitor for PTP lock stability
5. Verify no dropouts or glitches in meters

### Test 4: Long-Duration Stability

1. Start monitor: `./aes67-monitor --channels 2 --verbose`
2. Play continuous audio for 30+ minutes
3. Monitor for:
   - PTP staying locked
   - No meter freezing
   - Consistent latency
   - No memory leaks (check Activity Monitor)

### Test 5: Network Discovery

1. Start monitor on Machine A
2. Start `aes67-stream` on Machine B
3. Verify stream appears in "Discovered Streams" section
4. Check that address/port are correct
5. Test bidirectional discovery

---

## Technical Details

### Network Configuration

The monitor automatically:
- Joins PTP multicast groups (224.0.1.129)
- Listens for SAP announcements (239.255.255.255:9875)
- Receives RTP audio streams (configured multicast addresses)
- Synchronizes to IEEE 1588 PTP clock

### Audio Processing

1. **Input Path**: DAW → CoreAudio → AES67 Driver → Ring Buffer → Network Engine
2. **Level Calculation**: 512-sample RMS per channel
3. **Update Rate**: 10 Hz (100ms intervals)
4. **Format**: 32-bit integer samples, normalized to float for RMS

### Performance

- **Latency**: Adds < 1ms (only reading ring buffer, not in audio path)
- **CPU**: < 2% on modern Mac (M1/M2)
- **Network**: Minimal (only status JSON, ~1 KB every 100ms)
- **Memory**: ~10 MB (mostly static HTML/CSS)

### Browser Compatibility

| Browser | Status | Notes |
|---------|--------|-------|
| Chrome/Edge | ✅ Excellent | Best performance |
| Safari | ✅ Excellent | Native on macOS |
| Firefox | ✅ Good | May have slightly higher CPU |
| Opera | ✅ Good | Chromium-based |

---

## Advanced Usage

### Remote Monitoring

To access the monitor from another device on your network:

1. Find your Mac's IP address:
   ```bash
   ipconfig getifaddr en0
   ```

2. Start monitor with all interfaces:
   ```bash
   ./aes67-monitor --channels 8
   ```

3. Access from remote device:
   ```
   http://YOUR_MAC_IP:8080
   ```

4. Ensure macOS firewall allows incoming connections

### Custom Styling

The HTML interface is embedded in the binary but can be customized by editing `aes67-monitor.cpp` and recompiling:

- Modify CSS in the `<style>` section
- Change update rate in JavaScript (currently 100ms)
- Add additional metrics to JSON endpoint
- Customize color scheme for meters

### Scripting

The `/status` endpoint returns JSON for automated monitoring:

```bash
# Get current status
curl http://localhost:8080/status | jq

# Monitor PTP offset
watch -n 1 'curl -s http://localhost:8080/status | jq .ptpOffset'

# Check if any channel is clipping
curl -s http://localhost:8080/status | jq '.channels[] | select(.level > -6)'
```

### Multiple Monitors

Run multiple instances for different stream groups:

```bash
# Monitor input streams on port 8080
./aes67-monitor --channels 8 --port 8080 &

# Monitor output streams on port 8081  
./aes67-monitor --channels 8 --port 8081 &
```

---

## Next Steps

1. ✅ **Single Machine Test**: Monitor your DAW output
2. **Two-Machine Test**: Stream between two Macs
3. **Latency Optimization**: Tune for < 4ms round-trip
4. **Production Use**: Deploy in live environment

---

## Related Tools

- **aes67-list**: Discover streams on network
- **aes67-subscribe**: Manual stream subscription
- **aes67-stream**: Transmit audio files

See `docs/CLI_TOOLS.md` for complete documentation.

---

## Support

For issues or questions:
1. Check PTP status in browser interface
2. Run with `--verbose` flag for detailed logs
3. Verify network configuration
4. Check `docs/` folder for additional documentation

Happy monitoring! 🎵
