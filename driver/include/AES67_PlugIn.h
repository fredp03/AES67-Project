// AES67_PlugIn.h - Main AudioServerPlugIn entry point
// SPDX-License-Identifier: MIT

#pragma once

#include "AES67_Types.h"
#include <CoreAudio/AudioServerPlugIn.h>

namespace AES67 {

/// PlugIn singleton (owns device, handles HAL requests)
class PlugIn {
public:
    static PlugIn& Instance();
    
    // HAL interface callbacks
    OSStatus Initialize(AudioServerPlugInHostRef host);
    OSStatus Teardown();
    
    // Object property queries
    OSStatus HasProperty(
        AudioObjectID objectID,
        const AudioObjectPropertyAddress* address,
        Boolean* outHasProperty) const;
    
    OSStatus IsPropertySettable(
        AudioObjectID objectID,
        const AudioObjectPropertyAddress* address,
        Boolean* outIsSettable) const;
    
    OSStatus GetPropertyDataSize(
        AudioObjectID objectID,
        const AudioObjectPropertyAddress* address,
        UInt32 qualifierDataSize,
        const void* qualifierData,
        UInt32* outDataSize) const;
    
    OSStatus GetPropertyData(
        AudioObjectID objectID,
        const AudioObjectPropertyAddress* address,
        UInt32 qualifierDataSize,
        const void* qualifierData,
        UInt32 inDataSize,
        UInt32* outDataSize,
        void* outData) const;
    
    OSStatus SetPropertyData(
        AudioObjectID objectID,
        const AudioObjectPropertyAddress* address,
        UInt32 qualifierDataSize,
        const void* qualifierData,
        UInt32 inDataSize,
        const void* inData);
    
    // Device operations
    OSStatus StartIO(AudioObjectID deviceID, UInt32 clientID);
    OSStatus StopIO(AudioObjectID deviceID, UInt32 clientID);
    OSStatus GetZeroTimeStamp(
        AudioObjectID deviceID,
        UInt32 clientID,
        Float64* outSampleTime,
        UInt64* outHostTime,
        UInt64* outSeed);
    OSStatus WillDoIOOperation(
        AudioObjectID deviceID,
        UInt32 clientID,
        UInt32 operationID,
        Boolean* outWillDo,
        Boolean* outWillDoInPlace) const;
    OSStatus BeginIOOperation(
        AudioObjectID deviceID,
        UInt32 clientID,
        UInt32 operationID,
        UInt32 ioBufferFrameSize,
        const AudioServerPlugInIOCycleInfo* ioCycleInfo);
    OSStatus DoIOOperation(
        AudioObjectID deviceID,
        AudioObjectID streamID,
        UInt32 clientID,
        UInt32 operationID,
        UInt32 ioBufferFrameSize,
        const AudioServerPlugInIOCycleInfo* ioCycleInfo,
        void* ioMainBuffer,
        void* ioSecondaryBuffer);
    OSStatus EndIOOperation(
        AudioObjectID deviceID,
        UInt32 clientID,
        UInt32 operationID,
        UInt32 ioBufferFrameSize,
        const AudioServerPlugInIOCycleInfo* ioCycleInfo);

    AudioServerPlugInHostRef GetHost() const { return host_; }

private:
    PlugIn() = default;
    ~PlugIn() = default;
    
    // Singleton: no copy/move
    PlugIn(const PlugIn&) = delete;
    PlugIn& operator=(const PlugIn&) = delete;
    
    AudioServerPlugInHostRef host_ = nullptr;
};

} // namespace AES67

// ============================================================================
// C API (required by AudioServerPlugIn architecture)
// ============================================================================

extern "C" {

/// Factory function called by coreaudiod
void* AES67_PlugIn_Factory(CFAllocatorRef allocator, CFUUIDRef typeUUID);

/// PlugIn interface (dispatch table)
OSStatus AES67_PlugIn_QueryInterface(
    void* driver,
    REFIID iid,
    LPVOID* ppv);

ULONG AES67_PlugIn_AddRef(void* driver);
ULONG AES67_PlugIn_Release(void* driver);

OSStatus AES67_PlugIn_Initialize(
    AudioServerPlugInDriverRef driver,
    AudioServerPlugInHostRef host);

OSStatus AES67_PlugIn_CreateDevice(
    AudioServerPlugInDriverRef driver,
    CFDictionaryRef description,
    const AudioServerPlugInClientInfo* clientInfo,
    AudioObjectID* outDeviceObjectID);

OSStatus AES67_PlugIn_DestroyDevice(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID);

OSStatus AES67_PlugIn_AddDeviceClient(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    const AudioServerPlugInClientInfo* clientInfo);

OSStatus AES67_PlugIn_RemoveDeviceClient(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    const AudioServerPlugInClientInfo* clientInfo);

OSStatus AES67_PlugIn_PerformDeviceConfigurationChange(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt64 changeAction,
    void* changeInfo);

OSStatus AES67_PlugIn_AbortDeviceConfigurationChange(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt64 changeAction,
    void* changeInfo);

OSStatus AES67_PlugIn_HasProperty(
    AudioServerPlugInDriverRef driver,
    AudioObjectID objectID,
    pid_t clientProcessID,
    const AudioObjectPropertyAddress* address,
    Boolean* outHasProperty);

OSStatus AES67_PlugIn_IsPropertySettable(
    AudioServerPlugInDriverRef driver,
    AudioObjectID objectID,
    pid_t clientProcessID,
    const AudioObjectPropertyAddress* address,
    Boolean* outIsSettable);

OSStatus AES67_PlugIn_GetPropertyDataSize(
    AudioServerPlugInDriverRef driver,
    AudioObjectID objectID,
    pid_t clientProcessID,
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32* outDataSize);

OSStatus AES67_PlugIn_GetPropertyData(
    AudioServerPlugInDriverRef driver,
    AudioObjectID objectID,
    pid_t clientProcessID,
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32 inDataSize,
    UInt32* outDataSize,
    void* outData);

OSStatus AES67_PlugIn_SetPropertyData(
    AudioServerPlugInDriverRef driver,
    AudioObjectID objectID,
    pid_t clientProcessID,
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32 inDataSize,
    const void* inData);

OSStatus AES67_PlugIn_StartIO(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID);

OSStatus AES67_PlugIn_StopIO(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID);

OSStatus AES67_PlugIn_GetZeroTimeStamp(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID,
    Float64* outSampleTime,
    UInt64* outHostTime,
    UInt64* outSeed);

OSStatus AES67_PlugIn_WillDoIOOperation(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID,
    UInt32 operationID,
    Boolean* outWillDo,
    Boolean* outWillDoInPlace);

OSStatus AES67_PlugIn_BeginIOOperation(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID,
    UInt32 operationID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo);

OSStatus AES67_PlugIn_DoIOOperation(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    AudioObjectID streamObjectID,
    UInt32 clientID,
    UInt32 operationID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo,
    void* ioMainBuffer,
    void* ioSecondaryBuffer);

OSStatus AES67_PlugIn_EndIOOperation(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID,
    UInt32 operationID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo);

} // extern "C"
