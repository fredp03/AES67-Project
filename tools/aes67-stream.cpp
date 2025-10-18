// aes67-stream.cpp - Transmit audio files as AES67 streams
// SPDX-License-Identifier: MIT

#include "NetworkEngine.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <vector>
#include <cstring>

using namespace AES67;

static std::atomic<bool> running{true};

void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

void PrintUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] <audio_file>\n\n"
              << "Transmit an audio file as an AES67 stream.\n\n"
              << "Arguments:\n"
              << "  <audio_file>            RAW audio file (32-bit int, interleaved)\n\n"
              << "Options:\n"
              << "  -i, --interface <name>  Network interface to use (default: en0)\n"
              << "  -a, --address <addr>    Multicast address (default: 239.69.1.1)\n"
              << "  -p, --port <port>       UDP port (default: 5004)\n"
              << "  -c, --channels <num>    Number of channels (default: 2)\n"
              << "  -r, --rate <hz>         Sample rate (default: 48000)\n"
              << "  -n, --name <name>       Stream name for SAP (default: filename)\n"
              << "  -l, --loop              Loop playback continuously\n"
              << "  -s, --stats             Print statistics every second\n"
              << "  -v, --verbose           Verbose output\n"
              << "  -h, --help              Show this help message\n\n"
              << "File Format:\n"
              << "  The audio file must be raw PCM data:\n"
              << "  - Format: 32-bit signed integer\n"
              << "  - Layout: Interleaved (L, R, L, R, ...)\n"
              << "  - No header (raw samples only)\n\n"
              << "Examples:\n"
              << "  # Stream stereo file to default address\n"
              << "  " << progName << " -c 2 audio.raw\n\n"
              << "  # Stream 8-channel file, loop forever\n"
              << "  " << progName << " -c 8 -l multichannel.raw\n\n"
              << "  # Stream to specific address with name\n"
              << "  " << progName << " -a 239.69.1.10 -n \"Console Mix\" audio.raw\n\n"
              << "  # Convert from WAV to raw (using ffmpeg):\n"
              << "  ffmpeg -i input.wav -f s32le -ar 48000 -ac 2 output.raw\n"
              << std::endl;
}

struct AudioFileInfo {
    std::string filename;
    int channels;
    int sampleRate;
    size_t totalSamples;
    size_t totalFrames;
};

bool LoadAudioFile(const std::string& filename, int channels, int sampleRate,
                   std::vector<int32_t>& buffer, AudioFileInfo& info) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Error: Cannot open file: " << filename << std::endl;
        return false;
    }
    
    // Get file size
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Calculate samples
    size_t totalSamples = fileSize / sizeof(int32_t);
    
    if (totalSamples == 0) {
        std::cerr << "Error: File is empty or too small" << std::endl;
        return false;
    }
    
    if (totalSamples % channels != 0) {
        std::cerr << "Warning: File size not aligned to channel count" << std::endl;
        std::cerr << "  Total samples: " << totalSamples << std::endl;
        std::cerr << "  Channels: " << channels << std::endl;
        std::cerr << "  Remainder: " << (totalSamples % channels) << std::endl;
        totalSamples -= (totalSamples % channels);
    }
    
    // Load data
    buffer.resize(totalSamples);
    file.read(reinterpret_cast<char*>(buffer.data()), totalSamples * sizeof(int32_t));
    
    if (!file) {
        std::cerr << "Error: Failed to read file" << std::endl;
        return false;
    }
    
    // Fill info
    info.filename = filename;
    info.channels = channels;
    info.sampleRate = sampleRate;
    info.totalSamples = totalSamples;
    info.totalFrames = totalSamples / channels;
    
    return true;
}

