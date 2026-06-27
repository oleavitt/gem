//
//  ImageDocument.swift
//  Gem
//

import SwiftUI
import UniformTypeIdentifiers

/// Wraps a rendered CGImage so SwiftUI's .fileExporter can write PNG/TIFF.
struct ImageDocument: FileDocument {
    static var readableContentTypes: [UTType] { [.png, .tiff] }
    static var writableContentTypes: [UTType] { [.png, .tiff] }

    let image: CGImage

    init(image: CGImage) { self.image = image }

    init(configuration: ReadConfiguration) throws {
        throw CocoaError(.fileReadCorruptFile)   // import not supported
    }

    func fileWrapper(configuration: WriteConfiguration) throws -> FileWrapper {
        let type: UTType = (configuration.contentType == .tiff) ? .tiff : .png
        guard let data = ImageEncoding.data(from: image, type: type) else {
            throw CocoaError(.fileWriteUnknown)
        }
        return FileWrapper(regularFileWithContents: data)
    }
}
