//
//  GemFileTypes.swift
//  Gem
//

import UniformTypeIdentifiers

enum GemFileTypes {
    /// Allowed types when opening a scene. `.scn` has no registered system type,
    /// so fall back to a dynamic type plus plain data/text.
    static let scene: [UTType] = {
        var types: [UTType] = [.data, .plainText]
        if let scn = UTType(filenameExtension: "scn") { types.insert(scn, at: 0) }
        return types
    }()
}
