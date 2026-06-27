import XCTest
import ImageIO
import UniformTypeIdentifiers
@testable import Gem

final class PixelColorTests: XCTestCase {

    func testNormalizedComponentsMapToBytes() {
        XCTAssertEqual(PixelColor(red: 0, green: 0, blue: 0, alpha: 1),
                       PixelColor(a: 255, r: 0, g: 0, b: 0))
        XCTAssertEqual(PixelColor(red: 1, green: 1, blue: 1, alpha: 1),
                       PixelColor(a: 255, r: 255, g: 255, b: 255))
    }

    func testComponentsAreClampedAndRounded() {
        let p = PixelColor(red: -0.5, green: 2.0, blue: 0.5, alpha: 1)
        XCTAssertEqual(p, PixelColor(a: 255, r: 0, g: 255, b: 128))
    }
}

final class RenderUpdateTests: XCTestCase {

    func testEquatableStartedAndFinished() {
        XCTAssertEqual(RenderUpdate.started(width: 4, height: 3), .started(width: 4, height: 3))
        XCTAssertNotEqual(RenderUpdate.started(width: 4, height: 3), .started(width: 3, height: 4))
        XCTAssertNotEqual(RenderUpdate.finished, .started(width: 0, height: 0))
    }

    func testEquatableRowComparesPixels() {
        let a = RenderUpdate.row(y: 1, pixels: [PixelColor(r: 1, g: 2, b: 3)])
        let b = RenderUpdate.row(y: 1, pixels: [PixelColor(r: 1, g: 2, b: 3)])
        let c = RenderUpdate.row(y: 1, pixels: [PixelColor(r: 9, g: 2, b: 3)])
        XCTAssertEqual(a, b)
        XCTAssertNotEqual(a, c)
    }

    func testEquatableFailed() {
        XCTAssertEqual(RenderUpdate.failed(.sceneParseFailed), .failed(.sceneParseFailed))
        XCTAssertNotEqual(RenderUpdate.failed(.sceneParseFailed), .failed(.engineSetupFailed))
    }
}

final class RenderEngineTests: XCTestCase {

    private let unitBounds = CGRect(x: -1, y: 1, width: 2, height: 2)

    func testStreamsStartedThenRowsThenFinished() async {
        let engine = RenderEngine(renderer: TestRenderer())
        var updates: [RenderUpdate] = []
        let stream = engine.render(imageSize: NSSize(width: 4, height: 3),
                                   bounds: unitBounds, aspectMode: .none)
        for await update in stream { updates.append(update) }

        guard case .started(let w, let h)? = updates.first else {
            return XCTFail("expected .started first, got \(String(describing: updates.first))")
        }
        XCTAssertEqual(w, 4)
        XCTAssertEqual(h, 3)
        XCTAssertEqual(updates.last, .finished)

        let rows: [(Int, Int)] = updates.compactMap {
            if case .row(let y, let px) = $0 { return (y, px.count) }
            return nil
        }
        XCTAssertEqual(rows.map { $0.0 }, [0, 1, 2])
        XCTAssertTrue(rows.allSatisfy { $0.1 == 4 }, "every row should have width pixels")
    }

    func testCancellingConsumerStopsEarlyWithoutHanging() async {
        let engine = RenderEngine(renderer: TestRenderer())
        let stream = engine.render(imageSize: NSSize(width: 100, height: 100),
                                   bounds: unitBounds, aspectMode: .none)
        var rows = 0
        for await update in stream {
            if case .row = update {
                rows += 1
                if rows == 3 { break }   // terminates the stream -> engine task is cancelled
            }
        }
        XCTAssertEqual(rows, 3)          // reaching here proves it did not hang
    }
}

private final class FailingRenderer: Renderer {
    func setup(viewPortBounds: CGRect) throws { throw RenderError.sceneParseFailed }
    func color(at pointOnViewPort: CGPoint) -> PixelColor { PixelColor(r: 0, g: 0, b: 0) }
    func finished() {}
}

/// Regression test for the VM l-value double-free that corrupted the heap
/// (ASan: heap-use-after-free in vm_delete_lvalue, via symtab cleanup of a
/// parsed function argument). Drives the REAL C engine through several full
/// setup -> trace -> finished cycles using a scene that declares VM functions
/// with parameters. Best run with AddressSanitizer enabled on the scheme.
final class RayTraceRepeatedRenderTests: XCTestCase {

    /// Repo root derived from this source file's location:
    /// <root>/Mac/Gem/GemTests/GemTests.swift
    private static let repoRoot = URL(fileURLWithPath: #filePath)
        .deletingLastPathComponent().deletingLastPathComponent()
        .deletingLastPathComponent().deletingLastPathComponent().path

    override func setUp() {
        super.setUp()
        AppData.includeFilePaths = "\(Self.repoRoot)/scenes/library; \(Self.repoRoot)/scenes"
    }

    func testRepeatedRendersOfFunctionSceneDoNotCorruptHeap() throws {
        AppData.sceneFilePath = URL(fileURLWithPath: "\(Self.repoRoot)/scenes/Sierpinski.scn")
        let bounds = CGRect(x: -1, y: 1, width: 2, height: 2)
        let w = 24, h = 24
        for _ in 0 ..< 4 {
            let renderer = RayTraceRenderer()
            try renderer.setup(viewPortBounds: bounds)
            for yi in 0 ..< h {
                let cy = bounds.origin.y - (CGFloat(yi) / CGFloat(h) * bounds.height)
                for xi in 0 ..< w {
                    let cx = bounds.origin.x + (CGFloat(xi) / CGFloat(w) * bounds.width)
                    _ = renderer.color(at: CGPoint(x: cx, y: cy))
                }
            }
            renderer.finished()
        }
    }
}

final class RendererMigrationTests: XCTestCase {

