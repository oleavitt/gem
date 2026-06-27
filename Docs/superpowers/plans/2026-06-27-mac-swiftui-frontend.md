# Mac SwiftUI Frontend Conversion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the AppKit/storyboard frontend of the macOS `Gem` app with a 100% SwiftUI implementation (single unified window, native menus, file dialogs, Settings) driving the existing async `RenderEngine`.

**Architecture:** A `@MainActor @Observable RenderModel` owns the `RenderEngine`, consumes its `AsyncStream<RenderUpdate>`, accumulates rows in a `RenderImageBuffer`, and publishes a `CGImage` + progress. SwiftUI scenes (one `Window`, a `Settings`, a `Commands` block) render the model. The render backend is reused unchanged.

**Tech Stack:** Swift 5 language mode, SwiftUI (macOS 14), Observation (`@Observable`), Swift Concurrency, ImageIO/UniformTypeIdentifiers for export, the existing C ray-tracing engine via the ObjC/C bridge, XCTest. Project file edits use the `xcodeproj` Ruby gem (already installed).

## Global Constraints

- **Project root for build commands:** `/Users/orenleavitt/Workspace/gem/Mac/Gem`
- **Xcode project:** `Gem.xcodeproj`, scheme `Gem`, app target `Gem`, test target `GemTests` (the `Gem` scheme already runs `GemTests`).
- **Deployment target:** raise `MACOSX_DEPLOYMENT_TARGET` to `14.0` (done in Task 2). `SWIFT_VERSION = 5.0`.
- **App is NOT sandboxed** (empty `Gem/Gem.entitlements`) — `.fileImporter` URLs are accessed by plain path; no security-scoped bookmarks.
- **Reuse the render backend unchanged:** `RenderEngine`, `Renderer`/`RayTraceRenderer`/`TestRenderer`, `RenderTypes`, `PixelColor`, `OutputFile`, `AppData`, `CGRect+Aspect`, the C bridge, and all existing tests.
- `RenderEngine.init(renderer: Renderer, outputFile: OutputFile? = nil)`; `nonisolated func render(imageSize: NSSize, bounds: CGRect, aspectMode: ViewPortAspectMode) -> AsyncStream<RenderUpdate>`.
- `AppData.outputResolution: NSSize`, `AppData.sceneFilePath: URL?`, `AppData.includeFilePaths: String?` (UserDefaults key `gem.includeFilePaths`).
- `PixelColor` byte order is `(a, r, g, b)`.
- **No new third-party dependencies.**
- **Every task must leave the project building** and all `GemTests` passing.
- **Adding/removing Swift files** updates `Gem` target membership via the `xcodeproj` gem (classic file references).

### Build & test commands (used throughout)

Build:
```bash
xcodebuild build -project /Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj \
  -scheme Gem -configuration Debug -destination 'platform=macOS' -quiet
```

Run unit tests:
```bash
xcodebuild test -project /Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj \
  -scheme Gem -configuration Debug -destination 'platform=macOS' -only-testing:GemTests -quiet
```

### Helper: add files to the `Gem` target

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
target = proj.targets.find { |t| t.name == "Gem" }
ARGV.each { |p| target.add_file_references([proj.main_group.new_file(p)]) }
proj.save
' "Gem/Model/RenderImageBuffer.swift"
```

### Helper: remove files from the project

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
names = ARGV
proj.files.select { |f| f.path && names.any? { |n| f.real_path.to_s.end_with?(n) } }.each(&:remove_from_project)
proj.save
' "AppDelegate.swift"
```

---

## File Structure

**Created:**
- `Gem/Model/RenderImageBuffer.swift` — accumulates pixel rows into an ARGB byte buffer; produces a `CGImage`.
- `Gem/Model/ImageEncoding.swift` — `CGImage` → PNG/TIFF `Data` (ImageIO).
- `Gem/Model/PixelSize.swift` — `Sendable` width/height value type.
- `Gem/Model/RenderModel.swift` — `@MainActor @Observable` UI state + render driver.
- `Gem/Views/ImageDocument.swift` — `FileDocument` wrapping a `CGImage` for `.fileExporter`.
- `Gem/Views/GemFileTypes.swift` — allowed scene content types.
- `Gem/Views/RenderCanvasView.swift`, `SceneControlsView.swift`, `SettingsView.swift`, `ContentView.swift`.
- `Gem/App/GemApp.swift` — `App` scene graph (gets `@main` in Task 4).
- `Gem/App/GemCommands.swift` — File/Render menu commands.

