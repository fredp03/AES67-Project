// AES67_PlugIn.cpp - AudioServerPlugIn interface implementation
// SPDX-License-Identifier: MIT

#include "AES67_PlugIn.h"
#include "AES67_Device.h"
#include <CoreFoundation/CoreFoundation.h>
#include <memory>

namespace AES67 {

namespace {
    std::unique_ptr<Device> g_device;
}

// ============================================================================
// PlugIn Implementation
// ============================================================================

PlugIn& PlugIn::Instance() {
    static PlugIn instance;
    return instance;
}

OSStatus PlugIn::Initialize(AudioServerPlugInHostRef host) {
    if (host_ != nullptr) {
        return kAudioHardwareIllegalOperationError;
    }
    
    host_ = host;
    
    // Create the device
    g_device = std::make_unique<Device>();
    if (!g_device->Initialize()) {
        g_device.reset();
        host_ = nullptr;
        return kAudioHardwareUnspecifiedError;
    }
    
    return kAudioHardwareNoError;
}

OSStatus PlugIn::Teardown() {
    if (g_device) {
        g_device->Teardown();
        g_device.reset();
    }
    host_ = nullptr;
    return kAudioHardwareNoError;
}

OSStatus PlugIn::HasProperty(
    AudioObjectID objectID,
    const AudioObjectPropertyAddress* address,
    Boolean* outHasProperty) const
{
    if (!address || !outHasProperty) {
        return kAudioHardwareIllegalOperationError;
    }
    
    *outHasProperty = false;
    
    switch (objectID) {
        case kObjectID_PlugIn:
            // PlugIn properties
            switch (address->mSelector) {
                case kAudioObjectPropertyBaseClass:
                case kAudioObjectPropertyClass:
                case kAudioObjectPropertyOwner:
                case kAudioObjectPropertyManufacturer:
                case kAudioObjectPropertyOwnedObjects:
                case kAudioPlugInPropertyDeviceList:
                case kAudioPlugInPropertyTranslateUIDToDevice:
                case kAudioPlugInPropertyResourceBundle:
                    *outHasProperty = true;
                    break;
            }
            break;
            
        case kObjectID_Device:
            if (g_device) {
                *outHasProperty = g_device->HasProperty(address);
            }
            break;
            
        case kObjectID_InputStream:
            if (g_device && g_device->GetInputStream()) {
                *outHasProperty = g_device->GetInputStream()->HasProperty(address);
            }
            break;
            
        case kObjectID_OutputStream:
            if (g_device && g_device->GetOutputStream()) {
                *outHasProperty = g_device->GetOutputStream()->HasProperty(address);
            }
            break;
    }
    
    return kAudioHardwareNoError;
}

OSStatus PlugIn::IsPropertySettable(
    AudioObjectID objectID,
    const AudioObjectPropertyAddress* address,
    Boolean* outIsSettable) const
{
    if (!address || !outIsSettable) {
        return kAudioHardwareIllegalOperationError;
    }
    
    *outIsSettable = false;
    
    switch (objectID) {
        case kObjectID_PlugIn:
            // PlugIn properties are read-only
            break;
            
        case kObjectID_Device:
            if (g_device) {
                *outIsSettable = g_device->IsPropertySettable(address);
            }
            break;
            
        case kObjectID_InputStream:
        case kObjectID_OutputStream:
            // Stream properties are read-only
            break;
    }
    
    return kAudioHardwareNoError;
}

OSStatus PlugIn::GetPropertyDataSize(
    AudioObjectID objectID,
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32* outDataSize) const
{
    if (!address || !outDataSize) {
        return kAudioHardwareIllegalOperationError;
    }
    
    switch (objectID) {
        case kObjectID_PlugIn:
            switch (address->mSelector) {
                case kAudioObjectPropertyBaseClass:
                case kAudioObjectPropertyClass:
                    *outDataSize = sizeof(AudioClassID);
                    return kAudioHardwareNoError;
                    
                case kAudioObjectPropertyOwner:
                    *outDataSize = sizeof(AudioObjectID);
                    return kAudioHardwareNoError;
                    
                case kAudioObjectPropertyManufacturer:
                    *outDataSize = sizeof(CFStringRef);
                    return kAudioHardwareNoError;
                    
                case kAudioObjectPropertyOwnedObjects:
                case kAudioPlugInPropertyDeviceList:
                    *outDataSize = sizeof(AudioObjectID) * (g_device ? 1 : 0);
                    return kAudioHardwareNoError;
                    
                case kAudioPlugInPropertyTranslateUIDToDevice:
                    *outDataSize = sizeof(AudioObjectID);
                    return kAudioHardwareNoError;
                    
                case kAudioPlugInPropertyResourceBundle:
                    *outDataSize = sizeof(CFStringRef);
                    return kAudioHardwareNoError;
            }
            break;
            
        case kObjectID_Device:
            if (g_device) {
                return g_device->GetPropertyDataSize(
                    address, qualifierDataSize, qualifierData, outDataSize);
            }
            break;
            
        case kObjectID_InputStream:
            if (g_device && g_device->GetInputStream()) {
                return g_device->GetInputStream()->GetPropertyDataSize(
                    address, qualifierDataSize, qualifierData, outDataSize);
            }
            break;
            
        case kObjectID_OutputStream:
            if (g_device && g_device->GetOutputStream()) {
                return g_device->GetOutputStream()->GetPropertyDataSize(
                    address, qualifierDataSize, qualifierData, outDataSize);
            }
            break;
    }
    
    return kAudioHardwareUnknownPropertyError;
}

OSStatus PlugIn::GetPropertyData(
    AudioObjectID objectID,
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32 inDataSize,
    UInt32* outDataSize,
    void* outData) const
{
    if (!address || !outDataSize || !outData) {
        return kAudioHardwareIllegalOperationError;
    }
    
    switch (objectID) {
        case kObjectID_PlugIn:
            switch (address->mSelector) {
                case kAudioObjectPropertyBaseClass:
                    if (inDataSize >= sizeof(AudioClassID)) {
                        *static_cast<AudioClassID*>(outData) = kAudioObjectClassID;
                        *outDataSize = sizeof(AudioClassID);
                        return kAudioHardwareNoError;
                    }
                    break;
                    
                case kAudioObjectPropertyClass:
                    if (inDataSize >= sizeof(AudioClassID)) {
                        *static_cast<AudioClassID*>(outData) = kAudioPlugInClassID;
                        *outDataSize = sizeof(AudioClassID);
                        return kAudioHardwareNoError;
                    }
                    break;
                    
                case kAudioObjectPropertyOwner:
                    if (inDataSize >= sizeof(AudioObjectID)) {
                        *static_cast<AudioObjectID*>(outData) = kAudioObjectSystemObject;
                        *outDataSize = sizeof(AudioObjectID);
                        return kAudioHardwareNoError;
                    }
                    break;
                    
                case kAudioObjectPropertyManufacturer:
                    if (inDataSize >= sizeof(CFStringRef)) {
                        *static_cast<CFStringRef*>(outData) = 
                            CFSTR("AES67 Virtual Soundcard");
                        *outDataSize = sizeof(CFStringRef);
                        return kAudioHardwareNoError;
                    }
                    break;
                    
                case kAudioObjectPropertyOwnedObjects:
                case kAudioPlugInPropertyDeviceList:
                    if (g_device && inDataSize >= sizeof(AudioObjectID)) {
                        *static_cast<AudioObjectID*>(outData) = kObjectID_Device;
                        *outDataSize = sizeof(AudioObjectID);
                        return kAudioHardwareNoError;
                    }
                    *outDataSize = 0;
                    return kAudioHardwareNoError;
                    
                case kAudioPlugInPropertyTranslateUIDToDevice:
                    if (g_device && 
                        qualifierDataSize >= sizeof(CFStringRef) &&
                        inDataSize >= sizeof(AudioObjectID))
                    {
                        auto uid = *static_cast<const CFStringRef*>(qualifierData);
                        CFStringRef deviceUID = CFStringCreateWithCString(
                            nullptr, kDeviceUID, kCFStringEncodingUTF8);
                        
                        if (CFStringCompare(uid, deviceUID, 0) == kCFCompareEqualTo) {
                            *static_cast<AudioObjectID*>(outData) = kObjectID_Device;
                            *outDataSize = sizeof(AudioObjectID);
                            CFRelease(deviceUID);
                            return kAudioHardwareNoError;
                        }
                        CFRelease(deviceUID);
                    }
                    return kAudioHardwareBadObjectError;
                    
                case kAudioPlugInPropertyResourceBundle:
                    if (inDataSize >= sizeof(CFStringRef)) {
                        *static_cast<CFStringRef*>(outData) = 
                            CFSTR("com.aes67.vsc.driver");
                        *outDataSize = sizeof(CFStringRef);
                        return kAudioHardwareNoError;
                    }
                    break;
            }
            break;
            
        case kObjectID_Device:
            if (g_device) {
                return g_device->GetPropertyData(
                    address, qualifierDataSize, qualifierData,
                    inDataSize, outDataSize, outData);
            }
            break;
            
        case kObjectID_InputStream:
            if (g_device && g_device->GetInputStream()) {
                return g_device->GetInputStream()->GetPropertyData(
                    address, qualifierDataSize, qualifierData,
                    inDataSize, outDataSize, outData);
            }
            break;
            
        case kObjectID_OutputStream:
            if (g_device && g_device->GetOutputStream()) {
                return g_device->GetOutputStream()->GetPropertyData(
                    address, qualifierDataSize, qualifierData,
                    inDataSize, outDataSize, outData);
            }
            break;
    }
    
    return kAudioHardwareUnknownPropertyError;
}

OSStatus PlugIn::SetPropertyData(
    AudioObjectID objectID,
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32 inDataSize,
    const void* inData)
{
    if (!address || !inData) {
        return kAudioHardwareIllegalOperationError;
    }
    
    switch (objectID) {
        case kObjectID_Device:
            if (g_device) {
                return g_device->SetPropertyData(
                    address, qualifierDataSize, qualifierData,
                    inDataSize, inData);
            }
            break;
            
        default:
            return kAudioHardwareUnsupportedOperationError;
    }
    
    return kAudioHardwareUnknownPropertyError;
}

OSStatus PlugIn::StartIO(AudioObjectID deviceID, UInt32 clientID) {
    if (deviceID != kObjectID_Device || !g_device) {
        return kAudioHardwareBadObjectError;
    }
    return g_device->StartIO(clientID);
}

OSStatus PlugIn::StopIO(AudioObjectID deviceID, UInt32 clientID) {
    if (deviceID != kObjectID_Device || !g_device) {
        return kAudioHardwareBadObjectError;
    }
    return g_device->StopIO(clientID);
}

OSStatus PlugIn::GetZeroTimeStamp(
    AudioObjectID deviceID,
    UInt32 clientID,
    Float64* outSampleTime,
    UInt64* outHostTime,
    UInt64* outSeed)
{
    if (deviceID != kObjectID_Device || !g_device) {
        return kAudioHardwareBadObjectError;
    }
    return g_device->GetZeroTimeStamp(clientID, outSampleTime, outHostTime, outSeed);
}

OSStatus PlugIn::WillDoIOOperation(
    AudioObjectID deviceID,
    UInt32 clientID,
    UInt32 operationID,
    Boolean* outWillDo,
    Boolean* outWillDoInPlace) const
{
    if (deviceID != kObjectID_Device || !outWillDo || !outWillDoInPlace) {
        return kAudioHardwareBadObjectError;
    }
    
    *outWillDo = true;
    *outWillDoInPlace = true;
    return kAudioHardwareNoError;
}

OSStatus PlugIn::BeginIOOperation(
    AudioObjectID deviceID,
    UInt32 clientID,
    UInt32 operationID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo)
{
    if (deviceID != kObjectID_Device || !g_device) {
        return kAudioHardwareBadObjectError;
    }
    return g_device->BeginIOCycle(clientID, ioBufferFrameSize, ioCycleInfo);
}

OSStatus PlugIn::DoIOOperation(
    AudioObjectID deviceID,
    AudioObjectID streamID,
    UInt32 clientID,
    UInt32 operationID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo,
    void* ioMainBuffer,
    void* ioSecondaryBuffer)
{
    if (deviceID != kObjectID_Device || !g_device) {
        return kAudioHardwareBadObjectError;
    }
    return g_device->DoIOForStream(
        streamID, clientID, ioBufferFrameSize,
        ioCycleInfo, ioMainBuffer, ioSecondaryBuffer);
}

OSStatus PlugIn::EndIOOperation(
    AudioObjectID deviceID,
    UInt32 clientID,
    UInt32 operationID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo)
{
    if (deviceID != kObjectID_Device || !g_device) {
        return kAudioHardwareBadObjectError;
    }
    return g_device->EndIOCycle(clientID, ioBufferFrameSize, ioCycleInfo);
}

} // namespace AES67

