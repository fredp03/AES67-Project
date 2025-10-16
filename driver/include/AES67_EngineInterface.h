// AES67_EngineInterface.h - Interface between HAL driver and network engine
// SPDX-License-Identifier: MIT

#pragma once

#include "AES67_Types.h"
#include "AES67_RingBuffer.h"
#include <memory>
#include <functional>

namespace AES67 {

/// Callback from engine to notify driver of events
struct EngineCallbacks {
    std::function<void(bool locked, double offsetNs)> onPTPStatusChanged;
    std::function<void(uint32_t streamIdx, bool underrun)> onXrunDetected;
    std::function<void(const char* message)> onError;
};

/// Engine interface (implemented by network engine, called by driver)
class INetworkEngine {
public:
    virtual ~INetworkEngine() = default;
    
    /// Start engine (begin PTP sync, open sockets, start threads)
    virtual bool Start() = 0;
    
    /// Stop engine (graceful shutdown)
    virtual void Stop() = 0;
    
    /// Get current PTP time in nanoseconds (0 if not locked)
    virtual uint64_t GetPTPTimeNs() const = 0;
    
    /// Convert host time to PTP time using affine mapping
    /// hostTime: mach_absolute_time()
    /// Returns: PTP nanoseconds
    virtual uint64_t HostTimeToPTP(uint64_t hostTime) const = 0;
    
    /// Convert PTP time to host time
    virtual uint64_t PTPToHostTime(uint64_t ptpTimeNs) const = 0;
    
    /// Check if PTP is locked
    virtual bool IsPTPLocked() const = 0;
    
    /// Get PTP offset from master (nanoseconds)
    virtual double GetPTPOffset() const = 0;
    
    /// Get rate scalar (1.0 = nominal, >1.0 = fast, <1.0 = slow)
    virtual double GetRateScalar() const = 0;
    
    /// Set callbacks
    virtual void SetCallbacks(const EngineCallbacks& callbacks) = 0;
    
    /// Get ring buffer for input stream (network → driver)
    virtual AudioRingBuffer* GetInputRingBuffer(uint32_t streamIdx) = 0;
    
    /// Get ring buffer for output stream (driver → network)
    virtual AudioRingBuffer* GetOutputRingBuffer(uint32_t streamIdx) = 0;
    
    /// Notify engine of I/O cycle (for timestamp alignment)
    /// Called from driver's I/O thread at start of each cycle
    virtual void NotifyIOCycle(uint64_t hostTime, uint64_t sampleTime) = 0;
};

/// Factory function (implemented in engine library)
/// Will be called by driver to instantiate engine
extern "C" {
    INetworkEngine* CreateNetworkEngine(const char* configPath);
    void DestroyNetworkEngine(INetworkEngine* engine);
}

/// RAII wrapper for engine lifetime
class NetworkEnginePtr {
public:
    explicit NetworkEnginePtr(const char* configPath = nullptr)
        : engine_(CreateNetworkEngine(configPath))
    {}
    
    ~NetworkEnginePtr() {
        if (engine_) {
            DestroyNetworkEngine(engine_);
        }
    }
    
    // Disable copy
    NetworkEnginePtr(const NetworkEnginePtr&) = delete;
    NetworkEnginePtr& operator=(const NetworkEnginePtr&) = delete;
    
    // Allow move
    NetworkEnginePtr(NetworkEnginePtr&& other) noexcept
        : engine_(other.engine_)
    {
        other.engine_ = nullptr;
    }
    
    NetworkEnginePtr& operator=(NetworkEnginePtr&& other) noexcept {
        if (this != &other) {
            if (engine_) {
                DestroyNetworkEngine(engine_);
            }
            engine_ = other.engine_;
            other.engine_ = nullptr;
        }
        return *this;
    }
    
    INetworkEngine* Get() const { return engine_; }
    INetworkEngine* operator->() const { return engine_; }
    explicit operator bool() const { return engine_ != nullptr; }
    
private:
    INetworkEngine* engine_;
};

} // namespace AES67
