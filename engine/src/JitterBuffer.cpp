// JitterBuffer.cpp - Adaptive jitter buffer implementation
// SPDX-License-Identifier: MIT

#include "JitterBuffer.h"
#include <algorithm>
#include <cstring>

namespace AES67 {

JitterBuffer::JitterBuffer(uint32_t minPackets, uint32_t maxPackets, uint32_t sampleRate)
    : minPackets_(minPackets)
    , maxPackets_(maxPackets)
    , targetPackets_((minPackets + maxPackets) / 2)
    , sampleRate_(sampleRate)
    , underruns_(0)
    , overruns_(0)
    , lastPlayoutTime_(0)
{}

JitterBuffer::~JitterBuffer() {
    Reset();
}

void JitterBuffer::Insert(uint32_t timestamp, uint64_t arrivalTime, 
                          const int32_t* samples, uint32_t frameCount) {
    // Check for overrun
    if (queue_.size() >= maxPackets_) {
        overruns_++;
        return; // Drop packet
    }
    
    // Allocate and copy samples
    int32_t* sampleCopy = new int32_t[frameCount * 8]; // Assume 8 channels max
    std::memcpy(sampleCopy, samples, frameCount * 8 * sizeof(int32_t));
    
    // Create packet entry
    JitterBufferPacket packet;
    packet.timestamp = timestamp;
    packet.arrivalTime = arrivalTime;
    packet.frameCount = frameCount;
    packet.samples = sampleCopy;
    
    // Insert in timestamp order
    auto it = queue_.begin();
    while (it != queue_.end() && it->timestamp < timestamp) {
        ++it;
    }
    
    queue_.insert(it, packet);
    
    // Adjust buffer depth if needed
    AdjustDepth();
}

const JitterBufferPacket* JitterBuffer::GetNextPacket(uint64_t ptpTimeNs) {
    if (queue_.empty()) {
        underruns_++;
        return nullptr;
    }
    
    // Check if first packet is ready for playout
    const auto& firstPacket = queue_.front();
    
    // Calculate expected playout time for this packet
    // playout_time = arrival_time + target_depth * packet_duration
    const uint64_t packetDurationNs = (firstPacket.frameCount * 1000000000ULL) / sampleRate_;
    const uint64_t targetDelay = targetPackets_ * packetDurationNs;
    const uint64_t playoutTime = firstPacket.arrivalTime + targetDelay;
    
    if (ptpTimeNs >= playoutTime) {
        lastPlayoutTime_ = ptpTimeNs;
        return &queue_.front();
    }
    
    return nullptr; // Not ready yet
}

void JitterBuffer::ReleasePacket(const JitterBufferPacket* packet) {
    if (!packet || queue_.empty()) {
        return;
    }
    
    // Should be the front packet
    if (&queue_.front() == packet) {
        delete[] packet->samples;
        queue_.pop_front();
    }
}

void JitterBuffer::Reset() {
    for (auto& packet : queue_) {
        delete[] packet.samples;
    }
    queue_.clear();
    underruns_ = 0;
    overruns_ = 0;
    lastPlayoutTime_ = 0;
}

void JitterBuffer::AdjustDepth() {
    // Simple adaptive algorithm: adjust target based on underrun/overrun ratio
    const uint32_t currentDepth = static_cast<uint32_t>(queue_.size());
    
    // If we're consistently full, increase target
    if (currentDepth >= maxPackets_ - 1) {
        if (targetPackets_ < maxPackets_) {
            targetPackets_++;
        }
    }
    
    // If we're getting underruns, increase target
    if (underruns_ > 0 && targetPackets_ < maxPackets_) {
        targetPackets_++;
        underruns_ = 0; // Reset counter
    }
    
    // If buffer is too deep and no underruns, decrease target
    if (currentDepth > targetPackets_ + 2 && underruns_ == 0) {
        if (targetPackets_ > minPackets_) {
            targetPackets_--;
        }
    }
}

} // namespace AES67
