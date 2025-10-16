// NetworkEngine.cpp - Main engine implementation
// SPDX-License-Identifier: MIT

#include "NetworkEngine.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace AES67 {

NetworkEngine::NetworkEngine(const char* configPath) {
    (void)configPath; // TODO: Load from JSON file
    
    // Create PTP client
    ptpClient_ = std::make_unique<PTPClient>(config_.ptpDomain);
    
    // Create SAP announcer
    sapAnnouncer_ = std::make_unique<SAPAnnouncer>();
    
    // Create ring buffers (large enough for low latency)
    const size_t ringSize = 48000; // 1 second @ 48kHz
    for (uint32_t i = 0; i < 8; ++i) {
        inputRings_[i] = std::make_unique<AudioRingBuffer>(ringSize * 8);
        outputRings_[i] = std::make_unique<AudioRingBuffer>(ringSize * 8);
    }
    
    // Create RTP packetizers for TX
    for (uint32_t i = 0; i < 8; ++i) {
        const uint32_t ssrc = 0x12345678 + i;
        txPacketizers_[i] = std::make_unique<RTPPacketizer>(ssrc, 8, 48000);
    }
    
    // Create RTP depacketizers for RX
    for (uint32_t i = 0; i < 8; ++i) {
        rxDepacketizers_[i] = std::make_unique<RTPDepacketizer>(8, 48000);
    }
    
    // Create jitter buffers for RX
    for (uint32_t i = 0; i < 8; ++i) {
        rxJitterBuffers_[i] = std::make_unique<JitterBuffer>(
            config_.jitterBufferPackets, 
            config_.jitterBufferPackets * 2,
            48000);
    }
}

NetworkEngine::~NetworkEngine() {
    Stop();
}

bool NetworkEngine::Start() {
    if (running_) return true;
    
    // Start PTP client
    if (!ptpClient_->Start(config_.interface.c_str())) {
        return false;
    }
    
    running_ = true;
    
    // Start SAP discovery listener
    sapDiscoveryThread_ = std::thread(&NetworkEngine::SAPDiscoveryThread, this);
    
    // Start RTP threads (one per stream)
    for (uint32_t i = 0; i < 8; ++i) {
        rxThreads_[i] = std::thread(&NetworkEngine::RTPReceiveThread, this, i);
        txThreads_[i] = std::thread(&NetworkEngine::RTPTransmitThread, this, i);
        playoutThreads_[i] = std::thread(&NetworkEngine::JitterBufferPlayoutThread, this, i);
    }
    
    // Start SAP announcements
    std::vector<StreamDescription> streams;
    for (uint32_t i = 0; i < 8; ++i) {
        StreamDescription desc;
        desc.streamIndex = i;
        desc.name = "AES67 VSC - Stream " + std::to_string(i + 1);
        desc.multicastAddr = "239.69.1." + std::to_string(i + 1);
        desc.port = 5004;
        desc.channels = 8;
        desc.sampleRate = 48000;
        desc.packetTimeUs = config_.packetTimeUs;
        streams.push_back(desc);
    }
    
    sapAnnouncer_->Start(streams);
    
    return true;
}

