// AES67_Types.h - Common types and constants for AES67 VSC driver
// SPDX-License-Identifier: MIT

#pragma once

#include <CoreAudio/AudioServerPlugIn.h>
#include <cstdint>
#include <atomic>

namespace AES67 {

// ============================================================================
// Constants
// ============================================================================

constexpr uint32_t kSampleRate = 48000;
constexpr uint32_t kChannelsPerStream = 8;
constexpr uint32_t kTotalStreams = 8;
constexpr uint32_t kTotalChannels = kChannelsPerStream * kTotalStreams; // 64
constexpr uint32_t kBitsPerSample = 24;
constexpr uint32_t kBytesPerSample = 4; // 24-bit in 32-bit container
constexpr uint32_t kDefaultBufferFrames = 32;
constexpr uint32_t kSafetyOffset = 64; // frames

// Object IDs (must be unique across all devices)
constexpr AudioObjectID kObjectID_PlugIn = kAudioObjectPlugInObject;
constexpr AudioObjectID kObjectID_Box = 2;
constexpr AudioObjectID kObjectID_Device = 3;
constexpr AudioObjectID kObjectID_InputStream = 100;
constexpr AudioObjectID kObjectID_OutputStream = 200;
constexpr AudioObjectID kObjectID_InputClock = 300;
constexpr AudioObjectID kObjectID_OutputClock = 400;

// UUIDs (generate unique for production)
constexpr char kPlugInUUID[] = "AES67VSC-0001-0000-0000-000000000001";
constexpr char kDeviceUID[] = "AES67VSC-Device-Main";
constexpr char kDeviceModelUID[] = "AES67VSC-Model-1";

// ============================================================================
// Audio Format
// ============================================================================

struct AudioFormat {
    Float64 sampleRate = kSampleRate;
    AudioFormatID formatID = kAudioFormatLinearPCM;
    AudioFormatFlags formatFlags = 
        kAudioFormatFlagIsSignedInteger | 
        kAudioFormatFlagIsPacked;
    UInt32 bytesPerPacket = kBytesPerSample * kTotalChannels;
    UInt32 framesPerPacket = 1;
    UInt32 bytesPerFrame = kBytesPerSample * kTotalChannels;
    UInt32 channelsPerFrame = kTotalChannels;
    UInt32 bitsPerChannel = kBitsPerSample;
    
    AudioStreamBasicDescription ToASBD() const {
        AudioStreamBasicDescription asbd{};
        asbd.mSampleRate = sampleRate;
        asbd.mFormatID = formatID;
        asbd.mFormatFlags = formatFlags;
        asbd.mBytesPerPacket = bytesPerPacket;
        asbd.mFramesPerPacket = framesPerPacket;
        asbd.mBytesPerFrame = bytesPerFrame;
        asbd.mChannelsPerFrame = channelsPerFrame;
        asbd.mBitsPerChannel = bitsPerChannel;
        return asbd;
    }
};

// ============================================================================
// Timing structures (PTP-disciplined)
// ============================================================================

struct TimingInfo {
    std::atomic<uint64_t> hostTime{0};      // mach_absolute_time()
    std::atomic<uint64_t> sampleTime{0};    // Sample frame number
    std::atomic<uint64_t> ptpTime{0};       // PTP nanoseconds
    std::atomic<double> rateScalar{1.0};    // Servo-adjusted rate
    std::atomic<double> ptpOffset{0.0};     // Offset from PTP master (ns)
    std::atomic<bool> ptpLocked{false};     // PTP lock status
};

// ============================================================================
// Stream configuration
// ============================================================================

enum class StreamDirection : uint8_t {
    Input = 0,
    Output = 1
};

struct StreamConfig {
    StreamDirection direction;
    uint32_t streamIndex;        // 0-7
    uint32_t firstChannel;        // 0, 8, 16, ... 56
    uint32_t channelCount;        // Always 8
    char multicastAddr[16];       // e.g., "239.69.1.1"
    uint16_t port;                // e.g., 5004
    uint32_t ssrc;                // RTP SSRC
};

// ============================================================================
// Device state
// ============================================================================

struct DeviceState {
    std::atomic<bool> isRunning{false};
    std::atomic<uint32_t> bufferFrames{kDefaultBufferFrames};
    std::atomic<uint32_t> safetyOffset{kSafetyOffset};
    std::atomic<uint64_t> xrunCount{0};
    TimingInfo timing;
};

// ============================================================================
// Helper functions
// ============================================================================

inline constexpr bool IsInput(StreamDirection dir) {
    return dir == StreamDirection::Input;
}

inline constexpr bool IsOutput(StreamDirection dir) {
    return dir == StreamDirection::Output;
}

inline constexpr uint32_t FramesToBytes(uint32_t frames, uint32_t channels) {
    return frames * channels * kBytesPerSample;
}

inline constexpr uint32_t BytesToFrames(uint32_t bytes, uint32_t channels) {
    return bytes / (channels * kBytesPerSample);
}

// ============================================================================
// Error codes
// ============================================================================

enum class Error : OSStatus {
    None = 0,
    InvalidFormat = 'fmt?',
    InvalidOperation = 'oper',
    DeviceBusy = 'busy',
    NoMemory = 'mem?',
    EngineFailure = 'eng?',
    PTPNotLocked = 'ptp?'
};

inline OSStatus ToOSStatus(Error err) {
    return static_cast<OSStatus>(err);
}

} // namespace AES67