**Deleted (Task 4):**
- `Gem/AppDelegate.swift`, `Gem/Main.storyboard`,
  `Gem/View Controllers/MainViewController.swift`, `Gem/View Controllers/ImageViewController.swift`,
  `Gem/Coordinators/AppCoordinator.swift`, `Gem/Common/Constants.swift`.

**Modified:** `Gem/Info.plist` (remove `NSMainStoryboardFile`), `Gem.xcodeproj/project.pbxproj` (target membership + deployment target), `GemTests/GemTests.swift` (new test suites).

---

### Task 1: Image utilities (`RenderImageBuffer`, `ImageEncoding`)

Pure CoreGraphics/ImageIO helpers. No deployment-target change needed.

**Files:**
- Create: `Gem/Model/RenderImageBuffer.swift`, `Gem/Model/ImageEncoding.swift`
- Test: `GemTests/GemTests.swift`

**Interfaces:**
- Consumes: `PixelColor` (`init(a:r:g:b:)`, `(a,r,g,b)` order).
- Produces:
  - `final class RenderImageBuffer` — `init(width: Int, height: Int)`; `func setRow(_ pixels: [PixelColor], at y: Int)`; `func pixel(atX x: Int, y: Int) -> PixelColor?`; `func makeCGImage() -> CGImage?`. New buffers are opaque black (`a=255`, rgb=0).
  - `enum ImageEncoding { static func data(from image: CGImage, type: UTType) -> Data? }`.

- [ ] **Step 1: Write the failing tests**

Append to `GemTests/GemTests.swift`:

```swift
import UniformTypeIdentifiers

final class RenderImageBufferTests: XCTestCase {

    func testNewBufferIsOpaqueBlack() {
        let buf = RenderImageBuffer(width: 2, height: 2)
        XCTAssertEqual(buf.pixel(atX: 0, y: 0), PixelColor(a: 255, r: 0, g: 0, b: 0))
        XCTAssertEqual(buf.pixel(atX: 1, y: 1), PixelColor(a: 255, r: 0, g: 0, b: 0))
    }

    func testSetRowWritesPixelsReadableBack() {
        let buf = RenderImageBuffer(width: 2, height: 2)
        buf.setRow([PixelColor(r: 255, g: 0, b: 0), PixelColor(r: 0, g: 255, b: 0)], at: 1)
        XCTAssertEqual(buf.pixel(atX: 0, y: 1), PixelColor(a: 255, r: 255, g: 0, b: 0))
        XCTAssertEqual(buf.pixel(atX: 1, y: 1), PixelColor(a: 255, r: 0, g: 255, b: 0))
        XCTAssertEqual(buf.pixel(atX: 0, y: 0), PixelColor(a: 255, r: 0, g: 0, b: 0))
    }

    func testMakeCGImageHasBufferDimensions() {
        let img = RenderImageBuffer(width: 4, height: 3).makeCGImage()
        XCTAssertEqual(img?.width, 4)
        XCTAssertEqual(img?.height, 3)
    }

    func testZeroSizeMakesNoImage() {
        XCTAssertNil(RenderImageBuffer(width: 0, height: 0).makeCGImage())
    }
}

final class ImageEncodingTests: XCTestCase {

    func testEncodesPngThatDecodesToSameSize() {
        let buf = RenderImageBuffer(width: 5, height: 4)
        let image = try! XCTUnwrap(buf.makeCGImage())
        let data = try! XCTUnwrap(ImageEncoding.data(from: image, type: .png))
        XCTAssertFalse(data.isEmpty)
        let src = try! XCTUnwrap(CGImageSourceCreateWithData(data as CFData, nil))
        let decoded = try! XCTUnwrap(CGImageSourceCreateImageAtIndex(src, 0, nil))
        XCTAssertEqual(decoded.width, 5)
        XCTAssertEqual(decoded.height, 4)
    }
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run the unit-test command.
Expected: FAIL to compile / `cannot find 'RenderImageBuffer'` and `'ImageEncoding'`.

- [ ] **Step 3: Create `RenderImageBuffer.swift`**

```swift
//
//  RenderImageBuffer.swift
//  Gem
//

import Foundation
import CoreGraphics

