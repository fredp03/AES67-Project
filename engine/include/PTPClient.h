// PTPClient.h - IEEE 1588 PTP client with servo
// SPDX-License-Identifier: MIT

#pragma once

#include "PTPTypes.h"
#include <cstdint>
#include <atomic>
#include <functional>

namespace AES67 {

class PTPClient {
public:
    explicit PTPClient(uint8_t domain = kPTPDefaultDomain);
    ~PTPClient();
    
    bool Start(const char* interfaceName);
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
    
private:
    void ReceiveThread();
    void ServoUpdate(int64_t offsetNs, uint64_t hostTime);
    
    uint8_t domain_;
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
};

} // namespace AES67
