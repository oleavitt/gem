//
//  RenderCanvasView.swift
//  Gem
//

import SwiftUI

struct RenderCanvasView: View {
    @Environment(RenderModel.self) private var model

    var body: some View {
        Group {
            if let image = model.image {
                ScrollView([.horizontal, .vertical]) {
                    Image(decorative: image, scale: 1.0)
                        .interpolation(.none)
                        .background { Checkerboard() }
                }
            } else {
                ContentUnavailableView("No Render",
                                       systemImage: "photo",
                                       description: Text("Choose a scene and press Render."))
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(nsColor: .windowBackgroundColor))
    }
}
