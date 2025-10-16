// RTPTypes.h - RTP packet structures for AES67
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace AES67 {

// RTP header (12 bytes, RFC 3550)
struct RTPHeader {
    uint8_t vpxcc;      // V(2)|P(1)|X(1)|CC(4)
    uint8_t mpt;        // M(1)|PT(7)
    uint16_t sequence;  // Sequence number
    uint32_t timestamp; // RTP timestamp
    uint32_t ssrc;      // Synchronization source
    
    void SetVersion(uint8_t v) { vpxcc = (vpxcc & 0x3F) | (v << 6); }
    void SetPadding(bool p) { vpxcc = (vpxcc & 0xDF) | (p ? 0x20 : 0); }
    void SetExtension(bool x) { vpxcc = (vpxcc & 0xEF) | (x ? 0x10 : 0); }
    void SetCSRCCount(uint8_t cc) { vpxcc = (vpxcc & 0xF0) | (cc & 0x0F); }
    void SetMarker(bool m) { mpt = (mpt & 0x7F) | (m ? 0x80 : 0); }
    void SetPayloadType(uint8_t pt) { mpt = (mpt & 0x80) | (pt & 0x7F); }
    
    uint8_t GetVersion() const { return (vpxcc >> 6) & 0x03; }
    bool GetPadding() const { return (vpxcc & 0x20) != 0; }
    bool GetExtension() const { return (vpxcc & 0x10) != 0; }
    uint8_t GetCSRCCount() const { return vpxcc & 0x0F; }
    bool GetMarker() const { return (mpt & 0x80) != 0; }
    uint8_t GetPayloadType() const { return mpt & 0x7F; }
} __attribute__((packed));

static_assert(sizeof(RTPHeader) == 12, "RTP header must be 12 bytes");

// AES67 uses L24 payload (RFC 3190)
constexpr uint8_t kRTPPayloadType_L24 = 96; // Dynamic
constexpr uint32_t kRTPTimestampClockRate = 48000;

// L24 encoding: 3 bytes per sample, big-endian, sign-extended to 32-bit
inline int32_t L24ToInt32(const uint8_t* l24) {
    int32_t val = (static_cast<int32_t>(l24[0]) << 16) |
                  (static_cast<int32_t>(l24[1]) << 8) |
                  (static_cast<int32_t>(l24[2]));
    // Sign extend from 24 to 32 bits
    if (val & 0x00800000) {
        val |= 0xFF000000;
    }
    return val << 8; // Shift to 32-bit container
}

inline void Int32ToL24(int32_t val, uint8_t* l24) {
    // Shift from 32-bit container to 24-bit
    val >>= 8;
    l24[0] = static_cast<uint8_t>((val >> 16) & 0xFF);
    l24[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    l24[2] = static_cast<uint8_t>(val & 0xFF);
}

} // namespace AES67
