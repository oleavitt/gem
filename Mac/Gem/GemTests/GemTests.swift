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

    func testInitFromCGColor() {
        let cg = CGColor(red: 1.0, green: 0.0, blue: 0.5, alpha: 1.0)
        XCTAssertEqual(PixelColor(cgColor: cg), PixelColor(a: 255, r: 255, g: 0, b: 128))
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