/// Accumulates streamed pixel rows into a byte buffer in PixelColor's (a,r,g,b)
/// order and produces a CGImage. Not thread-safe; use from one actor/thread.
final class RenderImageBuffer {
    let width: Int
    let height: Int
    private let bytesPerPixel = 4
    private var bytes: [UInt8]

    init(width: Int, height: Int) {
        self.width = max(0, width)
        self.height = max(0, height)
        bytes = [UInt8](repeating: 0, count: self.width * self.height * bytesPerPixel)
        var i = 0
        while i < bytes.count {
            bytes[i] = 255   // opaque alpha; rgb stays 0 (black)
            i += bytesPerPixel
        }
    }

    func setRow(_ pixels: [PixelColor], at y: Int) {
        guard y >= 0, y < height else { return }
        let rowStart = y * width * bytesPerPixel
        let count = min(pixels.count, width)
        for x in 0 ..< count {
            let p = pixels[x]
            let o = rowStart + x * bytesPerPixel
            bytes[o + 0] = p.a
            bytes[o + 1] = p.r
            bytes[o + 2] = p.g
            bytes[o + 3] = p.b
        }
    }

    func pixel(atX x: Int, y: Int) -> PixelColor? {
        guard x >= 0, x < width, y >= 0, y < height else { return nil }
        let o = (y * width + x) * bytesPerPixel
        return PixelColor(a: bytes[o], r: bytes[o + 1], g: bytes[o + 2], b: bytes[o + 3])
    }

    func makeCGImage() -> CGImage? {
        guard width > 0, height > 0 else { return nil }
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        // Bytes are (a,r,g,b) big-endian => ARGB.
        let bitmapInfo = CGBitmapInfo(
            rawValue: CGImageAlphaInfo.premultipliedFirst.rawValue
                | CGBitmapInfo.byteOrder32Big.rawValue)
        guard let provider = CGDataProvider(data: Data(bytes) as CFData) else { return nil }
        return CGImage(width: width,
                       height: height,
                       bitsPerComponent: 8,
                       bitsPerPixel: 32,
                       bytesPerRow: width * bytesPerPixel,
                       space: colorSpace,
                       bitmapInfo: bitmapInfo,
                       provider: provider,
                       decode: nil,
                       shouldInterpolate: false,
                       intent: .defaultIntent)
    }
}
```

- [ ] **Step 4: Create `ImageEncoding.swift`**

```swift
//
//  ImageEncoding.swift
//  Gem
//

import Foundation
import CoreGraphics
import ImageIO
import UniformTypeIdentifiers

enum ImageEncoding {
    /// Encodes a CGImage to PNG or TIFF data. Returns nil on failure.
    static func data(from image: CGImage, type: UTType) -> Data? {
        let out = NSMutableData()
        guard let dest = CGImageDestinationCreateWithData(
            out as CFMutableData, type.identifier as CFString, 1, nil) else {
            return nil
        }
        CGImageDestinationAddImage(dest, image, nil)
        guard CGImageDestinationFinalize(dest) else { return nil }
        return out as Data
    }
}
```

- [ ] **Step 5: Add both files to the `Gem` target**

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
target = proj.targets.find { |t| t.name == "Gem" }
ARGV.each { |p| target.add_file_references([proj.main_group.new_file(p)]) }
proj.save
' "Gem/Model/RenderImageBuffer.swift" "Gem/Model/ImageEncoding.swift"
```

- [ ] **Step 6: Run the tests to verify they pass**

Run the unit-test command.
Expected: PASS (`RenderImageBufferTests` 4, `ImageEncodingTests` 1, plus all existing tests).

- [ ] **Step 7: Commit**

```bash
git add Gem/Model/RenderImageBuffer.swift Gem/Model/ImageEncoding.swift GemTests/GemTests.swift Gem.xcodeproj/project.pbxproj
git commit -m "Add RenderImageBuffer and ImageEncoding helpers"
```

---

### Task 2: `RenderModel` + `PixelSize` (raise deployment target to macOS 14)

`@Observable` requires macOS 14, so this task raises the deployment target.

**Files:**
- Create: `Gem/Model/PixelSize.swift`, `Gem/Model/RenderModel.swift`
- Modify: `Gem.xcodeproj/project.pbxproj` (deployment target)
- Test: `GemTests/GemTests.swift`

