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


## Full Send UX v0.4.0

Added in one integrated interaction pass:

- first-class hover/preselection presentation;
- selection-count HUD;
- left→right window and right→left crossing box selection;
- Ctrl+box subtraction;
- cursor-local Select Other popup with forward/back cycling and Enter acceptance;
- automatic bottom-right measurement HUD;
- movable/resizable feature dialogs;
- independently resizable Feature/Parts panel regions;
- custom semantic Top/Front/Right viewport labels while retaining FreeCAD plane pick targets;
- compact/contextual toolbar presentation;
- real Line and Corner Rectangle sketch-tool first slices;
- shared sketch inference policy with Shift suppression;
- updated roadmap and verification markers.


## v0.5.0

GUI/action-reaction pass based on the 2026-09-25 screencast:

- toolbar graphics enlarged and replaced with custom icon artwork;
- icon-only controls have FreeShape name/shortcut hover bubbles;
- active sketch tools visibly latch and toggle off with the same command;
- Escape exits the active tool before cancelling the parent Sketch;
- Center rectangle added;
- viewport default-plane subobject picks normalized to Top/Front/Right;
- hover/preselection participates in Hide and Normal-to behavior;
- feature dialog selection fields follow live selections;
- plane-specific context menu behavior added;
- off-screen reference labels no longer clamp to unrelated screen edges.
