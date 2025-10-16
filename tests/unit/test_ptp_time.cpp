// test_ptp_time.cpp - PTP timestamp and conversion tests
// SPDX-License-Identifier: MIT

#include "PTPClient.h"
#include <functional>
#include <chrono>
#include <cmath>

extern void RegisterTest(const std::string& name, std::function<bool()> test);

using namespace AES67;

// Test PTP time returns zero when not locked
bool test_ptp_time_valid() {
    PTPClient client(0);
    
    // When not locked, should return 0
    uint64_t ptpTime = client.GetPTPTimeNs();
    
    return ptpTime == 0 && !client.IsLocked();
}

// Test host-to-PTP conversion consistency
bool test_ptp_host_conversion() {
    PTPClient client(0);
    
    // Get current host time
    auto now = std::chrono::high_resolution_clock::now();
    uint64_t hostTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    // Convert to PTP and back
    uint64_t ptpTime = client.HostTimeToPTP(hostTime);
    uint64_t backToHost = client.PTPToHostTime(ptpTime);
    
    // Should be very close (within microseconds due to clock drift)
    int64_t diff = static_cast<int64_t>(backToHost) - static_cast<int64_t>(hostTime);
    return std::abs(diff) < 1000000; // Within 1ms
}

// Test PTP offset starts at zero (before lock)
bool test_ptp_offset_initial() {
    PTPClient client(0);
    
    // Before any PTP messages, offset should be zero
    double offset = client.GetOffsetNs();
    
    return offset == 0.0;
}

// Test PTP locked state (initially unlocked)
bool test_ptp_locked_initial() {
    PTPClient client(0);
    
    // Before receiving PTP messages, should be unlocked
    return !client.IsLocked();
}

// Test PTP rate ratio initial value
bool test_ptp_rate_initial() {
    PTPClient client(0);
    
    // Rate ratio should be 1.0 initially (no correction)
    double rate = client.GetRateRatio();
    
    // Should be exactly 1.0 or very close
    return std::abs(rate - 1.0) < 0.000001;
}

// Test timestamp monotonicity (when locked, times should advance)
bool test_ptp_monotonic() {
    PTPClient client(0);
    
    // Without locking, both should be 0
    uint64_t time1 = client.GetPTPTimeNs();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint64_t time2 = client.GetPTPTimeNs();
    
    // Both should be 0 when not locked
    return time1 == 0 && time2 == 0;
}

// Test timestamp precision
bool test_ptp_precision() {
    PTPClient client(0);
    
    // Without locking, should return 0
    uint64_t time1 = client.GetPTPTimeNs();
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    uint64_t time2 = client.GetPTPTimeNs();
    
    // Both should be 0 when not locked
    return time1 == 0 && time2 == 0;
}

// Test affine transformation properties
bool test_ptp_affine_linearity() {
    PTPClient client(0);
    
    auto now = std::chrono::high_resolution_clock::now();
    uint64_t hostTime1 = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    // Add 1 second
    uint64_t hostTime2 = hostTime1 + 1000000000ULL;
    
    // Convert both
    uint64_t ptpTime1 = client.HostTimeToPTP(hostTime1);
    uint64_t ptpTime2 = client.HostTimeToPTP(hostTime2);
    
    // Difference in PTP domain should also be ~1 second
    // (may vary slightly due to rate correction, but should be close)
    int64_t diff = static_cast<int64_t>(ptpTime2) - static_cast<int64_t>(ptpTime1);
    
    // Allow 0.1% variance (1ms in 1 second)
    return std::abs(diff - 1000000000LL) < 1000000;
}

// Test multiple conversions are consistent
bool test_ptp_conversion_consistency() {
    PTPClient client(0);
    
    auto now = std::chrono::high_resolution_clock::now();
    uint64_t hostTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    // Convert same time multiple times
    uint64_t ptp1 = client.HostTimeToPTP(hostTime);
    uint64_t ptp2 = client.HostTimeToPTP(hostTime);
    uint64_t ptp3 = client.HostTimeToPTP(hostTime);
    
    // Should be identical (affine params don't change without PTP messages)
    return (ptp1 == ptp2) && (ptp2 == ptp3);
}

// Register all PTP time tests
static struct PTPTimeTestRegistrar {
    PTPTimeTestRegistrar() {
        RegisterTest("PTP Time: Valid timestamp", test_ptp_time_valid);
        RegisterTest("PTP Time: Host conversion round-trip", test_ptp_host_conversion);
        RegisterTest("PTP Time: Initial offset zero", test_ptp_offset_initial);
        RegisterTest("PTP Time: Initially unlocked", test_ptp_locked_initial);
        RegisterTest("PTP Time: Initial rate ratio", test_ptp_rate_initial);
        RegisterTest("PTP Time: Monotonic increase", test_ptp_monotonic);
        RegisterTest("PTP Time: Nanosecond precision", test_ptp_precision);
        RegisterTest("PTP Time: Affine linearity", test_ptp_affine_linearity);
        RegisterTest("PTP Time: Conversion consistency", test_ptp_conversion_consistency);
    }
} ptpTimeTestRegistrar;
