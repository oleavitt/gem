//
//  PixelColor.swift
//  Gem
//

import CoreGraphics

/// A Sendable RGBA pixel. Byte order (a, r, g, b) matches
/// `NSBitmapImageRep` created with `.alphaFirst`.
struct PixelColor: Sendable, Equatable {
    var a: UInt8
    var r: UInt8
    var g: UInt8
    var b: UInt8

    init(a: UInt8 = 255, r: UInt8, g: UInt8, b: UInt8) {
        self.a = a
        self.r = r
        self.g = g
        self.b = b
    }

    /// Build from normalized 0...1 components, clamping and rounding to 0...255.
    init(red: Double, green: Double, blue: Double, alpha: Double = 1.0) {
        func byte(_ value: Double) -> UInt8 {
            let clamped = min(max(value, 0.0), 1.0)
            return UInt8((clamped * 255.0).rounded())
        }
        self.a = byte(alpha)
        self.r = byte(red)
        self.g = byte(green)
        self.b = byte(blue)
    }
}
