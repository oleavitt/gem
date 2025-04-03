//
//  CGRect+Aspect.swift
//  Gem
//
//  Created by Oren Leavitt on 3/24/20.
//  Copyright © 2020 Oren Leavitt. All rights reserved.
//

import Foundation

extension CGRect {
    func adjustedFor(aspectMode: ViewPortAspectMode, targetSize: NSSize) -> CGRect {
        switch aspectMode {
        case .none:
            return self
        case .fit:
            if targetSize.width > targetSize.height {
                let aspectRatio = targetSize.width / targetSize.height
                let newWidth = size.width * aspectRatio
                return CGRect(x: origin.x - (newWidth - size.width) / 2.0, y: origin.y, width: newWidth, height: size.height)
            } else {
                let aspectRatio = targetSize.height / targetSize.width
                let newHeight = size.height * aspectRatio
                return CGRect(x: origin.x, y: origin.y - (newHeight - size.height) / 2.0, width: size.width, height: newHeight)
            }
        case .fill:
            if targetSize.width > targetSize.height {
                let aspectRatio = targetSize.height / targetSize.width
                let newHeight = size.height * aspectRatio
                return CGRect(x: origin.x, y: origin.y - (newHeight - size.height) / 2.0, width: size.width, height: newHeight)
            } else {
                let aspectRatio = targetSize.width / targetSize.height
                let newWidth = size.width * aspectRatio
                return CGRect(x: origin.x - (newWidth - size.width) / 2.0, y: origin.y, width: newWidth, height: size.height)
            }
        }
    }
}