**Interfaces:**
- Consumes: `RenderImageBuffer`, `RenderEngine`, `Renderer`, `TestRenderer`, `RenderUpdate`, `RenderError`, `AppData`, `PixelColor`.
- Produces:
  - `struct PixelSize: Equatable, Sendable` — `var width: Int`, `var height: Int`, `static let default`, `var nsSize: NSSize`, `var isRenderable: Bool`.
  - `@MainActor @Observable final class RenderModel`:
    - `var sceneURL: URL?`, `var resolution: PixelSize`, `private(set) var phase: Phase`, `private(set) var image: CGImage?`, `var isPresentingImporter: Bool`, `var isPresentingExporter: Bool`.
    - `enum Phase: Equatable { case idle; case rendering(progress: Double); case finished(elapsed: TimeInterval); case failed(RenderError) }`.
    - `init(renderer: Renderer = RayTraceRenderer())`.
    - computed: `isRendering`, `canStart`, `progress`, `failureMessage`, `isShowingFailureAlert` (get/set).
    - `func start()`, `func stop()`, `func waitUntilIdle() async`.

- [ ] **Step 1: Write the failing tests**

Append to `GemTests/GemTests.swift`:

```swift
@MainActor
final class RenderModelTests: XCTestCase {

    private func makeModel(width: Int, height: Int) -> RenderModel {
        let model = RenderModel(renderer: TestRenderer())
        model.sceneURL = URL(fileURLWithPath: "/tmp/does-not-matter.scn")
        model.resolution = PixelSize(width: width, height: height)
        return model
    }

    func testStartDrivesToFinishedWithImage() async {
        let model = makeModel(width: 8, height: 6)
        XCTAssertTrue(model.canStart)
        model.start()
        await model.waitUntilIdle()

        guard case .finished = model.phase else {
            return XCTFail("expected .finished, got \(model.phase)")
        }
        XCTAssertEqual(model.progress, 1.0, accuracy: 0.0001)
        XCTAssertEqual(model.image?.width, 8)
        XCTAssertEqual(model.image?.height, 6)
    }

    func testStopPreventsFinish() async {
        let model = makeModel(width: 300, height: 300)
        model.start()
        model.stop()
        await model.waitUntilIdle()
        if case .finished = model.phase {
            XCTFail("render should not finish after immediate stop")
        }
    }

    func testCannotStartWithoutScene() {
        let model = RenderModel(renderer: TestRenderer())
        model.sceneURL = nil
        model.resolution = PixelSize(width: 8, height: 8)
        XCTAssertFalse(model.canStart)
    }
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run the unit-test command.
Expected: FAIL to compile / `cannot find 'RenderModel'` / `'PixelSize'`.

- [ ] **Step 3: Raise the deployment target to macOS 14**

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
proj.targets.each { |t| t.build_configurations.each { |c| c.build_settings["MACOSX_DEPLOYMENT_TARGET"] = "14.0" } }
proj.save
'
```

- [ ] **Step 4: Create `PixelSize.swift`**

```swift
//
//  PixelSize.swift
//  Gem
//

import Foundation

struct PixelSize: Equatable, Sendable {
    var width: Int
    var height: Int

    static let `default` = PixelSize(width: 200, height: 200)

    var nsSize: NSSize { NSSize(width: width, height: height) }
    var isRenderable: Bool { width > 0 && height > 0 }
}
```

- [ ] **Step 5: Create `RenderModel.swift`**

