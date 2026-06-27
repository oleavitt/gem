# Mac Frontend Async/Concurrency Refactor — Design

**Date:** 2026-06-26
**Component:** `Mac/Gem` (macOS AppKit frontend for the Gem ray tracer)

## Goal

Make the macOS frontend thread-safe and adopt modern Swift concurrency
(`async`/`await`, actors, structured cancellation) while preserving current
behavior. No change to rendering parallelism — the underlying C engine remains a
single global context and renders stay one-at-a-time.

### Decisions (confirmed with user)

- **Scope:** Thread-safety + async/await. Serialize the C engine, replace
  per-pixel main-queue dispatch with batched/streamed updates, adopt
  structured concurrency. No multi-core rendering.
- **Strictness:** Pragmatic — `@MainActor` on AppKit code, an actor around the
  engine, `Sendable` where natural. Do not fight the compiler over the legacy C
  bridge; full Swift 6 strict-concurrency mode is *not* a goal.
- **Cancellation:** Implement real Start/Stop. The Stop button must halt a render
  mid-frame and tear the engine down cleanly. (Today `stop()` is an empty TODO.)

## Current Architecture (and its problems)

- `MainViewController` — config UI; fires `renderStartNotification` via
  `NotificationCenter`.
- `ImageViewController` — observes the notification, builds a `BasicViewPort`,
  acts as its `ViewPortDelegate`, writes pixels into an `NSBitmapImageRep`.
- `BasicViewPort` — runs the render loop on `DispatchQueue.global()`, then does
  **one `DispatchQueue.main.async` per pixel** back to the delegate.
- `RayTraceRenderer` — wraps a global, single-context, non-thread-safe C engine
  (`Ray_Initialize`, `scn20_parse`, `Ray_TraceRayFromViewport`, `Ray_Close`, …).

Problems:

1. **Main-queue flooding:** ~40,000 `DispatchQueue.main.async` closures for a
   200×200 image.
2. **No real cancellation:** `stop()` / `stopRendering()` are no-op TODOs; a
   render can only end by running to completion.
3. **Unserialized global C state:** renders run on a concurrent global queue with
   no guard against overlap.
4. **Shared mutable `pixelBuffer` pointer** written from delegate callbacks.
5. **No `@MainActor` isolation** on AppKit code.
6. **Per-pixel `CGColor` allocation** in the hot loop, read back via `.components`.
7. **Silent setup failures:** `RayTraceRenderer.setup` `return`s on parse/setup
   failure with no signal to the UI.

## Target Architecture

### 1. `PixelColor` — `Sendable` value type

Replaces `CGColor` in the hot loop.

```swift
struct PixelColor: Sendable {
    var r, g, b, a: UInt8
}
```

Eliminates per-pixel `CGColor` allocation and makes pixel data trivially
`Sendable` across the concurrency boundary.

### 2. `Renderer` protocol (synchronous compute, engine-owned)

```swift
protocol Renderer {
    mutating func setup(viewPortBounds: CGRect) throws
    func color(at pointOnViewPort: CGPoint) -> PixelColor
    func finished()
}
```

- `setup` now `throws`; `RayTraceRenderer.setup` throws `RenderError` on
  `scn20_parse` / `Ray_Setup` failure instead of silently returning.
- The empty `extension Renderer { … }` default-method block is removed.
- `TestRenderer` and `RayTraceRenderer` updated to return `PixelColor`.

### 3. `RenderEngine` actor (replaces `BasicViewPort` threading)

Owns the `Renderer` and optional `OutputFile`. Serializes all access to the
global C state — the render loop is actor-isolated, so two renders can never run
concurrently (a second `render` waits for the first).

```swift
actor RenderEngine {
    init(renderer: Renderer, outputFile: OutputFile?)
    func render(imageSize: NSSize,
                bounds: CGRect,
                aspectMode: ViewPortAspectMode) -> AsyncStream<RenderUpdate>
}
```

- The hot loop does **not** `await` between rows, so the actor is not re-entered
  mid-frame.
- The loop checks `Task.isCancelled` once per row and stops promptly when set.
- The stream is built with the continuation form; `continuation.onTermination`
  cancels the producing task so that consumer cancellation (Stop, or the task
  being torn down) propagates into the loop.
- `renderer.finished()` (engine teardown) runs in a `defer` so it executes on
  normal completion **and** on cancellation/failure.

### 4. `RenderUpdate` enum (`Sendable`)

```swift
enum RenderUpdate: Sendable {
    case started(width: Int, height: Int)
    case row(y: Int, pixels: [PixelColor])
    case failed(RenderError)
    case finished
}
```

Updates are streamed **per row** (~200 updates for a 200-tall image) rather than
per pixel.

### 5. `ImageViewController` — `@MainActor`

```swift
@MainActor
final class ImageViewController: NSViewController {
    private var renderTask: Task<Void, Never>?
}
```

- **Start:** cancel any in-flight `renderTask`, then
  `renderTask = Task { for await update in engine.render(...) { apply(update) } }`.
- **`apply(_:)`** writes each `.row` into the `NSBitmapImageRep` (local row
  buffer — the shared `pixelBuffer` pointer is removed) and **throttles**
  `setNeedsDisplay` (e.g. every N rows or a short time interval).
- `.failed` surfaces the error to the UI (alert / window title); `.finished`
  records elapsed time and clears `renderTask`.
- **Stop:** `renderTask?.cancel()`.

### 6. Isolation & cleanup

- `@MainActor` on the AppKit view controllers (`ImageViewController`,
  `MainViewController`).
- Remove `ViewPort` / `ViewPortDelegate` protocols and `BasicViewPort` — fully
  superseded by `RenderEngine` + `AsyncStream`.
- `OutputFile.savePixel` adapts to `PixelColor`; the engine writes to the output
  file inside the (serialized) loop.

## Data Flow

```
MainViewController --(renderStartNotification)--> ImageViewController
ImageViewController @MainActor:
    renderTask = Task {
        for await update in engine.render(...) {   // RenderEngine actor
            apply(update)                            // bitmap write + throttled redraw
        }
    }
RenderEngine actor (off main):
    setup() throws  ->  loop rows { color(at:) -> PixelColor; check cancel; yield .row }
    defer { renderer.finished() }
```

## Error Handling

- `RenderError` enum covers scene-parse and engine-setup failures.
- Setup failure is yielded as `.failed(error)` (the stream still `finish()`es
  cleanly) and presented in the UI rather than silently swallowed.
- Cancellation is not an error: the loop exits, `defer` tears down the engine,
  the stream finishes.

## Testing

- `TestRenderer` (pure Swift, deterministic) drives `RenderEngine` in unit tests
  with no C dependency.
- Tests:
  - Streamed updates arrive in order: `.started`, rows `0..<height`, `.finished`.
  - Each `.row` has `width` pixels.
  - Cancelling the consuming task stops further `.row` emissions promptly and
    still delivers engine teardown (via `defer`).
  - `setup` throwing produces a single `.failed` followed by stream completion.

## Out of Scope (kept as-is)

- The `NotificationCenter` start/stop trigger between `MainViewController` and
  `ImageViewController`.
- The empty `AppCoordinator`.
- `RawOutputFile`'s empty method bodies.
- Multi-core / parallel rendering.
- Full Swift 6 strict-concurrency mode.
