//
//  RayTraceRenderer.swift
//  Gem
//
//  Created by Oren Leavitt on 4/9/20.
//  Copyright © 2020 Gem. All rights reserved.
//

import Foundation
import CoreGraphics

class RayTraceRenderer: Renderer {
    func setup(viewPortBounds: CGRect) {
        // TODO: setup protocol should include a success/fail Bool return result
        Ray_Initialize()
        var raySetupData = RaySetupData()
        Ray_GetSetup(&raySetupData)
        scn20_initialize()
        scn20_set_msgfn(Scn20StatusMessage)
        
        // TODO: Need way to pass parameters to setup. A Dictionary?
        guard let sceneFilePath = AppData.sceneFilePath?.path else {
            return
        }
        let searchPaths = AppData.includeFilePaths ?? "/Users/orenleavitt/Workspace/gem/scenes/library; /Users/orenleavitt/Workspace/gem/scenes"

        let result = scn20_parse(sceneFilePath, &raySetupData, searchPaths)
        guard result == SCN_OK else {
            return // false
        }
        
        guard Ray_Setup(&raySetupData) != 0 else {
            return // false
        }
        // return true
    }
    
    func color(at pointOnViewPort: CGPoint) -> CGColor {
        var color = Vec3()
        Ray_TraceRayFromViewport(Double(pointOnViewPort.x), Double(pointOnViewPort.y), &color)
        return CGColor(red: CGFloat(color.x), green: CGFloat(color.y), blue: CGFloat(color.z), alpha: 1.0)
    }
    
    func finished() {
        scn20_close()
        Ray_Close()
    }
}
