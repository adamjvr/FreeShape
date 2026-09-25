# Full Send UX v0.4.0

This pass closes a broad slice of the interaction foundation in one checkpoint.
It intentionally avoids adding more PartDesign feature buttons until selection,
command routing, dialogs and sketch interaction behave as one coherent product.

## Selection

- Hover/preselection is surfaced as a FreeShape cursor-local pill.
- Ordinary click remains additive/toggle.
- A selection count badge lives in the graphics area.
- Drag left→right invokes window selection.
- Drag right→left invokes crossing selection.
- Ctrl+drag subtracts the resulting box candidates from the previous selection.
- Box hit-testing uses current FreeCAD main's `Gui::applyBoxSelection`, preserving
  engine hierarchy/subelement visibility while FreeShape owns the gesture/UI.

## Select Other

The grave key now opens a real candidate popup from FreeCAD's picked list.
Repeated grave cycles forward; Shift+grave cycles backward; Enter accepts the
highlighted candidate.

## Measurement

The graphics area now exposes an automatic lower-right measurement HUD. One
selected subelement produces edge length, face area, vertex point or object
volume when available. Two picked entities produce picked-point distance.
This is intentionally automatic: no Measure command is needed for the common
inspection path.

## Reference geometry

The real FreeCAD datum planes remain the selectable engine objects. Native XY /
XZ / YZ labels are hidden and FreeShape overlays semantic Top / Front / Right
labels whose screen positions follow the active camera.

## Dialogs

Feature dialogs are now movable by their header and resizable with a size grip.
Extrude focuses/selects the primary depth field when opened.

## Part Studio panel

Feature and Parts regions now live in a vertical splitter, so the user can
allocate space between history and parts rather than accepting a fixed layout.

## Sketch

The same FreeShape SketchController now supports:

- Center-point Circle (`C`)
- Line (`L`), with continuous-line behavior
- Corner Rectangle (`G`)

Rectangle creation writes real Sketcher geometry plus horizontal/vertical
constraints. Shift suppresses the current automatic origin/axis inference
policy for all tools.

## Still intentionally next

- arbitrary-geometry screen-space inference graph;
- midpoint/parallel/perpendicular/tangent inference;
- line/rectangle direct dimension entry;
- full selection-field routing into generic feature dialogs;
- arbitrary-face Normal To;
- rollback/history editing;
- production FeatureService replacing bootstrap Python mutation.
