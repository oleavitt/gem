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
