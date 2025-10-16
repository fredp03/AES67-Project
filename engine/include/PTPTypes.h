// PTPTypes.h - PTP (IEEE 1588) structures
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace AES67 {

// PTP message types
enum class PTPMessageType : uint8_t {
    Sync = 0x0,
    Delay_Req = 0x1,
    Follow_Up = 0x8,
    Delay_Resp = 0x9,
    Announce = 0xB
};

// PTP clock identity (EUI-64)
struct ClockIdentity {
    uint8_t id[8];
} __attribute__((packed));

// PTP timestamp (seconds + nanoseconds)
struct PTPTimestamp {
    uint64_t seconds;    // 48-bit seconds (stored in 64-bit for convenience)
    uint32_t nanoseconds;
    
    uint64_t ToNanoseconds() const {
        return seconds * 1000000000ULL + nanoseconds;
    }
    
    static PTPTimestamp FromNanoseconds(uint64_t ns) {
        PTPTimestamp ts;
        ts.seconds = ns / 1000000000ULL;
        ts.nanoseconds = static_cast<uint32_t>(ns % 1000000000ULL);
        return ts;
    }
} __attribute__((packed));

// PTP header (34 bytes)
struct PTPHeader {
    uint8_t messageType;        // Message type and version
    uint8_t versionPTP;         // PTP version (2)
    uint16_t messageLength;
    uint8_t domainNumber;
    uint8_t reserved1;
    uint16_t flagField;
    int64_t correctionField;
    uint32_t reserved2;
    ClockIdentity sourcePortIdentity;
    uint16_t sourcePortId;
    uint16_t sequenceId;
    uint8_t controlField;
    int8_t logMessageInterval;
} __attribute__((packed));

static_assert(sizeof(PTPHeader) == 34, "PTP header must be 34 bytes");

// PTP Sync message body
struct PTPSyncMessage {
    PTPHeader header;
    PTPTimestamp originTimestamp;
} __attribute__((packed));

// PTP Announce message body
struct PTPAnnounceMessage {
    PTPHeader header;
    PTPTimestamp originTimestamp;
    int16_t currentUtcOffset;
    uint8_t reserved;
    uint8_t grandmasterPriority1;
    uint32_t grandmasterClockQuality;
    uint8_t grandmasterPriority2;
    ClockIdentity grandmasterIdentity;
    uint16_t stepsRemoved;
    uint8_t timeSource;
} __attribute__((packed));

// PTP domain (AES67 typically uses domain 0)
constexpr uint8_t kPTPDefaultDomain = 0;

// PTP multicast addresses
constexpr char kPTP_IPv4_MulticastAddr[] = "224.0.1.129";
constexpr uint16_t kPTP_Event_Port = 319;
constexpr uint16_t kPTP_General_Port = 320;

} // namespace AES67
