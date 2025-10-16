// ContentView.swift - Main menu bar popover content
// SPDX-License-Identifier: MIT

import SwiftUI

struct ContentView: View {
    @ObservedObject var engineInterface: EngineInterface
    @State private var selectedTab = 0
    
    var body: some View {
        VStack(spacing: 0) {
            // Header with status
            HeaderView(engineInterface: engineInterface)
            
            Divider()
            
            // Tab bar
            Picker("", selection: $selectedTab) {
                Text("Streams").tag(0)
                Text("Status").tag(1)
                Text("Settings").tag(2)
            }
            .pickerStyle(.segmented)
            .padding([.horizontal, .top], 12)
            
            // Content
            TabView(selection: $selectedTab) {
                StreamListView(engineInterface: engineInterface)
                    .tag(0)
                
                StatusView(engineInterface: engineInterface)
                    .tag(1)
                
                SettingsView(engineInterface: engineInterface)
                    .tag(2)
            }
            .frame(height: 400)
            
            Divider()
            
            // Footer
            FooterView()
        }
        .frame(width: 400)
    }
}

struct HeaderView: View {
    @ObservedObject var engineInterface: EngineInterface
    
    var body: some View {
        HStack {
            Image(systemName: ptpIcon)
                .foregroundColor(ptpColor)
                .font(.system(size: 16))
            
            VStack(alignment: .leading, spacing: 2) {
                Text("AES67 Virtual Soundcard")
                    .font(.headline)
                Text(statusText)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            Spacer()
            
            if engineInterface.isEngineRunning {
                Circle()
                    .fill(Color.green)
                    .frame(width: 8, height: 8)
            } else {
                Circle()
                    .fill(Color.red)
                    .frame(width: 8, height: 8)
            }
        }
        .padding(12)
        .background(Color(NSColor.controlBackgroundColor))
    }
    
    private var ptpIcon: String {
        engineInterface.isPTPLocked ? "clock.fill" : "clock"
    }
    
    private var ptpColor: Color {
        engineInterface.isPTPLocked ? .green : .orange
    }
    
    private var statusText: String {
        if !engineInterface.isEngineRunning {
            return "Engine Stopped"
        } else if engineInterface.isPTPLocked {
            return String(format: "PTP Locked • Offset: %.1f µs", engineInterface.ptpOffsetNs / 1000.0)
        } else {
            return "PTP Unlocked • Synchronizing..."
        }
    }
}

struct StatusView: View {
    @ObservedObject var engineInterface: EngineInterface
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                StatusSection(title: "PTP Synchronization") {
                    StatusRow(label: "Status", value: engineInterface.isPTPLocked ? "Locked" : "Unlocked")
                    StatusRow(label: "Offset", value: String(format: "%.2f µs", engineInterface.ptpOffsetNs / 1000.0))
                    StatusRow(label: "Rate Ratio", value: String(format: "%.9f", engineInterface.ptpRateRatio))
                }
                
                StatusSection(title: "Network Statistics") {
                    StatusRow(label: "Active Streams", value: "\(engineInterface.activeStreamCount)")
                    StatusRow(label: "RX Packets", value: "\(engineInterface.rxPacketCount)")
                    StatusRow(label: "TX Packets", value: "\(engineInterface.txPacketCount)")
                    StatusRow(label: "Packet Loss", value: "\(engineInterface.packetLossCount)")
                }
                
                StatusSection(title: "Audio Performance") {
                    StatusRow(label: "Sample Rate", value: "48000 Hz")
                    StatusRow(label: "Buffer Underruns", value: "\(engineInterface.underrunCount)")
                    StatusRow(label: "Buffer Overruns", value: "\(engineInterface.overrunCount)")
                }
                
                Spacer()
            }
            .padding()
        }
    }
}

struct StatusSection<Content: View>: View {
    let title: String
    @ViewBuilder let content: Content
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text(title)
                .font(.headline)
            
            VStack(alignment: .leading, spacing: 4) {
                content
            }
            .padding(8)
            .background(Color(NSColor.controlBackgroundColor))
            .cornerRadius(6)
        }
    }
}

struct StatusRow: View {
    let label: String
    let value: String
    
    var body: some View {
        HStack {
            Text(label)
                .foregroundColor(.secondary)
            Spacer()
            Text(value)
                .font(.system(.body, design: .monospaced))
        }
        .font(.caption)
    }
}

struct SettingsView: View {
    @ObservedObject var engineInterface: EngineInterface
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Network Interface")
                        .font(.headline)
                    
                    Picker("Interface", selection: $engineInterface.selectedInterface) {
                        ForEach(engineInterface.availableInterfaces, id: \.self) { interface in
                            Text(interface).tag(interface)
                        }
                    }
                }
                
                VStack(alignment: .leading, spacing: 8) {
                    Text("Engine Control")
                        .font(.headline)
                    
                    Button(engineInterface.isEngineRunning ? "Stop Engine" : "Start Engine") {
                        if engineInterface.isEngineRunning {
                            engineInterface.stopEngine()
                        } else {
                            engineInterface.startEngine()
                        }
                    }
                }
                
                Divider()
                
                VStack(alignment: .leading, spacing: 8) {
                    Text("About")
                        .font(.headline)
                    
                    Text("AES67 Virtual Soundcard v1.0")
                        .font(.caption)
                    Text("64 channels @ 48 kHz, 24-bit")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                
                Spacer()
            }
            .padding()
        }
    }
}

struct FooterView: View {
    var body: some View {
        HStack {
            Text("© 2025 AES67 VSC")
                .font(.caption2)
                .foregroundColor(.secondary)
            
            Spacer()
            
            Button("Quit") {
                NSApplication.shared.terminate(nil)
            }
            .buttonStyle(.borderless)
            .font(.caption)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(Color(NSColor.controlBackgroundColor))
    }
}

#Preview {
    ContentView(engineInterface: EngineInterface())
        .frame(width: 400, height: 500)
}
