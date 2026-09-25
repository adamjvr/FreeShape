# FreeShape Product Roadmap

This roadmap is ordered by product leverage, not by how many FreeCAD modules
exist. The goal is an Onshape-class modeling workflow on native Linux, not
feature-count parity with FreeCAD's Workbenches.

## Definition of done for every roadmap item

A feature is not done because the backend can technically perform it. It is
done when:

- the normal workflow is reachable from FreeShape UI/keyboard;
- common paths require no more deliberate user actions than the documented
  Onshape equivalent unless the engine makes that impossible;
- cancel/undo/redo are deterministic;
- the operation survives FCStd save/reload;
- the feature participates in semantic history and selection;
- tests and diagnostic markers exist;
- the patch is committed and pushed to `origin/main`.

---

## Phase 0 — Engine embedding foundation — substantially complete

Completed:

- pinned FreeCAD `main` source submodule;
- independent FreeCAD build;
- C++23 FreeCAD consumer bridge;
- FreeCAD App/Gui lifecycle;
- FreeShape-owned visible top-level window;
- populated embedded Coin viewport;
- clean startup/shutdown;
- FCStd round-trip smoke model;
- stale-binary prevention;
- deterministic diagnostic ZIP workflow.

Remaining cleanup:

- audit/remove hidden `Gui::MainWindow` dependency if feasible;
- turn bootstrap smoke model into formal automated engine test;
- stop relying on Python snippets for product feature creation as native CAD
  services mature.

Exit gate: embedding remains stable while product code grows.

---

## Phase 1 — Part Studio interaction foundation — CURRENT

### 1A Shell and default geometry

- [x] document bar foundation
- [x] feature toolbar foundation
- [x] Feature / Parts panel
- [x] document tabs shell
- [x] blank Part Studio
- [x] Origin / Top / Front / Right engine-backed reference geometry
- [x] reference plane labels and visibility
- [ ] resizable reference-plane display handles
- [x] independently resizable/scrollable Features and Parts regions
- [ ] production icon set

### 1B SelectionService

- [x] ordinary-click additive/toggle foundation
- [x] Space clears selection
- [x] initial Select Other picked-list plumbing
- [x] first-class hover/preselection state
- [x] engine-backed screen-space prehighlight surfaced through FreeShape HUD
- [x] selection count viewport badge
- [x] true Select Other popup at cursor
- [x] grave / Shift+grave candidate cycling
- [x] Enter accepts Select Other candidate
- [x] left→right containment box selection
- [x] right→left crossing box selection
- [x] Ctrl+box subtract
- [ ] feature-list ↔ viewport cross-highlighting
- [ ] active dialog selection-field routing

### 1C Navigation

- [x] Onshape-like RMB rotate / MMB pan / wheel zoom
- [x] F fit
- [x] standard Shift+1…7 views
- [x] Z / Shift+Z zoom
- [ ] N normal-to arbitrary hovered/selected face
- [ ] second N reverses normal
- [ ] Arrow ±15° rotation
- [ ] Shift+Arrow ±90°
- [ ] Ctrl+Arrow ±5°
- [ ] Ctrl+Shift+Arrow pan
- [ ] W zoom window
- [ ] animated orient-normal-to-sketch
- [ ] NavigationProfile abstraction for future SolidWorks/Creo/NX mappings

### 1D Command access

- [x] CommandRegistry
- [x] S cursor shortcut palette
- [x] context-sensitive Part Studio vs Sketch S palette
- [x] Alt+C Search Tools command palette
- [ ] user-configurable S toolbar
- [ ] command aliases/fuzzy ranking
- [ ] preferences editor for shortcuts

### 1E Dialog framework

- [x] floating FeaturePopup proof
- [x] selection-vs-keyboard visual field distinction
- [x] Enter / Esc basics
- [x] transaction-backed Extrude live preview
- [x] movable dialog
- [x] resizable dialog
- [ ] Tab field traversal contract
- [ ] Shift+Enter accept + repeat
- [ ] generic `FeatureDefinition` parameter schema
- [ ] generic ActiveInputSink for selections
- [ ] Preview opacity slider
- [ ] Final-state button when editing older features

