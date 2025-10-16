// SAPAnnouncer.h - SAP/SDP announcements
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

namespace AES67 {

struct StreamDescription {
    uint32_t streamIndex;
    std::string name;
    std::string multicastAddr;
    uint16_t port;
    uint8_t channels;
    uint32_t sampleRate;
    uint32_t packetTimeUs;
};

class SAPAnnouncer {
public:
    SAPAnnouncer();
    ~SAPAnnouncer();
    
    bool Start(const std::vector<StreamDescription>& streams);
    void Stop();
    
    void SetInterval(uint32_t seconds) { intervalSeconds_ = seconds; }
    
private:
    void AnnouncementThread();
    std::string GenerateSDP(const StreamDescription& stream);
    
    int socket_ = -1;
    std::atomic<bool> running_{false};
    std::thread thread_;
    
    std::vector<StreamDescription> streams_;
    uint32_t intervalSeconds_ = 30;
};

} // namespace AES67
