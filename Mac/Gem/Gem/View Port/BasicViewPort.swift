//
//  BasicViewPort.swift
//  Gem
//
//  Created by Oren Leavitt on 3/12/20.
//  Copyright © 2020 Oren Leavitt. All rights reserved.
//

import Foundation

class BasicViewPort: ViewPort {
    private var renderer: Renderer?
    private var outputFile: OutputFile?

    private var imageWidth = 0
    private var imageHeight = 0
    private var bounds = CGRect.zero
    private var delegate: ViewPortDelegate!
    private var aspectMode: ViewPortAspectMode = .none
    
    func setup(delegate: ViewPortDelegate, imageSize: NSSize, viewPortBounds: CGRect, aspectMode: ViewPortAspectMode) {
        self.imageWidth = Int(imageSize.width)
        self.imageHeight = Int(imageSize.height)
        self.bounds = viewPortBounds.adjustedFor(aspectMode: aspectMode, targetSize: imageSize)
        self.delegate = delegate
    }
    
    func set(renderer: Renderer) {
        self.renderer = renderer
    }
    
    func set(outputFile: OutputFile) {
        self.outputFile = outputFile
    }
    
    func render() {
        DispatchQueue.global().async { [weak self] in
            self?.startRender()
        }
    }
    
    private func startRender() {
        guard let renderer = renderer else { return }
        
        renderer.setup(viewPortBounds: bounds)
        DispatchQueue.main.async { [weak self] in
            guard let strongSelf = self else { return }
            strongSelf.delegate.viewPortRenderStarted(strongSelf, imageWidth: strongSelf.imageWidth, imageHeight: strongSelf.imageHeight)
        }
        for y in 0 ..< imageHeight {
            let currentY = bounds.origin.y - (CGFloat(y) / CGFloat(imageHeight) * bounds.height)
            for x in 0 ..< imageWidth {
                let currentX = bounds.origin.x + (CGFloat(x) / CGFloat(imageWidth) * bounds.width)
                let color = renderer.color(at: CGPoint(x: currentX, y: currentY))
                outputFile?.savePixel(color: color)
                DispatchQueue.main.async { [weak self] in
                    guard let strongSelf = self else { return }
                    strongSelf.delegate.viewPort(strongSelf, didSetPixel: color, atX: x, y: y)
                }
            }
        }
        renderer.finished()
        DispatchQueue.main.async { [weak self] in
            guard let strongSelf = self else { return }
            strongSelf.delegate.viewPortRenderEnded(strongSelf)
        }
    }
}
