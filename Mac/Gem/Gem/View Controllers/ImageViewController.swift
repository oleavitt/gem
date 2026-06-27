//
//  ImageViewController.swift
//  Gem
//

import Cocoa

@MainActor
final class ImageViewController: NSViewController {

    @IBOutlet weak var imageScrollView: NSScrollView!
    weak var imageView: NSImageView?

    private let engine = RenderEngine(renderer: RayTraceRenderer())
    private var bitmap: NSBitmapImageRep?
    private var imageSize = NSSize.zero
    private var startTime = Date()
    private var renderTask: Task<Void, Never>?
    private var rowsSinceRefresh = 0
    private static let refreshEveryRows = 16

    override func viewDidLoad() {
        super.viewDidLoad()
        registerForNotifications()
        setupImageView()
    }
}

// MARK: Private

private extension ImageViewController {

    func registerForNotifications() {
        NotificationCenter.default.addObserver(forName: renderStartNotification,
                                               object: nil, queue: .main) { [weak self] _ in
            Task { @MainActor in
                self?.prepareToRender()
                self?.startRendering()
            }
        }
        NotificationCenter.default.addObserver(forName: renderStopNotification,
                                               object: nil, queue: .main) { [weak self] _ in
            Task { @MainActor in
                self?.stopRendering()
            }
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
        guard let sceneFileName = AppData.sceneFilePath?.lastPathComponent else { return }
        view.window?.title = sceneFileName

        imageSize = AppData.outputResolution
        imageView?.setFrameSize(imageSize)
        view.window?.setContentSize(NSSize(width: max(100, imageSize.width),
                                           height: max(100, imageSize.height) + 20))
        createImageContext(imgSize: imageSize)
    }

    func startRendering() {
        renderTask?.cancel()
        let size = imageSize
        rowsSinceRefresh = 0
        renderTask = Task { @MainActor [weak self] in
            guard let self else { return }
            let stream = self.engine.render(imageSize: size,
                                            bounds: CGRect(x: -1, y: 1, width: 2, height: 2),
                                            aspectMode: .fit)
            for await update in stream {
                self.apply(update)
            }
        }
    }

    func stopRendering() {
        renderTask?.cancel()
    }

    func apply(_ update: RenderUpdate) {
        switch update {
        case .started(let width, let height):
            startTime = Date()
            rowsSinceRefresh = 0
            NSLog("View port started rendering an image of \(width) x \(height) pixels")
        case .row(let y, let pixels):
            write(row: pixels, at: y)
            rowsSinceRefresh += 1
            if rowsSinceRefresh >= Self.refreshEveryRows {
                rowsSinceRefresh = 0
                imageView?.setNeedsDisplay(.zero)
            }
        case .failed(let error):
            NSLog("View port render failed: \(error)")
            presentRenderError(error)
        case .finished:
            let timeElapsed = -startTime.timeIntervalSinceNow
            NSLog(String(format: "render_time_fmt".localized(), timeElapsed))
            imageView?.setNeedsDisplay(.zero)
        }
    }

    func write(row pixels: [PixelColor], at y: Int) {
        guard let bitmap else { return }
        var buffer = [Int](repeating: 0, count: 4)
        for (x, pixel) in pixels.enumerated() {
            buffer[0] = Int(pixel.a)
            buffer[1] = Int(pixel.r)
            buffer[2] = Int(pixel.g)
            buffer[3] = Int(pixel.b)
            bitmap.setPixel(&buffer, atX: x, y: y)
        }
    }

    func presentRenderError(_ error: RenderError) {
        let alert = NSAlert()
        alert.messageText = "render_failed".localized()
        alert.informativeText = String(describing: error)
        alert.runModal()
    }

    func createImageContext(imgSize: NSSize) {
        bitmap = nil

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

        let g = NSGraphicsContext(bitmapImageRep: offScreenRep)
        NSGraphicsContext.saveGraphicsState()
        NSGraphicsContext.current = g

        let path = NSBezierPath()
        path.move(to: NSMakePoint(0.0, 1.0))
        path.line(to: NSMakePoint(0.0, imgSize.height))
        path.line(to: NSMakePoint(imgSize.width, imgSize.height))
        path.line(to: NSMakePoint(imgSize.width, 1.0))
        NSColor.black.set()
        path.fill()

        NSGraphicsContext.restoreGraphicsState()

        let img = NSImage(size: imgSize)
        img.addRepresentation(offScreenRep)
        imageView?.image = img
    }
}
