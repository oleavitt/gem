# Mac Frontend Async/Concurrency Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the macOS `Gem` frontend thread-safe by replacing GCD/per-pixel dispatch and a non-thread-safe render pipeline with a Swift-concurrency design — an actor-isolated render engine, an `AsyncStream` of row updates, `@MainActor` view controllers, and real `Task`-based cancellation.

**Architecture:** A `RenderEngine` actor owns the `Renderer` and serializes all access to the global C ray-tracing engine. It vends an `AsyncStream<RenderUpdate>` that streams one update per row. `ImageViewController` (`@MainActor`) consumes the stream in a cancellable `Task`, writes rows into an `NSBitmapImageRep`, and throttles redraws. A `PixelColor` value type replaces `CGColor` in the hot loop.

**Tech Stack:** Swift 5 (language mode), AppKit/Cocoa, Swift Concurrency (actors, `AsyncStream`, structured `Task` cancellation), an Objective-C/C bridge to the Gem C engine, XCTest. Project file edits use the `xcodeproj` Ruby gem (already installed).

## Global Constraints

- **Project root for build commands:** `/Users/orenleavitt/Workspace/gem/Mac/Gem`
- **Xcode project:** `Gem.xcodeproj`, scheme `Gem`, app target `Gem`, test target `GemTests` (the `Gem` scheme already runs `GemTests`).
- **Deployment target:** macOS 11.5. `SWIFT_VERSION = 5.0`.
- **Concurrency strictness:** Pragmatic. Use `@MainActor` on AppKit code and an actor around the engine. Do **not** enable full Swift 6 strict-concurrency mode. A non-`Sendable` `Renderer` handed once into `RenderEngine.init` is an accepted single-handoff pattern; a resulting warning is acceptable, not a failure.
- **No new third-party dependencies** in the app target.
- **The C engine is a single global context** — exactly one render at a time. Never call it from two tasks concurrently; the actor guarantees this.
- **Every task must leave the project building** (`xcodebuild build ... -scheme Gem`) and all `GemTests` passing.
- **Adding/removing Swift files** must update the `Gem` target membership via the `xcodeproj` gem (the project uses classic explicit file references, not synchronized folders).

### Build & test commands (used throughout)

Build:
```bash
xcodebuild build -project /Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj \
  -scheme Gem -destination 'platform=macOS' -quiet
```

Run unit tests:
```bash
xcodebuild test -project /Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj \
  -scheme Gem -destination 'platform=macOS' -only-testing:GemTests -quiet
```

### Helper: add a file to the `Gem` target

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
target = proj.targets.find { |t| t.name == "Gem" }
ref = proj.main_group.new_file(ARGV[0])   # path relative to project source root (.../Mac/Gem)
target.add_file_references([ref])
proj.save
' "Gem/Common/PixelColor.swift"
```
(The file shows at the navigator root group — cosmetic only; target membership is what matters.)

### Helper: remove files from the project

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
names = ARGV
proj.files.select { |f| f.path && names.any? { |n| f.real_path.to_s.end_with?(n) } }.each(&:remove_from_project)
proj.save
' "BasicViewPort.swift" "ViewPort.swift"
```

---

## File Structure

**Created:**
- `Gem/Common/PixelColor.swift` — `Sendable` pixel value type (replaces per-pixel `CGColor`).
- `Gem/Renderer/RenderTypes.swift` — `ViewPortAspectMode` (relocated), `RenderError`, `RenderUpdate`.
- `Gem/Renderer/RenderEngine.swift` — the render-loop actor + `AsyncStream` producer.

**Modified:**
- `Gem/Renderer/Renderer.swift` — protocol now `AnyObject`, throwing `setup`, returns `PixelColor`.
- `Gem/Renderer/TestRenderer.swift`, `Gem/Renderer/RayTraceRenderer.swift` — conform to new protocol.
- `Gem/View Port/ViewPort.swift` — `ViewPortAspectMode` removed (moved to `RenderTypes.swift`); then deleted in Task 4.
- `Gem/OutputFile/OutputFile.swift`, `Gem/OutputFile/RawOutputFile.swift` — `savePixel(_ : PixelColor)`.
- `Gem/View Controllers/ImageViewController.swift` — `@MainActor`, consumes the engine stream.
- `Gem/View Controllers/MainViewController.swift` — `@MainActor`.
- `GemTests/GemTests.swift` — replaced with the test suites below.