### 1F Automatic measurement

- [x] bottom-right selection measurement HUD (edge length / face area / volume / point / picked-point distance)
- [ ] `[` detailed Measure panel
- [ ] edge length/radius
- [ ] face area
- [ ] two-entity minimum/maximum/parallel distance
- [ ] angle
- [ ] unit selection and copy/full precision

Exit gate: basic navigation, selection, command discovery and dialogs feel like
one coherent FreeShape interaction system rather than wrapped FreeCAD behavior.

---

## Phase 2 — Sketcher UX — ACTIVE IN PARALLEL WITH PHASE 1

### 2A Geometry tools

- [x] Sketch command / plane-selection foundation
- [x] Top-plane center-point Circle first slice
- [x] live circle rubber-band overlay
- [x] immediate diameter entry
- [x] Line first slice
- [x] corner rectangle first slice
- [ ] center rectangle
- [ ] 3-point arc
- [ ] spline
- [ ] point
- [ ] slot
- [ ] polygon
- [ ] ellipse

### 2B InferenceEngine

Replace tool-specific coordinate tolerance hacks with a unified screen-space
candidate graph.

- [ ] origin/coincident
- [ ] horizontal
- [ ] vertical
- [ ] midpoint
- [ ] parallel
- [ ] perpendicular
- [ ] tangent
- [ ] concentric
- [ ] equal
- [ ] wake-up geometry
- [ ] inference glyphs/guides
- [x] Shift temporarily suppresses automatic inference

### 2C Constraints and dimensions

Keyboard vocabulary:

- `D` dimension
- `Q` construction
- `I` coincident
- `H` horizontal
- `V` vertical
- `B` parallel
- `Shift+L` perpendicular
- `T` tangent
- `E` equal
- `Shift+M` midpoint
- `Shift+O` concentric
- `Shift+Q` symmetric
- `Shift+J` fix

Add:

- [ ] constraint glyph rendering
- [ ] under/fully/over-constrained status
- [ ] direct numeric line/rectangle dimensions
- [ ] dimension drag/edit
- [ ] projected/use geometry
- [ ] trim/extend/offset

### 2D Sketch support transforms

- [x] canonical Top/XY screen→sketch mapping
- [ ] Front/XZ
- [ ] Right/YZ
- [ ] arbitrary planar faces
- [ ] offset datum planes
- [ ] mate connector / local coordinate support later

Exit gate: the supplied Onshape screencast workflow can be reproduced naturally
and repeatedly without FreeCAD Sketcher visible workflow.

---

## Phase 3 — Core Part Studio features

Implement through `FeatureService`, not task-panel wrappers.

Order:

1. Extrude: New/Add/Remove/Intersect; Blind/Through/Up-to
2. Revolve
3. Fillet
4. Chamfer
5. Hole
6. Shell
7. Draft
8. Sweep
9. Loft
10. Boolean
11. Linear pattern
12. Circular pattern
13. Mirror
14. Transform
15. Split / Delete face / Replace face as needed

Each feature gets:

- declarative parameter schema;
- selection policy;
- preselection seeding;
- live preview transaction;
- semantic history item;
- edit/reopen behavior;
- FCStd regression model;
- error presentation.

Exit gate: ordinary single-part mechanical modeling is productive.

---

## Phase 4 — HistoryService and robust editing

- [ ] authoritative semantic global feature order
- [ ] draggable rollback bar
- [ ] Up/Down rollback keyboard navigation
- [ ] multi-feature drag reorder
- [ ] suppress/unsuppress
- [ ] feature folders
- [ ] edit old feature with rollback state
- [ ] Preview opacity blend
- [ ] Final result toggle
- [ ] error/missing-selection indicators
- [ ] filter syntax `:part`, `:type`, `:name`, `:errors`, `:warnings`,
      `:folder`, `:variable`, `:suppressed`, `:hidden`, `:shown`

