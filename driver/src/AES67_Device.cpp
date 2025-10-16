// AES67_Device.cpp - Device implementation (stub for now, will be completed)
// SPDX-License-Identifier: MIT

#include "AES67_Device.h"
#include "AES67_Stream.h"
#include "AES67_Clock.h"
#include <mach/mach_time.h>

namespace AES67 {

Device::Device() = default;
Device::~Device() = default;

bool Device::Initialize() {
    // Create network engine
    engine_ = NetworkEnginePtr(nullptr); // TODO: pass config path
    if (!engine_) {
        return false;
    }
    
    // Set up callbacks
    EngineCallbacks callbacks;
    callbacks.onPTPStatusChanged = [this](bool locked, double offsetNs) {
        HandlePTPStatusChange(locked, offsetNs);
    };
    callbacks.onXrunDetected = [this](uint32_t streamIdx, bool underrun) {
        HandleXrun(streamIdx, underrun);
    };
    callbacks.onError = [](const char* msg) {
        // TODO: Log error
        (void)msg;
    };
    
    engine_->SetCallbacks(callbacks);
    
    // Create streams
    inputStream_ = std::make_unique<Stream>(StreamDirection::Input, engine_.Get());
    outputStream_ = std::make_unique<Stream>(StreamDirection::Output, engine_.Get());
    
    // Create clock
    clock_ = std::make_unique<Clock>(engine_.Get());
    
    return true;
}

void Device::Teardown() {
    if (state_.isRunning) {
        engine_->Stop();
        state_.isRunning = false;
    }
    
    clock_.reset();
    outputStream_.reset();
    inputStream_.reset();
    engine_ = NetworkEnginePtr(nullptr);
}

OSStatus Device::StartIO(UInt32 clientID) {
    if (std::find(activeClients_.begin(), activeClients_.end(), clientID) != activeClients_.end()) {
        return kAudioHardwareNoError; // Already running
    }
    
    if (!state_.isRunning) {
        if (!engine_->Start()) {
            return kAudioHardwareUnspecifiedError;
        }
        state_.isRunning = true;
        UpdateZeroTimeStamp();
    }
    
    activeClients_.push_back(clientID);
    return kAudioHardwareNoError;
}

OSStatus Device::StopIO(UInt32 clientID) {
    auto it = std::find(activeClients_.begin(), activeClients_.end(), clientID);
    if (it != activeClients_.end()) {
        activeClients_.erase(it);
    }
    
    if (activeClients_.empty() && state_.isRunning) {
        engine_->Stop();
        state_.isRunning = false;
    }
    
    return kAudioHardwareNoError;
}

OSStatus Device::GetZeroTimeStamp(
    UInt32 clientID,
    Float64* outSampleTime,
    UInt64* outHostTime,
    UInt64* outSeed)
{
    (void)clientID;
    
    if (!outSampleTime || !outHostTime || !outSeed) {
        return kAudioHardwareIllegalOperationError;
    }
    
    *outSampleTime = static_cast<Float64>(zeroTimestampSampleTime_);
    *outHostTime = zeroTimestampHostTime_;
    *outSeed = zeroTimestampSeed_;
    
    return kAudioHardwareNoError;
}

OSStatus Device::BeginIOCycle(
    UInt32 clientID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo)
{
    (void)clientID;
    (void)ioBufferFrameSize;
    
    if (!ioCycleInfo) {
        return kAudioHardwareIllegalOperationError;
    }
    
    // Notify engine of I/O cycle start
    engine_->NotifyIOCycle(
        ioCycleInfo->mInputTime.mHostTime,
        static_cast<uint64_t>(ioCycleInfo->mInputTime.mSampleTime));
    
    return kAudioHardwareNoError;
}

OSStatus Device::DoIOForStream(
    AudioObjectID streamID,
    UInt32 clientID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo,
    void* ioMainBuffer,
    void* ioSecondaryBuffer)
{
    (void)clientID;
    
    Stream* stream = nullptr;
    if (streamID == kObjectID_InputStream) {
        stream = inputStream_.get();
    } else if (streamID == kObjectID_OutputStream) {
        stream = outputStream_.get();
    }
    
    if (!stream) {
        return kAudioHardwareBadObjectError;
    }
    
    return stream->DoIO(ioBufferFrameSize, ioCycleInfo, ioMainBuffer, ioSecondaryBuffer);
}

OSStatus Device::EndIOCycle(
    UInt32 clientID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo)
{
    (void)clientID;
    (void)ioBufferFrameSize;
    (void)ioCycleInfo;
    return kAudioHardwareNoError;
}

void Device::UpdateZeroTimeStamp() {
    zeroTimestampHostTime_ = mach_absolute_time();
    zeroTimestampSampleTime_ = 0;
    zeroTimestampSeed_++;
}

void Device::HandlePTPStatusChange(bool locked, double offsetNs) {
    state_.timing.ptpLocked = locked;
    state_.timing.ptpOffset = offsetNs;
    
    if (locked) {
        UpdateZeroTimeStamp();
    }
}

void Device::HandleXrun(uint32_t streamIdx, bool underrun) {
    (void)streamIdx;
    (void)underrun;
    state_.xrunCount++;
}

// Property implementation stubs (full implementation needed)
bool Device::HasProperty(const AudioObjectPropertyAddress* address) const {
    switch (address->mSelector) {
        case kAudioObjectPropertyBaseClass:
        case kAudioObjectPropertyClass:
        case kAudioObjectPropertyOwner:
        case kAudioObjectPropertyName:
        case kAudioObjectPropertyManufacturer:
        case kAudioObjectPropertyModelName:
        case kAudioDevicePropertyDeviceUID:
        case kAudioDevicePropertyModelUID:
        case kAudioDevicePropertyStreams:
        case kAudioDevicePropertyNominalSampleRate:
        case kAudioDevicePropertyAvailableNominalSampleRates:
        case kAudioDevicePropertyIsRunning:
        case kAudioDevicePropertyLatency:
        case kAudioDevicePropertySafetyOffset:
        case kAudioDevicePropertyClockDomain:
            return true;
        default:
            return false;
    }
}

bool Device::IsPropertySettable(const AudioObjectPropertyAddress* address) const {
    (void)address;
    return false; // All device properties read-only for now
}

OSStatus Device::GetPropertyDataSize(
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32* outDataSize) const
{
    (void)qualifierDataSize;
    (void)qualifierData;
    
    switch (address->mSelector) {
        case kAudioObjectPropertyBaseClass:
        case kAudioObjectPropertyClass:
        case kAudioObjectPropertyOwner:
        case kAudioDevicePropertyClockDomain:
        case kAudioDevicePropertyLatency:
        case kAudioDevicePropertySafetyOffset:
            *outDataSize = sizeof(UInt32);
            return kAudioHardwareNoError;
            
        case kAudioDevicePropertyNominalSampleRate:
            *outDataSize = sizeof(Float64);
            return kAudioHardwareNoError;
            
        case kAudioDevicePropertyIsRunning:
            *outDataSize = sizeof(UInt32);
            return kAudioHardwareNoError;
            
        case kAudioObjectPropertyName:
        case kAudioObjectPropertyManufacturer:
        case kAudioObjectPropertyModelName:
        case kAudioDevicePropertyDeviceUID:
        case kAudioDevicePropertyModelUID:
            *outDataSize = sizeof(CFStringRef);
            return kAudioHardwareNoError;
            
        case kAudioDevicePropertyStreams:
            *outDataSize = sizeof(AudioObjectID) * 2; // Input + Output
            return kAudioHardwareNoError;
            
        default:
            return kAudioHardwareUnknownPropertyError;
    }
}

OSStatus Device::GetPropertyData(
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32 inDataSize,
    UInt32* outDataSize,
    void* outData) const
{
    (void)qualifierDataSize;
    (void)qualifierData;
    
    switch (address->mSelector) {
        case kAudioObjectPropertyName:
            if (inDataSize >= sizeof(CFStringRef)) {
                *static_cast<CFStringRef*>(outData) = CFSTR("AES67 VSC (64×64 @ 48k)");
                *outDataSize = sizeof(CFStringRef);
                return kAudioHardwareNoError;
            }
            break;
            
        case kAudioDevicePropertyNominalSampleRate:
            if (inDataSize >= sizeof(Float64)) {
                *static_cast<Float64*>(outData) = kSampleRate;
                *outDataSize = sizeof(Float64);
                return kAudioHardwareNoError;
            }
            break;
            
        case kAudioDevicePropertyIsRunning:
            if (inDataSize >= sizeof(UInt32)) {
                *static_cast<UInt32*>(outData) = state_.isRunning ? 1 : 0;
                *outDataSize = sizeof(UInt32);
                return kAudioHardwareNoError;
            }
            break;
            
        case kAudioDevicePropertyStreams:
            if (inDataSize >= sizeof(AudioObjectID) * 2) {
                auto ids = static_cast<AudioObjectID*>(outData);
                if (address->mScope == kAudioObjectPropertyScopeInput) {
                    ids[0] = kObjectID_InputStream;
                    *outDataSize = sizeof(AudioObjectID);
                } else if (address->mScope == kAudioObjectPropertyScopeOutput) {
                    ids[0] = kObjectID_OutputStream;
                    *outDataSize = sizeof(AudioObjectID);
                } else {
                    ids[0] = kObjectID_InputStream;
                    ids[1] = kObjectID_OutputStream;
                    *outDataSize = sizeof(AudioObjectID) * 2;
                }
                return kAudioHardwareNoError;
            }
            break;
    }
    
    return kAudioHardwareUnknownPropertyError;
}

OSStatus Device::SetPropertyData(
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32 inDataSize,
    const void* inData)
{
    (void)address;
    (void)qualifierDataSize;
    (void)qualifierData;
    (void)inDataSize;
    (void)inData;
    return kAudioHardwareUnsupportedOperationError;
}

} // namespace AES67
