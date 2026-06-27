//
//  GemApp.swift
//  Gem
//

import SwiftUI

struct GemApp: App {
    @State private var model = RenderModel()

    var body: some Scene {
        Window("Gem", id: "main") {
            ContentView()
                .environment(model)
        }
        .commands {
            GemCommands(model: model)
        }

        Settings {
            SettingsView()
        }
    }
}
