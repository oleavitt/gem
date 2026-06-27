//
//  RenderEngine.swift
//  Gem
//

import Foundation
import CoreGraphics

/// Serializes all access to the (single, global, non-thread-safe) C render
/// engine and streams results one row at a time.
actor RenderEngine {
    private let renderer: Renderer
    private let outputFile: OutputFile?

    init(renderer: Renderer, outputFile: OutputFile? = nil) {
        self.renderer = renderer
        self.outputFile = outputFile
    }

    /// Starts a render and returns a stream of updates. The work runs on the
    /// actor (off the main thread); breaking/cancelling the consuming task
    /// cancels the render.
    nonisolated func render(imageSize: NSSize,
                            bounds viewPortBounds: CGRect,
                            aspectMode: ViewPortAspectMode) -> AsyncStream<RenderUpdate> {
        AsyncStream { continuation in
            let task = Task {
                await self.runRender(into: continuation,
                                     imageSize: imageSize,
                                     viewPortBounds: viewPortBounds,
                                     aspectMode: aspectMode)
            }
            continuation.onTermination = { _ in task.cancel() }
        }
    }

    // Actor-isolated: two renders can never touch the C engine concurrently.
    // Contains no `await`, so the actor is not re-entered mid-frame.
    private func runRender(into continuation: AsyncStream<RenderUpdate>.Continuation,
                           imageSize: NSSize,
                           viewPortBounds: CGRect,
                           aspectMode: ViewPortAspectMode) {
        let width = Int(imageSize.width)
        let height = Int(imageSize.height)
        let bounds = viewPortBounds.adjustedFor(aspectMode: aspectMode, targetSize: imageSize)

        renderer.setup(viewPortBounds: bounds)
        defer { renderer.finished() }

        continuation.yield(.started(width: width, height: height))

        for y in 0 ..< height {
            if Task.isCancelled { break }
            let currentY = bounds.origin.y - (CGFloat(y) / CGFloat(height) * bounds.height)
            var rowPixels = [PixelColor]()
            rowPixels.reserveCapacity(width)
            for x in 0 ..< width {
                let currentX = bounds.origin.x + (CGFloat(x) / CGFloat(width) * bounds.width)
                let cgColor = renderer.color(at: CGPoint(x: currentX, y: currentY))
                rowPixels.append(PixelColor(cgColor: cgColor))
                outputFile?.savePixel(color: cgColor)
            }
            continuation.yield(.row(y: y, pixels: rowPixels))
        }
        continuation.yield(.finished)
        continuation.finish()
    }
}