```swift
//
//  RenderModel.swift
//  Gem
//

import Foundation
import CoreGraphics
import Observation

@MainActor
@Observable
final class RenderModel {

    enum Phase: Equatable {
        case idle
        case rendering(progress: Double)
        case finished(elapsed: TimeInterval)
        case failed(RenderError)
    }

    var sceneURL: URL?
    var resolution: PixelSize
    var isPresentingImporter = false
    var isPresentingExporter = false

    private(set) var phase: Phase = .idle
    private(set) var image: CGImage?

    private let renderer: Renderer
    private var renderTask: Task<Void, Never>?

    init(renderer: Renderer = RayTraceRenderer()) {
        self.renderer = renderer
        self.sceneURL = AppData.sceneFilePath
        let stored = AppData.outputResolution
        self.resolution = PixelSize(width: Int(stored.width), height: Int(stored.height))
    }

    var isRendering: Bool {
        if case .rendering = phase { return true }
        return false
    }

    var canStart: Bool { sceneURL != nil && resolution.isRenderable && !isRendering }

    var progress: Double {
        switch phase {
        case .rendering(let p): return p
        case .finished: return 1.0
        default: return 0.0
        }
    }

    var failureMessage: String {
        if case .failed(let error) = phase { return String(describing: error) }
        return ""
    }

    var isShowingFailureAlert: Bool {
        get { if case .failed = phase { return true } else { return false } }
        set { if !newValue, case .failed = phase { phase = .idle } }
    }

    func start() {
        guard canStart else { return }
        AppData.sceneFilePath = sceneURL
        AppData.outputResolution = resolution.nsSize

        renderTask?.cancel()

        let size = resolution.nsSize
        let height = resolution.height
        let buffer = RenderImageBuffer(width: resolution.width, height: resolution.height)
        image = buffer.makeCGImage()
        phase = .rendering(progress: 0)

        let engine = RenderEngine(renderer: renderer)
        let startTime = Date()

        renderTask = Task { @MainActor [weak self] in
            var rowsSinceRefresh = 0
            let stream = engine.render(imageSize: size,
                                       bounds: CGRect(x: -1, y: 1, width: 2, height: 2),
                                       aspectMode: .fit)
            for await update in stream {
                guard let self else { break }
                switch update {
                case .started:
                    break
                case .row(let y, let pixels):
                    buffer.setRow(pixels, at: y)
                    rowsSinceRefresh += 1
                    if rowsSinceRefresh >= 16 {
                        rowsSinceRefresh = 0
                        self.image = buffer.makeCGImage()
                    }
                    if height > 0 {
                        self.phase = .rendering(progress: Double(y + 1) / Double(height))
                    }
                case .failed(let error):
                    self.image = buffer.makeCGImage()
                    self.phase = .failed(error)
                case .finished:
                    self.image = buffer.makeCGImage()
                    self.phase = .finished(elapsed: -startTime.timeIntervalSinceNow)
                }
            }
        }
    }

    func stop() {
        renderTask?.cancel()
    }

    /// Awaits the current render task (used by tests and on quit).
    func waitUntilIdle() async {
        await renderTask?.value
    }
}
```

- [ ] **Step 6: Add both files to the `Gem` target**

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
target = proj.targets.find { |t| t.name == "Gem" }
ARGV.each { |p| target.add_file_references([proj.main_group.new_file(p)]) }
proj.save
' "Gem/Model/PixelSize.swift" "Gem/Model/RenderModel.swift"
```

- [ ] **Step 7: Run the tests to verify they pass**

Run the unit-test command.
Expected: PASS (`RenderModelTests` 3, plus all earlier tests). The old AppKit code still compiles at the 14.0 target (deprecation warnings on `allowedFileTypes` are acceptable).

- [ ] **Step 8: Commit**

```bash
git add Gem/Model/PixelSize.swift Gem/Model/RenderModel.swift GemTests/GemTests.swift Gem.xcodeproj/project.pbxproj
git commit -m "Add RenderModel and PixelSize; raise deployment target to macOS 14"
```

---

### Task 3: SwiftUI views, commands, and App scene (no `@main` yet)

Additive SwiftUI layer. It compiles alongside the existing AppKit app (the storyboard remains the entry point until Task 4). No unit tests — the gate is a clean build.

**Files:**
- Create: `Gem/Views/GemFileTypes.swift`, `Gem/Views/ImageDocument.swift`, `Gem/Views/RenderCanvasView.swift`, `Gem/Views/SceneControlsView.swift`, `Gem/Views/SettingsView.swift`, `Gem/Views/ContentView.swift`, `Gem/App/GemApp.swift`, `Gem/App/GemCommands.swift`

**Interfaces:**
- Consumes: `RenderModel` (and its members above), `ImageEncoding`, `AppData` key `gem.includeFilePaths`.
- Produces: `struct GemApp: App` (without `@main`), `struct GemCommands: Commands`, and the views. `ImageDocument(image: CGImage)` is a `FileDocument` writing PNG/TIFF.

- [ ] **Step 1: Create `GemFileTypes.swift`**

```swift
//
//  GemFileTypes.swift
//  Gem
//

import UniformTypeIdentifiers

