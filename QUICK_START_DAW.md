# Quick Start: Testing AES67 with Your DAW

**Goal**: Play audio from your DAW and visualize it in real-time using the browser-based monitor.

**Time**: ~5 minutes

---

## Step 1: Start the Monitor (1 minute)

Open a terminal and run:

```bash
cd "/Users/fredparsons/Documents/Side Projects/AES67 Project"
./scripts/launch-monitor.sh 8
```

You should see:
```
🎵 Launching AES67 Web Monitor
   Channels: 8
   URL: http://localhost:8080

✓ Monitor running (PID: 12345)

Opening browser...
```

A browser window will open automatically showing the AES67 Audio Monitor interface.

> **Note**: If you see "Port 8080 already in use", someone else is using that port. Edit the script to use 8081 instead.

---

## Step 2: Check the Monitor (30 seconds)

In your browser, you should see:

### Top Panel (Status)
- **PTP Status**: May show "Unlocked" (orange/red) - this is OK for single-machine testing
- **PTP Offset**: Shows "-- µs" if not locked
- **Rate Scalar**: Shows "--" if not locked

### Middle Section (Level Meters)
- 8 horizontal meter bars (one per channel)
- All showing "-∞ dB" (no audio yet)
- Green/yellow/red color zones

### Bottom Section (Discovered Streams)
- May say "No streams discovered yet..."
- This is normal if no other AES67 devices on network

---

## Step 3: Configure Your DAW (2 minutes)

### Logic Pro X
1. Open **Logic Pro X**
2. Go to **Logic Pro X → Settings → Audio** (or press ⌘,)
3. Under **Output Device**, select **AES67 Virtual Soundcard**
4. Close preferences

### Ableton Live
1. Open **Ableton Live**
2. Go to **Live → Preferences** (or press ⌘,)
3. Click **Audio** tab
4. Under **Audio Output Device**, select **AES67 Virtual Soundcard**
5. Close preferences

### Other DAWs
Look for audio preferences and set the output device to **AES67 Virtual Soundcard**.

> **Important**: If you don't see "AES67 Virtual Soundcard" in your DAW:
> 1. The driver may not be installed
> 2. Run: `system_profiler SPAudioDataType | grep AES67`
> 3. If nothing appears, you need to install the driver first
> 4. Contact me if you need help with driver installation

---

## Step 4: Play Audio (1 minute)

### Quick Test

1. **Create a track** in your DAW (any track type)
2. **Load an audio file** or virtual instrument
3. **Press PLAY** (spacebar in most DAWs)

### What You Should See

In the browser:
- **Meters start moving!** 🎉
- Green bars extend from left based on audio level
- Numbers update in real-time (e.g., "-18.3 dB")
- All channels with audio show activity

### Troubleshooting

**Problem**: Meters stay at -∞ dB

Try this:
1. Verify DAW is actually playing (playhead moving?)
2. Check DAW volume (is it muted?)
3. Verify output device is set correctly in DAW
4. Try creating a new track with a loud sound
5. Check DAW's output meter - does IT show audio?

**Problem**: Only some channels show audio

This is normal! Depends on your track routing:
- Stereo track → Channels 1-2
- Mono track panned left → Channel 1
- Mono track panned right → Channel 2
- To test all 8: Create 8 mono tracks, pan each to different output

---

## Step 5: Test Different Scenarios (optional)

### Test A: Stereo Panning
1. Create a mono track with audio
2. Play it
3. Pan hard left - watch Channel 1 meter
4. Pan hard right - watch Channel 2 meter
5. Pan center - watch both meters

### Test B: Level Testing
1. Play audio at different volumes
2. Watch meters respond:
   - Quiet: -40 to -30 dB (small green bars)
   - Medium: -20 to -12 dB (yellow zone)
   - Loud: -6 to 0 dB (red zone - watch for clipping!)

### Test C: Multi-Channel
1. Create 4 stereo tracks (or 8 mono tracks)
2. Route each to different outputs:
   - Track 1 → Channels 1-2
   - Track 2 → Channels 3-4
   - Track 3 → Channels 5-6
   - Track 4 → Channels 7-8
3. Play all tracks simultaneously
4. Watch all 8 meters work independently

---

## Understanding the Display

### Meter Colors

| Color | Range | Meaning |
|-------|-------|---------|
| 🟢 **Green** | -∞ to -20 dBFS | Safe operating level |
| 🟡 **Yellow** | -20 to -6 dBFS | Moderate level |
| 🔴 **Red** | -6 to 0 dBFS | Hot level - watch for clipping |

### Level Numbers

