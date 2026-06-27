//
//  SettingsView.swift
//  Gem
//

import SwiftUI

struct SettingsView: View {
    // Maps to AppData.includeFilePaths (UserDefaults key "gem.includeFilePaths").
    @AppStorage("gem.includeFilePaths") private var includePaths: String = ""

    var body: some View {
        Form {
            Section("Include / Search Paths") {
                TextField("Semicolon-separated paths", text: $includePaths, axis: .vertical)
                    .lineLimit(3...8)
            }
        }
        .formStyle(.grouped)
        .frame(width: 460, height: 240)
    }
}
