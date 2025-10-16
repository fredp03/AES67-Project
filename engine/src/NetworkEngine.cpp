// NetworkEngine.cpp - Main engine implementation
// SPDX-License-Identifier: MIT

#include "NetworkEngine.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    
    // Start RTP threads (one per stream)
    for (uint32_t i = 0; i < 8; ++i) {
        rxThreads_[i] = std::thread(&NetworkEngine::RTPReceiveThread, this, i);
        txThreads_[i] = std::thread(&NetworkEngine::RTPTransmitThread, this, i);
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

void NetworkEngine::RTPReceiveThread(uint32_t streamIdx) {
    // Create UDP socket
    const int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    
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
    int32_t sampleBuf[8 * 64]; // Max 64 frames @ 8 channels
    
    while (running_) {
        const ssize_t bytes = recv(sock, packetBuf, sizeof(packetBuf), 0);
        if (bytes <= 0) continue;
        
        // Depacketize
        const uint32_t frames = rxDepacketizers_[streamIdx]->ParsePacket(
            packetBuf, bytes, sampleBuf);
        
        if (frames > 0) {
            // TODO: Insert into jitter buffer, then write to ring
            // For now, write directly to ring
            inputRings_[streamIdx]->Write(sampleBuf, frames);
        }
    }
    
    close(sock);
}

void NetworkEngine::RTPTransmitThread(uint32_t streamIdx) {
    // Create UDP socket
    const int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    
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
