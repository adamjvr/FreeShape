# UX Big Pass 2 — Real Sketch → Extrude

## Why this pass exists

UX Big Pass 1 established the shell and input vocabulary.  This pass removes
the biggest remaining fake: the visible bootstrap box.

The FCStd/engine smoke model now runs in a separate hidden document.  The
visible FreeShape session begins as a blank Part Studio with Body + default
reference geometry and no Sketch, Extrude, or Part.

## Canonical workflow

The first end-to-end modeling benchmark is now:

```
Shift+S
click Top
C
click center
click radius
type diameter
Enter
Shift+E
type/edit extrusion depth
Enter
```

That workflow is based on Onshape's documented interaction:

- Shift+S opens Sketch and asks for a sketch plane.
- C toggles Center Point Circle.
- first click = center, second click = radius.
- immediately typing a value after circle creation dimensions its diameter.
- Shift+E from the open sketch accepts the sketch and opens Extrude.
- closed regions in the open sketch are automatically supplied to Extrude.

## Implementation

### SketchController

New FreeShape-owned state machine:

```
Idle
  ↓ Shift+S
AwaitPlane
  ↓ Top
Editing
  ↓ C
Circle:center
  ↓ click
Circle:radius
  ↓ click
Circle geometry + optional diameter constraint
```

The controller owns interaction.  FreeCAD Sketcher remains the parametric
solver/data model beneath it.

### Cursor-to-sketch mapping

For the canonical Top-plane workflow, viewport pixels are projected through
the active Coin camera into a ray and intersected with the XY sketch plane.
This gives actual model-space coordinates without invoking FreeCAD TaskPanels
or Sketcher command handlers.

### Live sketch preview

A transparent Qt overlay draws the in-progress circle between center and
radius clicks.  This is deliberately FreeShape UI; it does not rely on a
FreeCAD sketch task panel.

### First inference behavior

The circle center snaps to the X axis, Y axis, or origin when sufficiently
close in model space and shows dotted inference guides.  This is the first
slice of the future screen-space inference graph.

### Immediate diameter entry

After the second circle click a compact numeric field appears beside the
geometry.  Entering a value and pressing Enter adds a real Sketcher Diameter
constraint.

### Sketch → Extrude handoff

Shift+E while sketching:

1. commits the sketch transaction,
2. treats Sketch 1 as the profile automatically,
3. opens an Extrude transaction,
4. creates Pad/Extrude 1,
5. immediately shows the live solid preview,
6. keeps Depth live,
7. Enter commits; Escape aborts.

## Still next

- screen-space wake-up inferencing to arbitrary existing geometry
- Line/rectangle tools
- geometry editing/dragging
- arbitrary datum/face coordinate transforms, not only canonical XY interaction
- constraint glyphs
- direct dimensions for lines/arcs
- sketch element selection priority
- box selection semantics
- manipulator arrow for Extrude depth

Those now sit on top of a real FreeShape interaction pipeline rather than
FreeCAD's visible workflow.
