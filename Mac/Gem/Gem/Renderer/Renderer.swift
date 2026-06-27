//
//  Renderer.swift
//  Gem
//

import CoreGraphics

protocol Renderer: AnyObject {
    func setup(viewPortBounds: CGRect) throws
    func color(at pointOnViewPort: CGPoint) -> PixelColor
    func finished()
}
