//
//  Renderer.swift
//  Gem
//
//  Created by Oren Leavitt on 3/11/20.
//  Copyright © 2020 Oren Leavitt. All rights reserved.
//

import Foundation
import CoreGraphics

protocol Renderer {
    func setup(viewPortBounds: CGRect)
    func start()
    func stop()
    func color(at pointOnViewPort: CGPoint) -> CGColor
    func finished()
}

extension Renderer {

    func start() {
        
    }
    
    func stop() {
        
    }
    
    func finished() {
        
    }
}
