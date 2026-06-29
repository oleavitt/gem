//
//  AppData.swift
//  Gem
//
//  Created by Oren Leavitt on 3/22/20.
//  Copyright © 2020 Oren Leavitt. All rights reserved.
//

import Foundation

class AppData {
    
    private static let baseKey = "gem."
    private static let outputResolutionSizeKey = baseKey + "outputResolutionSize"
    private static let sceneFilePathKey = baseKey + "sceneFilePath"
    private static let includeFilePathsKey = baseKey + "includeFilePaths"
    private static let autoSaveEnabledKey = baseKey + "autoSaveEnabled"

    static var outputResolution: NSSize {
        get {
            guard let intArray = UserDefaults.standard.array(forKey: outputResolutionSizeKey) as? [Int] else {
                return NSSize(width: 200, height: 200)
            }
            return NSSize(width: intArray[0], height: intArray[1])
        }
        set {
            let intArray = [Int(newValue.width), Int(newValue.height)]
            UserDefaults.standard.set(intArray, forKey: outputResolutionSizeKey)
        }
    }
    
    static var sceneFilePath: URL? {
        get {
            return UserDefaults.standard.url(forKey: sceneFilePathKey)
        }
        set {
            UserDefaults.standard.set(newValue, forKey: sceneFilePathKey)
        }
    }
    
    /// Whether finished renders are written to disk automatically.
    /// Defaults to `true` when the user has never set a preference.
    static var autoSaveEnabled: Bool {
        get {
            guard UserDefaults.standard.object(forKey: autoSaveEnabledKey) != nil else { return true }
            return UserDefaults.standard.bool(forKey: autoSaveEnabledKey)
        }
        set {
            UserDefaults.standard.set(newValue, forKey: autoSaveEnabledKey)
        }
    }

    static var includeFilePaths: String? {
        get {
            return UserDefaults.standard.string(forKey: includeFilePathsKey)
        }
        set {
            UserDefaults.standard.set(newValue, forKey: includeFilePathsKey)
        }
    }
}
