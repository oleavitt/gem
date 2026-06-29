//
//  SettingsView.swift
//  Gem
//

import SwiftUI

struct SettingsView: View {
    // Maps to AppData.includeFilePaths (UserDefaults key "gem.includeFilePaths").
    @AppStorage("gem.includeFilePaths") private var includePaths: String = ""
    // Maps to AppData.autoSaveEnabled (UserDefaults key "gem.autoSaveEnabled").
    @AppStorage("gem.autoSaveEnabled") private var autoSaveEnabled: Bool = true

    var body: some View {
        Form {
            Section("Include / Search Paths") {
                TextField("Semicolon-separated paths", text: $includePaths, axis: .vertical)
                    .lineLimit(3...8)
            }
            Section("Output") {
                Toggle("Automatically save finished renders", isOn: $autoSaveEnabled)
                Text("Saves a PNG next to the scene file, named after the scene.")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
        .formStyle(.grouped)
        .frame(width: 460, height: 300)
    }
}
