//
//  RenderImageBuffer.swift
//  Gem
//

import Foundation
import CoreGraphics

/// Accumulates streamed pixel rows into a byte buffer in PixelColor's (a,r,g,b)
/// order and produces a CGImage. Not thread-safe; use from one actor/thread.
final class RenderImageBuffer {
    let width: Int
    let height: Int
    private let bytesPerPixel = 4
    private var bytes: [UInt8]

    init(width: Int, height: Int) {
        self.width = max(0, width)
        self.height = max(0, height)
        bytes = [UInt8](repeating: 0, count: self.width * self.height * bytesPerPixel)
        var i = 0
        while i < bytes.count {
            bytes[i] = 0   // initialize to black and fully transparent
            i += bytesPerPixel
        }
    }

    func setRow(_ pixels: [PixelColor], at y: Int) {
        guard y >= 0, y < height else { return }
        let rowStart = y * width * bytesPerPixel
        let count = min(pixels.count, width)
        for x in 0 ..< count {
            let p = pixels[x]
            let o = rowStart + x * bytesPerPixel
            bytes[o + 0] = p.a
            bytes[o + 1] = p.r
            bytes[o + 2] = p.g
            bytes[o + 3] = p.b
        }
    }

    func pixel(atX x: Int, y: Int) -> PixelColor? {
        guard x >= 0, x < width, y >= 0, y < height else { return nil }
        let o = (y * width + x) * bytesPerPixel
        return PixelColor(a: bytes[o], r: bytes[o + 1], g: bytes[o + 2], b: bytes[o + 3])
    }

    func makeCGImage() -> CGImage? {
        guard width > 0, height > 0 else { return nil }
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        // Bytes are (a,r,g,b) big-endian => ARGB.
        let bitmapInfo = CGBitmapInfo(
            rawValue: CGImageAlphaInfo.premultipliedFirst.rawValue
                | CGBitmapInfo.byteOrder32Big.rawValue)
        guard let provider = CGDataProvider(data: Data(bytes) as CFData) else { return nil }
        return CGImage(width: width,
                       height: height,
                       bitsPerComponent: 8,
                       bitsPerPixel: 32,
                       bytesPerRow: width * bytesPerPixel,
                       space: colorSpace,
                       bitmapInfo: bitmapInfo,
                       provider: provider,
                       decode: nil,
                       shouldInterpolate: false,
                       intent: .defaultIntent)
    }
}
