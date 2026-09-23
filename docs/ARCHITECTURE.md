# FreeShape Architecture

## Product definition

FreeShape is a native Linux CAD application which embeds FreeCAD as a modeling
engine. It is not a reskin of FreeCAD and does not expose FreeCAD Workbenches,
Combo View or TaskPanels as product concepts.

```text
FreeShape application shell / interaction
                │
        FreeShape CAD services
                │
          FreeCAD adapter
        ┌───────┴────────┐
        │                │
   FreeCAD App       FreeCAD Gui
 Part/PartDesign     ViewProviders/Coin
    Sketcher              │
        └─────── OCCT ────┘
```

## Locked platform decisions

- Linux x86_64 during initial product development.
- C++23; current pinned FreeCAD development code requires it.
- Qt 6 Widgets.
- Wayland-first with Qt/XCB fallback.
- One native process.
- No Electron/browser/RPC frontend boundary.
- CMake + Ninja.
- FreeCAD and FreeShape are separate CMake universes.
- The same pinned Pixi environment supplies compiler, Qt, Python and binary
  dependencies to avoid ABI drift.

## FreeCAD dependency policy

`third_party/FreeCAD` is an upstream `main` Git submodule recorded at one exact
tested SHA. It never follows floating HEAD during normal builds.

FreeCAD is configured and built as its own top-level project. FreeShape then
consumes its build/source trees through `FreeShape::FreeCADBridge`.

The bridge exists because FreeCAD is not currently exported as a polished C++
consumer SDK. It reconstructs the transitive compile/link contract for:

- FreeCADBase / FreeCADApp / FreeCADGui
- Python / PyCXX
- FastSignals
- bundled Coin
- Qt modules
- generated FreeCAD configuration headers

All such build-system compatibility must stay inside that boundary.

## Document model

`App::Document` is the sole CAD source of truth. FreeShape does not maintain a
second geometry/document database.

FreeShape metadata may live as persistent dynamic FreeCAD properties, but the
geometry, feature dependency graph, recompute state and transactions remain in
the FreeCAD document.

## Part Studio model

A FreeShape Part Studio is a semantic container over one or more PartDesign
Bodies.

- one `PartDesign::Body` = one stable logical Part;
- do not equate an entire Part Studio with one Body;
- do not depend on experimental multi-solid Body behavior;
- FreeShape presents one global semantic feature history over the underlying
  engine objects.

## Interaction ownership

FreeCAD answers engine questions:

- what was picked;
- how a shape recomputes;
- how a Sketch constraint solves;
- how a feature is persisted;
- how ViewProviders represent geometry.

FreeShape decides product behavior:

- what one click means;
- additive/toggle selection;
- hover/preselection;
- context toolbars;
- keyboard shortcuts;
- command chaining;
- dialog routing;
- Part Studio history;
- Sketch inferencing;
- navigation profile;
- automatic measurement presentation.

This boundary is non-negotiable.

## Viewport

V1 uses FreeCADGui + ViewProviders + Coin3D because that preserves mature
rendering, picking, datum/sketch representation and camera plumbing.

The viewport is an implementation backend, not the interaction API. FreeShape
routes mouse/keyboard behavior before or around it through
`ViewportInteractionFilter` and subsequent input services.

A future renderer can replace Coin behind an `IViewportBackend` without
changing CAD/document semantics.

## Sketch architecture

FreeCAD Sketcher remains the solver and persisted sketch model.

FreeShape owns:

- active sketch-tool state;
- mouse → sketch coordinate mapping;
- inference candidates;
- snapping;
- direct numeric entry;
- preview geometry;
- constraint creation policy;
- Sketch → feature command handoff.

The current `SketchController` is the first implementation slice.

## Transactions

There is one modeling undo system: FreeCAD document transactions.

Feature preview pattern:

```text
open transaction
create/update temporary feature
recompute
live preview
    ├─ accept → commit transaction
    └─ cancel → abort transaction
```

Do not put model edits into a separate Qt undo stack.

## Threading

Live document mutation, FreeCADGui and ViewProvider access remain on the GUI
thread. FreeShape does not wrap live FreeCAD objects in an independent geometry
thread pool. FreeCAD decides which recompute work is worker-safe.

## Persistence

FCStd is the native container during initial development. A FreeShape-created
document should remain intelligible to normal FreeCAD wherever practical.

## Temporary compatibility debt

Current FreeCADGui still contains global `Gui::getMainWindow()` assumptions.
FreeShape therefore keeps a hidden FreeCAD `Gui::MainWindow` compatibility
anchor while the visible shell is entirely FreeShape-owned.

Removing or upstream-decoupling that dependency is a later engine-integration
task; it must not leak into the visible UX.