**Deleted (Task 4):**
- `Gem/View Port/BasicViewPort.swift`, `Gem/View Port/ViewPort.swift` (the `ViewPort`/`ViewPortDelegate` protocols and `BasicViewPort` are superseded).

---

### Task 1: `PixelColor` value type

**Files:**
- Create: `Gem/Common/PixelColor.swift`
- Test: `GemTests/GemTests.swift`

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `struct PixelColor: Sendable, Equatable`
  - `init(a: UInt8 = 255, r: UInt8, g: UInt8, b: UInt8)`
  - `init(red: Double, green: Double, blue: Double, alpha: Double = 1.0)` — clamps each to `0...1`, multiplies by 255, rounds to nearest.
  - `init(cgColor: CGColor)` — bridges `[r,g,b,a]` components (temporary; removed in Task 5).
  - Stored bytes in order `a, r, g, b` (matches `NSBitmapImageRep` `.alphaFirst`).

- [ ] **Step 1: Write the failing tests**

Replace the entire contents of `GemTests/GemTests.swift` with:

```swift
import XCTest
@testable import Gem

final class PixelColorTests: XCTestCase {

    func testNormalizedComponentsMapToBytes() {
        XCTAssertEqual(PixelColor(red: 0, green: 0, blue: 0, alpha: 1),
                       PixelColor(a: 255, r: 0, g: 0, b: 0))
        XCTAssertEqual(PixelColor(red: 1, green: 1, blue: 1, alpha: 1),
                       PixelColor(a: 255, r: 255, g: 255, b: 255))
    }

    func testComponentsAreClampedAndRounded() {
        let p = PixelColor(red: -0.5, green: 2.0, blue: 0.5, alpha: 1)
        XCTAssertEqual(p, PixelColor(a: 255, r: 0, g: 255, b: 128))
    }

    func testInitFromCGColor() {
        let cg = CGColor(red: 1.0, green: 0.0, blue: 0.5, alpha: 1.0)
        XCTAssertEqual(PixelColor(cgColor: cg), PixelColor(a: 255, r: 255, g: 0, b: 128))
    }
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run the unit-test command above.
Expected: FAIL to compile / `cannot find 'PixelColor' in scope`.

- [ ] **Step 3: Create `PixelColor`**

Create `Gem/Common/PixelColor.swift`:

```swift
//
//  PixelColor.swift
//  Gem
//

import CoreGraphics

/// A Sendable RGBA pixel. Byte order (a, r, g, b) matches
/// `NSBitmapImageRep` created with `.alphaFirst`.
struct PixelColor: Sendable, Equatable {
    var a: UInt8
    var r: UInt8
    var g: UInt8
    var b: UInt8

    init(a: UInt8 = 255, r: UInt8, g: UInt8, b: UInt8) {
        self.a = a
        self.r = r
        self.g = g
        self.b = b
    }

    /// Build from normalized 0...1 components, clamping and rounding to 0...255.
    init(red: Double, green: Double, blue: Double, alpha: Double = 1.0) {
        func byte(_ value: Double) -> UInt8 {
            let clamped = min(max(value, 0.0), 1.0)
            return UInt8((clamped * 255.0).rounded())
        }
        self.a = byte(alpha)
        self.r = byte(red)
        self.g = byte(green)
        self.b = byte(blue)
    }

    /// Temporary bridge used while renderers still return `CGColor`. Removed in Task 5.
    init(cgColor: CGColor) {
        let c = cgColor.components ?? [0, 0, 0, 1]
        let red   = c.count > 0 ? Double(c[0]) : 0
        let green = c.count > 1 ? Double(c[1]) : 0
        let blue  = c.count > 2 ? Double(c[2]) : 0
        let alpha = c.count > 3 ? Double(c[3]) : 1
        self.init(red: red, green: green, blue: blue, alpha: alpha)
    }
}
```

- [ ] **Step 4: Add the file to the `Gem` target**

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
target = proj.targets.find { |t| t.name == "Gem" }
ref = proj.main_group.new_file(ARGV[0])
target.add_file_references([ref])
proj.save
' "Gem/Common/PixelColor.swift"
```

