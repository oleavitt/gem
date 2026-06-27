//
//  Checkerboard.swift
//  Gem
//

import SwiftUI

/// A checkerboard pattern of fixed-size squares, like the transparency backdrop
/// in image-editing software. Fills its bounds; partial edge squares are clipped.
struct Checkerboard: View {
    var squareSize: CGFloat = 10
    var light = Color(white: 0.5)
    var dark = Color(white: 0.25)

    var body: some View {
        Canvas { context, size in
            guard squareSize > 0 else { return }
            let cols = Int(ceil(size.width / squareSize))
            let rows = Int(ceil(size.height / squareSize))
            for row in 0 ..< max(rows, 0) {
                for col in 0 ..< max(cols, 0) {
                    let isDark = (row + col) % 2 == 1
                    let rect = CGRect(x: CGFloat(col) * squareSize,
                                      y: CGFloat(row) * squareSize,
                                      width: squareSize,
                                      height: squareSize)
                    context.fill(Path(rect), with: .color(isDark ? dark : light))
                }
            }
        }
    }
}
