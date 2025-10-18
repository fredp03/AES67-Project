# Self-Signing the AES67 Driver for macOS

This guide shows how to create a self-signed certificate and use it to sign the AES67 audio driver, which may allow macOS to load it without disabling System Integrity Protection.

## Step 1: Create a Self-Signed Code Signing Certificate

1. **Open Keychain Access:**
   - Press `Cmd+Space` and type "Keychain Access"
   - Press Enter

2. **Open Certificate Assistant:**
   - Menu: **Keychain Access → Certificate Assistant → Create a Certificate...**

3. **Configure the Certificate:**
   - **Name:** `AES67 Audio Driver` (or any name you prefer)
   - **Identity Type:** `Self Signed Root`
   - **Certificate Type:** `Code Signing`
   - **☑ Let me override defaults** (CHECK THIS BOX)
   - Click **Continue**

4. **Certificate Information:**
   - Serial Number: (leave default)
   - Validity Period: `3650` days (10 years)
   - Click **Continue**

5. **Key Pair Information:**
   - **Key Size:** `2048 bits` (or 4096 for more security)
   - **Algorithm:** `RSA`
   - Click **Continue**

6. **Key Usage Extension:**
   - Keep defaults (Digital Signature should be checked)
   - Click **Continue**

7. **Extended Key Usage Extension:**
   - Check **Code Signing**
   - Click **Continue**

8. **Basic Constraints Extension:**
   - Keep defaults
   - Click **Continue**

9. **Subject Alternate Name Extension:**
   - Click **Continue** (can leave blank)

10. **Specify Location:**
    - **Keychain:** `login`
    - Click **Continue**

11. **Review and Create:**
    - Review your settings
    - Click **Create**
    - Click **Done**

---

## Step 2: Trust the Certificate (Important!)

1. **In Keychain Access:**
   - Select **login** keychain in the left sidebar
   - Select **My Certificates** category
   - Find your certificate (e.g., "AES67 Audio Driver")

2. **Double-click the certificate**

3. **Expand "Trust" section**
   - **When using this certificate:** Set to `Always Trust`
   - Close the window
   - Enter your password when prompted

---

## Step 3: Sign the Driver

Now use your self-signed certificate to sign the driver:

```bash
cd "/Users/fredparsons/Documents/Side Projects/AES67 Project"

# Remove old unsigned version from system
sudo rm -rf /Library/Audio/Plug-Ins/HAL/AES67VSC.driver

# Sign the driver with your certificate
codesign --force --deep --sign "AES67 Audio Driver" \
  --timestamp --options runtime \
  driver/build/AES67VSC.driver

# Verify the signature
codesign -dvvv driver/build/AES67VSC.driver

# You should see:
# Authority=AES67 Audio Driver
# Signature=adhoc
# (It's still adhoc, but now with a certificate)
```

---

## Step 4: Install the Signed Driver

```bash
# Copy to system location
sudo cp -R driver/build/AES67VSC.driver /Library/Audio/Plug-Ins/HAL/

# Set correct permissions
sudo chown -R root:wheel /Library/Audio/Plug-Ins/HAL/AES67VSC.driver
sudo chmod -R 755 /Library/Audio/Plug-Ins/HAL/AES67VSC.driver

# Restart CoreAudio
sudo killall coreaudiod
```

Wait 5-10 seconds, then check:

```bash
system_profiler SPAudioDataType | grep -i "aes67" -A 10
```

---

## Step 5: Test in Ableton

1. **Open Ableton Live**
2. **Preferences → Audio**
3. **Look for "AES67 Virtual Soundcard"** in Output Device dropdown

If it appears: **Success!** 🎉

If it doesn't appear: See troubleshooting below.

---

## Alternative: Use Automated Script

I'll create a script to do all the signing automatically:

```bash
cd "/Users/fredparsons/Documents/Side Projects/AES67 Project"
./scripts/sign-and-install.sh "AES67 Audio Driver"
```

---

## Troubleshooting

### Certificate Not Found

If you get "certificate not found" error:

```bash
# List available code signing identities
security find-identity -v -p codesigning

# Use the exact name from the list
```

### Still Not Loading

If the driver still doesn't load after self-signing:

1. **Check the signature:**
   ```bash
   codesign -dvvv /Library/Audio/Plug-Ins/HAL/AES67VSC.driver
   # Should show: Authority=AES67 Audio Driver
   ```

2. **Check Console logs:**
   ```bash
   log stream --predicate 'process == "coreaudiod"' --level debug
   # Then restart CoreAudio in another terminal:
   sudo killall coreaudiod
   # Watch for any errors
   ```

3. **Try with hardened runtime disabled:**
   ```bash
   codesign --force --deep --sign "AES67 Audio Driver" \
     driver/build/AES67VSC.driver
   # (Remove --options runtime)
   ```

4. **If still not working:**
   - Self-signed certificates may not be sufficient on modern macOS
   - You may still need to disable SIP or get a Developer ID certificate

---

## Why Self-Signing Might Not Be Enough

**Important:** Self-signed certificates may help, but modern macOS (especially Apple Silicon) may still reject them for system-level components like audio drivers because:

1. **Gatekeeper** checks for Apple-issued certificates
2. **System Integrity Protection (SIP)** validates certificate chains
3. **Notarization** may be required for certain system extensions

**However, it's worth trying!** Some users have had success with self-signed certificates for audio drivers.

---

## If Self-Signing Doesn't Work

You'll need to either:

1. **Disable SIP** (see `TROUBLESHOOTING_DRIVER.md`)
2. **Get Apple Developer certificate** ($99/year)
3. **Use CLI tools without driver** (test networking directly)

---

## Next Steps After Successful Signing

Once the driver loads:

1. **Test with web monitor:**
   ```bash
   ./scripts/launch-monitor.sh 8
   ```

2. **Configure Ableton:**
   - Output Device: AES67 Virtual Soundcard
   - Play audio
   - Watch meters in browser!

3. **See `QUICK_START_DAW.md` for detailed testing**

---

## Quick Reference

```bash
# 1. Create certificate in Keychain Access (GUI)

# 2. Sign driver
codesign --force --deep --sign "AES67 Audio Driver" \
  driver/build/AES67VSC.driver

# 3. Install
sudo rm -rf /Library/Audio/Plug-Ins/HAL/AES67VSC.driver
sudo cp -R driver/build/AES67VSC.driver /Library/Audio/Plug-Ins/HAL/
sudo killall coreaudiod

# 4. Test
system_profiler SPAudioDataType | grep -i aes67
```

---

Good luck! Let me know if the driver shows up after self-signing.
