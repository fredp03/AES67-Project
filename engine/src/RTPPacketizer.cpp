// Stub implementations for remaining engine components
// SPDX-License-Identifier: MIT

#include "RTPPacketizer.h"
#include "PTPClient.h"
#include "JitterBuffer.h"
#include "SAPAnnouncer.h"
#include "SDPParser.h"

namespace AES67 {

// RTPPacketizer stubs
RTPPacketizer::RTPPacketizer(uint32_t ssrc, uint8_t channels, uint32_t sampleRate)
    : ssrc_(ssrc), channels_(channels), sampleRate_(sampleRate) {}

std::vector<uint8_t> RTPPacketizer::CreatePacket(const int32_t* samples, uint32_t frameCount) {
    (void)samples; (void)frameCount;
    return {}; // TODO
}

// RTPDepacketizer stubs
RTPDepacketizer::RTPDepacketizer(uint8_t channels, uint32_t sampleRate)
    : channels_(channels), sampleRate_(sampleRate) {}

uint32_t RTPDepacketizer::ParsePacket(const uint8_t* packet, size_t packetSize, int32_t* outSamples) {
    (void)packet; (void)packetSize; (void)outSamples;
    return 0; // TODO
}

// PTPClient stubs
PTPClient::PTPClient(uint8_t domain) : domain_(domain) {}
PTPClient::~PTPClient() { Stop(); }

bool PTPClient::Start(const char* interfaceName) {
    (void)interfaceName;
    return false; // TODO
}

void PTPClient::Stop() {}
uint64_t PTPClient::GetPTPTimeNs() const { return 0; }
uint64_t PTPClient::HostTimeToPTP(uint64_t hostTime) const { return hostTime; }
uint64_t PTPClient::PTPToHostTime(uint64_t ptpTimeNs) const { return ptpTimeNs; }

// JitterBuffer stubs
JitterBuffer::JitterBuffer(uint32_t minPackets, uint32_t maxPackets, uint32_t sampleRate)
    : minPackets_(minPackets), maxPackets_(maxPackets), targetPackets_((minPackets + maxPackets) / 2), sampleRate_(sampleRate) {}

JitterBuffer::~JitterBuffer() { Reset(); }

void JitterBuffer::Insert(uint32_t timestamp, uint64_t arrivalTime, const int32_t* samples, uint32_t frameCount) {
    (void)timestamp; (void)arrivalTime; (void)samples; (void)frameCount;
    // TODO
}

const JitterBufferPacket* JitterBuffer::GetNextPacket(uint64_t ptpTimeNs) {
    (void)ptpTimeNs;
    return nullptr; // TODO
}

void JitterBuffer::ReleasePacket(const JitterBufferPacket* packet) {
    (void)packet;
    // TODO
}

void JitterBuffer::Reset() {
    queue_.clear();
}

// SAPAnnouncer stubs
SAPAnnouncer::SAPAnnouncer() {}
SAPAnnouncer::~SAPAnnouncer() { Stop(); }

bool SAPAnnouncer::Start(const std::vector<StreamDescription>& streams) {
    streams_ = streams;
    return false; // TODO
}

void SAPAnnouncer::Stop() {}

// SDPParser stubs
SDPSession SDPParser::Parse(const std::string& sdp) {
    (void)sdp;
    return {}; // TODO
}

std::string SDPParser::Generate(const SDPSession& session) {
    (void)session;
    return ""; // TODO
}

} // namespace AES67