- **-∞ dB**: No audio (silence or below noise floor)
- **-40 dB**: Very quiet
- **-20 dB**: Normal operating level
- **-12 dB**: Moderate level
- **-6 dB**: Loud
- **-3 dB**: Very loud
- **0 dB**: Maximum before clipping (digital full scale)

> **Note**: These are RMS (Root Mean Square) levels, not peak levels. Peak levels can be higher momentarily.

### PTP Status

For single-machine testing:
- **Unlocked** (Red/Orange): Normal - no PTP master on network
- **Locked** (Green): Would appear if PTP master present

For two-machine testing:
- **Locked** is required for synchronization
- Offset should be < 1 µs
- Rate scalar should be close to 1.000000

---

## Common Issues & Solutions

### Issue 1: "AES67 Virtual Soundcard" not in DAW

**Cause**: Driver not installed or not loaded

**Solution**:
```bash
# Check if driver is loaded
system_profiler SPAudioDataType | grep AES67

# If nothing appears, check driver build
ls "/Users/fredparsons/Documents/Side Projects/AES67 Project/driver/build/"

# You should see AES67VSC.driver

# May need to install/reinstall driver
# (requires sudo and code signing setup)
```

### Issue 2: Browser won't connect to localhost:8080

**Cause**: Monitor not running or port conflict

**Solution**:
```bash
# Check if monitor is running
ps aux | grep aes67-monitor

# Check what's on port 8080
lsof -i :8080

# Kill existing monitor
pkill aes67-monitor

# Restart
./scripts/launch-monitor.sh 8
```

### Issue 3: Meters update slowly or freeze

**Cause**: Browser performance issue

**Solution**:
1. Close other browser tabs
2. Try different browser (Chrome usually fastest)
3. Disable browser extensions temporarily
4. Restart browser

### Issue 4: DAW audio stutters/glitches

**Cause**: Buffer size too small or system overload

**Solution**:
1. Increase DAW buffer size (512 or 1024 samples)
2. Close other applications
3. Check CPU usage in Activity Monitor
4. Reduce DAW track count

---

## Next Steps

Once basic monitoring works:

### 1. Explore Remote Access
```bash
# Find your Mac's IP
ipconfig getifaddr en0

# Access from phone/tablet
# Open browser to: http://YOUR_MAC_IP:8080
```

### 2. Try Scripting
```bash
# Get current status as JSON
curl http://localhost:8080/status | jq

# Monitor PTP offset
watch -n 1 'curl -s http://localhost:8080/status | jq .ptpOffset'

# Check for clipping
curl -s http://localhost:8080/status | jq '.channels[] | select(.level > -6)'
```

### 3. Long-Term Stability Test
- Leave monitor running for 30+ minutes
- Play continuous audio
- Watch for stability issues
- Monitor CPU/memory usage

### 4. Two-Machine Test (Advanced)
- Set up two Macs on same network
- Use `aes67-stream` to transmit from Mac A
- Monitor on Mac B
- Measure latency

---

## Getting Help

If you run into issues:

1. **Check the logs**
   ```bash
   # Run monitor with verbose output
   cd tools/build
   ./aes67-monitor --verbose --channels 8
   ```

2. **Check documentation**
   - `docs/WEB_MONITOR_GUIDE.md` - Comprehensive guide
   - `docs/CLI_TOOLS.md` - All CLI tool docs
   - `scripts/README.md` - Script documentation

3. **Verify system state**
   ```bash
   # Check driver
   system_profiler SPAudioDataType | grep -A 10 AES67
   
   # Check network
   ifconfig en0
   
   # Check processes
   ps aux | grep aes67
   ```

---

## Success Criteria

✅ You've succeeded when:
- Browser shows the monitoring interface
- Meters move when DAW plays audio
- Levels roughly match what you expect
- Can control levels via DAW faders
- Can test multiple channels independently

---

## Quick Reference Card

### Start Monitor
```bash
./scripts/launch-monitor.sh 8
```

### Access Monitor
```
Browser: http://localhost:8080
```

### Stop Monitor
```
Press Ctrl+C in terminal where monitor is running
```

### Check Status
```bash
curl http://localhost:8080/status | jq
```

### Restart Everything
```bash
pkill aes67-monitor
./scripts/launch-monitor.sh 8
# Restart DAW
```

---

**Time to get started**: ~5 minutes  
**Difficulty**: Easy  
**Prerequisites**: DAW installed, project built  

Happy monitoring! 🎵

---

**Last Updated**: October 16, 2025  
**For**: AES67 Virtual Soundcard Project  
**Next**: See `docs/WEB_MONITOR_GUIDE.md` for advanced features