- [ ] **Step 5: Run the tests to verify they pass**

Run the unit-test command.
Expected: PASS (3 tests in `PixelColorTests`).

- [ ] **Step 6: Commit**

```bash
git add Gem/Common/PixelColor.swift GemTests/GemTests.swift Gem.xcodeproj/project.pbxproj
git commit -m "Add Sendable PixelColor value type"
```

---

### Task 2: Render types (`RenderError`, `RenderUpdate`, relocate `ViewPortAspectMode`)

**Files:**
- Create: `Gem/Renderer/RenderTypes.swift`
- Modify: `Gem/View Port/ViewPort.swift` (remove the `ViewPortAspectMode` enum)
- Test: `GemTests/GemTests.swift`

**Interfaces:**
- Consumes: `PixelColor` (Task 1).
- Produces:
  - `enum ViewPortAspectMode { case none, fit, fill }` (moved here from `ViewPort.swift`).
  - `enum RenderError: Error, Equatable, Sendable { case sceneParseFailed; case engineSetupFailed; case engineFailure(String) }`
  - `enum RenderUpdate: Sendable, Equatable { case started(width: Int, height: Int); case row(y: Int, pixels: [PixelColor]); case failed(RenderError); case finished }`

- [ ] **Step 1: Write the failing test**

Append this class to `GemTests/GemTests.swift`:

```swift
final class RenderUpdateTests: XCTestCase {

    func testEquatableStartedAndFinished() {
        XCTAssertEqual(RenderUpdate.started(width: 4, height: 3), .started(width: 4, height: 3))
        XCTAssertNotEqual(RenderUpdate.started(width: 4, height: 3), .started(width: 3, height: 4))
        XCTAssertNotEqual(RenderUpdate.finished, .started(width: 0, height: 0))
    }

    func testEquatableRowComparesPixels() {
        let a = RenderUpdate.row(y: 1, pixels: [PixelColor(r: 1, g: 2, b: 3)])
        let b = RenderUpdate.row(y: 1, pixels: [PixelColor(r: 1, g: 2, b: 3)])
        let c = RenderUpdate.row(y: 1, pixels: [PixelColor(r: 9, g: 2, b: 3)])
        XCTAssertEqual(a, b)
        XCTAssertNotEqual(a, c)
    }

    func testEquatableFailed() {
        XCTAssertEqual(RenderUpdate.failed(.sceneParseFailed), .failed(.sceneParseFailed))
        XCTAssertNotEqual(RenderUpdate.failed(.sceneParseFailed), .failed(.engineSetupFailed))
    }
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run the unit-test command.
Expected: FAIL to compile / `cannot find 'RenderUpdate' in scope`.

- [ ] **Step 3: Create `RenderTypes.swift`**

Create `Gem/Renderer/RenderTypes.swift`:

```swift
//
//  RenderTypes.swift
//  Gem
//

import Foundation

enum ViewPortAspectMode {
    case none, fit, fill
}

enum RenderError: Error, Equatable, Sendable {
    case sceneParseFailed
    case engineSetupFailed
    case engineFailure(String)
}

enum RenderUpdate: Sendable, Equatable {
    case started(width: Int, height: Int)
    case row(y: Int, pixels: [PixelColor])
    case failed(RenderError)
    case finished
}
```

- [ ] **Step 4: Remove the duplicate `ViewPortAspectMode` from `ViewPort.swift`**

In `Gem/View Port/ViewPort.swift`, delete these lines (the enum now lives in `RenderTypes.swift`):

```swift
enum ViewPortAspectMode {
    case none, fit, fill
}
```

Leave the rest of `ViewPort.swift` (the `ViewPort` and `ViewPortDelegate` protocols) unchanged for now.

- [ ] **Step 5: Add `RenderTypes.swift` to the `Gem` target**

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
target = proj.targets.find { |t| t.name == "Gem" }
ref = proj.main_group.new_file(ARGV[0])
target.add_file_references([ref])
proj.save
' "Gem/Renderer/RenderTypes.swift"
```

- [ ] **Step 6: Run the tests to verify they pass**

