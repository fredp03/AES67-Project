// NetworkEngine.h - Main network engine interface
// SPDX-License-Identifier: MIT

#pragma once

#include "AES67_EngineInterface.h"
#include "RTPPacketizer.h"
#include "PTPClient.h"
#include "JitterBuffer.h"
#include "SAPAnnouncer.h"
#include <memory>
#include <thread>
#include <atomic>

namespace AES67 {

class NetworkEngine : public INetworkEngine {
public:
    explicit NetworkEngine(const char* configPath);
    ~NetworkEngine() override;
    
    // INetworkEngine interface
    bool Start() override;
    void Stop() override;
    uint64_t GetPTPTimeNs() const override;
    uint64_t HostTimeToPTP(uint64_t hostTime) const override;
    uint64_t PTPToHostTime(uint64_t ptpTimeNs) const override;
    bool IsPTPLocked() const override;
    double GetPTPOffset() const override;
    double GetRateScalar() const override;
    void SetCallbacks(const EngineCallbacks& callbacks) override;
    AudioRingBuffer* GetInputRingBuffer(uint32_t streamIdx) override;
    AudioRingBuffer* GetOutputRingBuffer(uint32_t streamIdx) override;
    void NotifyIOCycle(uint64_t hostTime, uint64_t sampleTime) override;
    
private:
    void RTPReceiveThread(uint32_t streamIdx);
    void RTPTransmitThread(uint32_t streamIdx);
    void JitterBufferPlayoutThread(uint32_t streamIdx);
    void PTPThread();
    
    EngineCallbacks callbacks_;
    std::unique_ptr<PTPClient> ptpClient_;
    std::unique_ptr<SAPAnnouncer> sapAnnouncer_;
    
    // Per-stream components (8 streams each direction)
    std::array<std::unique_ptr<RTPPacketizer>, 8> txPacketizers_;
    std::array<std::unique_ptr<RTPDepacketizer>, 8> rxDepacketizers_;
    std::array<std::unique_ptr<JitterBuffer>, 8> rxJitterBuffers_;
    std::array<std::unique_ptr<AudioRingBuffer>, 8> inputRings_;
    std::array<std::unique_ptr<AudioRingBuffer>, 8> outputRings_;
    
    // Worker threads
    std::array<std::thread, 8> rxThreads_;
    std::array<std::thread, 8> txThreads_;
    std::array<std::thread, 8> playoutThreads_;
    std::thread ptpThread_;
    
    std::atomic<bool> running_{false};
    
    // Configuration
    struct Config {
        uint32_t packetTimeUs = 250;
        uint32_t jitterBufferPackets = 3;
        uint8_t ptpDomain = 0;
        bool multicast = true;
        std::string interface = "en0";
    } config_;
};

} // namespace AES67
