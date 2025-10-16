// aes67-list.cpp - List discovered AES67 streams
// SPDX-License-Identifier: MIT

#include "../engine/include/NetworkEngine.h"
#include "../engine/include/SDPParser.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace AES67;

int main(int argc, char* argv[]) {
    std::cout << "AES67 Stream Discovery Tool\n";
    std::cout << "============================\n\n";
    
    // Parse command line
    int listenTime = 10; // Default 10 seconds
    if (argc > 1) {
        listenTime = std::atoi(argv[1]);
        if (listenTime <= 0) listenTime = 10;
    }
    
    std::cout << "Listening for SAP announcements for " << listenTime << " seconds...\n\n";
    
    // Create engine (only need discovery, not full I/O)
    auto* engine = CreateNetworkEngine(nullptr);
    if (!engine) {
        std::cerr << "Failed to create network engine\n";
        return 1;
    }
    
    // Start engine (this starts SAP discovery thread)
    if (!engine->Start()) {
        std::cerr << "Failed to start network engine\n";
        DestroyNetworkEngine(engine);
        return 1;
    }
    
    // Wait for discoveries
    std::this_thread::sleep_for(std::chrono::seconds(listenTime));
    
    // Get discovered streams
    auto streamNames = static_cast<NetworkEngine*>(engine)->GetDiscoveredStreamNames();
    
    if (streamNames.empty()) {
        std::cout << "No AES67 streams discovered.\n";
    } else {
        std::cout << "Discovered " << streamNames.size() << " stream(s):\n\n";
        
        for (const auto& name : streamNames) {
            SDPSession session;
            if (static_cast<NetworkEngine*>(engine)->GetDiscoveredStream(name, session)) {
                std::cout << "Stream: " << name << "\n";
                std::cout << "  Address: " << session.connectionAddr << ":" << session.port << "\n";
                std::cout << "  Channels: " << static_cast<int>(session.channels) << "\n";
                std::cout << "  Sample Rate: " << session.sampleRate << " Hz\n";
                std::cout << "  Packet Time: " << session.packetTimeUs << " µs\n";
                if (!session.ptpRefClock.empty()) {
                    std::cout << "  PTP Clock: " << session.ptpRefClock << "\n";
                }
                std::cout << "\n";
            }
        }
    }
    
    // Cleanup
    engine->Stop();
    DestroyNetworkEngine(engine);
    
    return 0;
}
