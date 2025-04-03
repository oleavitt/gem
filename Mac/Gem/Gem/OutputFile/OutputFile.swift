//
//  OutputFile.swift
//  Gem
//
//  Created by Oren Leavitt on 3/12/20.
//  Copyright © 2020 Oren Leavitt. All rights reserved.
//

import Foundation
import CoreGraphics

protocol OutputFile {
    func create(name: String, width: Int, height: Int)
    func resume(name: String)
    func savePixel(color: CGColor)
}
