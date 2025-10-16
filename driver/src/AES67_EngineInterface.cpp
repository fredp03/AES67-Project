// AES67_EngineInterface.cpp - Stub implementation (real engine in engine/)
// SPDX-License-Identifier: MIT

#include "AES67_EngineInterface.h"

namespace AES67 {

// Stub engine for compilation (real implementation in engine library)
class StubNetworkEngine : public INetworkEngine {
public:
    bool Start() override { return false; }
    void Stop() override {}
    uint64_t GetPTPTimeNs() const override { return 0; }
    uint64_t HostTimeToPTP(uint64_t) const override { return 0; }
    uint64_t PTPToHostTime(uint64_t) const override { return 0; }
    bool IsPTPLocked() const override { return false; }
    double GetPTPOffset() const override { return 0.0; }
    double GetRateScalar() const override { return 1.0; }
    void SetCallbacks(const EngineCallbacks&) override {}
    AudioRingBuffer* GetInputRingBuffer(uint32_t) override { return nullptr; }
    AudioRingBuffer* GetOutputRingBuffer(uint32_t) override { return nullptr; }
    void NotifyIOCycle(uint64_t, uint64_t) override {}
};

} // namespace AES67

extern "C" {

AES67::INetworkEngine* CreateNetworkEngine(const char* configPath) {
    (void)configPath;
    return new AES67::StubNetworkEngine();
}

void DestroyNetworkEngine(AES67::INetworkEngine* engine) {
    delete engine;
}

}