enum GemFileTypes {
    /// Allowed types when opening a scene. `.scn` has no registered system type,
    /// so fall back to a dynamic type plus plain data/text.
    static let scene: [UTType] = {
        var types: [UTType] = [.data, .plainText]
        if let scn = UTType(filenameExtension: "scn") { types.insert(scn, at: 0) }
        return types
    }()
}
```

- [ ] **Step 2: Create `ImageDocument.swift`**

```swift
//
//  ImageDocument.swift
//  Gem
//

import SwiftUI
import UniformTypeIdentifiers

/// Wraps a rendered CGImage so SwiftUI's .fileExporter can write PNG/TIFF.
struct ImageDocument: FileDocument {
    static var readableContentTypes: [UTType] { [.png, .tiff] }
    static var writableContentTypes: [UTType] { [.png, .tiff] }

    let image: CGImage

    init(image: CGImage) { self.image = image }

    init(configuration: ReadConfiguration) throws {
        throw CocoaError(.fileReadCorruptFile)   // import not supported
    }

    func fileWrapper(configuration: WriteConfiguration) throws -> FileWrapper {
        let type: UTType = (configuration.contentType == .tiff) ? .tiff : .png
        guard let data = ImageEncoding.data(from: image, type: type) else {
            throw CocoaError(.fileWriteUnknown)
        }
        return FileWrapper(regularFile: data)
    }
}
```

- [ ] **Step 3: Create `RenderCanvasView.swift`**

```swift
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
```

- [ ] **Step 4: Create `SceneControlsView.swift`**

```swift
//
//  SceneControlsView.swift
//  Gem
//

import SwiftUI

struct SceneControlsView: View {
    @Environment(RenderModel.self) private var model

    var body: some View {
        @Bindable var model = model
        Form {
            Section("Scene") {
                LabeledContent("File") {
                    Text(model.sceneURL?.lastPathComponent ?? "None")
                        .foregroundStyle(model.sceneURL == nil ? .secondary : .primary)
                        .lineLimit(1)
                        .truncationMode(.middle)
                }
                Button("Choose…") { model.isPresentingImporter = true }
            }
            Section("Output Resolution") {
                LabeledContent("Width") {
                    TextField("Width", value: $model.resolution.width, format: .number)
                        .labelsHidden()
                        .frame(width: 90)
                }
                LabeledContent("Height") {
                    TextField("Height", value: $model.resolution.height, format: .number)
                        .labelsHidden()
                        .frame(width: 90)
                }
            }
        }
        .formStyle(.grouped)
    }
}
```

- [ ] **Step 5: Create `SettingsView.swift`**

```swift
//
//  SettingsView.swift
//  Gem
//

import SwiftUI

struct SettingsView: View {
    // Maps to AppData.includeFilePaths (UserDefaults key "gem.includeFilePaths").
    @AppStorage("gem.includeFilePaths") private var includePaths: String = ""

    var body: some View {
        Form {
            Section("Include / Search Paths") {
                TextField("Semicolon-separated paths", text: $includePaths, axis: .vertical)
                    .lineLimit(3...8)
            }
        }
        .formStyle(.grouped)
        .frame(width: 460, height: 240)
    }
}
```

- [ ] **Step 6: Create `ContentView.swift`**

```swift
//
//  ContentView.swift
//  Gem
//

import SwiftUI
import UniformTypeIdentifiers

struct ContentView: View {
    @Environment(RenderModel.self) private var model

    var body: some View {
        @Bindable var model = model
        NavigationSplitView {
            SceneControlsView()
                .navigationSplitViewColumnWidth(min: 220, ideal: 260, max: 360)
        } detail: {
            RenderCanvasView()
        }
        .toolbar {
            ToolbarItemGroup {
                if model.isRendering {
                    Button("Stop", systemImage: "stop.fill") { model.stop() }
                    ProgressView(value: model.progress)
                        .frame(width: 120)
                } else {
                    Button("Render", systemImage: "play.fill") { model.start() }
                        .disabled(!model.canStart)
                }
                if case .finished(let elapsed) = model.phase {
                    Text(String(format: "%.3g s", elapsed))
                        .foregroundStyle(.secondary)
                }
            }
        }
        .fileImporter(isPresented: $model.isPresentingImporter,
                      allowedContentTypes: GemFileTypes.scene) { result in
            if case .success(let url) = result { model.sceneURL = url }
        }
        .fileExporter(isPresented: $model.isPresentingExporter,
                      document: model.image.map { ImageDocument(image: $0) },
                      contentType: .png,
                      defaultFilename: model.sceneURL?.deletingPathExtension().lastPathComponent ?? "render") { _ in }
        .alert("Render Failed", isPresented: $model.isShowingFailureAlert) {
            Button("OK", role: .cancel) {}
        } message: {
            Text(model.failureMessage)
        }
    }
}
```

- [ ] **Step 7: Create `GemCommands.swift`**

```swift
//
//  GemCommands.swift
//  Gem
//

