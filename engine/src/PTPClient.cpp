// PTPClient.cpp - PTP (IEEE 1588) client/master implementation
// SPDX-License-Identifier: MIT

#include "PTPClient.h"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <ifaddrs.h>
#include <iostream>
#include <net/if.h>
#if defined(__APPLE__)
#include <net/if_dl.h>
#endif
#if defined(__linux__)
#include <netpacket/packet.h>
#endif
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

namespace AES67 {
namespace {

uint16_t ReadUint16(const uint8_t* buffer) {
    return static_cast<uint16_t>(buffer[0] << 8 | buffer[1]);
}

// Helper to obtain current system (wall-clock) time in nanoseconds.
uint64_t GetSystemTimeNsInternal() {
    struct timespec ts{};
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return 0;
    }
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL +
           static_cast<uint64_t>(ts.tv_nsec);
}

} // namespace

PTPClient::PTPClient(uint8_t domain, Mode mode)
    : domain_(domain)
    , mode_(mode) {
}

PTPClient::~PTPClient() {
    Stop();
}

bool PTPClient::Start(const char* interfaceName, Mode mode) {
    mode_ = mode;
    if (mode_ == Mode::Master) {
        return StartMaster(interfaceName);
    }
    return StartSlave(interfaceName);
}

bool PTPClient::StartSlave(const char* interfaceName) {
    if (running_) {
        return true;
    }

    (void)interfaceName; // TODO: Bind to specific interface

    socketEvent_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketEvent_ < 0) {
        return false;
    }

    socketGeneral_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketGeneral_ < 0) {
        CloseSockets();
        return false;
    }

    int reuse = 1;
    setsockopt(socketEvent_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(socketGeneral_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addrEvent{};
    addrEvent.sin_family = AF_INET;
    addrEvent.sin_addr.s_addr = INADDR_ANY;
    addrEvent.sin_port = htons(kPTP_Event_Port);

    if (bind(socketEvent_, reinterpret_cast<sockaddr*>(&addrEvent), sizeof(addrEvent)) < 0) {
        CloseSockets();
        return false;
    }

    sockaddr_in addrGeneral{};
    addrGeneral.sin_family = AF_INET;
    addrGeneral.sin_addr.s_addr = INADDR_ANY;
    addrGeneral.sin_port = htons(kPTP_General_Port);

    if (bind(socketGeneral_, reinterpret_cast<sockaddr*>(&addrGeneral), sizeof(addrGeneral)) < 0) {
        CloseSockets();
        return false;
    }

    ip_mreq mreq{};
    inet_pton(AF_INET, kPTP_IPv4_MulticastAddr, &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = INADDR_ANY;

    setsockopt(socketEvent_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    setsockopt(socketGeneral_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    fcntl(socketEvent_, F_SETFL, O_NONBLOCK);
    fcntl(socketGeneral_, F_SETFL, O_NONBLOCK);

    affineAnchorHost_ = GetSystemTimeNsInternal();
    affineAnchorPTP_ = 0;

    running_ = true;
    receiveThread_ = std::thread(&PTPClient::ReceiveThread, this);
    return true;
}

bool PTPClient::StartMaster(const char* interfaceName) {
    if (masterRunning_) {
        return true;
    }

    if (!InitializeInterface(interfaceName)) {
        std::cerr << "PTPClient: failed to resolve interface " << interfaceName << " for PTP master\n";
        return false;
    }

    socketEvent_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketEvent_ < 0) {
        std::cerr << "PTPClient: failed to create event socket: " << std::strerror(errno) << "\n";
        return false;
    }

    socketGeneral_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketGeneral_ < 0) {
        std::cerr << "PTPClient: failed to create general socket: " << std::strerror(errno) << "\n";
        CloseSockets();
        return false;
    }

    int reuse = 1;
    setsockopt(socketEvent_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(socketGeneral_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addrEvent{};
    addrEvent.sin_family = AF_INET;
    addrEvent.sin_addr.s_addr = INADDR_ANY;
    addrEvent.sin_port = htons(kPTP_Event_Port);
    if (bind(socketEvent_, reinterpret_cast<sockaddr*>(&addrEvent), sizeof(addrEvent)) < 0) {
        std::cerr << "PTPClient: failed to bind event socket to port 319: " << std::strerror(errno) << "\n";
        CloseSockets();
        return false;
    }

    sockaddr_in addrGeneral{};
    addrGeneral.sin_family = AF_INET;
    addrGeneral.sin_addr.s_addr = INADDR_ANY;
    addrGeneral.sin_port = htons(kPTP_General_Port);
    if (bind(socketGeneral_, reinterpret_cast<sockaddr*>(&addrGeneral), sizeof(addrGeneral)) < 0) {
        std::cerr << "PTPClient: failed to bind general socket to port 320: " << std::strerror(errno) << "\n";
        CloseSockets();
        return false;
    }

    fcntl(socketEvent_, F_SETFL, O_NONBLOCK);
    fcntl(socketGeneral_, F_SETFL, O_NONBLOCK);

    uint8_t ttl = 1;
    setsockopt(socketEvent_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    setsockopt(socketGeneral_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    setsockopt(socketEvent_, IPPROTO_IP, IP_MULTICAST_IF, &interfaceAddr_, sizeof(interfaceAddr_));
    setsockopt(socketGeneral_, IPPROTO_IP, IP_MULTICAST_IF, &interfaceAddr_, sizeof(interfaceAddr_));

    std::memset(&eventDestAddr_, 0, sizeof(eventDestAddr_));
    eventDestAddr_.sin_family = AF_INET;
    eventDestAddr_.sin_port = htons(kPTP_Event_Port);
    inet_pton(AF_INET, kPTP_IPv4_MulticastAddr, &eventDestAddr_.sin_addr);

    std::memset(&generalDestAddr_, 0, sizeof(generalDestAddr_));
    generalDestAddr_.sin_family = AF_INET;
    generalDestAddr_.sin_port = htons(kPTP_General_Port);
    inet_pton(AF_INET, kPTP_IPv4_MulticastAddr, &generalDestAddr_.sin_addr);

    syncSequenceId_ = 0;
    announceSequenceId_ = 0;

    const uint64_t nowNs = GetSystemTimeNs();
    affineAnchorHost_ = nowNs;
    affineAnchorPTP_ = nowNs;
    affineSlopeA_ = 1.0;
    affineOffsetB_ = 0.0;
    rateRatio_ = 1.0;
    offsetNs_ = 0.0;

    running_ = true;
    masterRunning_ = true;
    locked_ = true;

    if (statusCallback_) {
        statusCallback_(true, 0.0);
    }

    masterSendThread_ = std::thread(&PTPClient::MasterSendThread, this);
    masterEventThread_ = std::thread(&PTPClient::MasterEventThread, this);
    return true;
}

void PTPClient::Stop() {
    if (mode_ == Mode::Master) {
        if (!masterRunning_) {
            return;
        }

        masterRunning_ = false;
        running_ = false;

        if (masterSendThread_.joinable()) {
            masterSendThread_.join();
        }
        if (masterEventThread_.joinable()) {
            masterEventThread_.join();
        }

        CloseSockets();
        locked_ = false;
        return;
    }

    if (!running_) {
        return;
    }

    running_ = false;

    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }

    CloseSockets();
    locked_ = false;
}

uint64_t PTPClient::GetPTPTimeNs() const {
    if (mode_ == Mode::Master) {
        return GetSystemTimeNs();
    }

    if (!locked_) {
        return 0;
    }

    const uint64_t hostTime = GetSystemTimeNs();
    return HostTimeToPTP(hostTime);
}

uint64_t PTPClient::HostTimeToPTP(uint64_t hostTime) const {
    if (mode_ == Mode::Master) {
        return hostTime;
    }

    const int64_t hostDelta = static_cast<int64_t>(hostTime - affineAnchorHost_);
    const int64_t ptpDelta = static_cast<int64_t>(affineSlopeA_ * hostDelta);
    return affineAnchorPTP_ + ptpDelta;
}

uint64_t PTPClient::PTPToHostTime(uint64_t ptpTimeNs) const {
    if (mode_ == Mode::Master) {
        return ptpTimeNs;
    }

    const int64_t ptpDelta = static_cast<int64_t>(ptpTimeNs - affineAnchorPTP_);
    const int64_t hostDelta = static_cast<int64_t>(ptpDelta / affineSlopeA_);
    return affineAnchorHost_ + hostDelta;
}

void PTPClient::ReceiveThread() {
    uint8_t buffer[1500];

    while (running_) {
        const ssize_t bytesEvent = recv(socketEvent_, buffer, sizeof(buffer), 0);
        if (bytesEvent > 0) {
            if (!locked_ && bytesEvent >= static_cast<ssize_t>(sizeof(PTPSyncMessage))) {
                locked_ = true;
                offsetNs_ = 0.0;
                if (statusCallback_) {
                    statusCallback_(true, 0.0);
                }
            }
        }

        const ssize_t bytesGeneral = recv(socketGeneral_, buffer, sizeof(buffer), 0);
        if (bytesGeneral > 0) {
            (void)bytesGeneral;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void PTPClient::ServoUpdate(int64_t offsetNs, uint64_t hostTime) {
    const double error = static_cast<double>(offsetNs);
    const double pTerm = kp_ * error;

    integrator_ += error;
    if (integrator_ > 1e9) integrator_ = 1e9;
    if (integrator_ < -1e9) integrator_ = -1e9;
    const double iTerm = ki_ * integrator_;

    const double adjustment = pTerm + iTerm;
    rateRatio_ = 1.0 + (adjustment / 1e9);

    affineSlopeA_ = rateRatio_.load();
    affineAnchorHost_ = hostTime;
    offsetNs_ = error;

    const bool wasLocked = locked_;
    locked_ = (std::abs(error) < 500.0);

    if (locked_ != wasLocked && statusCallback_) {
        statusCallback_(locked_, error);
    }
}

bool PTPClient::InitializeInterface(const char* interfaceName) {
    bool haveIPv4 = false;
    bool haveMac = false;

    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0) {
        return false;
    }

    for (auto* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        if (std::strcmp(ifa->ifa_name, interfaceName) != 0) {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET) {
            interfaceAddr_ = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr)->sin_addr;
            haveIPv4 = true;
        }
#if defined(AF_LINK)
        else if (ifa->ifa_addr->sa_family == AF_LINK) {
            const auto* sdl = reinterpret_cast<sockaddr_dl*>(ifa->ifa_addr);
            if (sdl->sdl_alen == 6) {
                std::memcpy(macAddress_, LLADDR(sdl), 6);
                haveMac = true;
            }
        }
#endif
#if defined(AF_PACKET)
        else if (ifa->ifa_addr->sa_family == AF_PACKET) {
            const auto* sll = reinterpret_cast<sockaddr_ll*>(ifa->ifa_addr);
            if (sll->sll_halen == 6) {
                std::memcpy(macAddress_, sll->sll_addr, 6);
                haveMac = true;
            }
        }
#endif
    }

    freeifaddrs(ifaddr);

    if (!haveIPv4 || !haveMac) {
        return false;
    }

    clockIdentity_.id[0] = macAddress_[0];
    clockIdentity_.id[1] = macAddress_[1];
    clockIdentity_.id[2] = macAddress_[2];
    clockIdentity_.id[3] = 0xFF;
    clockIdentity_.id[4] = 0xFE;
    clockIdentity_.id[5] = macAddress_[3];
    clockIdentity_.id[6] = macAddress_[4];
    clockIdentity_.id[7] = macAddress_[5];

    return true;
}

void PTPClient::MasterSendThread() {
    using namespace std::chrono;

    const auto syncInterval = milliseconds(syncIntervalMs_);
    const auto announceInterval = milliseconds(announceIntervalMs_);

    auto nextSync = steady_clock::now();
    auto nextAnnounce = nextSync;

    while (masterRunning_) {
        const auto now = steady_clock::now();

        if (now >= nextSync) {
            const uint64_t timestamp = GetSystemTimeNs();
            SendSync(syncSequenceId_++, timestamp);
            do {
                nextSync += syncInterval;
            } while (nextSync <= now);
        }

        if (now >= nextAnnounce) {
            const uint64_t timestamp = GetSystemTimeNs();
            SendAnnounce(announceSequenceId_++, timestamp);
            do {
                nextAnnounce += announceInterval;
            } while (nextAnnounce <= now);
        }

        std::this_thread::sleep_for(milliseconds(1));
    }
}

void PTPClient::MasterEventThread() {
    uint8_t buffer[1500];

    while (masterRunning_) {
        sockaddr_in srcAddr{};
        socklen_t srcLen = sizeof(srcAddr);
        const ssize_t bytes = recvfrom(socketEvent_, buffer, sizeof(buffer), 0,
                                       reinterpret_cast<sockaddr*>(&srcAddr), &srcLen);

        if (bytes < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "PTPClient: recvfrom error on Delay_Req socket: " << std::strerror(errno) << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (bytes < 34) {
            continue;
        }

        const uint8_t messageType = buffer[0] & 0x0F;
        if (messageType != static_cast<uint8_t>(PTPMessageType::Delay_Req)) {
            continue;
        }

        if (bytes < 44) {
            continue;
        }

        ClockIdentity requester{};
        std::memcpy(requester.id, buffer + 20, sizeof(requester.id));
        const uint16_t requesterPortId = ReadUint16(buffer + 28);
        const uint16_t sequenceId = ReadUint16(buffer + 30);
        const uint64_t rxTimestamp = GetSystemTimeNs();

        sockaddr_in destAddr = srcAddr;
        destAddr.sin_port = htons(kPTP_General_Port);
        SendDelayResp(destAddr, requester, requesterPortId, sequenceId, rxTimestamp);
    }
}

void PTPClient::SendSync(uint16_t sequenceId, uint64_t timestampNs) {
    uint8_t message[44]{};
    BuildHeader(message, PTPMessageType::Sync, sizeof(message), sequenceId, 0, -3);
    WriteTimestamp(message + 34, timestampNs);

    if (sendto(socketEvent_, message, sizeof(message), 0,
               reinterpret_cast<sockaddr*>(&eventDestAddr_), sizeof(eventDestAddr_)) < 0) {
        std::cerr << "PTPClient: failed to send Sync: " << std::strerror(errno) << "\n";
    }
}

void PTPClient::SendAnnounce(uint16_t sequenceId, uint64_t timestampNs) {
    uint8_t message[64]{};
    BuildHeader(message, PTPMessageType::Announce, sizeof(message), sequenceId, 5, 0);
    WriteTimestamp(message + 34, timestampNs);

    const uint16_t currentUtcOffset = 37;
    message[44] = static_cast<uint8_t>((currentUtcOffset >> 8) & 0xFF);
    message[45] = static_cast<uint8_t>(currentUtcOffset & 0xFF);
    message[46] = 0x00; // reserved
    message[47] = 128;  // grandmasterPriority1

    message[48] = 248; // clockClass
    message[49] = 0xFE; // clockAccuracy unknown
    message[50] = 0xFF;
    message[51] = 0xFF; // offsetScaledLogVariance

    message[52] = 128; // grandmasterPriority2
    std::memcpy(message + 53, clockIdentity_.id, sizeof(clockIdentity_.id));

    const uint16_t stepsRemoved = 0;
    message[61] = static_cast<uint8_t>((stepsRemoved >> 8) & 0xFF);
    message[62] = static_cast<uint8_t>(stepsRemoved & 0xFF);
    message[63] = 0xA0; // timeSource: internal oscillator

    if (sendto(socketGeneral_, message, sizeof(message), 0,
               reinterpret_cast<sockaddr*>(&generalDestAddr_), sizeof(generalDestAddr_)) < 0) {
        std::cerr << "PTPClient: failed to send Announce: " << std::strerror(errno) << "\n";
    }
}

void PTPClient::SendDelayResp(const sockaddr_in& destAddr,
                              const ClockIdentity& requesterId,
                              uint16_t requesterPortId,
                              uint16_t sequenceId,
                              uint64_t receiveTimestampNs) {
    uint8_t message[54]{};
    BuildHeader(message, PTPMessageType::Delay_Resp, sizeof(message), sequenceId, 3, 0x7F);
    WriteTimestamp(message + 34, receiveTimestampNs);
    std::memcpy(message + 44, requesterId.id, sizeof(requesterId.id));
    message[52] = static_cast<uint8_t>((requesterPortId >> 8) & 0xFF);
    message[53] = static_cast<uint8_t>(requesterPortId & 0xFF);

    if (sendto(socketGeneral_, message, sizeof(message), 0,
               reinterpret_cast<const sockaddr*>(&destAddr), sizeof(destAddr)) < 0) {
        std::cerr << "PTPClient: failed to send Delay_Resp: " << std::strerror(errno) << "\n";
    }
}

void PTPClient::BuildHeader(uint8_t* buffer,
                            PTPMessageType type,
                            uint16_t messageLength,
                            uint16_t sequenceId,
                            uint8_t controlField,
                            int8_t logMessageInterval,
                            uint16_t flagField) {
    std::memset(buffer, 0, messageLength);
    buffer[0] = static_cast<uint8_t>(type) & 0x0F;
    buffer[1] = 0x02; // PTP v2
    buffer[2] = static_cast<uint8_t>((messageLength >> 8) & 0xFF);
    buffer[3] = static_cast<uint8_t>(messageLength & 0xFF);
    buffer[4] = domain_;
    buffer[5] = 0;
    buffer[6] = static_cast<uint8_t>((flagField >> 8) & 0xFF);
    buffer[7] = static_cast<uint8_t>(flagField & 0xFF);

    std::memcpy(buffer + 20, clockIdentity_.id, sizeof(clockIdentity_.id));
    buffer[28] = static_cast<uint8_t>((portNumber_ >> 8) & 0xFF);
    buffer[29] = static_cast<uint8_t>(portNumber_ & 0xFF);
    buffer[30] = static_cast<uint8_t>((sequenceId >> 8) & 0xFF);
    buffer[31] = static_cast<uint8_t>(sequenceId & 0xFF);
    buffer[32] = controlField;
    buffer[33] = static_cast<uint8_t>(logMessageInterval);
}

void PTPClient::WriteTimestamp(uint8_t* buffer, uint64_t timestampNs) const {
    uint64_t seconds = timestampNs / 1000000000ULL;
    uint32_t nanoseconds = static_cast<uint32_t>(timestampNs % 1000000000ULL);

    buffer[0] = static_cast<uint8_t>((seconds >> 40) & 0xFF);
    buffer[1] = static_cast<uint8_t>((seconds >> 32) & 0xFF);
    buffer[2] = static_cast<uint8_t>((seconds >> 24) & 0xFF);
    buffer[3] = static_cast<uint8_t>((seconds >> 16) & 0xFF);
    buffer[4] = static_cast<uint8_t>((seconds >> 8) & 0xFF);
    buffer[5] = static_cast<uint8_t>(seconds & 0xFF);

    buffer[6] = static_cast<uint8_t>((nanoseconds >> 24) & 0xFF);
    buffer[7] = static_cast<uint8_t>((nanoseconds >> 16) & 0xFF);
    buffer[8] = static_cast<uint8_t>((nanoseconds >> 8) & 0xFF);
    buffer[9] = static_cast<uint8_t>(nanoseconds & 0xFF);
}

uint64_t PTPClient::GetSystemTimeNs() const {
    return GetSystemTimeNsInternal();
}

void PTPClient::CloseSockets() {
    if (socketEvent_ >= 0) {
        close(socketEvent_);
        socketEvent_ = -1;
    }
    if (socketGeneral_ >= 0) {
        close(socketGeneral_);
        socketGeneral_ = -1;
    }
}

} // namespace AES67
