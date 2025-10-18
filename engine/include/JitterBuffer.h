// JitterBuffer.h - Adaptive playout buffer
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <deque>
#include <vector>

namespace AES67 {

struct JitterBufferPacket {
    uint32_t timestamp;     // RTP timestamp
    uint64_t arrivalTime;   // PTP nanoseconds
    uint32_t frameCount;
    std::vector<int32_t> samples;
};

class JitterBuffer {
public:
    JitterBuffer(uint32_t minPackets, uint32_t maxPackets, uint32_t sampleRate);
    ~JitterBuffer();
    
    // Insert packet (takes ownership of samples)
    void Insert(uint32_t timestamp, uint64_t arrivalTime, 
                const int32_t* samples, uint32_t frameCount);
    
    // Get next packet for playout at given PTP time
    // Returns null if buffer underrun
    const JitterBufferPacket* GetNextPacket(uint64_t ptpTimeNs);
    
    // Release packet after consumption
    void ReleasePacket(const JitterBufferPacket* packet);
    
    // Statistics
    uint32_t GetDepth() const { return static_cast<uint32_t>(queue_.size()); }
    uint32_t GetUnderrunCount() const { return underruns_; }
    uint32_t GetOverrunCount() const { return overruns_; }
    
    void Reset();
    
private:
    void AdjustDepth();
    
    uint32_t minPackets_;
    uint32_t maxPackets_;
    uint32_t targetPackets_;
    uint32_t sampleRate_;
    
    std::deque<JitterBufferPacket> queue_;
    
    uint32_t underruns_ = 0;
    uint32_t overruns_ = 0;
    uint64_t lastPlayoutTime_ = 0;
};

} // namespace AES67
