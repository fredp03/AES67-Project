// SAPAnnouncer.cpp - SAP/SDP announcement implementation
// SPDX-License-Identifier: MIT

#include "SAPAnnouncer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace AES67 {

// SAP header structure
struct SAPHeader {
    uint8_t vat_flags;  // V(3)|A(1)|R(1)|T(1)|E(1)|C(1)
    uint8_t auth_len;   // Authentication length
    uint16_t msg_id;    // Message identifier hash
    
    void SetVersion(uint8_t v) { vat_flags = (vat_flags & 0x1F) | (v << 5); }
    void SetAnnounce(bool a) { vat_flags = (vat_flags & 0xEF) | (a ? 0 : 0x10); }
} __attribute__((packed));

SAPAnnouncer::SAPAnnouncer()
    : socket_(-1)
    , running_(false)
    , intervalSeconds_(30)
{}

SAPAnnouncer::~SAPAnnouncer() {
    Stop();
}

bool SAPAnnouncer::Start(const std::vector<StreamDescription>& streams) {
    if (running_) {
        return true;
    }
    
    streams_ = streams;
    
    // Create UDP socket
    socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ < 0) {
        return false;
    }
    
    // Set socket options for multicast
    int ttl = 32;
    setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    
    int loop = 0; // Don't loop back to sender
    setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    
    running_ = true;
    thread_ = std::thread(&SAPAnnouncer::AnnouncementThread, this);
    
    return true;
}

void SAPAnnouncer::Stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (thread_.joinable()) {
        thread_.join();
    }
    
    if (socket_ >= 0) {
        close(socket_);
        socket_ = -1;
    }
}

void SAPAnnouncer::AnnouncementThread() {
    sockaddr_in sapAddr{};
    sapAddr.sin_family = AF_INET;
    sapAddr.sin_port = htons(9875); // SAP port
    inet_pton(AF_INET, "239.255.255.255", &sapAddr.sin_addr);
    
    while (running_) {
        for (const auto& stream : streams_) {
            // Generate SDP
            const std::string sdp = GenerateSDP(stream);
            
            // Build SAP packet
            std::vector<uint8_t> packet;
            
            // SAP header
            SAPHeader header{};
            header.SetVersion(1);
            header.SetAnnounce(true); // Announcement (not deletion)
            header.auth_len = 0;
            header.msg_id = htons(stream.streamIndex);
            
            packet.resize(sizeof(SAPHeader) + 4 + sdp.size()); // +4 for originating source
            
            std::memcpy(packet.data(), &header, sizeof(SAPHeader));
            
            // Originating source (IPv4 address) - use 0.0.0.0 for now
            uint32_t origin = 0;
            std::memcpy(packet.data() + sizeof(SAPHeader), &origin, 4);
            
            // SDP payload
            std::memcpy(packet.data() + sizeof(SAPHeader) + 4, sdp.data(), sdp.size());
            
            // Send
            sendto(socket_, packet.data(), packet.size(), 0,
                   reinterpret_cast<sockaddr*>(&sapAddr), sizeof(sapAddr));
        }
        
        // Sleep until next announcement interval
        for (uint32_t i = 0; i < intervalSeconds_ && running_; ++i) {
            sleep(1);
        }
    }
}

std::string SAPAnnouncer::GenerateSDP(const StreamDescription& stream) {
    std::ostringstream sdp;
    
    // Session-level attributes
    sdp << "v=0\r\n";
    sdp << "o=aes67-vsc " << (3928736891ULL + stream.streamIndex) << " " 
        << (3928736891ULL + stream.streamIndex) << " IN IP4 192.168.1.10\r\n";
    sdp << "s=" << stream.name << "\r\n";
    sdp << "i=" << static_cast<int>(stream.channels) << "-channel L24 audio stream\r\n";
    sdp << "c=IN IP4 " << stream.multicastAddr << "/32\r\n";
    sdp << "t=0 0\r\n";
    sdp << "a=recvonly\r\n";
    
    // Media-level attributes
    sdp << "m=audio " << stream.port << " RTP/AVP 96\r\n";
    sdp << "a=rtpmap:96 L24/" << stream.sampleRate << "/" 
        << static_cast<int>(stream.channels) << "\r\n";
    
    // Packet time (in seconds)
    const double ptimeSec = stream.packetTimeUs / 1000000.0;
    sdp << "a=ptime:" << std::fixed << std::setprecision(3) << ptimeSec << "\r\n";
    
    // AES67-specific attributes
    sdp << "a=mediaclk:direct=0\r\n";
    sdp << "a=ts-refclk:ptp=IEEE1588-2008:00-1B-21-AB-CD-EF:0\r\n";
    sdp << "a=sync-time:0\r\n";
    
    return sdp.str();
}

} // namespace AES67
