# Mac Frontend SwiftUI Conversion — Design

**Date:** 2026-06-27
**Component:** `Mac/Gem` (macOS frontend for the Gem ray tracer)
**Branch:** `refactor/convert-to-swiftui` (builds on the merged async/concurrency work)

## Goal

Replace the AppKit/storyboard frontend with a 100% SwiftUI implementation,
reimplemented optimally for SwiftUI rather than preserving the old look and feel.
The async render backend (`RenderEngine` actor + `AsyncStream`) is reused
unchanged. The rebuild also fixes the old window-placement problems and finishes
two gaps: functional image export and a Preferences window.

### Decisions (confirmed with user)

- **Deployment target:** raise to **macOS 14** to use modern SwiftUI windowing,
  menus, file dialogs, the `Settings` scene, and the `@Observable` macro.
- **Window architecture:** a **single unified window** (controls + render canvas),
  not the old separate controls/output windows.
- **Feature scope:** straight UI port **plus** finishing the obvious gaps —
  working image export (PNG/TIFF, currently a non-functional stub) and a SwiftUI
  Settings window for include/search paths and default resolution.
- **Base:** this branch, on top of the merged `RenderEngine`/`AsyncStream`
  backend.

## Reused As-Is (render backend, no changes)

`RenderEngine`, `Renderer`/`RayTraceRenderer`/`TestRenderer`, `RenderTypes`
(`RenderUpdate`/`RenderError`/`ViewPortAspectMode`), `PixelColor`,
`OutputFile`/`RawOutputFile`, `AppData`, `CGRect+Aspect`, the C bridge
(`GemCtoSwift`, `Gem-Bridging-Header.h`), and the existing engine tests
(including `RayTraceRepeatedRenderTests`).

## Removed (AppKit UI)

- `AppDelegate.swift` (`@NSApplicationMain`)
- `Main.storyboard`
- `View Controllers/MainViewController.swift`, `View Controllers/ImageViewController.swift`
- `Coordinators/AppCoordinator.swift` (empty)
- `Common/Constants.swift` (the `renderStart`/`renderStop` `NotificationCenter`
  names — superseded by direct model calls)
- `Info.plist`: remove `NSMainStoryboardFile`; the app adopts the SwiftUI `App`
  lifecycle. `MACOSX_DEPLOYMENT_TARGET` → `14.0`.

`String+Localization` and `Localizable.strings` are retained; SwiftUI views use
`LocalizedStringKey` directly and the render-time string can stay in the catalog.
`BinaryInteger+String.swift` is unused and left untouched (out of scope).

## Architecture

A `@MainActor @Observable RenderModel` is the single source of UI truth. It owns
the `RenderEngine`, drives renders, and publishes a `CGImage` + progress for the
views. The SwiftUI scene graph is one `Window`, a `Settings` scene, and a
`Commands` block; menu/toolbar actions call the model.

```
Menu/Toolbar ──▶ RenderModel.start()/stop()/export()
                     │
                     ▼
              RenderEngine.render(...) ──AsyncStream<RenderUpdate>──▶ for-await (MainActor)
                     │                                                     │
                     ▼                                                     ▼
             RenderImageBuffer.setRow(_:at:)              throttled makeCGImage() ─▶ model.image
                                                                                        │
                                                                                        ▼
                                                                              SwiftUI redraw
```

### Components

**`RenderModel`** (`@MainActor @Observable`, new — `Model/RenderModel.swift`)
- State: `sceneURL: URL?`, `resolution: PixelSize` (width/height), `phase`
  (`.idle` / `.rendering(progress: Double)` / `.finished(elapsed: TimeInterval)` /
  `.failed(RenderError)`), `image: CGImage?`, `renderTask: Task<Void, Never>?`.
- Init: `init(renderer: Renderer = RayTraceRenderer())` — injectable for tests.
- `start()`: validate `sceneURL`; cancel any prior task; allocate a
  `RenderImageBuffer` sized to `resolution`; set `phase = .rendering(0)`; spawn
  `renderTask` that consumes `engine.render(imageSize:bounds:aspectMode:)`
  (`bounds` = `CGRect(x:-1,y:1,width:2,height:2)`, `aspectMode: .fit`, matching
  the old behavior), writing each `.row` into the buffer, throttling
  `makeCGImage()` (≈ every 16 rows) into `image`, updating progress, and handling
  `.failed`/`.finished`.
