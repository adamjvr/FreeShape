# FreeShape

FreeShape is a Linux-native parametric solid-modeling CAD application with an
Onshape-class Part Studio workflow, powered by a pinned development build of
FreeCAD.

FreeCAD supplies the expensive CAD infrastructure: document/recompute,
PartDesign, Sketcher, OCCT geometry, FCStd compatibility, ViewProviders and the
initial Coin3D viewport. FreeShape owns the visible application, selection
semantics, input language, feature workflow, dialogs, history presentation and
Linux desktop UX.

## Current state

**UX foundation / early Part Studio implementation.**

Working foundations include:

- native C++23 / Qt 6 application shell;
- current FreeCAD `main` embedded as a pinned Git submodule;
- independently built FreeCAD engine; no `add_subdirectory(FreeCAD)`;
- FreeCAD App + Gui lifecycle inside a FreeShape-owned top-level window;
- embedded populated `View3DInventor` viewport;
- blank Part Studio with Origin / Top / Front / Right reference geometry;
- FreeShape Feature/Parts panel and document-tab shell;
- FCStd create → save → close → reopen regression gate;
- Onshape-like RMB orbit / MMB pan / wheel zoom profile;
- FreeShape command registry, keyboard commands and cursor shortcut palette;
- first native SketchController path for Top-plane center-point circles;
- first Sketch → Extrude transaction/preview handoff;
- deterministic diagnostic ZIPs after each build/run.

Pinned FreeCAD commit:

```text
df7dd2eefe5afa54e86b65b5a7a1ce721907c721
```

## Target

- Linux x86_64 first
- C++23
- Qt 6 Widgets
- Wayland first; Qt/XCB fallback
- CMake + Ninja
- Pixi environment shared with the pinned FreeCAD build
- FreeCAD/OCCT as CAD engine
- FreeCADGui/Coin3D as the current rendering/picking backend
- FCStd as the native persistence container
- AppImage as first release package

## Development rule

A patch is not complete until it:

1. builds,
2. passes the interactive/runtime gate,
3. commits,
4. pushes `main`,
5. verifies `origin/main` equals local `HEAD`.

Canonical workflow:

```bash
./scripts/patch-cycle.sh "Describe the patch" ./scripts/ux-big-pass-3.sh
```

See:

- `docs/ROADMAP.md`
- `docs/ARCHITECTURE.md`
- `docs/UX-SPEC.md`
- `docs/PATCH-WORKFLOW.md`
- `docs/STATUS.md`
- `docs/TESTING.md`
