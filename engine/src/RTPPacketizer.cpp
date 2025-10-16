// RTPPacketizer.cpp - RTP L24 packet creation and parsing
// SPDX-License-Identifier: MIT

#include "RTPPacketizer.h"
#include <cstring>
#include <arpa/inet.h>

namespace AES67 {

// ============================================================================
// RTPPacketizer Implementation
// ============================================================================

RTPPacketizer::RTPPacketizer(uint32_t ssrc, uint8_t channels, uint32_t sampleRate)
    : ssrc_(ssrc)
    , channels_(channels)
    , sampleRate_(sampleRate)
    , sequence_(0)
    , timestamp_(0)
{}

std::vector<uint8_t> RTPPacketizer::CreatePacket(const int32_t* samples, uint32_t frameCount) {
    if (!samples || frameCount == 0 || channels_ == 0) {
        return {};
    }
    
    // Calculate payload size: frames × channels × 3 bytes (L24)
    const size_t payloadSize = frameCount * channels_ * 3;
    const size_t packetSize = sizeof(RTPHeader) + payloadSize;
    
    std::vector<uint8_t> packet(packetSize);
    
    // Fill RTP header
    auto* header = reinterpret_cast<RTPHeader*>(packet.data());
    header->SetVersion(2);
    header->SetPadding(false);
    header->SetExtension(false);
    header->SetCSRCCount(0);
    header->SetMarker(false);
    header->SetPayloadType(kRTPPayloadType_L24);
    header->sequence = htons(sequence_);
    header->timestamp = htonl(timestamp_);
    header->ssrc = htonl(ssrc_);
    
    // Encode audio samples to L24 payload
    uint8_t* payload = packet.data() + sizeof(RTPHeader);
    
    for (uint32_t frame = 0; frame < frameCount; ++frame) {
        for (uint8_t ch = 0; ch < channels_; ++ch) {
            const int32_t sample = samples[frame * channels_ + ch];
            Int32ToL24(sample, payload);
            payload += 3;
        }
    }
    
    // Update state for next packet
    sequence_++;
    timestamp_ += frameCount;
    
    return packet;
}

// ============================================================================
// RTPDepacketizer Implementation
// ============================================================================

RTPDepacketizer::RTPDepacketizer(uint8_t channels, uint32_t sampleRate)
    : channels_(channels)
    , sampleRate_(sampleRate)
    , lastSequence_(0)
    , lastTimestamp_(0)
    , packetLoss_(0)
    , firstPacket_(true)
{}

uint32_t RTPDepacketizer::ParsePacket(const uint8_t* packet, size_t packetSize, int32_t* outSamples) {
    if (!packet || !outSamples || packetSize < sizeof(RTPHeader)) {
        return 0;
    }
    
    // Parse RTP header
    const auto* header = reinterpret_cast<const RTPHeader*>(packet);
    
    // Validate header
    if (header->GetVersion() != 2) {
        return 0; // Invalid RTP version
    }
    
    if (header->GetPayloadType() != kRTPPayloadType_L24) {
        return 0; // Wrong payload type
    }
    
    const uint16_t sequence = ntohs(header->sequence);
    const uint32_t timestamp = ntohl(header->timestamp);
    
    // Detect packet loss (sequence gap)
    if (!firstPacket_) {
        const int16_t seqDelta = static_cast<int16_t>(sequence - lastSequence_);
        if (seqDelta > 1) {
            packetLoss_ += (seqDelta - 1);
        } else if (seqDelta < 0) {
            // Out of order or duplicate (ignore)
            return 0;
        }
    }
    
    lastSequence_ = sequence;
    lastTimestamp_ = timestamp;
    firstPacket_ = false;
    
    // Calculate payload size and frame count
    const size_t headerSize = sizeof(RTPHeader) + (header->GetCSRCCount() * 4);
    if (packetSize <= headerSize) {
        return 0;
    }
    
    const size_t payloadSize = packetSize - headerSize;
    const size_t bytesPerFrame = channels_ * 3; // L24
    
    if (payloadSize % bytesPerFrame != 0) {
        return 0; // Invalid payload size
    }
    
    const uint32_t frameCount = static_cast<uint32_t>(payloadSize / bytesPerFrame);
    
    // Decode L24 payload to int32 samples
    const uint8_t* payload = packet + headerSize;
    
    for (uint32_t frame = 0; frame < frameCount; ++frame) {
        for (uint8_t ch = 0; ch < channels_; ++ch) {
            outSamples[frame * channels_ + ch] = L24ToInt32(payload);
            payload += 3;
        }
    }
    
    return frameCount;
}

} // namespace AES67