void PrintStats(const NetworkEngine& engine, const AudioFileInfo& info, 
                size_t currentFrame, bool verbose) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    double elapsed = static_cast<double>(currentFrame) / info.sampleRate;
    double total = static_cast<double>(info.totalFrames) / info.sampleRate;
    double percent = (static_cast<double>(currentFrame) / info.totalFrames) * 100.0;
    
    std::cout << "\n=== Statistics at " << std::put_time(std::localtime(&time), "%H:%M:%S") << " ===\n";
    
    // Playback position
    std::cout << "Playback:\n";
    std::cout << "  Position:    " << std::setw(7) << currentFrame << " / " << info.totalFrames << " frames\n";
    std::cout << "  Time:        " << std::fixed << std::setprecision(1) 
              << elapsed << " / " << total << " seconds (" << percent << "%)\n";
    
    // PTP status
    std::cout << "\nPTP Status:\n";
    std::cout << "  Locked:      " << (engine.IsPTPLocked() ? "Yes" : "No") << "\n";
    if (engine.IsPTPLocked()) {
        std::cout << "  Offset:      " << std::fixed << std::setprecision(2) 
                  << engine.GetPTPOffset() / 1000.0 << " µs\n";
        std::cout << "  Rate Scalar: " << std::fixed << std::setprecision(9) 
                  << engine.GetRateScalar() << "\n";
    }
    
    // TODO: Add TX statistics
    if (verbose) {
        std::cout << "\nTransmit Statistics:\n";
        // TODO: Packets sent, data rate, etc.
    }
    
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Default parameters
    std::string interface = "en0";
    std::string multicastAddr = "239.69.1.1";
    uint16_t port = 5004;
    std::string filename;
    std::string streamName;
    int channels = 2;
    int sampleRate = 48000;
    bool loop = false;
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
        else if (arg == "-a" || arg == "--address") {
            if (++i >= argc) {
                std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                return 1;
            }
            multicastAddr = argv[i];
        }
        else if (arg == "-p" || arg == "--port") {
            if (++i >= argc) {
                std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                return 1;
            }
            port = std::atoi(argv[i]);
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
        else if (arg == "-r" || arg == "--rate") {
            if (++i >= argc) {
                std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                return 1;
            }
            sampleRate = std::atoi(argv[i]);
            if (sampleRate != 48000 && sampleRate != 96000) {
                std::cerr << "Warning: Only 48000 and 96000 Hz are AES67-compliant" << std::endl;
            }
        }
        else if (arg == "-n" || arg == "--name") {
            if (++i >= argc) {
                std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                return 1;
            }
            streamName = argv[i];
        }
        else if (arg == "-l" || arg == "--loop") {
            loop = true;
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
        else if (filename.empty()) {
            filename = arg;
        }
        else {
            std::cerr << "Error: Too many arguments" << std::endl;
            PrintUsage(argv[0]);
            return 1;
        }
    }
    
    // Validate required arguments
    if (filename.empty()) {
        std::cerr << "Error: Missing audio file" << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }
    
    // Use filename as stream name if not specified
    if (streamName.empty()) {
        size_t lastSlash = filename.find_last_of("/\\");
        streamName = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;
        // Remove extension
        size_t lastDot = streamName.find_last_of('.');
        if (lastDot != std::string::npos) {
            streamName = streamName.substr(0, lastDot);
        }
    }
    
    // Set up signal handlers
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    std::cout << "AES67 Stream Transmitter\n";
    std::cout << "========================\n";
    std::cout << "File:       " << filename << "\n";
    std::cout << "Stream:     " << streamName << "\n";
    std::cout << "Multicast:  " << multicastAddr << ":" << port << "\n";
    std::cout << "Interface:  " << interface << "\n";
    std::cout << "Channels:   " << channels << "\n";
    std::cout << "Rate:       " << sampleRate << " Hz\n";
    std::cout << "Loop:       " << (loop ? "Yes" : "No") << "\n";
    std::cout << std::endl;
    
    // Load audio file
    std::cout << "Loading audio file..." << std::endl;
    std::vector<int32_t> audioBuffer;
    AudioFileInfo info;
    
    if (!LoadAudioFile(filename, channels, sampleRate, audioBuffer, info)) {
        return 1;
    }
    
    double duration = static_cast<double>(info.totalFrames) / sampleRate;
    std::cout << "Loaded " << info.totalFrames << " frames (" 
              << std::fixed << std::setprecision(2) << duration << " seconds)" << std::endl;
    
    std::cout << std::endl;
    
    // Create network engine (use default config)
    NetworkEngine engine("../configs/engine.json");
    engine.SetNetworkInterface(interface);
    
    // Start engine
    std::cout << "\nStarting network engine..." << std::endl;
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
    
    // TODO: Configure stream output
    // engine.SetOutputStreamConfig(0, multicastAddr, port, channels, streamName);
    
    std::cout << "\nStreaming (Ctrl+C to stop)...\n" << std::endl;
    
    // Streaming loop
    const int streamIdx = 0;
    size_t currentFrame = 0;
    auto lastStatsTime = std::chrono::steady_clock::now();
    
    // Get ring buffer for writing
    auto* ringBuffer = engine.GetOutputRingBuffer(streamIdx);
    if (!ringBuffer) {
        std::cerr << "Error: Failed to get output ring buffer" << std::endl;
        engine.Stop();
        return 1;
    }
    
    while (running) {
        // Write audio to ring buffer
        size_t framesToWrite = std::min(
            static_cast<size_t>(480),  // ~10ms chunks @ 48kHz
            info.totalFrames - currentFrame
        );
        
        if (framesToWrite > 0) {
            size_t samplesToWrite = framesToWrite * channels;
            size_t written = ringBuffer->Write(&audioBuffer[currentFrame * channels], samplesToWrite);
            currentFrame += written / channels;
        }
        
        // Check for end of file
        if (currentFrame >= info.totalFrames) {
            if (loop) {
                currentFrame = 0;
                if (verbose) {
                    std::cout << "Looping..." << std::endl;
                }
            } else {
                std::cout << "\nEnd of file reached." << std::endl;
                break;
            }
        }
        
        // Print statistics
        if (showStats) {
            auto now = std::chrono::steady_clock::now();
            auto statsDelta = std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsTime).count();
            if (statsDelta >= 1) {
                PrintStats(engine, info, currentFrame, verbose);
                lastStatsTime = now;
            }
        }
        
        // Sleep to maintain timing (rough approximation)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    // Cleanup
    std::cout << "\nStopping engine..." << std::endl;
    engine.Stop();
    
    std::cout << "\n=== Session Summary ===\n";
    std::cout << "File:         " << filename << "\n";
    std::cout << "Frames sent:  " << currentFrame << " / " << info.totalFrames << "\n";
    // TODO: Add total packets sent, data rate
    
    std::cout << "\nDone." << std::endl;
    
    return 0;
}