import SwiftUI

struct GemCommands: Commands {
    @Bindable var model: RenderModel

    var body: some Commands {
        CommandGroup(replacing: .newItem) {
            Button("Open Scene…") { model.isPresentingImporter = true }
                .keyboardShortcut("o")
            Button("Export Image…") { model.isPresentingExporter = true }
                .keyboardShortcut("s")
                .disabled(model.image == nil)
        }
        CommandMenu("Render") {
            Button("Start") { model.start() }
                .keyboardShortcut("r")
                .disabled(!model.canStart)
            Button("Stop") { model.stop() }
                .keyboardShortcut(".", modifiers: .command)
                .disabled(!model.isRendering)
        }
    }
}
```

- [ ] **Step 8: Create `GemApp.swift` (no `@main` yet)**

```swift
//
//  GemApp.swift
//  Gem
//

import SwiftUI

struct GemApp: App {
    @State private var model = RenderModel()

    var body: some Scene {
        Window("Gem", id: "main") {
            ContentView()
                .environment(model)
        }
        .commands {
            GemCommands(model: model)
        }

        Settings {
            SettingsView()
        }
    }
}
```

- [ ] **Step 9: Add all eight files to the `Gem` target**

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
target = proj.targets.find { |t| t.name == "Gem" }
ARGV.each { |p| target.add_file_references([proj.main_group.new_file(p)]) }
proj.save
' "Gem/Views/GemFileTypes.swift" "Gem/Views/ImageDocument.swift" "Gem/Views/RenderCanvasView.swift" "Gem/Views/SceneControlsView.swift" "Gem/Views/SettingsView.swift" "Gem/Views/ContentView.swift" "Gem/App/GemCommands.swift" "Gem/App/GemApp.swift"
```

- [ ] **Step 10: Build and run tests**

Run the build command, then the unit-test command.
Expected: BUILD SUCCEEDED and all existing tests PASS. The SwiftUI types compile but are not yet the entry point.

- [ ] **Step 11: Commit**

```bash
git add Gem/Views Gem/App GemTests/GemTests.swift Gem.xcodeproj/project.pbxproj
git commit -m "Add SwiftUI views, commands, and App scene (not yet the entry point)"
```

---

### Task 4: App-shell cutover to SwiftUI lifecycle

Make `GemApp` the entry point and remove the AppKit shell. Gate: clean build + the app launches.

**Files:**
- Modify: `Gem/App/GemApp.swift` (add `@main`), `Gem/Info.plist` (remove `NSMainStoryboardFile`)
- Delete: `Gem/AppDelegate.swift`, `Gem/Main.storyboard`, `Gem/View Controllers/MainViewController.swift`, `Gem/View Controllers/ImageViewController.swift`, `Gem/Coordinators/AppCoordinator.swift`, `Gem/Common/Constants.swift`

**Interfaces:**
- Consumes: everything from Task 3. No new types.

- [ ] **Step 1: Make `GemApp` the entry point**

In `Gem/App/GemApp.swift`, change:
```swift
struct GemApp: App {
```
to:
```swift
@main
struct GemApp: App {
```

- [ ] **Step 2: Remove the storyboard key from `Info.plist`**

