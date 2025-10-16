// test_rtp_codec.cpp - Unit tests for RTP L24 codec
// SPDX-License-Identifier: MIT

#include "../../engine/include/RTPPacketizer.h"
#include <iostream>
#include <functional>
#include <string>

extern void RegisterTest(const std::string& name, std::function<bool()> test);

using namespace AES67;

// Test L24 encoding and decoding round-trip
bool test_rtp_l24_roundtrip() {
    RTPPacketizer packetizer(0x12345678, 2, 48000);
    RTPDepacketizer depacketizer(2, 48000);
    
    // Create test samples
    int32_t originalSamples[16]; // 8 frames × 2 channels
    for (int i = 0; i < 16; ++i) {
        originalSamples[i] = (i - 8) * 1000000; // Mix of positive and negative
    }
    
    // Encode
    auto packet = packetizer.CreatePacket(originalSamples, 8);
    if (packet.empty()) return false;
    
    // Decode
    int32_t decodedSamples[16];
    uint32_t frames = depacketizer.ParsePacket(packet.data(), packet.size(), decodedSamples);
    
    if (frames != 8) return false;
    
    // Verify (L24 has some precision loss in LSBs)
    for (int i = 0; i < 16; ++i) {
        // Mask off bottom 8 bits for comparison (L24 precision)
        int32_t orig = originalSamples[i] & 0xFFFFFF00;
        int32_t decoded = decodedSamples[i] & 0xFFFFFF00;
        if (orig != decoded) return false;
    }
    
    return true;
}

// Test RTP sequence number incrementing
bool test_rtp_sequence() {
    RTPPacketizer packetizer(0x12345678, 2, 48000);
    
    int32_t samples[8];
    std::memset(samples, 0, sizeof(samples));
    
    // Create 3 packets
    auto packet1 = packetizer.CreatePacket(samples, 4);
    auto packet2 = packetizer.CreatePacket(samples, 4);
    auto packet3 = packetizer.CreatePacket(samples, 4);
    
    // Extract sequence numbers (bytes 2-3, big endian)
    uint16_t seq1 = (packet1[2] << 8) | packet1[3];
    uint16_t seq2 = (packet2[2] << 8) | packet2[3];
    uint16_t seq3 = (packet3[2] << 8) | packet3[3];
    
    // Should increment by 1 each time
    return (seq2 == seq1 + 1) && (seq3 == seq2 + 1);
}

// Test RTP timestamp incrementing
bool test_rtp_timestamp() {
    RTPPacketizer packetizer(0x12345678, 2, 48000);
    
    int32_t samples[8];
    std::memset(samples, 0, sizeof(samples));
    
    // Create 2 packets with 10 frames each
    auto packet1 = packetizer.CreatePacket(samples, 4);
    auto packet2 = packetizer.CreatePacket(samples, 4);
    
    // Extract timestamps (bytes 4-7, big endian)
    uint32_t ts1 = (packet1[4] << 24) | (packet1[5] << 16) | (packet1[6] << 8) | packet1[7];
    uint32_t ts2 = (packet2[4] << 24) | (packet2[5] << 16) | (packet2[6] << 8) | packet2[7];
    
    // Timestamp should increment by frame count (4 frames)
    return ts2 == ts1 + 4;
}

// Test SSRC is set correctly
bool test_rtp_ssrc() {
    const uint32_t testSSRC = 0xABCDEF12;
    RTPPacketizer packetizer(testSSRC, 2, 48000);
    
    int32_t samples[8];
    std::memset(samples, 0, sizeof(samples));
    
    auto packet = packetizer.CreatePacket(samples, 4);
    
    // Extract SSRC (bytes 8-11, big endian)
    uint32_t ssrc = (packet[8] << 24) | (packet[9] << 16) | (packet[10] << 8) | packet[11];
    
    return ssrc == testSSRC;
}

// Test packet loss detection
bool test_rtp_packet_loss() {
    RTPDepacketizer depacketizer(2, 48000);
    
    int32_t samples[8];
    RTPPacketizer packetizer(0x12345678, 2, 48000);
    
    // Create packet 1
    auto packet1 = packetizer.CreatePacket(samples, 4);
    depacketizer.ParsePacket(packet1.data(), packet1.size(), samples);
    
    uint32_t lossCount = depacketizer.GetPacketLossCount();
    if (lossCount != 0) return false; // No loss yet
    
    // Create packet 2
    auto packet2 = packetizer.CreatePacket(samples, 4);
    depacketizer.ParsePacket(packet2.data(), packet2.size(), samples);
    
    lossCount = depacketizer.GetPacketLossCount();
    if (lossCount != 0) return false; // Still no loss
    
    // Create packet 3 but skip it (simulate loss)
    auto packet3 = packetizer.CreatePacket(samples, 4);
    
    // Create and receive packet 4 (gap detected)
    auto packet4 = packetizer.CreatePacket(samples, 4);
    depacketizer.ParsePacket(packet4.data(), packet4.size(), samples);
    
    lossCount = depacketizer.GetPacketLossCount();
    return lossCount == 1; // Should detect 1 lost packet
}

// Test payload type is L24
bool test_rtp_payload_type() {
    RTPPacketizer packetizer(0x12345678, 2, 48000);
    
    int32_t samples[8];
    std::memset(samples, 0, sizeof(samples));
    
    auto packet = packetizer.CreatePacket(samples, 4);
    
    // Extract payload type (byte 1, lower 7 bits)
    uint8_t payloadType = packet[1] & 0x7F;
    
    // L24 uses dynamic payload type 96
    return payloadType == 96;
}

// Test zero samples encode/decode
bool test_rtp_silence() {
    RTPPacketizer packetizer(0x12345678, 2, 48000);
    RTPDepacketizer depacketizer(2, 48000);
    
    int32_t silence[16];
    std::memset(silence, 0, sizeof(silence));
    
    auto packet = packetizer.CreatePacket(silence, 8);
    
    int32_t decoded[16];
    uint32_t frames = depacketizer.ParsePacket(packet.data(), packet.size(), decoded);
    
    if (frames != 8) return false;
    
    // All samples should be zero (or very close due to encoding)
    for (int i = 0; i < 16; ++i) {
        if ((decoded[i] & 0xFFFFFF00) != 0) return false;
    }
    
    return true;
}

// Register all RTP codec tests
static struct RTPCodecTestRegistrar {
    RTPCodecTestRegistrar() {
        RegisterTest("RTP L24: Encode/decode round-trip", test_rtp_l24_roundtrip);
        RegisterTest("RTP: Sequence number incrementing", test_rtp_sequence);
        RegisterTest("RTP: Timestamp incrementing", test_rtp_timestamp);
        RegisterTest("RTP: SSRC field", test_rtp_ssrc);
        RegisterTest("RTP: Packet loss detection", test_rtp_packet_loss);
        RegisterTest("RTP: Payload type L24", test_rtp_payload_type);
        RegisterTest("RTP: Silence encoding", test_rtp_silence);
    }
} rtpCodecTestRegistrar;
