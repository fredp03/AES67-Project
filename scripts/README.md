# Scripts Directory

Helper scripts for building, testing, and running the AES67 Virtual Soundcard.

## Build Scripts

### `build.sh`
Main build script - compiles all components.

```bash
./scripts/build.sh
```

Builds in order:
1. Core Audio Driver
2. Network Engine
3. CLI Tools
4. Unit Tests (runs automatically)
5. SwiftUI Menu Bar App

### `dev-sign.sh`
Development code signing for driver installation.

```bash
./scripts/dev-sign.sh
```

Required before installing driver on macOS.

---

## Testing Scripts

### `test-web-monitor.sh`
Interactive test for web-based audio monitor with DAW integration.

```bash
# Test with 8 channels on default port
./scripts/test-web-monitor.sh 8

# Test with 2 channels on port 3000
./scripts/test-web-monitor.sh 2 3000
```

Features:
- Checks if driver is loaded
- Starts web monitor
- Opens browser automatically
- Provides step-by-step instructions
- Validates server and PTP status

### `latency-test.sh`
*(Placeholder for future two-machine latency testing)*

---

## Launch Scripts

### `launch-monitor.sh`
Quick start for web-based audio monitor.

```bash
# Monitor 8 channels
./scripts/launch-monitor.sh 8

# Monitor 2 channels (default)
./scripts/launch-monitor.sh
```

Features:
- One-command launch
- Automatically opens browser
- Checks for port conflicts
- Provides helpful error messages

---

## Usage Examples

### Complete Build & Test Workflow

```bash
# 1. Build everything
./scripts/build.sh

# 2. Sign the driver (macOS only)
./scripts/dev-sign.sh

# 3. Install driver (requires sudo)
cd driver/build
sudo ./install.sh

# 4. Test with web monitor
./scripts/launch-monitor.sh 8

# 5. Configure DAW to use AES67 Virtual Soundcard
# 6. Play audio and watch meters in browser!
```

### Development Workflow

```bash
# Make code changes...

# Rebuild
./scripts/build.sh

# Test specific component
cd tools/build
./aes67-monitor --help

# Full integration test
./scripts/test-web-monitor.sh 8
```

### Automated Testing

```bash
# Run all unit tests
./scripts/build.sh
# (Tests run automatically at end)

# Or manually
cd tests/build
./run_all_tests

# Monitor with scripting
./scripts/launch-monitor.sh 2 &
MONITOR_PID=$!
sleep 2
curl -s http://localhost:8080/status | jq
kill $MONITOR_PID
```

---

## Script Requirements

### Build Scripts
- **build.sh**: cmake, make, clang
- **dev-sign.sh**: codesign (macOS)

### Test Scripts
- **test-web-monitor.sh**: curl, lsof, system_profiler
- **launch-monitor.sh**: lsof, open (macOS) or xdg-open (Linux)

### All Scripts
- Bash 4.0+
- Run from any directory (use absolute paths internally)

---

## Environment Variables

Scripts respect these optional variables:

```bash
# Build options
export CMAKE_BUILD_TYPE=Debug  # Default: Release
export CMAKE_GENERATOR=Ninja   # Default: Unix Makefiles

# Test options  
export AES67_TEST_INTERFACE=en1  # Default: en0
export AES67_TEST_PORT=3000      # Default: 8080
```

---

## Troubleshooting

### build.sh fails
```bash
# Check cmake version
cmake --version  # Need 3.20+

# Clean build
rm -rf driver/build engine/build tools/build tests/build ui/Aes67VSC/build
./scripts/build.sh

# Verbose output
./scripts/build.sh 2>&1 | tee build.log
```

### test-web-monitor.sh says driver not found
```bash
# Check driver installation
system_profiler SPAudioDataType | grep AES67

# Reinstall driver
cd driver/build
sudo ./install.sh
sudo killall coreaudiod  # Restart audio system
```

### launch-monitor.sh says port in use
```bash
# Find what's using the port
lsof -i :8080

# Kill existing monitor
pkill aes67-monitor

# Or use different port
./scripts/launch-monitor.sh 8  # Uses default port
# Edit script to change default
```

---

## Adding New Scripts

When creating new scripts:

1. **Use absolute paths**
   ```bash
   SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
   PROJECT_ROOT="$SCRIPT_DIR/.."
   ```

2. **Add error checking**
   ```bash
   set -e  # Exit on error
   # Check prerequisites
   if ! command -v cmake >/dev/null 2>&1; then
       echo "Error: cmake not found"
       exit 1
   fi
   ```

3. **Make executable**
   ```bash
   chmod +x scripts/new-script.sh
   ```

4. **Document in this README**

5. **Test from different directories**
   ```bash
   ./scripts/new-script.sh
   cd /tmp && /path/to/scripts/new-script.sh
   ```

---

## Future Scripts

Planned additions:

- [ ] `two-machine-test.sh` - Automated two-Mac testing
- [ ] `benchmark.sh` - Performance profiling
- [ ] `package.sh` - Create distributable package
- [ ] `uninstall.sh` - Complete removal of driver
- [ ] `network-check.sh` - Validate network configuration
- [ ] `ptp-master.sh` - Start local PTP master clock

---

**See Also**:
- `docs/WEB_MONITOR_GUIDE.md` - Web monitor usage
- `docs/CLI_TOOLS.md` - CLI tool documentation
- `README.md` - Project overview
