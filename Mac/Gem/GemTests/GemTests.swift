import XCTest
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