- `stop()`: `renderTask?.cancel()`.
- `export(to: URL, format:)`: encode the current `CGImage` to PNG/TIFF.
- Loads/saves `resolution` and search paths via `AppData`.

**`RenderImageBuffer`** (new — `Model/RenderImageBuffer.swift`)
- Owns a contiguous byte buffer (`width*height*4`) in `PixelColor`'s `(a,r,g,b)`
  order, initialized to opaque black (`a=255`, rgb=0).
- `setRow(_ pixels: [PixelColor], at y: Int)` writes one row.
- `makeCGImage() -> CGImage?` builds a `CGImage` (8 bits/component, 32 bits/pixel,
  `CGColorSpace(deviceRGB)`, `bitmapInfo = premultipliedFirst | byteOrder32Big` so
  the bytes are interpreted as ARGB matching the buffer) via a `CGDataProvider`.
  With opaque alpha, premultiplied vs. straight is identical.
- Pure value logic, no SwiftUI/AppKit — unit-testable in isolation.

**`GemApp`** (`@main`, new — `App/GemApp.swift`)
- `@State private var model = RenderModel()`.
- `Window("Gem", id: "main") { ContentView().environment(model) }`
  `.commands { GemCommands(model: model) }`.
- `Settings { SettingsView() }`.
- `.windowResizability(.contentMinSize)` (or default) for sane window sizing.

**`GemCommands`** (new — `App/GemCommands.swift`)
- `File`: "Open Scene…" (⌘O, `.fileImporter` trigger), "Export Image…" (⌘S,
  enabled only when `model.image != nil`).
- `Render`: "Start" (⌘R, disabled unless a scene is set and not rendering),
  "Stop" (⌘., enabled while rendering). Replaces `CommandGroup` File-New default
  as appropriate.

**`ContentView`** (new — `Views/ContentView.swift`)
- `NavigationSplitView { SceneControlsView() } detail: { RenderCanvasView() }`.
- Toolbar: Start/Stop button (state-dependent), determinate `ProgressView` while
  rendering, elapsed-time label on finish.
- `.alert(...)` bound to `.failed` phase; hosts the `.fileImporter` /
  `.fileExporter` presenters.

**`SceneControlsView`** (new — `Views/SceneControlsView.swift`)
- Scene file row (name + "Choose…" button) and width/height numeric fields
  bound to `model.resolution`.

**`RenderCanvasView`** (new — `Views/RenderCanvasView.swift`)
- Scrollable `Image(decorative: cgImage, scale: 1)` at native pixel size, with a
  placeholder ("Open a scene to render") when `model.image == nil`.

**`SettingsView`** (new — `Views/SettingsView.swift`)
- Include/search paths editor and default-resolution fields, persisted via
  `AppData`.

### File / Group Layout (under `Gem/`)

```
App/    GemApp.swift, GemCommands.swift
Model/  RenderModel.swift, RenderImageBuffer.swift, PixelSize.swift
Views/  ContentView.swift, SceneControlsView.swift, RenderCanvasView.swift, SettingsView.swift
```
(`PixelSize` is a small `Sendable` width/height value type replacing ad-hoc
`NSSize`/`Int` pairs in the UI layer.)

## Error Handling

- Render setup/parse failure → `.failed(RenderError)` → SwiftUI `.alert` with the
  error description; no crash, engine torn down via the existing `defer`.
- Export failure (encode/write) → alert; the in-memory image is preserved.
- Cancellation (Stop / starting a new render / closing) is not an error: the task
  is cancelled and the stream ends cleanly.

## Testing

- **Kept:** all existing engine/unit tests, including `RayTraceRepeatedRenderTests`.
- **`RenderImageBuffer`:** set known rows → `makeCGImage()` returns an image of the
  expected width/height; spot-check pixel bytes for a couple of coordinates.
- **`RenderModel`:** with an injected `TestRenderer`, `start()` drives `phase` to
  `.finished` with a non-nil `image` and progress reaching 1.0; `stop()` mid-render
  leaves `phase` non-`.finished` and cancels the task without hanging.
- **Views:** verified by building and running the app (SwiftUI views are not unit
  tested).

## Out of Scope

- Changes to the C ray-tracing engine or the async render backend.
- Multi-document / multi-window scene handling (the engine is single-context).
- Zoom/pan beyond simple scrolling, render-tile previews, or batch rendering.
- Removing unrelated dead code (`BinaryInteger+String`).
- Full Swift 6 strict-concurrency mode.
