// AES67_Device.h - Virtual audio device representing the 64x64 interface
// SPDX-License-Identifier: MIT

#pragma once

#include "AES67_Types.h"
#include "AES67_EngineInterface.h"
#include <CoreAudio/AudioServerPlugIn.h>
#include <memory>
#include <vector>

namespace AES67 {

class Stream;
class Clock;

/// Main device object (64 in, 64 out @ 48kHz)
class Device {
public:
    Device();
    ~Device();
    
    // Device lifecycle
    bool Initialize();
    void Teardown();
    
    // I/O operations
    OSStatus StartIO(UInt32 clientID);
    OSStatus StopIO(UInt32 clientID);
    
    // Timing
    OSStatus GetZeroTimeStamp(
        UInt32 clientID,
        Float64* outSampleTime,
        UInt64* outHostTime,
        UInt64* outSeed);
    
    // I/O cycle (called by PlugIn)
    OSStatus BeginIOCycle(
        UInt32 clientID,
        UInt32 ioBufferFrameSize,
        const AudioServerPlugInIOCycleInfo* ioCycleInfo);
    
    OSStatus DoIOForStream(
        AudioObjectID streamID,
        UInt32 clientID,
        UInt32 ioBufferFrameSize,
        const AudioServerPlugInIOCycleInfo* ioCycleInfo,
        void* ioMainBuffer,
        void* ioSecondaryBuffer);
    
    OSStatus EndIOCycle(
        UInt32 clientID,
        UInt32 ioBufferFrameSize,
        const AudioServerPlugInIOCycleInfo* ioCycleInfo);
    
    // Property queries
    bool HasProperty(const AudioObjectPropertyAddress* address) const;
    bool IsPropertySettable(const AudioObjectPropertyAddress* address) const;
    OSStatus GetPropertyDataSize(
        const AudioObjectPropertyAddress* address,
        UInt32 qualifierDataSize,
        const void* qualifierData,
        UInt32* outDataSize) const;
    OSStatus GetPropertyData(
        const AudioObjectPropertyAddress* address,
        UInt32 qualifierDataSize,
        const void* qualifierData,
        UInt32 inDataSize,
        UInt32* outDataSize,
        void* outData) const;
    OSStatus SetPropertyData(
        const AudioObjectPropertyAddress* address,
        UInt32 qualifierDataSize,
        const void* qualifierData,
        UInt32 inDataSize,
        const void* inData);
    
    // Accessors
    AudioObjectID GetObjectID() const { return kObjectID_Device; }
    Stream* GetInputStream() const { return inputStream_.get(); }
    Stream* GetOutputStream() const { return outputStream_.get(); }
    const DeviceState& GetState() const { return state_; }
    DeviceState& GetState() { return state_; }
    
private:
    void UpdateZeroTimeStamp();
    void HandlePTPStatusChange(bool locked, double offsetNs);
    void HandleXrun(uint32_t streamIdx, bool underrun);
    
    DeviceState state_;
    std::unique_ptr<Stream> inputStream_;
    std::unique_ptr<Stream> outputStream_;
    std::unique_ptr<Clock> clock_;
    NetworkEnginePtr engine_;
    
    // Active clients
    std::vector<UInt32> activeClients_;
    
    // Zero timestamp anchor (updated when PTP locked)
    uint64_t zeroTimestampSampleTime_ = 0;
    uint64_t zeroTimestampHostTime_ = 0;
    uint64_t zeroTimestampSeed_ = 1;
};

} // namespace AES67
