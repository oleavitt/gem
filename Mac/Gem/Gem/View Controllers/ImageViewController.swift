//
//  ImageViewController.swift
//  Gem
//
//  Created by Oren Leavitt on 3/4/20.
//  Copyright © 2020 Oren Leavitt. All rights reserved.
//

import Cocoa

class ImageViewController: NSViewController {

    @IBOutlet weak var imageScrollView: NSScrollView!
    weak var imageView: NSImageView?
    
    //private let renderer = TestRenderer()
    private let renderer = RayTraceRenderer()
    private var viewPort: ViewPort?
    private var bitmap: NSBitmapImageRep?
    private var imageSize = NSSize.zero
    private var startTime = Date()
    private let pixelBuffer = UnsafeMutablePointer<Int>.allocate(capacity: 4)
    
    deinit {
        pixelBuffer.deallocate()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()

        registerForNotifications()
        setupImageView()
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
}

// MARK: Private

private extension ImageViewController {
    
    func registerForNotifications() {
        NotificationCenter.default.addObserver(forName: renderStartNotification, object: nil, queue: nil) { [weak self] (notification) in
            self?.prepareToRender()
            self?.startRendering()
        }
        NotificationCenter.default.addObserver(forName: renderStopNotification, object: nil, queue: nil) { (notification) in
            
        }
    }

    func setupImageView() {
        let nsImageView = NSImageView()
        nsImageView.imageScaling = .scaleNone
        imageScrollView.documentView = nsImageView
        imageScrollView.hasVerticalScroller = true
        imageScrollView.hasHorizontalScroller = true
        imageView = nsImageView
    }
    
    func prepareToRender() {
        guard viewPort == nil else { return }
        
        guard let sceneFileName = AppData.sceneFilePath?.lastPathComponent else {
            // No scene file - cant do anything.
            return
        }
        view.window?.title = sceneFileName
        
        imageSize = AppData.outputResolution
        imageView?.setFrameSize(imageSize)
        view.window?.setContentSize(NSSize(width: max(100, imageSize.width),
                                           height: max(100, imageSize.height) + 20))
        createImageContext(imgSize: imageSize)

        viewPort = BasicViewPort()
        viewPort?.setup(delegate: self, imageSize: imageSize, viewPortBounds: CGRect(x: -1, y: 1, width: 2, height: 2), aspectMode: .fit)
        viewPort?.set(renderer: renderer)
    }
    
    func startRendering() {
        viewPort?.render()
    }
    
    func stopRendering() {
        NSLog("TODO: Stop rendering.")
    }
    
    // TODO: Move this image context stuff out of the VC
    func createImageContext(imgSize:NSSize) {
        
        bitmap = nil
        
        // Create the bitmap object
        guard let offScreenRep = NSBitmapImageRep(
            bitmapDataPlanes: nil,
            pixelsWide: Int(imgSize.width),
            pixelsHigh: Int(imgSize.height),
            bitsPerSample: 8,
            samplesPerPixel: 4,
            hasAlpha: true,
            isPlanar: false,
            colorSpaceName: .deviceRGB,
            bitmapFormat: .alphaFirst,
            bytesPerRow: 0,
            bitsPerPixel: 0) else { return }
        
        bitmap = offScreenRep
        
        // Fill it with black
        let g = NSGraphicsContext(bitmapImageRep: offScreenRep)
        NSGraphicsContext.saveGraphicsState()
        NSGraphicsContext.current = g

        let p1 = NSMakePoint(0.0, 1.0)
        let p2 = NSMakePoint(0.0, imgSize.height)
        let p3 = NSMakePoint(imgSize.width, imgSize.height)
        let p4 = NSMakePoint(imgSize.width, 1.0)

        let path = NSBezierPath()
        path.move(to: p1)
        path.line(to: p2)
        path.line(to: p3)
        path.line(to: p4)

        NSColor.black.set()
        path.fill()

        NSGraphicsContext.restoreGraphicsState()
        
        // Show it in the image view
        let img = NSImage(size: imgSize)
        img.addRepresentation(offScreenRep)
        
        imageView?.image = img
    }
    
    func updateStartTime() {
        startTime = Date()
    }
    
    func updateEndTime() {
        let timeElapsed = -startTime.timeIntervalSinceNow
        NSLog(String(format: "render_time_fmt".localized(), timeElapsed))
    }
}

// MARK: ViewPortDelegate

extension ImageViewController: ViewPortDelegate {
    
    func viewPortRenderStarted(_ viewPort: ViewPort, imageWidth: Int, imageHeight: Int) {
        NSLog("View port started rendering an image of \(imageWidth) x \(imageHeight) pixels")
        updateStartTime()
    }

    func viewPort(_ viewPort: ViewPort, didSetPixel color: CGColor, atX x: Int, y: Int) {
        //NSLog("View port pixel: \(color.components)")

        guard let colorComponents = color.components else { return }

        pixelBuffer[0] = Int(colorComponents[3] * 255)
        pixelBuffer[1] = Int(colorComponents[0] * 255)
        pixelBuffer[2] = Int(colorComponents[1] * 255)
        pixelBuffer[3] = Int(colorComponents[2] * 255)

        bitmap?.setPixel(pixelBuffer, atX: x, y: y)
        
        // Refresh display every 20000 pixels
        // TODO: Refresh every n seconds?
        if (x + (y * Int(imageSize.height))) % 20000 == 0 {
            imageView?.setNeedsDisplay(NSRect.zero)
        }
    }
    
    func viewPortRenderEnded(_ viewPort: ViewPort) {
        NSLog("View port finished rendering.")
        updateEndTime()
        imageView?.setNeedsDisplay(NSRect.zero)
        self.viewPort = nil
    }
}

