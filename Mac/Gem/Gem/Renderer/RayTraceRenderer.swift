//
//  RayTraceRenderer.swift
//  Gem
//

import Foundation
import CoreGraphics

class RayTraceRenderer: Renderer {
    func setup(viewPortBounds: CGRect) throws {
        Ray_Initialize()
        var raySetupData = RaySetupData()
        Ray_GetSetup(&raySetupData)
        scn20_initialize()
        scn20_set_msgfn(Scn20StatusMessage)

        guard let sceneFilePath = AppData.sceneFilePath?.path else {
            throw RenderError.sceneParseFailed
        }
        let searchPaths = AppData.includeFilePaths ?? "/Users/orenleavitt/Workspace/gem/scenes/library; /Users/orenleavitt/Workspace/gem/scenes"

        let result = scn20_parse(sceneFilePath, &raySetupData, searchPaths)
        guard result == SCN_OK else {
            throw RenderError.sceneParseFailed
        }
        guard Ray_Setup(&raySetupData) != 0 else {
            throw RenderError.engineSetupFailed
        }
    }

    func color(at pointOnViewPort: CGPoint) -> PixelColor {
        var color = RGBA()
        Ray_TraceRayFromViewport(Double(pointOnViewPort.x), Double(pointOnViewPort.y), &color)
        return PixelColor(red: color.r, green: color.g, blue: color.b, alpha: color.a)
    }

    func finished() {
        scn20_close()
        Ray_Close()
    }
}
