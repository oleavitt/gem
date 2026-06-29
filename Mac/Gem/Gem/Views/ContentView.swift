//
//  ContentView.swift
//  Gem
//

import SwiftUI
import UniformTypeIdentifiers

struct ContentView: View {
    @Environment(RenderModel.self) private var model

    var body: some View {
        @Bindable var model = model
        NavigationSplitView {
            SceneControlsView()
                .navigationSplitViewColumnWidth(min: 220, ideal: 260, max: 360)
        } detail: {
            RenderCanvasView()
        }
        .toolbar {
            ToolbarItemGroup {
                if model.isRendering {
                    Button("Stop", systemImage: "stop.fill") { model.stop() }
                    ProgressView(value: model.progress)
                        .frame(width: 120)
                } else {
                    Button("Render", systemImage: "play.fill") { model.start() }
                        .disabled(!model.canStart)
                }
                Button("Save Image…", systemImage: "square.and.arrow.down") {
                    model.isPresentingExporter = true
                }
                .disabled(model.image == nil)
                if case .finished(let elapsed) = model.phase {
                    Text(String(format: "%.3g s", elapsed))
                        .foregroundStyle(.secondary)
                }
            }
        }
        .fileImporter(isPresented: $model.isPresentingImporter,
                      allowedContentTypes: GemFileTypes.scene) { result in
            if case .success(let url) = result { model.sceneURL = url }
        }
        .fileExporter(isPresented: $model.isPresentingExporter,
                      document: model.image.map { ImageDocument(image: $0) },
                      contentType: .png,
                      defaultFilename: model.sceneURL?.deletingPathExtension().lastPathComponent ?? "render") { _ in }
        .alert("Render Failed", isPresented: $model.isShowingFailureAlert) {
            Button("OK", role: .cancel) {}
        } message: {
            Text(model.failureMessage)
        }
        .alert("Auto-Save Failed", isPresented: $model.isShowingAutoSaveError) {
            Button("OK", role: .cancel) {}
        } message: {
            Text(model.autoSaveError ?? "")
        }
    }
}
