//
//  GemCommands.swift
//  Gem
//

import SwiftUI

struct GemCommands: Commands {
    @Bindable var model: RenderModel

    var body: some Commands {
        CommandGroup(replacing: .newItem) {
            Button("Open Scene…") { model.isPresentingImporter = true }
                .keyboardShortcut("o")
            Button("Export Image…") { model.isPresentingExporter = true }
                .keyboardShortcut("s")
                .disabled(model.image == nil)
        }
        CommandMenu("Render") {
            Button("Start") { model.start() }
                .keyboardShortcut("r")
                .disabled(!model.canStart)
            Button("Stop") { model.stop() }
                .keyboardShortcut(".", modifiers: .command)
                .disabled(!model.isRendering)
        }
    }
}
