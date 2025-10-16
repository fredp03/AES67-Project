// SDPParser.cpp - SDP session description parser
// SPDX-License-Identifier: MIT

#include "SDPParser.h"
#include <sstream>
#include <regex>

namespace AES67 {

SDPSession SDPParser::Parse(const std::string& sdp) {
    SDPSession session;
    
    std::istringstream stream(sdp);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.size() < 2 || line[1] != '=') {
            continue;
        }
        
        const char type = line[0];
        const std::string value = line.substr(2);
        
        switch (type) {
            case 'o': // Origin
                session.origin = value;
                break;
                
            case 's': // Session name
                session.sessionName = value;
                break;
                
            case 'c': { // Connection
                // Format: IN IP4 239.69.1.1/32
                std::regex connRegex(R"(IN IP4 ([0-9.]+))");
                std::smatch match;
                if (std::regex_search(value, match, connRegex)) {
                    session.connectionAddr = match[1];
                }
                break;
            }
                
            case 'm': { // Media
                // Format: audio 5004 RTP/AVP 96
                std::regex mediaRegex(R"(audio (\d+) RTP/AVP (\d+))");
                std::smatch match;
                if (std::regex_search(value, match, mediaRegex)) {
                    session.port = static_cast<uint16_t>(std::stoi(match[1]));
                    session.payloadType = static_cast<uint8_t>(std::stoi(match[2]));
                }
                break;
            }
                
            case 'a': { // Attribute
                if (value.find("rtpmap:") == 0) {
                    // Format: rtpmap:96 L24/48000/8
                    session.rtpmap = value.substr(7); // Skip "rtpmap:"
                    
                    std::regex rtpmapRegex(R"(L24/(\d+)/(\d+))");
                    std::smatch match;
                    if (std::regex_search(value, match, rtpmapRegex)) {
                        session.sampleRate = std::stoul(match[1]);
                        session.channels = static_cast<uint8_t>(std::stoul(match[2]));
                    }
                } else if (value.find("ptime:") == 0) {
                    // Format: ptime:0.250
                    const double ptimeSec = std::stod(value.substr(6));
                    session.packetTimeUs = static_cast<uint32_t>(ptimeSec * 1000000);
                } else if (value.find("ts-refclk:") == 0) {
                    session.ptpRefClock = value.substr(10);
                } else if (value.find("mediaclk:") == 0) {
                    session.mediaClk = value.substr(9);
                }
                break;
            }
        }
    }
    
    return session;
}

std::string SDPParser::Generate(const SDPSession& session) {
    std::ostringstream sdp;
    
    sdp << "v=0\r\n";
    sdp << "o=" << session.origin << "\r\n";
    sdp << "s=" << session.sessionName << "\r\n";
    sdp << "c=IN IP4 " << session.connectionAddr << "/32\r\n";
    sdp << "t=0 0\r\n";
    sdp << "a=recvonly\r\n";
    sdp << "m=audio " << session.port << " RTP/AVP " 
        << static_cast<int>(session.payloadType) << "\r\n";
    sdp << "a=rtpmap:" << static_cast<int>(session.payloadType) << " " 
        << session.rtpmap << "\r\n";
    
    const double ptimeSec = session.packetTimeUs / 1000000.0;
    sdp << "a=ptime:" << ptimeSec << "\r\n";
    
    if (!session.mediaClk.empty()) {
        sdp << "a=mediaclk:" << session.mediaClk << "\r\n";
    }
    
    if (!session.ptpRefClock.empty()) {
        sdp << "a=ts-refclk:" << session.ptpRefClock << "\r\n";
    }
    
    sdp << "a=sync-time:0\r\n";
    
    return sdp.str();
}

std::map<std::string, std::string> SDPParser::ParseAttributes(const std::string& sdp) {
    std::map<std::string, std::string> attributes;
    
    std::istringstream stream(sdp);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.size() >= 2 && line[0] == 'a' && line[1] == '=') {
            const std::string attr = line.substr(2);
            const size_t colonPos = attr.find(':');
            
            if (colonPos != std::string::npos) {
                const std::string key = attr.substr(0, colonPos);
                const std::string value = attr.substr(colonPos + 1);
                attributes[key] = value;
            } else {
                attributes[attr] = "";
            }
        }
    }
    
    return attributes;
}

} // namespace AES67
