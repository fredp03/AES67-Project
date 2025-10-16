// AES67_Stream.h - Input/Output stream objects
// SPDX-License-Identifier: MIT

#pragma once

#include "AES67_Types.h"
#include "AES67_RingBuffer.h"
#include <CoreAudio/AudioServerPlugIn.h>
#include <array>

namespace AES67 {

class INetworkEngine;

/// Audio stream (input or output)
class Stream {
public:
    Stream(StreamDirection direction, INetworkEngine* engine);
    ~Stream();
    
    // I/O operations
    OSStatus DoIO(
        UInt32 ioBufferFrameSize,
        const AudioServerPlugInIOCycleInfo* ioCycleInfo,
        void* ioMainBuffer,
        void* ioSecondaryBuffer);
    
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
    AudioObjectID GetObjectID() const {
        return direction_ == StreamDirection::Input 
            ? kObjectID_InputStream 
            : kObjectID_OutputStream;
    }
    
    StreamDirection GetDirection() const { return direction_; }
    bool IsInput() const { return direction_ == StreamDirection::Input; }
    bool IsOutput() const { return direction_ == StreamDirection::Output; }
    
private:
    void ReadFromEngine(void* buffer, UInt32 frames);
    void WriteToEngine(const void* buffer, UInt32 frames);
    
    StreamDirection direction_;
    INetworkEngine* engine_;
    
    // Per-stream ring buffers (one per 8-channel block)
    std::array<AudioRingBuffer*, kTotalStreams> ringBuffers_;
    
    // Format
    AudioFormat format_;
};

} // namespace AES67
