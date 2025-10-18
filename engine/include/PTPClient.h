// PTPClient.h - IEEE 1588 PTP client with servo
// SPDX-License-Identifier: MIT

#pragma once

#include "PTPTypes.h"
#include <cstdint>
#include <atomic>
#include <functional>
#include <thread>
#include <netinet/in.h>

namespace AES67 {

class PTPClient {
public:
    enum class Mode {
        Slave,
        Master
    };

    explicit PTPClient(uint8_t domain = kPTPDefaultDomain, Mode mode = Mode::Slave);
    ~PTPClient();
    
    bool Start(const char* interfaceName, Mode mode = Mode::Slave);
    void Stop();
    
    // Get current PTP time (nanoseconds since epoch)
    uint64_t GetPTPTimeNs() const;
    
    // Affine mapping: hostTime = a * ptpTime + b
    uint64_t HostTimeToPTP(uint64_t hostTime) const;
    uint64_t PTPToHostTime(uint64_t ptpTimeNs) const;
    
    bool IsLocked() const { return locked_.load(); }
    double GetOffsetNs() const { return offsetNs_.load(); }
    double GetRateRatio() const { return rateRatio_.load(); }
    
    // Callbacks
    using StatusCallback = std::function<void(bool locked, double offsetNs)>;
    void SetStatusCallback(StatusCallback cb) { statusCallback_ = cb; }
    Mode GetMode() const { return mode_; }

private:
    void ReceiveThread();
    void ServoUpdate(int64_t offsetNs, uint64_t hostTime);

    // Master-mode helpers
    bool StartSlave(const char* interfaceName);
    bool StartMaster(const char* interfaceName);
    bool InitializeInterface(const char* interfaceName);
    void MasterSendThread();
    void MasterEventThread();
    void SendSync(uint16_t sequenceId, uint64_t timestampNs);
    void SendAnnounce(uint16_t sequenceId, uint64_t timestampNs);
    void SendDelayResp(const sockaddr_in& destAddr,
                       const ClockIdentity& requesterId,
                       uint16_t requesterPortId,
                       uint16_t sequenceId,
                       uint64_t receiveTimestampNs);
    void BuildHeader(uint8_t* buffer,
                     PTPMessageType type,
                     uint16_t messageLength,
                     uint16_t sequenceId,
                     uint8_t controlField,
                     int8_t logMessageInterval,
                     uint16_t flagField = 0);
    void WriteTimestamp(uint8_t* buffer, uint64_t timestampNs) const;
    uint64_t GetSystemTimeNs() const;
    void CloseSockets();
    
    uint8_t domain_;
    Mode mode_;
    int socketEvent_ = -1;
    int socketGeneral_ = -1;
    
    std::atomic<bool> running_{false};
    std::atomic<bool> locked_{false};
    std::atomic<double> offsetNs_{0.0};
    std::atomic<double> rateRatio_{1.0};
    
    // Affine coefficients (protected by servo logic)
    double affineSlopeA_ = 1.0;
    double affineOffsetB_ = 0.0;
    uint64_t affineAnchorHost_ = 0;
    uint64_t affineAnchorPTP_ = 0;
    
    // PI servo state
    double integrator_ = 0.0;
    double kp_ = 0.001;
    double ki_ = 0.0001;
    
    StatusCallback statusCallback_;
    std::thread receiveThread_;
    
    // Master mode state
    std::atomic<bool> masterRunning_{false};
    std::thread masterSendThread_;
    std::thread masterEventThread_;
    sockaddr_in eventDestAddr_{};
    sockaddr_in generalDestAddr_{};
    in_addr interfaceAddr_{};
    ClockIdentity clockIdentity_{};
    uint8_t macAddress_[6]{};
    uint16_t portNumber_ = 1;
    uint16_t syncSequenceId_ = 0;
    uint16_t announceSequenceId_ = 0;
    int syncIntervalMs_ = 125;      // 8 Hz default
    int announceIntervalMs_ = 1000; // 1 Hz default
};

} // namespace AES67