```bash
/usr/libexec/PlistBuddy -c "Delete :NSMainStoryboardFile" /Users/orenleavitt/Workspace/gem/Mac/Gem/Gem/Info.plist
```
(`NSPrincipalClass = NSApplication` stays — it's the SwiftUI default.)

- [ ] **Step 3: Remove the AppKit files from the project**

```bash
ruby -e '
require "xcodeproj"
proj = Xcodeproj::Project.open("/Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj")
names = ARGV
proj.files.select { |f| f.path && names.any? { |n| f.real_path.to_s.end_with?(n) } }.each(&:remove_from_project)
proj.save
' "AppDelegate.swift" "Main.storyboard" "MainViewController.swift" "ImageViewController.swift" "AppCoordinator.swift" "Constants.swift"
```

- [ ] **Step 4: Delete the files from disk (git)**

```bash
git rm "Gem/AppDelegate.swift" "Gem/Main.storyboard" \
  "Gem/View Controllers/MainViewController.swift" "Gem/View Controllers/ImageViewController.swift" \
  "Gem/Coordinators/AppCoordinator.swift" "Gem/Common/Constants.swift"
```

- [ ] **Step 5: Build and run the tests**

Run the build command, then the unit-test command.
Expected: BUILD SUCCEEDED (one `@main`, no storyboard) and all `GemTests` PASS.

- [ ] **Step 6: Launch the app to confirm it starts**

```bash
xcodebuild build -project /Users/orenleavitt/Workspace/gem/Mac/Gem/Gem.xcodeproj -scheme Gem -configuration Debug -destination 'platform=macOS' -derivedDataPath /tmp/gem-dd -quiet
open /tmp/gem-dd/Build/Products/Debug/Gem.app
```
Expected: the app launches showing a single window with a controls sidebar (Scene / Output Resolution) and an empty render canvas ("No Render"), plus File ▸ Open Scene…/Export Image… and a Render menu. Manually choose a scene and press Render (⌘R) to confirm a progressive render and that Stop (⌘.) works; then quit.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "Switch app to SwiftUI lifecycle; remove AppKit storyboard shell"
```

---

## Self-Review

**Spec coverage:**
- Reused backend untouched (RenderEngine/Renderer/PixelColor/AppData/tests) → Global Constraints; no task modifies them. ✓
- Removed AppKit (AppDelegate, Main.storyboard, both VCs, AppCoordinator, Constants) + Info.plist key + macOS 14 + SwiftUI lifecycle → Tasks 2 (target bump) & 4 (deletions, `@main`, plist). ✓
- `RenderModel` (`@MainActor @Observable`, phases, image, start/stop, injectable renderer) → Task 2. ✓
- `RenderImageBuffer` (rows → CGImage, ARGB) → Task 1. ✓
- Single `Window` + `NavigationSplitView` (controls sidebar + canvas) + toolbar (Start/Stop, progress, elapsed) → Tasks 3 (ContentView) & 4 (entry). ✓
- `Settings` scene (include/search paths) → Task 3 (SettingsView). *Deviation from spec:* default-resolution dropped from Settings (YAGNI — resolution is editable in the main window and persisted via `AppData` on render). ✓
- `Commands` menus: Open Scene…/Export Image…/Render Start/Stop → Task 3 (GemCommands). ✓
- Image export (PNG/TIFF) → Tasks 1 (`ImageEncoding`) & 3 (`ImageDocument` + `.fileExporter`). *Refinement from spec:* encoding lives in `ImageEncoding` (testable) used by `ImageDocument`, rather than a `RenderModel.export` method — same outcome, DRY and unit-tested. ✓
- `.fileImporter` for scene selection → Task 3 (ContentView + GemFileTypes). ✓
- Error handling via `.alert` on `.failed`; cancellation via Task cancel → Tasks 2 & 3. ✓
- Testing: kept engine tests incl. `RayTraceRepeatedRenderTests`; added `RenderImageBuffer`, `ImageEncoding`, `RenderModel` tests; views build-verified → Tasks 1–4. ✓

**Placeholder scan:** No TBD/TODO/"handle edge cases"/"similar to Task N" — every code step is complete. ✓

**Type consistency:** `PixelColor(a:r:g:b:)`, `RenderImageBuffer(width:height:)`/`setRow(_:at:)`/`pixel(atX:y:)`/`makeCGImage()`, `ImageEncoding.data(from:type:)`, `PixelSize(width:height:)`/`nsSize`/`isRenderable`, `RenderModel` members (`sceneURL`, `resolution`, `phase`, `image`, `isPresentingImporter`/`isPresentingExporter`, `isRendering`, `canStart`, `progress`, `failureMessage`, `isShowingFailureAlert`, `start()`/`stop()`/`waitUntilIdle()`), `RenderEngine.render(imageSize:bounds:aspectMode:)`, `ImageDocument(image:)` are used identically across tasks. ✓
