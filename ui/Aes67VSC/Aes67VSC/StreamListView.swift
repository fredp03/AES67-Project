// StreamListView.swift - Discovered streams list
// SPDX-License-Identifier: MIT

import SwiftUI

struct StreamListView: View {
    @ObservedObject var engineInterface: EngineInterface
    @State private var selectedStream: String?
    
    var body: some View {
        VStack(spacing: 0) {
            // Discovered streams header
            HStack {
                Text("Discovered Streams")
                    .font(.subheadline)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Button(action: { engineInterface.refreshStreams() }) {
                    Image(systemName: "arrow.clockwise")
                }
                .buttonStyle(.borderless)
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            
            Divider()
            
            // Stream list
            if engineInterface.discoveredStreams.isEmpty {
                VStack(spacing: 12) {
                    Image(systemName: "antenna.radiowaves.left.and.right.slash")
                        .font(.system(size: 48))
                        .foregroundColor(.secondary)
                    
                    Text("No streams discovered")
                        .font(.headline)
                    
                    Text("Waiting for SAP announcements...")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            } else {
                ScrollView {
                    LazyVStack(spacing: 0) {
                        ForEach(engineInterface.discoveredStreams, id: \.name) { stream in
                            StreamRow(stream: stream, isSelected: selectedStream == stream.name)
                                .onTapGesture {
                                    selectedStream = stream.name
                                }
                                .contextMenu {
                                    Button("Subscribe") {
                                        engineInterface.subscribeToStream(stream.name)
                                    }
                                    Button("Copy Multicast Address") {
                                        NSPasteboard.general.clearContents()
                                        NSPasteboard.general.setString(stream.multicastAddress, forType: .string)
                                    }
                                }
                        }
                    }
                }
            }
            
            Divider()
            
            // Subscribed streams
            if !engineInterface.subscribedStreams.isEmpty {
                VStack(spacing: 0) {
                    HStack {
                        Text("Subscribed Streams")
                            .font(.subheadline)
                            .foregroundColor(.secondary)
                        Spacer()
                    }
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    
                    Divider()
                    
                    ScrollView {
                        LazyVStack(spacing: 0) {
                            ForEach(engineInterface.subscribedStreams, id: \.self) { streamName in
                                SubscribedStreamRow(streamName: streamName) {
                                    engineInterface.unsubscribeFromStream(streamName)
                                }
                            }
                        }
                    }
                    .frame(maxHeight: 120)
                }
            }
        }
    }
}

struct StreamRow: View {
    let stream: DiscoveredStream
    let isSelected: Bool
    
    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: "waveform")
                .foregroundColor(.blue)
                .frame(width: 24)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(stream.name)
                    .font(.system(.body, design: .default))
                
                HStack(spacing: 8) {
                    Label("\(stream.channels) ch", systemImage: "speaker.wave.2")
                    Label(stream.multicastAddress, systemImage: "network")
                }
                .font(.caption)
                .foregroundColor(.secondary)
            }
            
            Spacer()
            
            VStack(alignment: .trailing, spacing: 2) {
                Text("\(stream.sampleRate / 1000) kHz")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                if stream.isActive {
                    Circle()
                        .fill(Color.green)
                        .frame(width: 6, height: 6)
                }
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(isSelected ? Color.blue.opacity(0.1) : Color.clear)
    }
}

struct SubscribedStreamRow: View {
    let streamName: String
    let onUnsubscribe: () -> Void
    
    var body: some View {
        HStack {
            Image(systemName: "checkmark.circle.fill")
                .foregroundColor(.green)
            
            Text(streamName)
                .font(.caption)
            
            Spacer()
            
            Button(action: onUnsubscribe) {
                Image(systemName: "xmark.circle")
            }
            .buttonStyle(.borderless)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
    }
}

#Preview {
    StreamListView(engineInterface: EngineInterface())
        .frame(width: 400, height: 400)
}
