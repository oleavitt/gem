//
//  PixelSize.swift
//  Gem
//

import Foundation

struct PixelSize: Equatable, Sendable {
    var width: Int
    var height: Int

    static let `default` = PixelSize(width: 200, height: 200)

    var nsSize: NSSize { NSSize(width: CGFloat(width), height: CGFloat(height)) }
    var isRenderable: Bool { width > 0 && height > 0 }
}
