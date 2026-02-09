# Rendering & Presentation Architecture Design Notes
*(Platform-agnostic renderer, platform-defined presentation modes)*

## Goals
- Support **Android, Linux, and Windows** from a shared rendering foundation.
- Keep the renderer backend (EGL/GL/etc.) **platform-agnostic** and “dumb.”
- Allow **platform-specific presentation modes** (e.g., Android immersive behaviors) without contaminating core code.
- Establish a stable set of **presentation primitives** (rectangles + sizing) that core/UI can rely on.
- Prepare for future rendering upgrades:
  - Letterboxing / pillarboxing
  - Fixed internal render resolution (render-to-texture)
  - Integer scaling
  - Safe UI layout regions
  - Multiple rendering backends (future)

---

## Core Concepts (Spaces / Layers)

### 1) Surface / Backbuffer (System Space)
**What it is**
- The pixel area provided by the OS for rendering.

**Examples**
- Android: `EGLSurface` bound to an `ANativeWindow`
- Desktop: window framebuffer or fullscreen framebuffer

**Properties**
- Defined entirely by the platform
- Hard clipping boundary (cannot render outside it)
- Size can change due to rotation / resize / OS behavior

**Rule**
> The engine does **not** decide the surface size — it consumes it.

---

### 2) Output Rect (Presentation Space)
**What it is**
- A rectangular region *within the surface* where the engine presents its output.

**Purpose**
- Simulate “Classic Fit” style behavior (avoid system UI areas)
- Implement letterbox/pillarbox
- Control overscan and safe-area margins
- Guarantee consistent behavior regardless of OS quirks

**Implementation**
- `glViewport(x, y, w, h)`
- Optional `glScissor(x, y, w, h)` to hard-clip output

**Rule**
> Output rect is computed outside the renderer and applied as a primitive.

---

### 3) Render Target (Internal Render Frame)
**What it is**
- The resolution the engine renders into.

**Examples**
- Match output rect (simplest)
- Fixed internal resolution (e.g., 1280×720)
- Dynamic resolution scaling

**Implementation**
- Default framebuffer **or**
- Offscreen FBO + final blit/quad

**Rule**
> Render target resolution is independent of surface and output rect.

---

### 4) World / Canvas Space (Engine Space)
**What it is**
- Logical coordinate system for the game/app world and UI layout.
- Controlled by camera, projection, and layout systems.

**Relationship**
- Camera defines which slice of the canvas appears in the render target.
- Objects outside the camera view are not rendered.

**Rule**
> This space is entirely engine-level and platform-agnostic.

---

## Platform Responsibilities
Platforms provide **facts** and perform **OS-level actions**, but do not directly “own rendering.”

### Platform supplies facts
- Surface width / height (pixels)
- Insets (pixels), as applicable:
  - system bars
  - cutouts/notches
  - (later: IME/keyboard, if needed)
- Lifecycle and input events

### Platform performs OS actions
- Example (Android):
  - enable immersive mode
  - show/hide system UI
  - configure edge-to-edge behavior

### Platform does NOT
- Contain GL viewport/scissor logic inside platform files
- Decide renderer internals
- Force the OS to implement “classic fit” by layout tricks (avoid relying on deprecated/quirky behavior)

---

## Engine Responsibilities

### Presentation primitives (portable contract)
Core/UI should rely on a stable set of primitives, regardless of platform:

- `surface_size` (pixels)
- `output_rect` (pixels, inside surface)
- `safe_rect` (pixels, recommended UI region)
- `render_target_size` (pixels, if used)

**Portable contract**
> Every presentation mode produces these primitives.

---

## Renderer Backend Responsibilities (EGL / GL)
The renderer backend consumes primitives and does not interpret “modes.”

Renderer must:
- Accept surface size changes
- Accept output rect (viewport) updates
- Optionally accept scissor rect enable/disable
- Render/present frames

Renderer must remain ignorant of:
- Android APIs
- Insets meaning
- Presentation mode names (Classic/Modern/etc.)

**Rule**
> The renderer only understands rectangles and pixels.

---

## Presentation Modes (Platform-defined)
Presentation modes are **defined by the platform bridge**, and may differ across platforms.

### Key idea
Platforms expose a **list of modes** (strings or IDs). Core chooses one by config or runtime preference.

**Mode names are not required to match cross-platform.**

Examples (Android might expose):
- `fit_modern`  (edge-to-edge)
- `fit_classic` (avoid system UI areas)
- `immersive`   (fullscreen / hide system UI)

Windows/Linux might expose different ones:
- `windowed`
- `borderless_fullscreen`
- `exclusive_fullscreen`

---

## Required Output of a Presentation Mode (Portable Result)

Each mode must output a common result structure:

- `output_rect` — where the app is presented (GL viewport target)
- `safe_rect` — where UI should avoid overlap (system UI/cutouts)
- optional `platform_requests` — e.g. “request immersive,” “show system UI”

This keeps core/UI portable even when mode names differ.

---

## Architectural Data Flow

Platform (Android / Windows / Linux)

- provides surface metrics (w/h, insets)

- provides mode list

- performs OS UI actions → Presentation Mode (platform-defined)

- computes output_rect + safe_rect (+ optional requests) → Renderer Backend (EGL / GL)

- applies viewport/scissor

- draws/presents

---

## Key Design Principle

> **Insets are layout data, not rendering constraints.**  
> The engine/platform computes rects; the renderer applies rects.

---

## Why This Exists
- Modern Android defaults to edge-to-edge; OS behavior varies by device/version.
- Relying on deprecated layout behavior is fragile and inconsistent.
- Simulated presentation guarantees:
  - consistency
  - portability
  - debuggability
  - long-term stability

---

## Summary
- Surface/backbuffer = OS
- Modes = platform-defined policy
- Output/safe rects = portable primitives
- Renderer = dumb, fast, portable
- Render target & canvas = engine-level concerns