void NetworkEngine::Stop() {
    if (!running_) return;
    
    running_ = false;
    
    // Stop PTP
    ptpClient_->Stop();
    
    // Stop SAP
    sapAnnouncer_->Stop();
    
    // Join all threads
    if (sapDiscoveryThread_.joinable()) {
        sapDiscoveryThread_.join();
    }
    
    for (auto& thread : rxThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    for (auto& thread : txThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    for (auto& thread : playoutThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

uint64_t NetworkEngine::GetPTPTimeNs() const {
    return ptpClient_->GetPTPTimeNs();
}

uint64_t NetworkEngine::HostTimeToPTP(uint64_t hostTime) const {
    return ptpClient_->HostTimeToPTP(hostTime);
}

uint64_t NetworkEngine::PTPToHostTime(uint64_t ptpTimeNs) const {
    return ptpClient_->PTPToHostTime(ptpTimeNs);
}

bool NetworkEngine::IsPTPLocked() const {
    return ptpClient_->IsLocked();
}

double NetworkEngine::GetPTPOffset() const {
    return ptpClient_->GetOffsetNs();
}

double NetworkEngine::GetRateScalar() const {
    return ptpClient_->GetRateRatio();
}

void NetworkEngine::SetCallbacks(const EngineCallbacks& callbacks) {
    callbacks_ = callbacks;
    
    // Forward PTP status to driver
    ptpClient_->SetStatusCallback([this](bool locked, double offset) {
        if (callbacks_.onPTPStatusChanged) {
            callbacks_.onPTPStatusChanged(locked, offset);
        }
    });
}

AudioRingBuffer* NetworkEngine::GetInputRingBuffer(uint32_t streamIdx) {
    if (streamIdx >= 8) return nullptr;
    return inputRings_[streamIdx].get();
}

AudioRingBuffer* NetworkEngine::GetOutputRingBuffer(uint32_t streamIdx) {
    if (streamIdx >= 8) return nullptr;
    return outputRings_[streamIdx].get();
}

void NetworkEngine::NotifyIOCycle(uint64_t hostTime, uint64_t sampleTime) {
    (void)hostTime;
    (void)sampleTime;
    // TODO: Use for precise timestamp alignment
}

std::vector<std::string> NetworkEngine::GetDiscoveredStreamNames() const {
    std::lock_guard<std::mutex> lock(discoveryMutex_);
    std::vector<std::string> names;
    names.reserve(discoveredStreams_.size());
    
    for (const auto& pair : discoveredStreams_) {
        names.push_back(pair.first);
    }
    
    return names;
}

bool NetworkEngine::GetDiscoveredStream(const std::string& name, SDPSession& outSession) const {
    std::lock_guard<std::mutex> lock(discoveryMutex_);
    auto it = discoveredStreams_.find(name);
    
    if (it != discoveredStreams_.end()) {
        outSession = it->second;
        return true;
    }
    
    return false;
}

void NetworkEngine::RTPReceiveThread(uint32_t streamIdx) {
    // Create UDP socket
    const int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    
    // Set receive buffer size for low latency
    int recvBufSize = 256 * 1024; // 256KB
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize));
    
    // Set socket priority for real-time audio (SO_PRIORITY is Linux-specific)
    #ifdef __linux__
    int priority = 6; // Real-time priority
    setsockopt(sock, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));
    #endif
    
    // Set DSCP for QoS (EF = 46 for expedited forwarding)
    int dscp = 46 << 2; // DSCP is in top 6 bits of TOS byte
    setsockopt(sock, IPPROTO_IP, IP_TOS, &dscp, sizeof(dscp));
    
    // Bind to port 5004 (TODO: unique port per stream)
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(5004);
    
    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock);
        return;
    }
    
    // Join multicast group (239.69.2.x for RX)
    ip_mreq mreq{};
    const std::string mcastAddr = "239.69.2." + std::to_string(streamIdx + 1);
    inet_pton(AF_INET, mcastAddr.c_str(), &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = INADDR_ANY;
    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    
    uint8_t packetBuf[1500];
    
    while (running_) {
        const ssize_t bytes = recv(sock, packetBuf, sizeof(packetBuf), 0);
        if (bytes <= 0) continue;
        
        // Depacketize into temporary buffer
        int32_t sampleBuf[8 * 64]; // Max 64 frames @ 8 channels
        const uint32_t frames = rxDepacketizers_[streamIdx]->ParsePacket(
            packetBuf, bytes, sampleBuf);
        
        if (frames > 0) {
            // Get current PTP time and RTP timestamp
            const uint64_t arrivalTime = ptpClient_->GetPTPTimeNs();
            const uint32_t rtpTimestamp = rxDepacketizers_[streamIdx]->GetLastTimestamp();
            
            // Insert into jitter buffer (buffer takes ownership via copy)
            rxJitterBuffers_[streamIdx]->Insert(rtpTimestamp, arrivalTime, 
                                                 sampleBuf, frames);
        }
    }
    
    close(sock);
}

void NetworkEngine::JitterBufferPlayoutThread(uint32_t streamIdx) {
    // Buffer for playout
    int32_t playoutBuf[8 * 64]; // Max 64 frames @ 8 channels
    
    while (running_) {
        // Get current PTP time
        const uint64_t ptpTimeNs = ptpClient_->GetPTPTimeNs();
        
        // Try to get next packet for playout
        const auto* packet = rxJitterBuffers_[streamIdx]->GetNextPacket(ptpTimeNs);
        
        if (packet) {
            // Copy samples to playout buffer
            const size_t sampleCount = packet->frameCount * 8; // 8 channels per stream
            std::memcpy(playoutBuf, packet->samples, sampleCount * sizeof(int32_t));
            
            // Write to ring buffer for driver to consume
            inputRings_[streamIdx]->Write(playoutBuf, packet->frameCount);
            
            // Release packet back to jitter buffer
            rxJitterBuffers_[streamIdx]->ReleasePacket(packet);
        } else {
            // Underrun - write silence
            const uint32_t silenceFrames = 6; // 125µs @ 48kHz
            std::memset(playoutBuf, 0, silenceFrames * 8 * sizeof(int32_t));
            inputRings_[streamIdx]->Write(playoutBuf, silenceFrames);
        }
        
        // Sleep for packet time (match config_.packetTimeUs)
        usleep(config_.packetTimeUs);
    }
}

