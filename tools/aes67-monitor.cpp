// aes67-monitor.cpp - Web-based audio monitoring tool
// SPDX-License-Identifier: MIT

#include "NetworkEngine.h"
#include "AES67_RingBuffer.h"
#include <iostream>
#include <csignal>
#include <cstring>
#include <cmath>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <sstream>
#include <iomanip>

// Simple HTTP server includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

using namespace AES67;

static std::atomic<bool> g_running(true);
static NetworkEngine* g_engine = nullptr;

void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down...\n";
    g_running = false;
    if (g_engine) {
        g_engine->Stop();
    }
}

// Calculate RMS level from audio samples
float CalculateRMS(const int32_t* samples, size_t count) {
    if (count == 0) return 0.0f;
    
    double sum = 0.0;
    for (size_t i = 0; i < count; ++i) {
        double normalized = samples[i] / 2147483648.0; // Normalize int32 to [-1.0, 1.0]
        sum += normalized * normalized;
    }
    
    double rms = std::sqrt(sum / count);
    
    // Convert to dBFS
    if (rms < 1e-10) return -96.0f;
    return static_cast<float>(20.0 * std::log10(rms));
}

// Generate JSON status for web client
std::string GenerateStatusJSON(NetworkEngine& engine, uint32_t numChannels) {
    static int callCount = 0;
    callCount++;
    
    std::stringstream json;
    json << std::fixed << std::setprecision(2);
    
    json << "{\n";
    json << "  \"timestamp\": " << engine.GetPTPTimeNs() / 1000000 << ",\n";
    json << "  \"ptpLocked\": " << (engine.IsPTPLocked() ? "true" : "false") << ",\n";
    json << "  \"ptpOffset\": " << engine.GetPTPOffset() << ",\n";
    json << "  \"rateScalar\": " << engine.GetRateScalar() << ",\n";
    json << "  \"channels\": [\n";
    
    // Get audio levels for each channel
    AudioRingBuffer* ringBuffer = engine.GetInputRingBuffer(0);
    if (ringBuffer) {
        size_t available = ringBuffer->ReadAvailable();
        
        if (callCount % 10 == 0) {
            std::cerr << "GenerateStatusJSON: Ring buffer has " << available 
                      << " frames available\n";
        }
        
        // Read up to 512 frames of interleaved audio
        const size_t framesToRead = std::min(available, size_t(512));
        std::vector<int32_t> interleaved(framesToRead * numChannels);
        
        if (framesToRead > 0) {
            ringBuffer->Read(interleaved.data(), framesToRead * numChannels);
        }
        
        for (uint32_t ch = 0; ch < numChannels; ++ch) {
            // Extract this channel's samples from interleaved data
            std::vector<int32_t> channelSamples(framesToRead);
            for (size_t i = 0; i < framesToRead; ++i) {
                channelSamples[i] = interleaved[i * numChannels + ch];
            }
            
            float level = CalculateRMS(channelSamples.data(), framesToRead);
            
            json << "    {\"channel\": " << ch 
                 << ", \"level\": " << level 
                 << ", \"peak\": " << level << "}";
            
            if (ch < numChannels - 1) json << ",";
            json << "\n";
        }
    } else {
        // No ring buffer available
        for (uint32_t ch = 0; ch < numChannels; ++ch) {
            json << "    {\"channel\": " << ch 
                 << ", \"level\": -96.0"
                 << ", \"peak\": -96.0}";
            if (ch < numChannels - 1) json << ",";
            json << "\n";
        }
    }
    
    json << "  ],\n";
    
    // Add discovered streams
    json << "  \"streams\": [\n";
    auto streamNames = engine.GetDiscoveredStreamNames();
    for (size_t i = 0; i < streamNames.size(); ++i) {
        SDPSession session;
        if (engine.GetDiscoveredStream(streamNames[i], session)) {
            json << "    {\"name\": \"" << streamNames[i] << "\", "
                 << "\"address\": \"" << session.connectionAddr << "\", "
                 << "\"port\": " << session.port << "}";
            if (i < streamNames.size() - 1) json << ",";
            json << "\n";
        }
    }
    json << "  ]\n";
    
    json << "}\n";
    
    return json.str();
}