Run the unit-test command.
Expected: PASS (`PixelColorTests` + `RenderUpdateTests`). The project still builds because `CGRect+Aspect.swift` and `BasicViewPort.swift` now resolve `ViewPortAspectMode` from `RenderTypes.swift`.

- [ ] **Step 7: Commit**

```bash
git add Gem/Renderer/RenderTypes.swift "Gem/View Port/ViewPort.swift" GemTests/GemTests.swift Gem.xcodeproj/project.pbxproj
git commit -m "Add RenderError/RenderUpdate; relocate ViewPortAspectMode"
```

---

### Task 3: `RenderEngine` actor + streaming/cancellation

The engine consumes the **current** `Renderer` (which still returns `CGColor` and has a non-throwing `setup`) and converts each pixel to `PixelColor` before yielding. This keeps the project building; Task 5 switches the renderer to return `PixelColor` directly.

**Files:**
- Create: `Gem/Renderer/RenderEngine.swift`
- Test: `GemTests/GemTests.swift`

**Interfaces:**
- Consumes: `Renderer` (current `CGColor` protocol), `OutputFile`, `PixelColor`, `RenderUpdate`, `ViewPortAspectMode`, `CGRect.adjustedFor(aspectMode:targetSize:)`.
- Produces:
  - `actor RenderEngine`
  - `init(renderer: Renderer, outputFile: OutputFile? = nil)`
  - `nonisolated func render(imageSize: NSSize, bounds: CGRect, aspectMode: ViewPortAspectMode) -> AsyncStream<RenderUpdate>`
  - Emission order: `.started(width:height:)`, then `.row(y:pixels:)` for `y` in `0..<height` (each `pixels.count == width`), then `.finished`. Breaking/cancelling the consumer stops further rows.

- [ ] **Step 1: Write the failing tests**

Append to `GemTests/GemTests.swift`:

```swift
final class RenderEngineTests: XCTestCase {

    private let unitBounds = CGRect(x: -1, y: 1, width: 2, height: 2)

    func testStreamsStartedThenRowsThenFinished() async {
        let engine = RenderEngine(renderer: TestRenderer())
        var updates: [RenderUpdate] = []
        let stream = engine.render(imageSize: NSSize(width: 4, height: 3),
                                   bounds: unitBounds, aspectMode: .none)
        for await update in stream { updates.append(update) }

        guard case .started(let w, let h)? = updates.first else {
            return XCTFail("expected .started first, got \(String(describing: updates.first))")
        }
        XCTAssertEqual(w, 4)
        XCTAssertEqual(h, 3)
        XCTAssertEqual(updates.last, .finished)

        let rows: [(Int, Int)] = updates.compactMap {
            if case .row(let y, let px) = $0 { return (y, px.count) }
            return nil
        }
        XCTAssertEqual(rows.map { $0.0 }, [0, 1, 2])
        XCTAssertTrue(rows.allSatisfy { $0.1 == 4 }, "every row should have width pixels")
    }

    func testCancellingConsumerStopsEarlyWithoutHanging() async {
        let engine = RenderEngine(renderer: TestRenderer())
        let stream = engine.render(imageSize: NSSize(width: 100, height: 100),
                                   bounds: unitBounds, aspectMode: .none)
        var rows = 0
        for await update in stream {
            if case .row = update {
                rows += 1
                if rows == 3 { break }   // terminates the stream -> engine task is cancelled
            }
        }
        XCTAssertEqual(rows, 3)          // reaching here proves it did not hang
    }
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run the unit-test command.
Expected: FAIL to compile / `cannot find 'RenderEngine' in scope`.

- [ ] **Step 3: Create `RenderEngine.swift`**

Create `Gem/Renderer/RenderEngine.swift`:

```swift
//
//  RenderEngine.swift
//  Gem
//

import Foundation
import CoreGraphics