void NetworkEngine::RTPTransmitThread(uint32_t streamIdx) {
    // Create UDP socket
    const int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    
    // Set send buffer size for low latency
    int sendBufSize = 256 * 1024; // 256KB
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendBufSize, sizeof(sendBufSize));
    
    // Set socket priority for real-time audio (SO_PRIORITY is Linux-specific)
    #ifdef __linux__
    int priority = 6; // Real-time priority
    setsockopt(sock, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));
    #endif
    
    // Set DSCP for QoS (EF = 46 for expedited forwarding)
    int dscp = 46 << 2; // DSCP is in top 6 bits of TOS byte
    setsockopt(sock, IPPROTO_IP, IP_TOS, &dscp, sizeof(dscp));
    
    // Set multicast TTL
    int ttl = 32;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    
    // Destination address (239.69.1.x for TX)
    sockaddr_in destAddr{};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(5004);
    const std::string mcastAddr = "239.69.1." + std::to_string(streamIdx + 1);
    inet_pton(AF_INET, mcastAddr.c_str(), &destAddr.sin_addr);
    
    // Calculate frames per packet based on packet time
    const uint32_t framesPerPacket = (config_.packetTimeUs * 48000) / 1000000;
    int32_t sampleBuf[8 * 64]; // Max 64 frames @ 8 channels
    
    while (running_) {
        // Read from output ring
        const size_t framesRead = outputRings_[streamIdx]->Read(sampleBuf, framesPerPacket);
        
        if (framesRead > 0) {
            // Packetize
            const auto packet = txPacketizers_[streamIdx]->CreatePacket(
                sampleBuf, framesRead);
            
            if (!packet.empty()) {
                // Send
                sendto(sock, packet.data(), packet.size(), 0,
                       reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));
            }
        }
        
        // Sleep for packet time
        usleep(config_.packetTimeUs);
    }
    
    close(sock);
}

void NetworkEngine::SAPDiscoveryThread() {
    // Create UDP socket for SAP listening
    const int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    
    // Allow address reuse
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    #ifdef SO_REUSEPORT
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    #endif
    
    // Bind to SAP port 9875
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(9875);
    
    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock);
        return;
    }
    
    // Join SAP multicast group (239.255.255.255)
    ip_mreq mreq{};
    inet_pton(AF_INET, "239.255.255.255", &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = INADDR_ANY;
    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    
    uint8_t buffer[2048];
    
    while (running_) {
        // Set receive timeout so we can check running_ periodically
        timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        const ssize_t bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) continue;
        
        // Parse SAP header (minimum 8 bytes)
        if (bytes < 8) continue;
        
        const uint8_t version = (buffer[0] >> 5) & 0x7;
        const bool isAnnouncement = (buffer[0] & 0x04) == 0; // A bit (deletion if set)
        
        if (version != 1 || !isAnnouncement) continue;
        
        // Skip SAP header to get SDP payload
        // SAP header: V(3) A(1) R(1) T(1) E(1) C(1) + auth_len(1) + msg_id_hash(2) + origin(4) [+ auth_data]
        size_t sdpOffset = 8; // Minimum SAP header size
        
        const uint8_t authLen = buffer[1];
        sdpOffset += authLen * 4; // Auth data is in 32-bit words
        
        if (sdpOffset >= static_cast<size_t>(bytes)) continue;
        
        // Extract SDP
        const std::string sdp(reinterpret_cast<const char*>(buffer + sdpOffset), 
                             bytes - sdpOffset);
        
        try {
            // Parse SDP
            const SDPSession session = SDPParser::Parse(sdp);
            
            // Create unique stream name from origin
            const std::string streamName = session.sessionName.empty() 
                ? session.origin 
                : session.sessionName;
            
            // Notify about discovered stream
            OnStreamDiscovered(streamName, session);
            
        } catch (...) {
            // Ignore malformed SDP
        }
    }
    
    close(sock);
}

void NetworkEngine::OnStreamDiscovered(const std::string& streamName, const SDPSession& sdp) {
    std::lock_guard<std::mutex> lock(discoveryMutex_);
    
    // Check if this is a new stream or an update
    auto it = discoveredStreams_.find(streamName);
    const bool isNew = (it == discoveredStreams_.end());
    
    // Store/update stream info
    discoveredStreams_[streamName] = sdp;
    
    // Log discovery (in production, this could trigger UI updates or auto-subscription)
    if (isNew) {
        // New stream discovered
        // TODO: Notify callback or log
        // For now, streams are stored and can be queried via a future API
    }
}

} // namespace AES67

// C interface implementation
extern "C" {

AES67::INetworkEngine* CreateNetworkEngine(const char* configPath) {
    return new AES67::NetworkEngine(configPath);
}

void DestroyNetworkEngine(AES67::INetworkEngine* engine) {
    delete engine;
}

}
