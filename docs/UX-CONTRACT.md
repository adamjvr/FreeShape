# FreeShape UX Contract — Onshape-class interaction

FreeShape is not a FreeCAD skin.  FreeCAD is the engine.  FreeShape owns the
workflow, input language, command chaining, dialogs, selection semantics, and
visible application shell.

## Measured product rule

For common modeling operations, FreeShape should require no more explicit user
actions than the equivalent Onshape workflow unless a Linux-native constraint
makes that impossible.

## Interaction contract implemented in UX Big Pass 1

### Viewport navigation

Default desktop mouse profile:

- Right mouse drag: rotate
- Middle mouse drag: pan
- Mouse wheel: zoom
- F: fit
- Shift+1..7: standard camera views

The implementation uses FreeCAD's TinkerCAD navigation style because its mouse
mapping matches the Onshape desktop default while retaining the current Coin
viewer.

### Selection

FreeShape adds a wrapper over FreeCAD selection to approximate Onshape toggle
selection:

- click new entity: add to selection
- click selected entity: remove it
- click empty graphics space: clear selection
- Space: clear selection
- grave accent: cycle the FreeCAD picked list ("Select Other" foundation)

This is intentionally a FreeShape interaction layer; FreeCAD's Ctrl-required
multi-selection must not define product behavior.

### Shell

Visible layout now follows the Onshape Part Studio hierarchy:

- compact document bar
- context toolbar
- left Feature/Part panel
- filter field
- Origin / Top / Front / Right default geometry
- semantic Sketch / Extrude items
- rollback bar
- Parts section
- graphics area
- bottom document tabs

### Command language

- S: cursor-local shortcut palette
- Shift+S: Sketch
- Shift+E: Extrude
- Shift+F: Fillet
- Enter: accept an open feature command
- Escape: cancel an open feature command
- P: toggle datum planes
- Shift+H: toggle sketches
- Y: hide selected
- Shift+Y: show entities hidden by the last hide action
- N: normal-to foundation
- Shift+1..7: standard views

### Feature dialogs

Feature commands are floating overlays in the graphics area rather than
FreeCAD TaskPanels.

The Extrude dialog already demonstrates a real transaction-backed live preview
against the bootstrap Pad:

- open transaction
- adjust depth
- recompute live
- Enter/checkmark commits
- Escape/X aborts and restores

This is a transitional proof of the UI transaction pattern; native feature
creation will move behind `FeatureService`.

## Deliberately not claimed complete yet

- native SketchController geometry placement
- automatic sketch inferencing
- live on-the-fly dimensions
- arbitrary-face "Normal to"
- true Select Other candidate UI
- left-to-right vs right-to-left box selection semantics
- production feature creation for Extrude/Fillet/etc.
- rollback execution
- drag reorder of semantic features
- full command search
- all toolbar icons

Those are the next interaction passes, not reasons to fall back to FreeCAD's
visible workflow.
