// PTPClient.cpp - PTP (IEEE 1588) client implementation
// SPDX-License-Identifier: MIT

#include "PTPClient.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <mach/mach_time.h>

namespace AES67 {

PTPClient::PTPClient(uint8_t domain)
    : domain_(domain)
    , socketEvent_(-1)
    , socketGeneral_(-1)
    , running_(false)
    , locked_(false)
    , offsetNs_(0.0)
    , rateRatio_(1.0)
    , affineSlopeA_(1.0)
    , affineOffsetB_(0.0)
    , affineAnchorHost_(0)
    , affineAnchorPTP_(0)
    , integrator_(0.0)
{}

PTPClient::~PTPClient() {
    Stop();
}

bool PTPClient::Start(const char* interfaceName) {
    if (running_) {
        return true;
    }
    
    (void)interfaceName; // TODO: Bind to specific interface
    
    // Create event socket (port 319)
    socketEvent_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketEvent_ < 0) {
        return false;
    }
    
    // Create general socket (port 320)
    socketGeneral_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketGeneral_ < 0) {
        close(socketEvent_);
        socketEvent_ = -1;
        return false;
    }
    
    // Set socket options
    int reuse = 1;
    setsockopt(socketEvent_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(socketGeneral_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // Bind event socket
    sockaddr_in addrEvent{};
    addrEvent.sin_family = AF_INET;
    addrEvent.sin_addr.s_addr = INADDR_ANY;
    addrEvent.sin_port = htons(kPTP_Event_Port);
    
    if (bind(socketEvent_, reinterpret_cast<sockaddr*>(&addrEvent), sizeof(addrEvent)) < 0) {
        close(socketEvent_);
        close(socketGeneral_);
        socketEvent_ = -1;
        socketGeneral_ = -1;
        return false;
    }
    
    // Bind general socket
    sockaddr_in addrGeneral{};
    addrGeneral.sin_family = AF_INET;
    addrGeneral.sin_addr.s_addr = INADDR_ANY;
    addrGeneral.sin_port = htons(kPTP_General_Port);
    
    if (bind(socketGeneral_, reinterpret_cast<sockaddr*>(&addrGeneral), sizeof(addrGeneral)) < 0) {
        close(socketEvent_);
        close(socketGeneral_);
        socketEvent_ = -1;
        socketGeneral_ = -1;
        return false;
    }
    
    // Join multicast group
    ip_mreq mreq{};
    inet_pton(AF_INET, kPTP_IPv4_MulticastAddr, &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = INADDR_ANY;
    
    setsockopt(socketEvent_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    setsockopt(socketGeneral_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    
    // Set non-blocking
    fcntl(socketEvent_, F_SETFL, O_NONBLOCK);
    fcntl(socketGeneral_, F_SETFL, O_NONBLOCK);
    
    // Initialize anchor points
    affineAnchorHost_ = mach_absolute_time();
    affineAnchorPTP_ = 0; // Will be set on first sync
    
    // Start receive thread
    running_ = true;
    receiveThread_ = std::thread(&PTPClient::ReceiveThread, this);
    
    return true;
}

void PTPClient::Stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    
    if (socketEvent_ >= 0) {
        close(socketEvent_);
        socketEvent_ = -1;
    }
    
    if (socketGeneral_ >= 0) {
        close(socketGeneral_);
        socketGeneral_ = -1;
    }
    
    locked_ = false;
}

uint64_t PTPClient::GetPTPTimeNs() const {
    if (!locked_) {
        return 0;
    }
    
    const uint64_t hostTime = mach_absolute_time();
    return HostTimeToPTP(hostTime);
}

uint64_t PTPClient::HostTimeToPTP(uint64_t hostTime) const {
    // Affine transformation: ptp = a * (host - anchor_host) + anchor_ptp
    const int64_t hostDelta = static_cast<int64_t>(hostTime - affineAnchorHost_);
    const int64_t ptpDelta = static_cast<int64_t>(affineSlopeA_ * hostDelta);
    return affineAnchorPTP_ + ptpDelta;
}

uint64_t PTPClient::PTPToHostTime(uint64_t ptpTimeNs) const {
    // Inverse: host = (ptp - anchor_ptp) / a + anchor_host
    const int64_t ptpDelta = static_cast<int64_t>(ptpTimeNs - affineAnchorPTP_);
    const int64_t hostDelta = static_cast<int64_t>(ptpDelta / affineSlopeA_);
    return affineAnchorHost_ + hostDelta;
}

void PTPClient::ReceiveThread() {
    uint8_t buffer[1500];
    
    while (running_) {
        // Poll event socket for Sync messages
        const ssize_t bytesEvent = recv(socketEvent_, buffer, sizeof(buffer), 0);
        if (bytesEvent > 0) {
            // TODO: Parse Sync message and update servo
            // For now, just simulate lock after a delay
            if (!locked_ && bytesEvent >= static_cast<ssize_t>(sizeof(PTPSyncMessage))) {
                locked_ = true;
                offsetNs_ = 0.0;
                
                if (statusCallback_) {
                    statusCallback_(true, 0.0);
                }
            }
        }
        
        // Poll general socket for Follow_Up and Announce
        const ssize_t bytesGeneral = recv(socketGeneral_, buffer, sizeof(buffer), 0);
        if (bytesGeneral > 0) {
            // TODO: Parse Follow_Up and Announce messages
        }
        
        // Sleep briefly to avoid busy-wait
        usleep(1000); // 1 ms
    }
}

void PTPClient::ServoUpdate(int64_t offsetNs, uint64_t hostTime) {
    // PI servo controller
    const double error = static_cast<double>(offsetNs);
    
    // Proportional term
    const double pTerm = kp_ * error;
    
    // Integral term (with anti-windup)
    integrator_ += error;
    if (integrator_ > 1e9) integrator_ = 1e9;
    if (integrator_ < -1e9) integrator_ = -1e9;
    const double iTerm = ki_ * integrator_;
    
    // Compute rate adjustment
    const double adjustment = pTerm + iTerm;
    rateRatio_ = 1.0 + (adjustment / 1e9);
    
    // Update affine coefficients
    affineSlopeA_ = rateRatio_.load();
    affineAnchorHost_ = hostTime;
    // affineAnchorPTP_ updated from sync messages
    
    // Store offset
    offsetNs_ = error;
    
    // Check lock threshold
    const bool wasLocked = locked_;
    locked_ = (std::abs(error) < 500.0); // 500 ns threshold
    
    if (locked_ != wasLocked && statusCallback_) {
        statusCallback_(locked_, error);
    }
}

} // namespace AES67
