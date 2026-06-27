//
//  SceneControlsView.swift
//  Gem
//

import SwiftUI

struct SceneControlsView: View {
    @Environment(RenderModel.self) private var model

    var body: some View {
        @Bindable var model = model
        Form {
            Section("Scene") {
                LabeledContent("File") {
                    Text(model.sceneURL?.lastPathComponent ?? "None")
                        .foregroundStyle(model.sceneURL == nil ? .secondary : .primary)
                        .lineLimit(1)
                        .truncationMode(.middle)
                }
                Button("Choose…") { model.isPresentingImporter = true }
            }
            Section("Output Resolution") {
                LabeledContent("Width") {
                    TextField("Width", value: $model.resolution.width, format: .number)
                        .labelsHidden()
                        .frame(width: 90)
                }
                LabeledContent("Height") {
                    TextField("Height", value: $model.resolution.height, format: .number)
                        .labelsHidden()
                        .frame(width: 90)
                }
            }
        }
        .formStyle(.grouped)
    }
}
