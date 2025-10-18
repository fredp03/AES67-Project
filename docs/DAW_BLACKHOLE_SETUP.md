# DAW to Web Monitor Setup (Using BlackHole)

This guide shows you how to route audio from your DAW (like Ableton Live) to the web-based monitor using BlackHole as an intermediate virtual audio device.

## Why Use BlackHole?

Since macOS requires Apple Developer ID certificates for CoreAudio drivers (which costs $99/year), we can use the free BlackHole virtual audio device as a workaround. BlackHole acts as a bridge between your DAW and the AES67 network tools.

## Audio Flow

```
┌─────────────┐      ┌──────────────┐      ┌──────────────┐      ┌─────────────┐
│             │      │              │      │              │      │             │
│  Ableton    │─────→│  BlackHole   │─────→│ aes67-stream │─────→│   Network   │
│    Live     │      │    64ch      │      │              │      │  (Multicast)│
│             │      │              │      │              │      │             │
└─────────────┘      └──────────────┘      └──────────────┘      └─────────────┘
                                                                         │
                                                                         ↓
                                                                  ┌─────────────┐
                                                                  │             │
                                                                  │ aes67-      │
                                                                  │  monitor    │
                                                                  │             │
                                                                  └─────────────┘
                                                                         │
                                                                         ↓
                                                                  ┌─────────────┐
                                                                  │             │
                                                                  │   Browser   │
                                                                  │ http://     │
                                                                  │ localhost   │
                                                                  │   :8080     │
                                                                  │             │
                                                                  └─────────────┘
```

## Prerequisites

1. **BlackHole installed** - You already have BlackHole 64ch installed ✓
2. **AES67 tools built** - Will be checked/built automatically
3. **Ableton Live** (or any DAW that supports multi-channel output)

## Quick Start (5 Minutes)

### 1. Run the Setup Script

```bash
./scripts/daw-to-web-monitor.sh
```

This script will:
- Check for BlackHole
- Build the tools if needed
- Guide you through Ableton configuration
- Start the complete audio pipeline
- Open the web monitor

### 2. Configure Ableton

When prompted, configure Ableton Live:

1. Open **Ableton Live**
2. Go to **Preferences → Audio**
3. Set **Audio Output Device** to: `BlackHole 64ch`
4. Set **Sample Rate** to: `48000 Hz`
5. Click **Output Channel Configuration**
6. Enable channels 1-8 (or more if you need them)
7. Press Enter in the terminal to continue

### 3. Test It!

1. The script will start both `aes67-stream` and `aes67-monitor`
2. Open http://localhost:8080 in your browser
3. Play audio in Ableton
4. Watch the level meters update in real-time! 🎉

## Manual Setup (If You Prefer)

### Terminal 1: Start the Streamer

```bash
./build/aes67-stream \
    --device "BlackHole 64ch" \
    --channels 8 \
    --sample-rate 48000 \
    --stream-name "Ableton_Output" \
    --multicast-ip 239.69.83.1 \
    --port 5004 \
    --interface en0
```

### Terminal 2: Start the Monitor

```bash
./build/aes67-monitor \
    --multicast-ip 239.69.83.1 \
    --port 5004 \
    --interface en0 \
    --http-port 8080
```

### Terminal 3: Open the Web Monitor

```bash
open http://localhost:8080
```

## Configuration Options

### Change Number of Channels

Stream more or fewer channels:

```bash
./scripts/daw-to-web-monitor.sh 16  # Stream 16 channels
./scripts/daw-to-web-monitor.sh 32  # Stream 32 channels
./scripts/daw-to-web-monitor.sh 2   # Stream just stereo
```

Remember to enable the same number of output channels in Ableton!

### Change Network Interface

If you're using a different network interface (like Wi-Fi):

Edit `scripts/daw-to-web-monitor.sh` and change:
```bash
INTERFACE="en0"  # Change to en1, en2, etc.
```

Find your interface with:
```bash
ifconfig | grep "^[a-z]"
```

## Ableton Configuration Tips

### Routing Audio to BlackHole

1. **Master Output**: Set your Master track output to BlackHole 64ch
   - This sends everything to the monitor

2. **Individual Tracks**: Send specific tracks to BlackHole channels
   - Track 1 → BlackHole Ch 1-2 (stereo)
   - Track 2 → BlackHole Ch 3-4 (stereo)
   - Track 3 → BlackHole Ch 5-6 (stereo)
   - etc.

3. **Return Tracks**: You can also route return tracks
   - Reverb → BlackHole Ch 9-10
   - Delay → BlackHole Ch 11-12

