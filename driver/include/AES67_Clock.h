// AES67_Clock.h - PTP-disciplined device clock
// SPDX-License-Identifier: MIT

#pragma once

#include "AES67_Types.h"
#include <CoreAudio/AudioServerPlugIn.h>

namespace AES67 {

class INetworkEngine;

/// Custom clock domain synchronized to PTP
class Clock {
public:
    explicit Clock(INetworkEngine* engine);
    ~Clock();
    
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
    
    // Accessors
    AudioObjectID GetObjectID() const { return kObjectID_InputClock; }
    
private:
    INetworkEngine* engine_;
};

} // namespace AES67