// ============================================================================
// C API Implementation
// ============================================================================

using namespace AES67;

extern "C" {

// AudioServerPlugIn interface vtable
static AudioServerPlugInDriverInterface gPlugInInterface = {
    nullptr,  // _reserved
    AES67_PlugIn_QueryInterface,
    AES67_PlugIn_AddRef,
    AES67_PlugIn_Release,
    AES67_PlugIn_Initialize,
    AES67_PlugIn_CreateDevice,
    AES67_PlugIn_DestroyDevice,
    AES67_PlugIn_AddDeviceClient,
    AES67_PlugIn_RemoveDeviceClient,
    AES67_PlugIn_PerformDeviceConfigurationChange,
    AES67_PlugIn_AbortDeviceConfigurationChange,
    AES67_PlugIn_HasProperty,
    AES67_PlugIn_IsPropertySettable,
    AES67_PlugIn_GetPropertyDataSize,
    AES67_PlugIn_GetPropertyData,
    AES67_PlugIn_SetPropertyData,
    AES67_PlugIn_StartIO,
    AES67_PlugIn_StopIO,
    AES67_PlugIn_GetZeroTimeStamp,
    AES67_PlugIn_WillDoIOOperation,
    AES67_PlugIn_BeginIOOperation,
    AES67_PlugIn_DoIOOperation,
    AES67_PlugIn_EndIOOperation
};

void* AES67_PlugIn_Factory(CFAllocatorRef allocator, CFUUIDRef typeUUID) {
    (void)allocator;
    
    if (!CFEqual(typeUUID, kAudioServerPlugInTypeUUID)) {
        return nullptr;
    }
    
    return &gPlugInInterface;
}

OSStatus AES67_PlugIn_QueryInterface(void* driver, REFIID iid, LPVOID* ppv) {
    if (!ppv) return kAudioHardwareIllegalOperationError;
    
    CFUUIDRef interfaceID = CFUUIDCreateFromUUIDBytes(nullptr, iid);
    
    if (CFEqual(interfaceID, kAudioServerPlugInDriverInterfaceUUID)) {
        *ppv = driver;
        CFRelease(interfaceID);
        return kAudioHardwareNoError;
    }
    
    CFRelease(interfaceID);
    return E_NOINTERFACE;
}

ULONG AES67_PlugIn_AddRef(void* driver) {
    (void)driver;
    return 1; // Static singleton
}

ULONG AES67_PlugIn_Release(void* driver) {
    (void)driver;
    return 1; // Static singleton
}

OSStatus AES67_PlugIn_Initialize(
    AudioServerPlugInDriverRef driver,
    AudioServerPlugInHostRef host)
{
    (void)driver;
    return PlugIn::Instance().Initialize(host);
}

OSStatus AES67_PlugIn_CreateDevice(
    AudioServerPlugInDriverRef driver,
    CFDictionaryRef description,
    const AudioServerPlugInClientInfo* clientInfo,
    AudioObjectID* outDeviceObjectID)
{
    (void)driver;
    (void)description;
    (void)clientInfo;
    
    if (!outDeviceObjectID) {
        return kAudioHardwareIllegalOperationError;
    }
    
    // We create device in Initialize; just return its ID
    *outDeviceObjectID = kObjectID_Device;
    return kAudioHardwareNoError;
}

OSStatus AES67_PlugIn_DestroyDevice(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID)
{
    (void)driver;
    (void)deviceObjectID;
    return kAudioHardwareUnsupportedOperationError;
}

OSStatus AES67_PlugIn_AddDeviceClient(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    const AudioServerPlugInClientInfo* clientInfo)
{
    (void)driver;
    (void)deviceObjectID;
    (void)clientInfo;
    return kAudioHardwareNoError;
}

OSStatus AES67_PlugIn_RemoveDeviceClient(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    const AudioServerPlugInClientInfo* clientInfo)
{
    (void)driver;
    (void)deviceObjectID;
    (void)clientInfo;
    return kAudioHardwareNoError;
}

OSStatus AES67_PlugIn_PerformDeviceConfigurationChange(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt64 changeAction,
    void* changeInfo)
{
    (void)driver;
    (void)deviceObjectID;
    (void)changeAction;
    (void)changeInfo;
    return kAudioHardwareNoError;
}

OSStatus AES67_PlugIn_AbortDeviceConfigurationChange(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt64 changeAction,
    void* changeInfo)
{
    (void)driver;
    (void)deviceObjectID;
    (void)changeAction;
    (void)changeInfo;
    return kAudioHardwareNoError;
}

#define DELEGATE_TO_PLUGIN(method, ...) \
    return PlugIn::Instance().method(__VA_ARGS__)

OSStatus AES67_PlugIn_HasProperty(
    AudioServerPlugInDriverRef driver,
    AudioObjectID objectID,
    pid_t clientProcessID,
    const AudioObjectPropertyAddress* address,
    Boolean* outHasProperty)
{
    (void)driver;
    (void)clientProcessID;
    DELEGATE_TO_PLUGIN(HasProperty, objectID, address, outHasProperty);
}

OSStatus AES67_PlugIn_IsPropertySettable(
    AudioServerPlugInDriverRef driver,
    AudioObjectID objectID,
    pid_t clientProcessID,
    const AudioObjectPropertyAddress* address,
    Boolean* outIsSettable)
{
    (void)driver;
    (void)clientProcessID;
    DELEGATE_TO_PLUGIN(IsPropertySettable, objectID, address, outIsSettable);
}

OSStatus AES67_PlugIn_GetPropertyDataSize(
    AudioServerPlugInDriverRef driver,
    AudioObjectID objectID,
    pid_t clientProcessID,
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32* outDataSize)
{
    (void)driver;
    (void)clientProcessID;
    DELEGATE_TO_PLUGIN(GetPropertyDataSize, objectID, address,
                      qualifierDataSize, qualifierData, outDataSize);
}

OSStatus AES67_PlugIn_GetPropertyData(
    AudioServerPlugInDriverRef driver,
    AudioObjectID objectID,
    pid_t clientProcessID,
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32 inDataSize,
    UInt32* outDataSize,
    void* outData)
{
    (void)driver;
    (void)clientProcessID;
    DELEGATE_TO_PLUGIN(GetPropertyData, objectID, address,
                      qualifierDataSize, qualifierData,
                      inDataSize, outDataSize, outData);
}

OSStatus AES67_PlugIn_SetPropertyData(
    AudioServerPlugInDriverRef driver,
    AudioObjectID objectID,
    pid_t clientProcessID,
    const AudioObjectPropertyAddress* address,
    UInt32 qualifierDataSize,
    const void* qualifierData,
    UInt32 inDataSize,
    const void* inData)
{
    (void)driver;
    (void)clientProcessID;
    DELEGATE_TO_PLUGIN(SetPropertyData, objectID, address,
                      qualifierDataSize, qualifierData,
                      inDataSize, inData);
}

OSStatus AES67_PlugIn_StartIO(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID)
{
    (void)driver;
    DELEGATE_TO_PLUGIN(StartIO, deviceObjectID, clientID);
}

OSStatus AES67_PlugIn_StopIO(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID)
{
    (void)driver;
    DELEGATE_TO_PLUGIN(StopIO, deviceObjectID, clientID);
}

OSStatus AES67_PlugIn_GetZeroTimeStamp(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID,
    Float64* outSampleTime,
    UInt64* outHostTime,
    UInt64* outSeed)
{
    (void)driver;
    DELEGATE_TO_PLUGIN(GetZeroTimeStamp, deviceObjectID, clientID,
                      outSampleTime, outHostTime, outSeed);
}

OSStatus AES67_PlugIn_WillDoIOOperation(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID,
    UInt32 operationID,
    Boolean* outWillDo,
    Boolean* outWillDoInPlace)
{
    (void)driver;
    DELEGATE_TO_PLUGIN(WillDoIOOperation, deviceObjectID, clientID,
                      operationID, outWillDo, outWillDoInPlace);
}

OSStatus AES67_PlugIn_BeginIOOperation(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID,
    UInt32 operationID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo)
{
    (void)driver;
    DELEGATE_TO_PLUGIN(BeginIOOperation, deviceObjectID, clientID,
                      operationID, ioBufferFrameSize, ioCycleInfo);
}

OSStatus AES67_PlugIn_DoIOOperation(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    AudioObjectID streamObjectID,
    UInt32 clientID,
    UInt32 operationID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo,
    void* ioMainBuffer,
    void* ioSecondaryBuffer)
{
    (void)driver;
    DELEGATE_TO_PLUGIN(DoIOOperation, deviceObjectID, streamObjectID,
                      clientID, operationID, ioBufferFrameSize,
                      ioCycleInfo, ioMainBuffer, ioSecondaryBuffer);
}

OSStatus AES67_PlugIn_EndIOOperation(
    AudioServerPlugInDriverRef driver,
    AudioObjectID deviceObjectID,
    UInt32 clientID,
    UInt32 operationID,
    UInt32 ioBufferFrameSize,
    const AudioServerPlugInIOCycleInfo* ioCycleInfo)
{
    (void)driver;
    DELEGATE_TO_PLUGIN(EndIOOperation, deviceObjectID, clientID,
                      operationID, ioBufferFrameSize, ioCycleInfo);
}

} // extern "C"
