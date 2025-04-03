//
//  MainViewController.swift
//  Gem
//
//  Created by Oren Leavitt on 3/20/20.
//  Copyright © 2020 Oren Leavitt. All rights reserved.
//

import Cocoa

// MARK: MainViewController

class MainViewController: NSViewController {

    @IBOutlet weak var sceneFileButton: NSButton!
    @IBOutlet private weak var sceneFilePathLabel: NSTextField!
    
    @IBOutlet private weak var outputResolutionLabel: NSTextField!
    @IBOutlet private weak var widthLabel: NSTextField!
    @IBOutlet private weak var widthTextField: NSTextField!
    @IBOutlet private weak var heightLabel: NSTextField!
   @IBOutlet private weak var heightTextField: NSTextField!
    @IBOutlet weak var startStopButton: NSButton!
    
    private var imageWindowController: NSWindowController?

    override func viewDidLoad() {
        super.viewDidLoad()
        
        loadFields()
    }
    
    @IBAction func fileOpenMenuSelected(_ sender: Any) {
        guard let window = view.window else { return }
        
        let panel = NSOpenPanel()
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false

        panel.beginSheetModal(for: window) { [weak self](response) in
            if response == .OK {
                let selectedFile = panel.urls[0]
                print("Selected file: '\(selectedFile)'")
                AppData.sceneFilePath = selectedFile
                self?.updateSceneFileInfo()
            }
        }
    }

    @IBAction func fileOpenButtonSelected(_ sender: Any) {
        fileOpenMenuSelected(sender)
    }
    
    @IBAction func fileSaveMenuSelected(_ sender: Any) {
        guard let window = view.window else { return }
        
        let panel = NSSavePanel()
        panel.canCreateDirectories = true
        panel.allowedFileTypes = ["jpg", "png", "tiff"]
        
        panel.beginSheetModal(for: window) { (response) in
            if response == .OK {
                if let url = panel.url {
                    print("Saving image to: \(url.path)")
                }
            }
        }
    }
    
    @IBAction func fileSaveButtonSelected(_ sender: Any) {
        fileSaveMenuSelected(sender)
    }
}

// MARK: Private

private extension MainViewController {
    
    func loadFields() {
        let outputResolution = AppData.outputResolution
        widthTextField.integerValue = Int(outputResolution.width)
        heightTextField.integerValue = Int(outputResolution.height)
        updateSceneFileInfo()
    }
    
    func updateSceneFileInfo() {
        guard let sceneFileUrl = AppData.sceneFilePath else {
            sceneFilePathLabel.stringValue = ""
            return
        }
        
        sceneFilePathLabel.stringValue = sceneFileUrl.lastPathComponent
    }
    
    func saveFields() {
        // Store the output resolution from the input fields
        let width = widthTextField.integerValue
        let height = heightTextField.integerValue
        if width > 0 && height > 0 {
            AppData.outputResolution = NSSize(width: width, height: height)
        }
    }
}

// MARK: Menus/Buttons

extension MainViewController {
    
    // Render Menu -> Start/Stop
    @IBAction func onRenderMenuStart(_ sender: Any) {
        onRenderStart(sender)
    }
    
    @IBAction func onRenderStart(_ sender: Any) {
        saveFields()
        
        guard let sb = storyboard else { return }
        if imageWindowController == nil {
            imageWindowController = sb.instantiateController(withIdentifier: "ImageWindowControllerScene") as? NSWindowController
        }
        
        //guard let imageVC = imageWindowController?.contentViewController as? ImageViewController else { return }
        imageWindowController?.showWindow(self)
        
        NotificationCenter.default.post(name: renderStartNotification, object: nil)
    }
}
