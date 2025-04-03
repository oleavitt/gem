//
//  BinaryInteger+String.swift
//  Gem
//
//  Created by Oren Leavitt on 4/17/20.
//  Copyright © 2020 Gem. All rights reserved.
//

import Foundation

extension BinaryInteger {
    
    var binaryDescription: String {
        var binaryString = ""
        var internalNumber = self
        var counter = 0
        
        for _ in (1...self.bitWidth) {
            binaryString.insert(contentsOf: "\(internalNumber & 1)", at: binaryString.startIndex)
            internalNumber >>= 1
            counter += 1
            if counter % 4 == 0 {
                binaryString.insert(contentsOf: " ", at: binaryString.startIndex)
            }
        }
        
        return binaryString
    }
}
