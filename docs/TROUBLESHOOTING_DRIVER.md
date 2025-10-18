# Troubleshooting: Driver Not Loading in macOS

## Problem

The AES67VSC.driver is installed in `/Library/Audio/Plug-Ins/HAL/` but doesn't appear in DAW audio device lists.

## Root Cause

Modern macOS (especially Apple Silicon with macOS 12+) has strict security requirements for system-level audio drivers:

1. **Code Signing**: Drivers must be properly signed with an Apple Developer certificate
2. **Notarization**: May require notarization for distribution
3. **System Integrity Protection (SIP)**: May block unsigned or adhoc-signed drivers
4. **Entitlements**: Audio drivers may need specific entitlements

Currently, the driver is only adhoc-signed (development signing), which CoreAudio silently rejects on modern macOS.

---

## Solutions

### Option 1: Disable SIP (For Development Only)

**⚠️ WARNING: This reduces system security. Only do this on a development machine.**

1. **Restart in Recovery Mode:**
   - Apple Silicon: Shut down, hold power button until "Loading startup options"
   - Intel Mac: Restart, hold Cmd+R during boot

2. **Open Terminal in Recovery Mode:**
   - Utilities → Terminal

3. **Disable SIP:**
   ```bash
   csrutil disable
   ```

4. **Restart normally**

5. **Verify SIP is disabled:**
   ```bash
   csrutil status
   # Should show: System Integrity Protection status: disabled.
   ```

6. **Reload CoreAudio:**
   ```bash
   sudo killall coreaudiod
   # Wait 5 seconds
   system_profiler SPAudioDataType | grep -i aes67
   ```

7. **Check in Ableton:**
   - Preferences → Audio
   - Look for "AES67 Virtual Soundcard"

**To re-enable SIP later** (recommended):
- Boot to Recovery Mode
- Terminal: `csrutil enable`
- Restart

---

### Option 2: Get Apple Developer Certificate (Recommended for Distribution)

1. **Join Apple Developer Program:**
   - Cost: $99/year
   - https://developer.apple.com/programs/

2. **Create Developer ID Certificate:**
   - Xcode → Preferences → Accounts
   - Add Apple ID
   - Download certificates

3. **Re-sign the driver:**
   ```bash
   cd "/Users/fredparsons/Documents/Side Projects/AES67 Project"
   
   # Sign with your Developer ID
   codesign --force --sign "Developer ID Application: Your Name (TEAMID)" \
     --timestamp \
     driver/build/AES67VSC.driver
   
   # Verify
   codesign -dvvv driver/build/AES67VSC.driver
   
   # Reinstall
   sudo rm -rf /Library/Audio/Plug-Ins/HAL/AES67VSC.driver
   sudo cp -R driver/build/AES67VSC.driver /Library/Audio/Plug-Ins/HAL/
   sudo killall coreaudiod
   ```

4. **(Optional) Notarize:**
   ```bash
   # Package as DMG
   # Submit to Apple for notarization
   # Staple notarization ticket
   ```

---

### Option 3: Use Existing Unsigned Drivers as Template

Since you have other working virtual audio drivers (BlackHole, WaveLink, etc.), they may be:
- Properly signed
- Using special entitlements
- Installed differently

Let's check BlackHole's signing:

```bash
codesign -dvvv /Library/Audio/Plug-Ins/HAL/BlackHole2ch.driver/Contents/MacOS/*
```

Compare with AES67:

```bash
codesign -dvvv /Library/Audio/Plug-Ins/HAL/AES67VSC.driver/Contents/MacOS/AES67VSC
```

---

### Option 4: Use BlackHole as Workaround (Temporary)

Until the driver loading issue is resolved, you can use BlackHole as an intermediate:

1. **Set Ableton output to BlackHole 16ch**
2. **Run aes67-monitor to read from BlackHole**
   - Would require modifying monitor to use BlackHole device

This is not ideal but allows testing the monitoring tool.

---

### Option 5: Build with Xcode (May Help with Signing)

Instead of CMake, use Xcode's build system which handles signing automatically:

1. **Create Xcode project for driver**
2. **Set up code signing in Xcode**
3. **Build from Xcode**
4. **Install**

However, this still requires a developer certificate.

---

## Quick Check: What's Different?

Let's compare your working drivers with AES67:

```bash
# Check BlackHole signing
codesign -dvvv /Library/Audio/Plug-Ins/HAL/BlackHole2ch.driver 2>&1 | grep -E "Authority|Identifier|TeamIdentifier"

# Check AES67 signing
codesign -dvvv /Library/Audio/Plug-Ins/HAL/AES67VSC.driver 2>&1 | grep -E "Authority|Identifier|TeamIdentifier"

# Compare Info.plist
plutil -p /Library/Audio/Plug-Ins/HAL/BlackHole2ch.driver/Contents/Info.plist | grep -i "bundle\|factory"
plutil -p /Library/Audio/Plug-Ins/HAL/AES67VSC.driver/Contents/Info.plist | grep -i "bundle\|factory"
```

---

## Recommended Immediate Action

**For testing/development:**

1. **Disable SIP** (Option 1 above) - easiest for immediate testing
2. Test the driver works
3. Use the web monitor with your DAW
4. Re-enable SIP when done

**For long-term/distribution:**

1. Get Apple Developer certificate (Option 2)
2. Properly sign the driver
3. Consider notarization for distribution

---

## Alternative: Test Without Driver

You can still test the **network engine and CLI tools** without the driver:

### Test Network Streaming Between Two Processes

**Terminal 1: Start receiver**
```bash
cd tools/build
./aes67-monitor --channels 8
```

**Terminal 2: Stream a file**
```bash
cd tools/build

# Create test audio (if you have ffmpeg)
ffmpeg -f lavfi -i "sine=frequency=1000:duration=10" -f s32le -ar 48000 -ac 2 test.raw

# Stream it
./aes67-stream -c 2 test.raw
```

This tests the AES67 network stack without needing the driver.

---

## Current Status

✅ **Built:** Driver, Engine, Tools  
✅ **Installed:** Driver in correct location  
❌ **Loaded:** CoreAudio won't load unsigned driver  
❌ **Visible:** Not appearing in DAW  

**Blocker:** Code signing / SIP  
**Solution:** Disable SIP (dev) or get certificate (prod)

---

## Need Help?

If you want to proceed with disabling SIP, let me know and I'll guide you through it step-by-step.

If you want to pursue the certificate route, I can help set up proper signing in the build process.

Alternatively, we can focus on testing the network engine components which don't require the driver to be loaded.