/// Serializes all access to the (single, global, non-thread-safe) C render
/// engine and streams results one row at a time.
actor RenderEngine {
    private let renderer: Renderer
    private let outputFile: OutputFile?

    init(renderer: Renderer, outputFile: OutputFile? = nil) {
        self.renderer = renderer
        self.outputFile = outputFile
    }

    /// Starts a render and returns a stream of updates. The work runs on the
    /// actor (off the main thread); breaking/cancelling the consuming task
    /// cancels the render.
    nonisolated func render(imageSize: NSSize,
                            bounds viewPortBounds: CGRect,
                            aspectMode: ViewPortAspectMode) -> AsyncStream<RenderUpdate> {
        AsyncStream { continuation in
            let task = Task {
                await self.runRender(into: continuation,
                                     imageSize: imageSize,
                                     viewPortBounds: viewPortBounds,
                                     aspectMode: aspectMode)
            }
            continuation.onTermination = { _ in task.cancel() }
        }
    }

    // Actor-isolated: two renders can never touch the C engine concurrently.
    // Contains no `await`, so the actor is not re-entered mid-frame.
    private func runRender(into continuation: AsyncStream<RenderUpdate>.Continuation,
                           imageSize: NSSize,
                           viewPortBounds: CGRect,
                           aspectMode: ViewPortAspectMode) {
        let width = Int(imageSize.width)
        let height = Int(imageSize.height)
        let bounds = viewPortBounds.adjustedFor(aspectMode: aspectMode, targetSize: imageSize)

        renderer.setup(viewPortBounds: bounds)
        defer { renderer.finished() }

        continuation.yield(.started(width: width, height: height))

        for y in 0 ..< height {
            if Task.isCancelled { break }
            let currentY = bounds.origin.y - (CGFloat(y) / CGFloat(height) * bounds.height)
            var rowPixels = [PixelColor]()
            rowPixels.reserveCapacity(width)
            for x in 0 ..< width {
                let currentX = bounds.origin.x + (CGFloat(x) / CGFloat(width) * bounds.width)
                let cgColor = renderer.color(at: CGPoint(x: currentX, y: currentY))
                rowPixels.append(PixelColor(cgColor: cgColor))
                outputFile?.savePixel(color: cgColor)
            }
            continuation.yield(.row(y: y, pixels: rowPixels))
        }
        continuation.finish()
    }
}
```

- [ ] **Step 4: Add `RenderEngine.swift` to the `Gem` target**

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
target = proj.targets.find { |t| t.name == "Gem" }
ref = proj.main_group.new_file(ARGV[0])
target.add_file_references([ref])
proj.save
' "Gem/Renderer/RenderEngine.swift"
```

- [ ] **Step 5: Run the tests to verify they pass**

Run the unit-test command.
Expected: PASS (`RenderEngineTests` streaming + cancellation tests included). A warning about passing a non-`Sendable` `Renderer` into the actor initializer is acceptable per Global Constraints.

- [ ] **Step 6: Commit**

```bash
git add Gem/Renderer/RenderEngine.swift GemTests/GemTests.swift Gem.xcodeproj/project.pbxproj
git commit -m "Add RenderEngine actor with row streaming and cancellation"
```

---

### Task 4: View-controller cutover to `RenderEngine`

Rewrite `ImageViewController` to consume the engine stream on the main actor, drop the per-pixel delegate path and shared `pixelBuffer`, and add real Stop via `Task` cancellation. Mark both view controllers `@MainActor` and delete the now-unused `BasicViewPort.swift` / `ViewPort.swift`.

**Files:**
- Modify: `Gem/View Controllers/ImageViewController.swift` (full rewrite)
- Modify: `Gem/View Controllers/MainViewController.swift` (add `@MainActor`)
- Delete: `Gem/View Port/BasicViewPort.swift`, `Gem/View Port/ViewPort.swift`

**Interfaces:**
- Consumes: `RenderEngine.render(imageSize:bounds:aspectMode:)`, `RenderUpdate`, `PixelColor`, `RayTraceRenderer`, `AppData`, `renderStartNotification`, `renderStopNotification`.
- Produces: no new public types. `ImageViewController` holds `private var renderTask: Task<Void, Never>?` and a `private let engine = RenderEngine(renderer: RayTraceRenderer())`.

Note: this task has no unit test (AppKit + storyboard + C engine). Its gate is a clean build; engine behavior is already covered by `RenderEngineTests`.

- [ ] **Step 1: Rewrite `ImageViewController.swift`**

Replace the entire contents of `Gem/View Controllers/ImageViewController.swift` with:

