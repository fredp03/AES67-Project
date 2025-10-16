// EngineInterface.swift - Swift interface to C++ engine
// SPDX-License-Identifier: MIT

import Foundation
import Combine

// Model for discovered stream
struct DiscoveredStream {
    let name: String
    let multicastAddress: String
    let channels: Int
    let sampleRate: Int
    let isActive: Bool
}

class EngineInterface: ObservableObject {
    // Engine state
    @Published var isEngineRunning = false
    @Published var isPTPLocked = false
    @Published var ptpOffsetNs: Double = 0.0
    @Published var ptpRateRatio: Double = 1.0
    
    // Stream discovery
    @Published var discoveredStreams: [DiscoveredStream] = []
    @Published var subscribedStreams: [String] = []
    
    // Statistics
    @Published var activeStreamCount: Int = 0
    @Published var rxPacketCount: Int = 0
    @Published var txPacketCount: Int = 0
    @Published var packetLossCount: Int = 0
    @Published var underrunCount: Int = 0
    @Published var overrunCount: Int = 0
    
    // Settings
    @Published var selectedInterface = "en0"
    @Published var availableInterfaces = ["en0", "en1", "en2"]
    
    private var timer: Timer?
    
    init() {
        // Start periodic updates
        timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.updateStatus()
        }
    }
    
    deinit {
        timer?.invalidate()
        stopEngine()
    }
    
    // MARK: - Engine Control
    
    func startEngine() {
        // TODO: Call C++ NetworkEngine::Start()
        // For now, simulate engine running
        isEngineRunning = true
        
        // Simulate some discovered streams
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) { [weak self] in
            self?.discoveredStreams = [
                DiscoveredStream(
                    name: "Mac Studio Audio",
                    multicastAddress: "239.69.1.1:5004",
                    channels: 8,
                    sampleRate: 48000,
                    isActive: true
                ),
                DiscoveredStream(
                    name: "Console Mix",
                    multicastAddress: "239.69.1.2:5004",
                    channels: 2,
                    sampleRate: 48000,
                    isActive: true
                )
            ]
        }
        
        // Simulate PTP locking
        DispatchQueue.main.asyncAfter(deadline: .now() + 5.0) { [weak self] in
            self?.isPTPLocked = true
        }
    }
    
    func stopEngine() {
        // TODO: Call C++ NetworkEngine::Stop()
        isEngineRunning = false
        isPTPLocked = false
        discoveredStreams = []
        subscribedStreams = []
    }
    
    // MARK: - Stream Management
    
    func refreshStreams() {
        // TODO: Call C++ NetworkEngine::GetDiscoveredStreamNames()
        // For now, just update the timestamp
        objectWillChange.send()
    }
    
    func subscribeToStream(_ streamName: String) {
        guard !subscribedStreams.contains(streamName) else { return }
        
        // TODO: Call C++ NetworkEngine::SubscribeToStream()
        subscribedStreams.append(streamName)
        activeStreamCount += 1
    }
    
    func unsubscribeFromStream(_ streamName: String) {
        // TODO: Call C++ NetworkEngine::UnsubscribeFromStream()
        subscribedStreams.removeAll { $0 == streamName }
        if activeStreamCount > 0 {
            activeStreamCount -= 1
        }
    }
    
    // MARK: - Status Updates
    
    private func updateStatus() {
        guard isEngineRunning else { return }
        
        // TODO: Call C++ engine methods to get real status
        // For now, simulate some activity
        
        if isPTPLocked {
            // Simulate PTP offset drift
            ptpOffsetNs = Double.random(in: -1000...1000)
            ptpRateRatio = 1.0 + Double.random(in: -0.00001...0.00001)
        }
        
        // Simulate packet counts
        if activeStreamCount > 0 {
            rxPacketCount += Int.random(in: 50...150)
            txPacketCount += Int.random(in: 50...150)
            
            // Occasional packet loss
            if Int.random(in: 0...100) > 95 {
                packetLossCount += 1
            }
        }
    }
}
