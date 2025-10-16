// AES67_Clock.cpp - Clock implementation
// SPDX-License-Identifier: MIT

#include "AES67_Clock.h"
#include "AES67_EngineInterface.h"

namespace AES67 {

Clock::Clock(INetworkEngine* engine)
    : engine_(engine)
{
}

Clock::~Clock() = default;

bool Clock::HasProperty(const AudioObjectPropertyAddress* address) const {
    switch (address->mSelector) {
        case kAudioObjectPropertyBaseClass:
        case kAudioObjectPropertyClass:
        case kAudioObjectPropertyOwner:
        case kAudioClockDevicePropertyClockDomain:
            return true;
        default:
            return false;
    }
}

bool Clock::IsPropertySettable(const AudioObjectPropertyAddress* address) const {
    (void)address;
    return false;
}

OSStatus Clock::GetPropertyDataSize(
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
        case kAudioClockDevicePropertyClockDomain:
            *outDataSize = sizeof(UInt32);
            return kAudioHardwareNoError;
        default:
            return kAudioHardwareUnknownPropertyError;
    }
}

OSStatus Clock::GetPropertyData(
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
        case kAudioClockDevicePropertyClockDomain:
            if (inDataSize >= sizeof(UInt32)) {
                *static_cast<UInt32*>(outData) = 0; // Custom clock domain
                *outDataSize = sizeof(UInt32);
                return kAudioHardwareNoError;
            }
            break;
    }
    
    return kAudioHardwareUnknownPropertyError;
}

} // namespace AES67