```swift
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
```

- [ ] **Step 2: Add `@MainActor` to `MainViewController`**

In `Gem/View Controllers/MainViewController.swift`, change the class declaration:

```swift
class MainViewController: NSViewController {
```
to:
```swift
@MainActor
class MainViewController: NSViewController {
```

- [ ] **Step 3: Add the `render_failed` localized string**

Confirm a `Localizable.strings` exists and how `render_time_fmt` is defined:

Run: `grep -rn "render_time_fmt" /Users/orenleavitt/Workspace/gem/Mac/Gem/Gem --include=*.strings`
- If a `.strings` file is found, add a sibling line in the same file: `"render_failed" = "Render failed";`
- If no `.strings` file is found (the `.localized()` extension falls back to the key), no action is needed — `presentRenderError` will display the key text.

- [ ] **Step 4: Delete the obsolete view-port files from the project**

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
names = ARGV
proj.files.select { |f| f.path && names.any? { |n| f.real_path.to_s.end_with?(n) } }.each(&:remove_from_project)
proj.save
' "BasicViewPort.swift" "ViewPort.swift"
git rm "Gem/View Port/BasicViewPort.swift" "Gem/View Port/ViewPort.swift"
```

- [ ] **Step 5: Build and run the tests**

Run the build command, then the unit-test command.
Expected: BUILD SUCCEEDED and all tests PASS. (Acceptable: a single non-`Sendable` `Renderer` handoff warning.)

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "Cut ImageViewController over to RenderEngine; @MainActor view controllers; remove BasicViewPort/ViewPort"
```

---

### Task 5: Migrate `Renderer` to `PixelColor` + throwing setup

Switch the `Renderer` protocol to return `PixelColor` and throw on setup failure, update both conformers, remove the temporary `CGColor` bridging from the engine (and surface setup failures as `.failed`), and update `OutputFile` to `PixelColor`.

**Files:**
- Modify: `Gem/Renderer/Renderer.swift`
- Modify: `Gem/Renderer/TestRenderer.swift`
- Modify: `Gem/Renderer/RayTraceRenderer.swift`
- Modify: `Gem/Renderer/RenderEngine.swift`
- Modify: `Gem/OutputFile/OutputFile.swift`, `Gem/OutputFile/RawOutputFile.swift`
- Modify: `Gem/Common/PixelColor.swift` (remove the `init(cgColor:)` bridge)
- Test: `GemTests/GemTests.swift`

**Interfaces:**
- Consumes: `PixelColor`, `RenderError`, `RenderUpdate`.
- Produces:
  - `protocol Renderer: AnyObject { func setup(viewPortBounds: CGRect) throws; func color(at: CGPoint) -> PixelColor; func finished() }`
  - `protocol OutputFile { func create(name:width:height:); func resume(name:); func savePixel(_ color: PixelColor) }`
  - `RenderEngine` now emits `.failed(RenderError)` then `.finished` when `setup()` throws.

- [ ] **Step 1: Write the failing tests**

Append to `GemTests/GemTests.swift`:

```swift
private final class FailingRenderer: Renderer {
    func setup(viewPortBounds: CGRect) throws { throw RenderError.sceneParseFailed }
    func color(at pointOnViewPort: CGPoint) -> PixelColor { PixelColor(r: 0, g: 0, b: 0) }
    func finished() {}
}

final class RendererMigrationTests: XCTestCase {

    func testTestRendererIsDeterministicAndOpaque() throws {
        let renderer = TestRenderer()
        try renderer.setup(viewPortBounds: CGRect(x: -1, y: 1, width: 2, height: 2))
        let first = renderer.color(at: CGPoint(x: 0, y: 0))
        let second = renderer.color(at: CGPoint(x: 0, y: 0))
        XCTAssertEqual(first, second)
        XCTAssertEqual(first.a, 255)
    }

    func testEngineEmitsFailedWhenSetupThrows() async {
        let engine = RenderEngine(renderer: FailingRenderer())
        var updates: [RenderUpdate] = []
        let stream = engine.render(imageSize: NSSize(width: 4, height: 4),
                                   bounds: CGRect(x: -1, y: 1, width: 2, height: 2),
                                   aspectMode: .none)
        for await update in stream { updates.append(update) }
        XCTAssertEqual(updates, [.failed(.sceneParseFailed)])
    }
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run the unit-test command.
Expected: FAIL to compile — `setup` is not `throws` yet, `color(at:)` returns `CGColor`, and `FailingRenderer` does not satisfy the protocol.

- [ ] **Step 3: Update the `Renderer` protocol**

Replace the entire contents of `Gem/Renderer/Renderer.swift` with:

```swift
//
//  Renderer.swift
//  Gem
//