### Monitoring Your Audio

**Problem**: If you set Ableton's output to BlackHole, you won't hear anything!

**Solutions**:

#### Option A: Multi-Output Device (Recommended)

Create an Aggregate Device to output to both your speakers AND BlackHole:

1. Open **Audio MIDI Setup** app (in /Applications/Utilities/)
2. Click **+** → **Create Multi-Output Device**
3. Check both:
   - Your regular audio interface/speakers
   - BlackHole 64ch
4. In Ableton, select this Multi-Output Device

Now you'll hear your audio AND it will go to the web monitor!

#### Option B: Use BlackHole 2ch for Monitor

1. Install BlackHole 2ch if you haven't
2. Create a Multi-Output Device with:
   - Your speakers
   - BlackHole 2ch (for your monitor mix)
3. Use BlackHole 64ch for the full multi-channel stream
4. Send your monitor mix to BlackHole 2ch
5. Send your full stems to BlackHole 64ch

#### Option C: External Monitoring

If you have an audio interface with multiple outputs:
1. Route some outputs to your monitors/headphones
2. Route other outputs to BlackHole via software routing

## Troubleshooting

### No Audio in Web Monitor

1. **Check Ableton is playing**: Make sure audio is actually playing
2. **Check BlackHole sample rate**: Must be 48000 Hz
3. **Check channel routing**: Make sure Ableton tracks are routed to BlackHole
4. **Check logs**:
   ```bash
   tail -f /tmp/aes67-stream.log
   tail -f /tmp/aes67-monitor.log
   ```

### Can't Hear Audio

- Use a Multi-Output Device (see "Monitoring Your Audio" above)
- Or use headphones/speakers on a different output

### High Latency

- Reduce Ableton's buffer size (Preferences → Audio)
- BlackHole itself adds minimal latency (~5ms)

### Web Monitor Not Updating

1. Check the browser console (F12) for errors
2. Refresh the page (Cmd+R)
3. Check that aes67-monitor is running:
   ```bash
   ps aux | grep aes67-monitor
   ```

### "Device Not Found" Error

Make sure BlackHole 64ch is selected, not "BlackHole 16ch" or "BlackHole 2ch":

```bash
system_profiler SPAudioDataType | grep BlackHole
```

Should show:
```
BlackHole 64ch:
  Input Channels: 64
  Output Channels: 64
```

## Advanced Usage

### Multiple DAW Instances

You can route multiple DAWs to different channel ranges:

- **Ableton**: Channels 1-16 → BlackHole Ch 1-16
- **Logic Pro**: Channels 17-32 → BlackHole Ch 17-32
- **Reaper**: Channels 33-48 → BlackHole Ch 33-48

Then stream all 64 channels:
```bash
./scripts/daw-to-web-monitor.sh 64
```

### Recording the Stream

Capture the BlackHole output to a file:

```bash
./build/aes67-record \
    --device "BlackHole 64ch" \
    --channels 8 \
    --sample-rate 48000 \
    --output recording.wav \
    --duration 60
```

### Network Streaming to Other Computers

The AES67 stream is on the network, so you can:

1. Run `aes67-stream` on Computer A (with DAW)
2. Run `aes67-monitor` on Computer B (viewing on http://localhost:8080)
3. They just need to be on the same network/subnet

## Performance Tips

1. **Use the right channel count**: Don't stream 64 channels if you only need 8
2. **48kHz is optimal**: AES67 standard sample rate
3. **Wired Ethernet**: For best performance, use wired connection
4. **Close other apps**: Give Ableton and the AES67 tools priority

## What This Proves

Even though the CoreAudio driver doesn't work without an Apple Developer certificate, **all the AES67 network functionality works perfectly**:

- ✓ Multi-channel audio streaming
- ✓ AES67 RTP packets
- ✓ Real-time level monitoring
- ✓ Web-based visualization
- ✓ Sample-accurate audio transmission

The only thing missing is the direct integration into Ableton—but BlackHole provides that bridge!

## Next Steps

- Experiment with different channel counts
- Try streaming between multiple computers
- Explore the other CLI tools (`aes67-list`, `aes67-subscribe`)
- Build your own custom monitoring interface

## See Also

- [Web Monitor Guide](WEB_MONITOR_GUIDE.md) - Detailed web monitor documentation
- [Troubleshooting](TROUBLESHOOTING_DRIVER.md) - Driver issues and solutions
- [BlackHole Website](https://existential.audio/blackhole/) - More about BlackHole
