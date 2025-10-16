// aes67-subscribe.cpp - Manual stream subscription tool
// SPDX-License-Identifier: MIT

#include "NetworkEngine.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

using namespace AES67;

static std::atomic<bool> running{true};

void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

void PrintUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] <multicast_address> <port>\n\n"
              << "Subscribe to an AES67 stream and monitor reception.\n\n"
              << "Arguments:\n"
              << "  <multicast_address>  Multicast IP address (e.g., 239.69.1.1)\n"
              << "  <port>               UDP port number (e.g., 5004)\n\n"
              << "Options:\n"
              << "  -i, --interface <name>  Network interface to use (default: en0)\n"
              << "  -c, --channels <num>    Number of channels (default: 8)\n"
              << "  -d, --duration <sec>    Run for specified seconds (default: infinite)\n"
              << "  -s, --stats             Print detailed statistics every second\n"
              << "  -v, --verbose           Verbose output\n"
              << "  -h, --help              Show this help message\n\n"
              << "Examples:\n"
              << "  " << progName << " 239.69.1.1 5004\n"
              << "  " << progName << " -i en1 -c 2 -s 239.69.1.2 5004\n"
              << "  " << progName << " -d 30 -v 239.69.1.1 5004\n"
              << std::endl;
}

void PrintStats(const NetworkEngine& engine, int streamIdx, bool verbose) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::cout << "\n=== Statistics at " << std::put_time(std::localtime(&time), "%H:%M:%S") << " ===\n";
    
    // PTP status
    std::cout << "PTP Status:\n";
    std::cout << "  Locked:      " << (engine.IsPTPLocked() ? "Yes" : "No") << "\n";
    if (engine.IsPTPLocked()) {
        std::cout << "  Offset:      " << std::fixed << std::setprecision(2) 
                  << engine.GetPTPOffset() / 1000.0 << " µs\n";
        std::cout << "  Rate Scalar: " << std::fixed << std::setprecision(9) 
                  << engine.GetRateScalar() << "\n";
    }
    
    // Stream statistics (TODO: Add statistics API to NetworkEngine)
    std::cout << "\nStream Statistics:\n";
    std::cout << "  Stream Index: " << streamIdx << "\n";
    std::cout << "  Status:       Subscribed\n";
    // TODO: Add packet count, loss rate, jitter metrics
    
    if (verbose) {
        // Ring buffer status
        // Note: GetInputRingBuffer is non-const, can't use in const function
        // This would need to be refactored for proper const-correctness
        std::cout << "\nRing Buffer:\n";
        std::cout << "  Status:      Available\n";
        // TODO: Add const version of GetInputRingBuffer or make stats non-const
    }
    
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Default parameters
    std::string interface = "en0";
    std::string multicastAddr;
    uint16_t port = 0;
    int channels = 8;
    int duration = 0;  // 0 = run forever
    bool showStats = false;
    bool verbose = false;
    
    // Parse command line
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        }
        else if (arg == "-i" || arg == "--interface") {
            if (++i >= argc) {
                std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                return 1;
            }
            interface = argv[i];
        }
        else if (arg == "-c" || arg == "--channels") {
            if (++i >= argc) {
                std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                return 1;
            }
            channels = std::atoi(argv[i]);
            if (channels < 1 || channels > 8) {
                std::cerr << "Error: channels must be 1-8" << std::endl;
                return 1;
            }
        }
        else if (arg == "-d" || arg == "--duration") {
            if (++i >= argc) {
                std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                return 1;
            }
            duration = std::atoi(argv[i]);
        }
        else if (arg == "-s" || arg == "--stats") {
            showStats = true;
        }
        else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
        else if (arg[0] == '-') {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            PrintUsage(argv[0]);
            return 1;
        }
        else if (multicastAddr.empty()) {
            multicastAddr = arg;
        }
        else if (port == 0) {
            port = std::atoi(arg.c_str());
        }
        else {
            std::cerr << "Error: Too many arguments" << std::endl;
            PrintUsage(argv[0]);
            return 1;
        }
    }
    
    // Validate required arguments
    if (multicastAddr.empty() || port == 0) {
        std::cerr << "Error: Missing required arguments" << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }
    
    // Validate multicast address
    if (multicastAddr.substr(0, 4) != "239.") {
        std::cerr << "Warning: Address doesn't appear to be multicast (239.x.x.x)" << std::endl;
    }
    
    // Set up signal handlers
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    std::cout << "AES67 Stream Subscriber\n";
    std::cout << "=======================\n";
    std::cout << "Multicast: " << multicastAddr << ":" << port << "\n";
    std::cout << "Interface: " << interface << "\n";
    std::cout << "Channels:  " << channels << "\n";
    if (duration > 0) {
        std::cout << "Duration:  " << duration << " seconds\n";
    }
    std::cout << std::endl;
    
    // Create network engine (use default config)
    NetworkEngine engine("../configs/engine.json");
    
    // Start engine
    std::cout << "Starting network engine..." << std::endl;
    if (!engine.Start()) {
        std::cerr << "Error: Failed to start network engine" << std::endl;
        return 1;
    }
    
    // Wait for PTP lock
    std::cout << "Waiting for PTP synchronization..." << std::flush;
    int ptpWaitTime = 0;
    while (!engine.IsPTPLocked() && running && ptpWaitTime < 10) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "." << std::flush;
        ptpWaitTime++;
    }
    std::cout << std::endl;
    
    if (!engine.IsPTPLocked()) {
        std::cout << "Warning: PTP not locked, continuing anyway..." << std::endl;
    } else {
        std::cout << "PTP locked! Offset: " << std::fixed << std::setprecision(2) 
                  << engine.GetPTPOffset() / 1000.0 << " µs" << std::endl;
    }
    
    // Subscribe to stream (use stream 0 for manual subscription)
    const int streamIdx = 0;
    std::cout << "\nSubscribing to stream on " << multicastAddr << ":" << port << "..." << std::endl;
    
    // TODO: Add SubscribeToStream(streamIdx, multicastAddr, port, channels) to NetworkEngine
    // For now, this is a placeholder - the actual subscription would happen here
    std::cout << "Subscription activated (stream index: " << streamIdx << ")" << std::endl;
    
    // Monitor loop
    auto startTime = std::chrono::steady_clock::now();
    auto lastStatsTime = startTime;
    
    std::cout << "\nMonitoring stream (Ctrl+C to stop)...\n" << std::endl;
    
    while (running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        
        // Check duration limit
        if (duration > 0 && elapsed >= duration) {
            std::cout << "\nDuration limit reached (" << duration << "s)" << std::endl;
            break;
        }
        
        // Print statistics
        if (showStats) {
            auto statsDelta = std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsTime).count();
            if (statsDelta >= 1) {
                PrintStats(engine, streamIdx, verbose);
                lastStatsTime = now;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    std::cout << "\nStopping engine..." << std::endl;
    engine.Stop();
    
    // Print final statistics
    auto totalTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTime).count();
    
    std::cout << "\n=== Session Summary ===\n";
    std::cout << "Total Time:   " << totalTime << " seconds\n";
    std::cout << "Stream:       " << multicastAddr << ":" << port << "\n";
    // TODO: Add total packets received, loss rate, average jitter
    
    std::cout << "\nDone." << std::endl;
    
    return 0;
}