// Simple HTTP server handler
void HandleHTTPRequest(int clientSocket, NetworkEngine& engine, uint32_t numChannels) {
    char buffer[4096];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
        close(clientSocket);
        return;
    }
    
    buffer[bytesRead] = '\0';
    std::string request(buffer);
    
    // Parse request line
    if (request.find("GET / ") == 0 || request.find("GET /index.html") == 0) {
        // Serve HTML page
        std::string html = R"(<!DOCTYPE html>
<html>
<head>
    <title>AES67 Audio Monitor</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            margin: 0;
            padding: 20px;
            background: #1a1a1a;
            color: #e0e0e0;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        h1 {
            color: #4a9eff;
            margin-bottom: 10px;
        }
        .status {
            background: #2a2a2a;
            border-radius: 8px;
            padding: 15px;
            margin-bottom: 20px;
            border-left: 4px solid #4a9eff;
        }
        .status-item {
            display: inline-block;
            margin-right: 30px;
            margin-bottom: 10px;
        }
        .status-label {
            color: #888;
            font-size: 12px;
            text-transform: uppercase;
        }
        .status-value {
            font-size: 20px;
            font-weight: 500;
        }
        .ptp-locked { color: #4ade80; }
        .ptp-unlocked { color: #f87171; }
        .channels {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
            gap: 15px;
            margin-bottom: 30px;
        }
        .channel {
            background: #2a2a2a;
            border-radius: 8px;
            padding: 15px;
        }
        .channel-label {
            font-size: 14px;
            margin-bottom: 8px;
            color: #888;
        }
        .meter-container {
            background: #1a1a1a;
            height: 20px;
            border-radius: 4px;
            overflow: hidden;
            position: relative;
        }
        .meter-bar {
            height: 100%;
            transition: width 0.1s ease-out;
            background: linear-gradient(90deg, #4ade80 0%, #fbbf24 70%, #f87171 90%);
        }
        .meter-value {
            position: absolute;
            right: 8px;
            top: 2px;
            font-size: 12px;
            font-weight: 600;
            color: #fff;
            text-shadow: 0 1px 2px rgba(0,0,0,0.8);
        }
        .streams {
            background: #2a2a2a;
            border-radius: 8px;
            padding: 15px;
        }
        .streams h2 {
            margin-top: 0;
            color: #4a9eff;
            font-size: 18px;
        }
        .stream-item {
            padding: 10px;
            background: #1a1a1a;
            border-radius: 4px;
            margin-bottom: 8px;
        }
        .stream-name {
            font-weight: 500;
            margin-bottom: 4px;
        }
        .stream-info {
            font-size: 12px;
            color: #888;
        }
        .error {
            color: #f87171;
            padding: 15px;
            background: #2a2a2a;
            border-radius: 8px;
            border-left: 4px solid #f87171;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎵 AES67 Audio Monitor</h1>
        
        <div class="status" id="status">
            <div class="status-item">
                <div class="status-label">PTP Status</div>
                <div class="status-value" id="ptpStatus">Connecting...</div>
            </div>
            <div class="status-item">
                <div class="status-label">PTP Offset</div>
                <div class="status-value" id="ptpOffset">-- µs</div>
            </div>
            <div class="status-item">
                <div class="status-label">Rate Scalar</div>
                <div class="status-value" id="rateScalar">--</div>
            </div>
        </div>
        
        <div class="channels" id="channels"></div>
        
        <div class="streams">
            <h2>Discovered Streams</h2>
            <div id="streamsList">No streams discovered yet...</div>
        </div>
    </div>
    
    <script>
        let updateInterval = null;
        
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    // Update PTP status
                    const ptpStatus = document.getElementById('ptpStatus');
                    if (data.ptpLocked) {
                        ptpStatus.textContent = 'Locked';
                        ptpStatus.className = 'status-value ptp-locked';
                    } else {
                        ptpStatus.textContent = 'Unlocked';
                        ptpStatus.className = 'status-value ptp-unlocked';
                    }
                    
                    document.getElementById('ptpOffset').textContent = 
                        data.ptpOffset.toFixed(2) + ' µs';
                    document.getElementById('rateScalar').textContent = 
                        data.rateScalar.toFixed(6);
                    
                    // Update channel meters
                    const channelsDiv = document.getElementById('channels');
                    if (data.channels.length > 0) {
                        if (channelsDiv.children.length === 0) {
                            // Create channel elements
                            data.channels.forEach(ch => {
                                const div = document.createElement('div');
                                div.className = 'channel';
                                div.innerHTML = `
                                    <div class="channel-label">Channel ${ch.channel + 1}</div>
                                    <div class="meter-container">
                                        <div class="meter-bar" id="meter-${ch.channel}"></div>
                                        <div class="meter-value" id="value-${ch.channel}">-∞ dB</div>
                                    </div>
                                `;
                                channelsDiv.appendChild(div);
                            });
                        }
                        
                        // Update meter levels
                        data.channels.forEach(ch => {
                            const meter = document.getElementById(`meter-${ch.channel}`);
                            const value = document.getElementById(`value-${ch.channel}`);
                            
                            // Convert dBFS to percentage (0 dB = 100%, -96 dB = 0%)
                            let percentage = ((ch.level + 96) / 96) * 100;
                            percentage = Math.max(0, Math.min(100, percentage));
                            
                            meter.style.width = percentage + '%';
                            value.textContent = ch.level > -96 ? 
                                ch.level.toFixed(1) + ' dB' : '-∞ dB';
                        });
                    }
                    
                    // Update streams list
                    const streamsList = document.getElementById('streamsList');
                    if (data.streams.length > 0) {
                        streamsList.innerHTML = data.streams.map(stream => `
                            <div class="stream-item">
                                <div class="stream-name">${stream.name}</div>
                                <div class="stream-info">${stream.address}:${stream.port}</div>
                            </div>
                        `).join('');
                    } else {
                        streamsList.innerHTML = 'No streams discovered yet...';
                    }
                })
                .catch(error => {
                    console.error('Update failed:', error);
                });
        }
        
        // Start updates
        updateStatus();
        updateInterval = setInterval(updateStatus, 100); // 10 Hz updates
    </script>
</body>
</html>)";
        
        std::string response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "Content-Length: " + std::to_string(html.length()) + "\r\n"
                              "Connection: close\r\n"
                              "\r\n" + html;
        
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    else if (request.find("GET /status") == 0) {
        // Serve JSON status
        std::string json = GenerateStatusJSON(engine, numChannels);
        
        std::string response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: " + std::to_string(json.length()) + "\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Connection: close\r\n"
                              "\r\n" + json;
        
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    else {
        // 404 Not Found
        std::string response = "HTTP/1.1 404 Not Found\r\n"
                              "Content-Type: text/plain\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "Not Found\n";
        
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    
    close(clientSocket);
}

void PrintUsage(const char* progName) {
    std::cout << "AES67 Web-Based Audio Monitor\n\n";
    std::cout << "Usage: " << progName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -p, --port PORT      HTTP server port (default: 8080)\n";
    std::cout << "  -c, --channels NUM   Number of channels to monitor (default: 2)\n";
    std::cout << "  -i, --interface IF   Network interface (default: en0)\n";
    std::cout << "  -v, --verbose        Verbose output\n";
    std::cout << "  -h, --help           Show this help message\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << progName << " --port 8080 --channels 8\n\n";
    std::cout << "Then open http://localhost:8080 in your browser to view real-time audio levels.\n";
}

int main(int argc, char* argv[]) {
    uint16_t port = 8080;
    uint32_t numChannels = 2;
    std::string interface = "en0";
    bool verbose = false;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        
        if (arg == "-h" || arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        }
        else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                port = static_cast<uint16_t>(std::atoi(argv[++i]));
            }
        }
        else if (arg == "-c" || arg == "--channels") {
            if (i + 1 < argc) {
                numChannels = static_cast<uint32_t>(std::atoi(argv[++i]));
            }
        }
        else if (arg == "-i" || arg == "--interface") {
            if (i + 1 < argc) {
                interface = argv[++i];
            }
        }
        else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
    }
    
    // Install signal handlers
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    std::cout << "AES67 Web-Based Audio Monitor\n";
    std::cout << "==============================\n\n";
    
    // Initialize network engine
    NetworkEngine engine("../configs/engine.json");
    engine.SetNetworkInterface(interface);
    g_engine = &engine;
    
    std::cout << "Starting network engine...\n";
    if (!engine.Start()) {
        std::cerr << "Error: Failed to start network engine\n";
        return 1;
    }
    
    // Wait for PTP lock
    std::cout << "Waiting for PTP lock...\n";
    int attempts = 0;
    while (!engine.IsPTPLocked() && attempts < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ++attempts;
    }
    
    if (engine.IsPTPLocked()) {
        std::cout << "PTP locked! Offset: " << engine.GetPTPOffset() << " µs\n";
    } else {
        std::cout << "Warning: PTP not locked, continuing anyway...\n";
    }
    
    // Create HTTP server
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error: Failed to create socket\n";
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind to port
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Failed to bind to port " << port << "\n";
        close(serverSocket);
        return 1;
    }
    
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Error: Failed to listen on socket\n";
        close(serverSocket);
        return 1;
    }
    
    // Set non-blocking
    fcntl(serverSocket, F_SETFL, O_NONBLOCK);
    
    std::cout << "\n✓ Server running on http://localhost:" << port << "\n";
    std::cout << "  Monitoring " << numChannels << " channels\n";
    std::cout << "  Network interface: " << interface << "\n\n";
    std::cout << "Open the URL in your browser to view real-time audio levels.\n";
    std::cout << "Press Ctrl+C to stop.\n\n";
    
    // Main server loop
    while (g_running) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket >= 0) {
            if (verbose) {
                std::cout << "Connection from " 
                         << inet_ntoa(clientAddr.sin_addr) << "\n";
            }
            
            // Handle request in separate thread to avoid blocking
            std::thread([clientSocket, &engine, numChannels]() {
                HandleHTTPRequest(clientSocket, engine, numChannels);
            }).detach();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Cleanup
    close(serverSocket);
    engine.Stop();
    
    std::cout << "\nMonitor stopped.\n";
    
    return 0;
}
