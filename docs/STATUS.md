# FreeShape Status

## Current baseline

- Product shell: native Linux Qt 6
- Language: C++23
- Engine: FreeCAD `main`
- FreeCAD pin: `df7dd2eefe5afa54e86b65b5a7a1ce721907c721`
- Rendering/picking: FreeCADGui + Coin3D
- Persistence: FCStd
- Visible editor: blank Part Studio
- Current roadmap phase: Phase 1 / Part Studio interaction foundation

## Proven

- independent FreeCAD build;
- FreeShape external consumer bridge;
- embedded FreeCAD App + Gui lifecycle;
- FreeShape-only visible main window;
- populated embedded 3D viewport;
- FCStd create/save/close/reload gate;
- clean `rc0` builds and runs;
- default PartDesign origin/reference planes visible;
- basic Onshape-like navigation profile;
- semantic Feature/Parts panel shell;
- first sketch/circle/extrude controller code path;
- stale executable prevention;
- deterministic diagnostics.

## Current pass

UX Big Pass 3:

- semantic Top / Front / Right viewport labels;
- larger default reference-plane presentation;
- context-sensitive `S` palette;
- `Alt+C` Search Tools command palette;
- Z / Shift+Z zoom commands;
- mandatory build → run → commit → push patch cycle;
- cumulative roadmap/documentation.

## Highest-priority next engineering work

1. hover/preselection service;
2. real Select Other popup;
3. directional box selection;
4. screen-space sketch inference engine;
5. Line + rectangle sketch tools;
6. generic FeatureDialogController;
7. automatic Measure HUD;
8. arbitrary-face Normal To;
9. real rollback/history controller;
10. native FeatureService replacing Python bootstrap snippets.
