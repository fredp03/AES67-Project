// NetworkEngine.cpp - Main engine implementation (stub)
// SPDX-License-Identifier: MIT

#include "NetworkEngine.h"

namespace AES67 {

NetworkEngine::NetworkEngine(const char* configPath) {
    (void)configPath; // TODO: Load config
    
    // Create PTP client
    ptpClient_ = std::make_unique<PTPClient>(config_.ptpDomain);
    
    // Create SAP announcer
    sapAnnouncer_ = std::make_unique<SAPAnnouncer>();
    
    // Create ring buffers (large enough for low latency)
    const size_t ringSize = 48000; // 1 second @ 48kHz
    for (uint32_t i = 0; i < 8; ++i) {
        inputRings_[i] = std::make_unique<AudioRingBuffer>(ringSize * 8); // 8 channels
        outputRings_[i] = std::make_unique<AudioRingBuffer>(ringSize * 8);
    }
    
    // Create packetizers, depacketizers, jitter buffers (TODO)
}

NetworkEngine::~NetworkEngine() {
    Stop();
}

bool NetworkEngine::Start() {
    if (running_) return true;
    
    // Start PTP client
    if (!ptpClient_->Start(config_.interface.c_str())) {
        return false;
    }
    
    running_ = true;
    
    // TODO: Start RTP threads, SAP announcer
    
    return true;
}

void NetworkEngine::Stop() {
    if (!running_) return;
    
    running_ = false;
    
    // Stop PTP
    ptpClient_->Stop();
    
    // TODO: Stop all threads
}

uint64_t NetworkEngine::GetPTPTimeNs() const {
    return ptpClient_->GetPTPTimeNs();
}

uint64_t NetworkEngine::HostTimeToPTP(uint64_t hostTime) const {
    return ptpClient_->HostTimeToPTP(hostTime);
}

uint64_t NetworkEngine::PTPToHostTime(uint64_t ptpTimeNs) const {
    return ptpClient_->PTPToHostTime(ptpTimeNs);
}

bool NetworkEngine::IsPTPLocked() const {
    return ptpClient_->IsLocked();
}

double NetworkEngine::GetPTPOffset() const {
    return ptpClient_->GetOffsetNs();
}

double NetworkEngine::GetRateScalar() const {
    return ptpClient_->GetRateRatio();
}

void NetworkEngine::SetCallbacks(const EngineCallbacks& callbacks) {
    callbacks_ = callbacks;
    
    // Forward PTP status to driver
    ptpClient_->SetStatusCallback([this](bool locked, double offset) {
        if (callbacks_.onPTPStatusChanged) {
            callbacks_.onPTPStatusChanged(locked, offset);
        }
    });
}

AudioRingBuffer* NetworkEngine::GetInputRingBuffer(uint32_t streamIdx) {
    if (streamIdx >= 8) return nullptr;
    return inputRings_[streamIdx].get();
}

AudioRingBuffer* NetworkEngine::GetOutputRingBuffer(uint32_t streamIdx) {
    if (streamIdx >= 8) return nullptr;
    return outputRings_[streamIdx].get();
}

void NetworkEngine::NotifyIOCycle(uint64_t hostTime, uint64_t sampleTime) {
    (void)hostTime;
    (void)sampleTime;
    // TODO: Use for timestamp alignment
}

} // namespace AES67

// C interface implementation
extern "C" {

AES67::INetworkEngine* CreateNetworkEngine(const char* configPath) {
    return new AES67::NetworkEngine(configPath);
}

void DestroyNetworkEngine(AES67::INetworkEngine* engine) {
    delete engine;
}

}
