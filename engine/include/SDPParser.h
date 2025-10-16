// SDPParser.h - SDP session description parser
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>
#include <map>

namespace AES67 {

struct SDPSession {
    std::string origin;
    std::string sessionName;
    std::string connectionAddr;
    uint16_t port;
    uint8_t payloadType;
    std::string rtpmap;
    uint32_t sampleRate;
    uint8_t channels;
    uint32_t packetTimeUs;
    
    // AES67-specific
    std::string ptpRefClock;
    std::string mediaClk;
};

class SDPParser {
public:
    static SDPSession Parse(const std::string& sdp);
    static std::string Generate(const SDPSession& session);
    
private:
    static std::map<std::string, std::string> ParseAttributes(const std::string& sdp);
};

} // namespace AES67
