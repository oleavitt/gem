//
//  RenderModel.swift
//  Gem
//

import Foundation
import CoreGraphics
import Observation

@MainActor
@Observable
final class RenderModel {

    enum Phase: Equatable {
        case idle
        case rendering(progress: Double)
        case finished(elapsed: TimeInterval)
        case failed(RenderError)
    }

    var sceneURL: URL?
    var resolution: PixelSize
    var isPresentingImporter = false
    var isPresentingExporter = false

    private(set) var phase: Phase = .idle
    private(set) var image: CGImage?

    private let renderer: Renderer
    private var renderTask: Task<Void, Never>?

    init(renderer: Renderer = RayTraceRenderer()) {
        self.renderer = renderer
        self.sceneURL = AppData.sceneFilePath
        let stored = AppData.outputResolution
        self.resolution = PixelSize(width: Int(stored.width), height: Int(stored.height))
    }

    var isRendering: Bool {
        if case .rendering = phase { return true }
        return false
    }

    var canStart: Bool { sceneURL != nil && resolution.isRenderable && !isRendering }

    var progress: Double {
        switch phase {
        case .rendering(let p): return p
        case .finished: return 1.0
        default: return 0.0
        }
    }

    var failureMessage: String {
        if case .failed(let error) = phase { return String(describing: error) }
        return ""
    }

    var isShowingFailureAlert: Bool {
        get { if case .failed = phase { return true } else { return false } }
        set { if !newValue, case .failed = phase { phase = .idle } }
    }

    func start() {
        guard canStart else { return }
        AppData.sceneFilePath = sceneURL
        AppData.outputResolution = resolution.nsSize

        renderTask?.cancel()

        let size = resolution.nsSize
        let height = resolution.height
        let buffer = RenderImageBuffer(width: resolution.width, height: resolution.height)
        image = buffer.makeCGImage()
        phase = .rendering(progress: 0)

        let engine = RenderEngine(renderer: renderer)
        let startTime = Date()

        renderTask = Task { @MainActor [weak self] in
            var rowsSinceRefresh = 0
            let stream = engine.render(imageSize: size,
                                       bounds: CGRect(x: -1, y: 1, width: 2, height: 2),
                                       aspectMode: .fit)
            for await update in stream {
                guard let self else { break }
                switch update {
                case .started:
                    break
                case .row(let y, let pixels):
                    buffer.setRow(pixels, at: y)
                    rowsSinceRefresh += 1
                    if rowsSinceRefresh >= 16 {
                        rowsSinceRefresh = 0
                        self.image = buffer.makeCGImage()
                    }
                    if height > 0 {
                        self.phase = .rendering(progress: Double(y + 1) / Double(height))
                    }
                case .failed(let error):
                    self.image = buffer.makeCGImage()
                    self.phase = .failed(error)
                case .finished:
                    self.image = buffer.makeCGImage()
                    self.phase = .finished(elapsed: -startTime.timeIntervalSinceNow)
                }
            }
        }
    }

    func stop() {
        renderTask?.cancel()
    }

    /// Awaits the current render task (used by tests and on quit).
    func waitUntilIdle() async {
        await renderTask?.value
    }
}