import CoreGraphics

protocol Renderer: AnyObject {
    func setup(viewPortBounds: CGRect) throws
    func color(at pointOnViewPort: CGPoint) -> PixelColor
    func finished()
}
```

- [ ] **Step 4: Update `TestRenderer`**

Replace the entire contents of `Gem/Renderer/TestRenderer.swift` with:

```swift
//
//  TestRenderer.swift
//  Gem
//

import Foundation
import CoreGraphics

class TestRenderer: Renderer {
    private var bounds: CGRect = .zero
    private let radiusSquared: CGFloat = 3.0

    func setup(viewPortBounds: CGRect) throws {
        bounds = viewPortBounds
    }

    func color(at pointOnViewPort: CGPoint) -> PixelColor {
        var dx = pointOnViewPort.x
        var dy = pointOnViewPort.y - 0.875
        let redDistSquared = dx * dx + dy * dy
        if redDistSquared < radiusSquared {
            dx = pointOnViewPort.x - 0.866
            dy = pointOnViewPort.y + 0.625
            let greenDistSquared = dx * dx + dy * dy
            if greenDistSquared < radiusSquared {
                dx = pointOnViewPort.x + 0.866
                let blueDistSquared = dx * dx + dy * dy
                if blueDistSquared < radiusSquared {
                    return PixelColor(red: Double(1.0 - redDistSquared / radiusSquared),
                                      green: Double(1.0 - greenDistSquared / radiusSquared),
                                      blue: Double(1.0 - blueDistSquared / radiusSquared),
                                      alpha: 1.0)
                }
            }
        }

        dx = pointOnViewPort.x + 1.0
        dy = 1.0 - pointOnViewPort.y
        let gray = Double(min(dx, dy) / 2.0)
        return PixelColor(red: gray, green: gray, blue: gray, alpha: 1.0)
    }

    func finished() {}
}
```

- [ ] **Step 5: Update `RayTraceRenderer`**

Replace the entire contents of `Gem/Renderer/RayTraceRenderer.swift` with:

```swift
//
//  RayTraceRenderer.swift
//  Gem
//

import Foundation
import CoreGraphics

class RayTraceRenderer: Renderer {
    func setup(viewPortBounds: CGRect) throws {
        Ray_Initialize()
        var raySetupData = RaySetupData()
        Ray_GetSetup(&raySetupData)
        scn20_initialize()
        scn20_set_msgfn(Scn20StatusMessage)

        guard let sceneFilePath = AppData.sceneFilePath?.path else {
            throw RenderError.sceneParseFailed
        }
        let searchPaths = AppData.includeFilePaths ?? "/Users/orenleavitt/Workspace/gem/scenes/library; /Users/orenleavitt/Workspace/gem/scenes"

        let result = scn20_parse(sceneFilePath, &raySetupData, searchPaths)
        guard result == SCN_OK else {
            throw RenderError.sceneParseFailed
        }
        guard Ray_Setup(&raySetupData) != 0 else {
            throw RenderError.engineSetupFailed
        }
    }

    func color(at pointOnViewPort: CGPoint) -> PixelColor {
        var color = Vec3()
        Ray_TraceRayFromViewport(Double(pointOnViewPort.x), Double(pointOnViewPort.y), &color)
        return PixelColor(red: color.x, green: color.y, blue: color.z, alpha: 1.0)
    }

    func finished() {
        scn20_close()
        Ray_Close()
    }
}
```

- [ ] **Step 6: Update `RenderEngine` to use `PixelColor` directly and emit `.failed`**

In `Gem/Renderer/RenderEngine.swift`, replace the body of `runRender(...)` from the `renderer.setup(...)` call through the inner pixel loop. Specifically, replace:

```swift
        renderer.setup(viewPortBounds: bounds)
        defer { renderer.finished() }
