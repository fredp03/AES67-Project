// Aes67VSCApp.swift - Main menu bar application
// SPDX-License-Identifier: MIT

import SwiftUI

@main
struct Aes67VSCApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate
    
    var body: some Scene {
        Settings {
            EmptyView()
        }
    }
}

class AppDelegate: NSObject, NSApplicationDelegate {
    var statusBar: StatusBarController?
    
    func applicationDidFinishLaunching(_ notification: Notification) {
        // Create menu bar item
        statusBar = StatusBarController()
    }
}