    func testTestRendererIsDeterministicAndOpaque() throws {
        let renderer = TestRenderer()
        try renderer.setup(viewPortBounds: CGRect(x: -1, y: 1, width: 2, height: 2))
        let first = renderer.color(at: CGPoint(x: 0, y: 0))
        let second = renderer.color(at: CGPoint(x: 0, y: 0))
        XCTAssertEqual(first, second)
        XCTAssertEqual(first.a, 255)
    }

    func testEngineEmitsFailedWhenSetupThrows() async {
        let engine = RenderEngine(renderer: FailingRenderer())
        var updates: [RenderUpdate] = []
        let stream = engine.render(imageSize: NSSize(width: 4, height: 4),
                                   bounds: CGRect(x: -1, y: 1, width: 2, height: 2),
                                   aspectMode: .none)
        for await update in stream { updates.append(update) }
        XCTAssertEqual(updates, [.failed(.sceneParseFailed)])
    }
}

final class RenderImageBufferTests: XCTestCase {

    func testNewBufferIsOpaqueBlack() {
        let buf = RenderImageBuffer(width: 2, height: 2)
        XCTAssertEqual(buf.pixel(atX: 0, y: 0), PixelColor(a: 255, r: 0, g: 0, b: 0))
        XCTAssertEqual(buf.pixel(atX: 1, y: 1), PixelColor(a: 255, r: 0, g: 0, b: 0))
    }

    func testSetRowWritesPixelsReadableBack() {
        let buf = RenderImageBuffer(width: 2, height: 2)
        buf.setRow([PixelColor(r: 255, g: 0, b: 0), PixelColor(r: 0, g: 255, b: 0)], at: 1)
        XCTAssertEqual(buf.pixel(atX: 0, y: 1), PixelColor(a: 255, r: 255, g: 0, b: 0))
        XCTAssertEqual(buf.pixel(atX: 1, y: 1), PixelColor(a: 255, r: 0, g: 255, b: 0))
        XCTAssertEqual(buf.pixel(atX: 0, y: 0), PixelColor(a: 255, r: 0, g: 0, b: 0))
    }

    func testMakeCGImageHasBufferDimensions() {
        let img = RenderImageBuffer(width: 4, height: 3).makeCGImage()
        XCTAssertEqual(img?.width, 4)
        XCTAssertEqual(img?.height, 3)
    }

    func testZeroSizeMakesNoImage() {
        XCTAssertNil(RenderImageBuffer(width: 0, height: 0).makeCGImage())
    }
}

final class ImageEncodingTests: XCTestCase {

    func testEncodesPngThatDecodesToSameSize() {
        let buf = RenderImageBuffer(width: 5, height: 4)
        let image = try! XCTUnwrap(buf.makeCGImage())
        let data = try! XCTUnwrap(ImageEncoding.data(from: image, type: .png))
        XCTAssertFalse(data.isEmpty)
        let src = try! XCTUnwrap(CGImageSourceCreateWithData(data as CFData, nil))
        let decoded = try! XCTUnwrap(CGImageSourceCreateImageAtIndex(src, 0, nil))
        XCTAssertEqual(decoded.width, 5)
        XCTAssertEqual(decoded.height, 4)
    }
}

@MainActor
final class RenderModelTests: XCTestCase {

    private func makeModel(width: Int, height: Int) -> RenderModel {
        let model = RenderModel(renderer: TestRenderer())
        model.sceneURL = URL(fileURLWithPath: "/tmp/does-not-matter.scn")
        model.resolution = PixelSize(width: width, height: height)
        return model
    }

    func testStartDrivesToFinishedWithImage() async {
        let model = makeModel(width: 8, height: 6)
        XCTAssertTrue(model.canStart)
        model.start()
        await model.waitUntilIdle()

        guard case .finished = model.phase else {
            return XCTFail("expected .finished, got \(model.phase)")
        }
        XCTAssertEqual(model.progress, 1.0, accuracy: 0.0001)
        XCTAssertEqual(model.image?.width, 8)
        XCTAssertEqual(model.image?.height, 6)
    }

    func testStopPreventsFinish() async {
        let model = makeModel(width: 300, height: 300)
        model.start()
        model.stop()
        await model.waitUntilIdle()
        if case .finished = model.phase {
            XCTFail("render should not finish after immediate stop")
        }
    }

    func testCannotStartWithoutScene() {
        let model = RenderModel(renderer: TestRenderer())
        model.sceneURL = nil
        model.resolution = PixelSize(width: 8, height: 8)
        XCTAssertFalse(model.canStart)
    }
}
