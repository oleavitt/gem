//
//  RenderTypes.swift
//  Gem
//

import Foundation

enum ViewPortAspectMode {
    case none, fit, fill
}

enum RenderError: Error, Equatable, Sendable {
    case sceneParseFailed
    case engineSetupFailed
    case engineFailure(String)
}

enum RenderUpdate: Sendable, Equatable {
    case started(width: Int, height: Int)
    case row(y: Int, pixels: [PixelColor])
    case failed(RenderError)
    case finished
}