Exit gate: feature history behaves like a modern parametric timeline rather than
a FreeCAD object tree.

---

## Phase 5 — ReferenceService / topological resilience

- [ ] FreeShape stable ObjectId / FeatureId / SelectionRef
- [ ] FreeCAD element mapping integration
- [ ] persistent subshape references
- [ ] geometric repair hints
- [ ] missing-reference UI
- [ ] explicit reference repair workflow
- [ ] golden models for topology-sensitive edits

Exit gate: ordinary upstream edits do not casually break dependent features.

---

## Phase 6 — Multi-part Part Studio

- [ ] create New/Add/Remove/Intersect parts correctly across Bodies
- [ ] Parts list backed by semantic PartId
- [ ] part appearance/name/visibility
- [ ] multi-part booleans
- [ ] Derived/insert-style reuse
- [ ] composite part strategy
- [ ] cross-part feature references

Exit gate: FreeShape Part Studio semantics are not constrained by one
PartDesign Body.

---

## Phase 7 — Documents, tabs and application lifecycle

- [ ] real new/open/save/save-as
- [ ] recent documents
- [ ] multiple Part Studio tabs
- [ ] `+` tab insertion menu
- [ ] tab drag reorder
- [ ] tab context menus
- [ ] Ctrl+Space recent-tab switching
- [ ] per-tab undo/session state
- [ ] autosave/recovery
- [ ] preferences
- [ ] units

---

## Phase 8 — Assembly

Do not start before Part Studio architecture is mature.

- [ ] Assembly tab
- [ ] insert parts/assemblies
- [ ] mate connectors
- [ ] fastened/revolute/slider/etc.
- [ ] snap mode
- [ ] mate visibility
- [ ] assembly tree
- [ ] motion preview

Reuse current FreeCAD Assembly App capabilities where appropriate, but expose a
FreeShape interaction model.

---

## Phase 9 — Drawings and manufacturing contexts

- [ ] Drawing tab / TechDraw adapter
- [ ] standard views
- [ ] section/detail views
- [ ] dimensions/annotations
- [ ] PDF/DXF export
- [ ] CAM tab only after Part Studio/Drawing are stable

---

## Phase 10 — Productization

- [ ] remove/minimize hidden FreeCAD MainWindow compatibility debt
- [ ] AppImage canonical package
- [ ] desktop integration / MIME / icons
- [ ] crash diagnostics
- [ ] preferences migration
- [ ] performance profiles
- [ ] accessibility / keyboard-only audit
- [ ] dark theme
- [ ] extension Python API
- [ ] public stable FreeShape CAD API
- [ ] release/update process

## Continuous upstream lane

In parallel with all phases:

```text
FreeCAD upstream/main
        ↓
candidate SHA
        ↓
independent engine build
        ↓
upstream baseline tests
        ↓
FreeShape adapter tests
        ↓
golden models
        ↓
interactive smoke
        ↓
accept gitlink
```

Never float against unrecorded upstream HEAD.


## v0.5.0 GUI/action-reaction checkpoint

- [x] large custom toolbar icon system
- [x] delayed name/shortcut/detail hover bubbles for icon-only tools
- [x] explicit Part Studio tool groups
- [x] explicit Sketch tool groups with Extrude/Revolve retained
- [x] active sketch-tool checked state
- [x] same-tool shortcut/click exits the tool
- [x] layered Escape exits sketch tool before containing Sketch command
- [x] center-point rectangle tool (`R`) first implementation
- [x] nested Body/Origin datum-plane selection normalization
- [x] semantic Top/Front/Right preselection naming
- [x] datum-specific context menu foundation
- [x] selection-driven feature-dialog field updates
- [x] hover Hide path
- [x] off-screen reference-label suppression

Next GUI/interaction work remains high priority: native sketch dimensions for
Line/Rectangle, richer inference, recently-used tools, toolbar customization,
eye/visibility controls in the Feature list, Preview/Final editing semantics,
and arbitrary planar-face Sketch/Normal-to transforms.
