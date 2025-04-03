//
//  ViewPort.swift
//  Gem
//
//  Created by Oren Leavitt on 3/11/20.
//  Copyright © 2020 Oren Leavitt. All rights reserved.
//

import Foundation
import CoreGraphics

enum ViewPortAspectMode {
    case none, fit, fill
}

protocol ViewPort {
    func setup(delegate: ViewPortDelegate, imageSize: NSSize, viewPortBounds: CGRect, aspectMode: ViewPortAspectMode)
    func set(renderer: Renderer)
    func set(outputFile: OutputFile)
    func render()
}

protocol ViewPortDelegate {
    func viewPortRenderStarted(_ viewPort: ViewPort, imageWidth: Int, imageHeight: Int)
    func viewPort(_ viewPort: ViewPort, didSetPixel color: CGColor, atX x: Int, y:Int)
    func viewPortRenderEnded(_ viewPort: ViewPort)
}
