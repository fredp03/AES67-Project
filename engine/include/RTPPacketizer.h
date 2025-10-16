// RTPPacketizer.h - RTP L24 packet creation and parsing
// SPDX-License-Identifier: MIT

#pragma once

#include "RTPTypes.h"
#include <cstdint>
#include <vector>

namespace AES67 {

class RTPPacketizer {
public:
    RTPPacketizer(uint32_t ssrc, uint8_t channels, uint32_t sampleRate);
    
    // Create RTP packet from audio samples
    // samples: interleaved int32_t (24-bit in 32-bit containers)
    // Returns packet ready to send
    std::vector<uint8_t> CreatePacket(const int32_t* samples, uint32_t frameCount);
    
    void SetSequenceNumber(uint16_t seq) { sequence_ = seq; }
    void SetTimestamp(uint32_t ts) { timestamp_ = ts; }
    
private:
    uint32_t ssrc_;
    uint8_t channels_;
    uint32_t sampleRate_;
    uint16_t sequence_ = 0;
    uint32_t timestamp_ = 0;
};

class RTPDepacketizer {
public:
    RTPDepacketizer(uint8_t channels, uint32_t sampleRate);
    
    // Parse RTP packet into audio samples
    // Returns number of frames decoded
    uint32_t ParsePacket(const uint8_t* packet, size_t packetSize, int32_t* outSamples);
    
    uint16_t GetLastSequence() const { return lastSequence_; }
    uint32_t GetLastTimestamp() const { return lastTimestamp_; }
    uint32_t GetPacketLossCount() const { return packetLoss_; }
    
private:
    uint8_t channels_;
    uint32_t sampleRate_;
    uint16_t lastSequence_ = 0;
    uint32_t lastTimestamp_ = 0;
    uint32_t packetLoss_ = 0;
    bool firstPacket_ = true;
};

} // namespace AES67
