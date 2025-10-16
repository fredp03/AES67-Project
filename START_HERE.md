# 🎵 You're Ready to Test!

Your AES67 web monitor is built and ready. Here's what to do next:

---

## Step 1: Launch the Monitor (30 seconds)

Open Terminal and run:

```bash
cd "/Users/fredparsons/Documents/Side Projects/AES67 Project"
./scripts/launch-monitor.sh 8
```

This will:
- ✅ Start the web server
- ✅ Open your browser automatically
- ✅ Show 8-channel level meters

---

## Step 2: Configure Your DAW (1 minute)

### If you're using Logic Pro X:
1. Open **Logic Pro X**
2. **Logic Pro X menu → Settings → Audio** (or press ⌘,)
3. Set **Output Device** to **"AES67 Virtual Soundcard"**
4. Close preferences

### If you're using another DAW:
Look in audio preferences for output device settings and select **"AES67 Virtual Soundcard"**

---

## Step 3: Play Audio! (1 second)

1. Create a track (or open existing project)
2. Hit **PLAY** (spacebar)
3. **Watch the meters move!** 🎉

---

## What You Should See

### In Your Browser:

**Top section** (Status):
- PTP Status indicator (may show "Unlocked" - that's OK)
- PTP Offset in microseconds
- Rate Scalar

**Middle section** (Meters):
- 8 horizontal bar meters
- Real-time levels in dBFS
- Green/yellow/red color zones
- Numbers updating 10 times per second

**Bottom section** (Streams):
- Discovered AES67 streams on your network

---

## Troubleshooting

### "AES67 Virtual Soundcard" not in DAW?

The driver may not be installed. Check with:
```bash
system_profiler SPAudioDataType | grep AES67
```

If nothing appears, the driver needs to be installed.

### Meters stay at -∞ dB?

1. Verify DAW is actually playing (playhead moving?)
2. Check DAW volume isn't muted
3. Try increasing the volume in your DAW
4. Create a new track with loud audio (sine wave test)

### Browser won't connect?

1. Check monitor is running: `ps aux | grep aes67-monitor`
2. Try restarting: Kill it and run `./scripts/launch-monitor.sh 8` again
3. Try different browser

---

## What's Next?

Once you see meters working:

### Test 1: Panning
- Create a mono track
- Pan it left → Watch channel 1
- Pan it right → Watch channel 2

### Test 2: Multiple Channels
- Create 8 tracks
- Route each to different output
- Play all simultaneously
- Watch all 8 meters work independently

### Test 3: Remote Access
- Find your Mac's IP: `ipconfig getifaddr en0`
- Open `http://YOUR_IP:8080` on phone/tablet
- Watch meters from another device!

---

## Need Help?

### Documentation
- **QUICK_START_DAW.md** - Detailed 5-minute guide
- **docs/WEB_MONITOR_GUIDE.md** - Complete documentation
- **docs/CLI_TOOLS.md** - All CLI tools

### Commands
```bash
# Stop the monitor
Press Ctrl+C in the terminal

# Restart
./scripts/launch-monitor.sh 8

# Verbose output
cd tools/build
./aes67-monitor --verbose --channels 8

# Check driver
system_profiler SPAudioDataType | grep AES67
```

---

## Have Fun! 🎉

You now have a professional-grade audio monitoring tool running in your browser!

**Questions?** Check the documentation in the `docs/` folder.

---

**Ready?** Run `./scripts/launch-monitor.sh 8` and start playing! 🎵
