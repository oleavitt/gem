//
//  TestRenderer.swift
//  Gem
//
//  Created by Oren Leavitt on 3/11/20.
//  Copyright © 2020 Oren Leavitt. All rights reserved.
//

import Foundation
import CoreGraphics

class TestRenderer: Renderer {
    private var bounds: CGRect = CGRect.zero
    private let radiusSquared: CGFloat = 3.0
    
    func setup(viewPortBounds: CGRect) {
        bounds = viewPortBounds
    }

    func color(at pointOnViewPort: CGPoint) -> CGColor {
        var dx = pointOnViewPort.x
        var dy = pointOnViewPort.y - 0.875
        let redDistSquared = dx * dx + dy * dy
        if redDistSquared < radiusSquared {
            dx = pointOnViewPort.x - 0.866
            dy = pointOnViewPort.y + 0.625
            let greenDistSquared = dx * dx + dy * dy
            if greenDistSquared < radiusSquared {
                dx = pointOnViewPort.x + 0.866
                let blueDistSquared = dx * dx + dy * dy
                if blueDistSquared < radiusSquared {
                    return CGColor(red: 1.0 - redDistSquared / radiusSquared,
                                   green: 1.0 - greenDistSquared / radiusSquared,
                                   blue: 1.0 - blueDistSquared / radiusSquared,
                                   alpha: 1.0)
                }
            }
        }

        dx = pointOnViewPort.x + 1.0
        dy = 1.0 - pointOnViewPort.y
        let gray = min(dx, dy) / 2.0
        return CGColor(red: gray,
                       green: gray,
                       blue: gray,
                       alpha: 1.0)
    }
}
