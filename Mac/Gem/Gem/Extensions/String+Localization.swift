//
//  String+Localization.swift
//  Gem
//
//  Created by Oren Leavitt on 3/20/20.
//  Copyright © 2020 Oren Leavitt. All rights reserved.
//

import Foundation

extension String {
    
    func localized() -> String {
        return NSLocalizedString(self, comment: "")
    }
}
