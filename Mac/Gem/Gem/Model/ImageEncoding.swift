//
//  ImageEncoding.swift
//  Gem
//

import Foundation
import CoreGraphics
import ImageIO
import UniformTypeIdentifiers

enum ImageEncoding {
    /// Encodes a CGImage to PNG or TIFF data. Returns nil on failure.
    static func data(from image: CGImage, type: UTType) -> Data? {
        let out = NSMutableData()
        guard let dest = CGImageDestinationCreateWithData(
            out as CFMutableData, type.identifier as CFString, 1, nil) else {
            return nil
        }
        CGImageDestinationAddImage(dest, image, nil)
        guard CGImageDestinationFinalize(dest) else { return nil }
        return out as Data
    }
}
