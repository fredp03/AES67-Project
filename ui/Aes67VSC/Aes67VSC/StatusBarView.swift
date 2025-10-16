// StatusBarView.swift - Menu bar controller
// SPDX-License-Identifier: MIT

import SwiftUI
import AppKit

class StatusBarController {
    private var statusItem: NSStatusItem
    private var popover: NSPopover
    private var engineInterface: EngineInterface
    
    init() {
        statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
        popover = NSPopover()
        engineInterface = EngineInterface()
        
        setupStatusItem()
        setupPopover()
        
        // Start engine automatically
        engineInterface.startEngine()
    }
    
    private func setupStatusItem() {
        if let button = statusItem.button {
            button.image = NSImage(systemSymbolName: "waveform.circle", accessibilityDescription: "AES67")
            button.action = #selector(togglePopover)
            button.target = self
        }
    }
    
    private func setupPopover() {
        popover.contentSize = NSSize(width: 400, height: 500)
        popover.behavior = .transient
        popover.contentViewController = NSHostingController(rootView: ContentView(engineInterface: engineInterface))
    }
    
    @objc func togglePopover() {
        if let button = statusItem.button {
            if popover.isShown {
                popover.performClose(nil)
            } else {
                popover.show(relativeTo: button.bounds, of: button, preferredEdge: .minY)
            }
        }
    }
}
