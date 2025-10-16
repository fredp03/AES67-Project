// AES67_Stream.cpp - Stream implementation
// SPDX-License-Identifier: MIT

#include "AES67_Stream.h"
#include "AES67_EngineInterface.h"
#include <cstring>
#include <vector>

namespace AES67 {

Stream::Stream(StreamDirection direction, INetworkEngine* engine)
    : direction_(direction)
    , engine_(engine)
{
    // Get ring buffers from engine
    for (uint32_t i = 0; i < kTotalStreams; ++i) {
        if (IsInput()) {
            ringBuffers_[i] = engine_->GetInputRingBuffer(i);
        } else {
            ringBuffers_[i] = engine_->GetOutputRingBuffer(i);
        }
    }
}

Stream::~Stream() = default;

OSStatus Stream::DoIO(
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo,
    void* ioMainBuffer,
    void* ioSecondaryBuffer)
{
    (void)ioCycleInfo;
    (void)ioSecondaryBuffer;
    
    if (!ioMainBuffer) {
        return kAudioHardwareIllegalOperationError;
    }
    
    if (IsInput()) {
        ReadFromEngine(ioMainBuffer, ioBufferFrameSize);
    } else {
        WriteToEngine(ioMainBuffer, ioBufferFrameSize);
    }
    
    return kAudioHardwareNoError;
}

void Stream::ReadFromEngine(void* buffer, UInt32 frames) {
    auto* output = static_cast<int32_t*>(buffer);
    
    // Read from each 8-channel ring buffer
    for (uint32_t streamIdx = 0; streamIdx < kTotalStreams; ++streamIdx) {
        auto* ring = ringBuffers_[streamIdx];
        if (!ring) continue;
        
        // Temporary buffer for this stream's channels (max 64 frames * 8 channels)
        std::vector<int32_t> temp(kChannelsPerStream * frames);
        size_t framesRead = ring->Read(temp.data(), frames);
        
        // Deinterleave into main buffer
        for (size_t f = 0; f < framesRead; ++f) {
            for (uint32_t c = 0; c < kChannelsPerStream; ++c) {
                uint32_t globalChannel = streamIdx * kChannelsPerStream + c;
                output[f * kTotalChannels + globalChannel] = temp[f * kChannelsPerStream + c];
            }
        }
        
        // Fill remaining with silence
        if (framesRead < frames) {
            for (size_t f = framesRead; f < frames; ++f) {
                for (uint32_t c = 0; c < kChannelsPerStream; ++c) {
                    uint32_t globalChannel = streamIdx * kChannelsPerStream + c;
                    output[f * kTotalChannels + globalChannel] = 0;
                }
            }
        }
    }
}

void Stream::WriteToEngine(const void* buffer, UInt32 frames) {
    const auto* input = static_cast<const int32_t*>(buffer);
    
    // Write to each 8-channel ring buffer
    for (uint32_t streamIdx = 0; streamIdx < kTotalStreams; ++streamIdx) {
        auto* ring = ringBuffers_[streamIdx];
        if (!ring) continue;
        
        // Temporary buffer for this stream's channels (max 64 frames * 8 channels)
        std::vector<int32_t> temp(kChannelsPerStream * frames);
        
        // Interleave from main buffer
        for (uint32_t f = 0; f < frames; ++f) {
            for (uint32_t c = 0; c < kChannelsPerStream; ++c) {
                uint32_t globalChannel = streamIdx * kChannelsPerStream + c;
                temp[f * kChannelsPerStream + c] = input[f * kTotalChannels + globalChannel];
            }
        }
        
        ring->Write(temp.data(), frames);
    }
}

bool Stream::HasProperty(const AudioObjectPropertyAddress* address) const {
    switch (address->mSelector) {
        case kAudioObjectPropertyBaseClass:
        case kAudioObjectPropertyClass:
        case kAudioObjectPropertyOwner:
        case kAudioStreamPropertyDirection:
        case kAudioStreamPropertyStartingChannel:
        case kAudioStreamPropertyLatency:
        case kAudioStreamPropertyVirtualFormat:
        case kAudioStreamPropertyPhysicalFormat:
        case kAudioStreamPropertyAvailableVirtualFormats:
        case kAudioStreamPropertyAvailablePhysicalFormats:
            return true;
        default:
            return false;
    }
}

bool Stream::IsPropertySettable(const AudioObjectPropertyAddress* address) const {
    (void)address;
    return false; // All stream properties read-only
}

OSStatus Stream::GetPropertyDataSize(
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
        case kAudioStreamPropertyDirection:
        case kAudioStreamPropertyStartingChannel:
        case kAudioStreamPropertyLatency:
            *outDataSize = sizeof(UInt32);
            return kAudioHardwareNoError;
            
        case kAudioStreamPropertyVirtualFormat:
        case kAudioStreamPropertyPhysicalFormat:
            *outDataSize = sizeof(AudioStreamBasicDescription);
            return kAudioHardwareNoError;
            
        case kAudioStreamPropertyAvailableVirtualFormats:
        case kAudioStreamPropertyAvailablePhysicalFormats:
            *outDataSize = sizeof(AudioStreamRangedDescription);
            return kAudioHardwareNoError;
            
        default:
            return kAudioHardwareUnknownPropertyError;
    }
}

OSStatus Stream::GetPropertyData(
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
        case kAudioStreamPropertyDirection:
            if (inDataSize >= sizeof(UInt32)) {
                *static_cast<UInt32*>(outData) = IsInput() ? 1 : 0;
                *outDataSize = sizeof(UInt32);
                return kAudioHardwareNoError;
            }
            break;
            
        case kAudioStreamPropertyStartingChannel:
            if (inDataSize >= sizeof(UInt32)) {
                *static_cast<UInt32*>(outData) = 1;
                *outDataSize = sizeof(UInt32);
                return kAudioHardwareNoError;
            }
            break;
            
        case kAudioStreamPropertyVirtualFormat:
        case kAudioStreamPropertyPhysicalFormat:
            if (inDataSize >= sizeof(AudioStreamBasicDescription)) {
                *static_cast<AudioStreamBasicDescription*>(outData) = format_.ToASBD();
                *outDataSize = sizeof(AudioStreamBasicDescription);
                return kAudioHardwareNoError;
            }
            break;
            
        case kAudioStreamPropertyAvailableVirtualFormats:
        case kAudioStreamPropertyAvailablePhysicalFormats:
            if (inDataSize >= sizeof(AudioStreamRangedDescription)) {
                auto* desc = static_cast<AudioStreamRangedDescription*>(outData);
                desc->mFormat = format_.ToASBD();
                desc->mSampleRateRange.mMinimum = kSampleRate;
                desc->mSampleRateRange.mMaximum = kSampleRate;
                *outDataSize = sizeof(AudioStreamRangedDescription);
                return kAudioHardwareNoError;
            }
            break;
    }
    
    return kAudioHardwareUnknownPropertyError;
}

} // namespace AES67