```
with:

```swift
        do {
            try renderer.setup(viewPortBounds: bounds)
        } catch let error as RenderError {
            continuation.yield(.failed(error))
            continuation.finish()
            return
        } catch {
            continuation.yield(.failed(.engineFailure(String(describing: error))))
            continuation.finish()
            return
        }
        defer { renderer.finished() }
```

and replace the inner pixel-loop body:

```swift
                let cgColor = renderer.color(at: CGPoint(x: currentX, y: currentY))
                rowPixels.append(PixelColor(cgColor: cgColor))
                outputFile?.savePixel(color: cgColor)
```
with:

```swift
                let pixel = renderer.color(at: CGPoint(x: currentX, y: currentY))
                rowPixels.append(pixel)
                outputFile?.savePixel(pixel)
```

- [ ] **Step 7: Update `OutputFile` and `RawOutputFile`**

In `Gem/OutputFile/OutputFile.swift`, change the protocol method:
```swift
    func savePixel(color: CGColor)
```
to:
```swift
    func savePixel(_ color: PixelColor)
```

In `Gem/OutputFile/RawOutputFile.swift`, change:
```swift
    func savePixel(color: CGColor) {

    }
```
to:
```swift
    func savePixel(_ color: PixelColor) {

    }
```

- [ ] **Step 8: Remove the temporary `init(cgColor:)` from `PixelColor`**

In `Gem/Common/PixelColor.swift`, delete the `init(cgColor:)` initializer (and its doc comment) — nothing references it now. Also delete `testInitFromCGColor()` from `PixelColorTests` in `GemTests/GemTests.swift`.

- [ ] **Step 9: Run the build and tests to verify they pass**

Run the build command, then the unit-test command.
Expected: BUILD SUCCEEDED; all tests PASS, including `RendererMigrationTests`.

- [ ] **Step 10: Commit**

```bash
git add -A
git commit -m "Migrate Renderer/OutputFile to PixelColor and throwing setup"
```

---

## Self-Review

**Spec coverage:**
- `PixelColor` Sendable value type → Task 1. ✓
- `Renderer` protocol (`AnyObject`, throwing `setup`, `PixelColor`) → Task 5. ✓
- `RenderEngine` actor + `AsyncStream<RenderUpdate>`, row streaming, actor-serialized C access, `Task.isCancelled` + `onTermination`, `defer { finished() }` → Tasks 3 & 5. ✓
- `RenderUpdate` enum (`.started/.row/.failed/.finished`, Sendable) → Task 2. ✓
- `@MainActor ImageViewController`, `renderTask`, throttled `setNeedsDisplay`, Stop cancels, `pixelBuffer` removed → Task 4. ✓
- `@MainActor` view controllers; remove `ViewPort`/`ViewPortDelegate`/`BasicViewPort` → Tasks 2 (enum move) & 4 (deletion). ✓
- `RenderError` + setup failure surfaced to UI (`.failed`) → Tasks 2, 4 (`presentRenderError`), 5 (emission). ✓
- `OutputFile.savePixel` adapts to `PixelColor` → Task 5. ✓
- Testing via `TestRenderer` (ordering, row width, cancellation, setup-failure) → Tasks 3 & 5. ✓
- Out of scope (NotificationCenter trigger, `AppCoordinator`, `RawOutputFile` empties, multi-core, full Swift 6) → untouched. ✓

**Placeholder scan:** No TBD/TODO/"handle edge cases"/"similar to Task N" — every code step shows complete code. ✓

**Type consistency:** `PixelColor(a:r:g:b:)` / `PixelColor(red:green:blue:alpha:)`, `RenderUpdate.row(y:pixels:)`, `RenderEngine.render(imageSize:bounds:aspectMode:)`, `Renderer.setup(viewPortBounds:) throws`, `Renderer.color(at:) -> PixelColor`, `OutputFile.savePixel(_:)`, `RenderError.sceneParseFailed/.engineSetupFailed/.engineFailure(_)` are used identically across tasks. The `init(cgColor:)` bridge is introduced (Task 1), used (Task 3), and removed (Task 5) consistently. ✓
