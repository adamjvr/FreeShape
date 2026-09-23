# Onshape evidence applied in UX Big Pass 2

Current official Onshape documentation describes the workflow this pass
implements:

- Center Point Circle uses `C`; click once for center and again for radius.
- Immediately after the circle is created, a numerical diameter can be entered
  and accepted with Enter, avoiding a separate Dimension-tool pass.
- Sketch tools are presented when a sketch is created/opened.
- `S` opens the sketch shortcut toolbar while an active Sketch dialog is open.
- Starting Extrude or Revolve from the Sketch toolbar accepts the open sketch
  and opens the feature with the sketch regions automatically selected.
- `Shift+E` opens Extrude while working on a sketch.
- Automatic inferencing is part of normal sketch creation and can infer
  horizontal, vertical, midpoint, parallel, coincident, and other relations;
  Shift suppresses automatic inference.

FreeShape therefore treats click-count and command chaining as architecture,
not polish.
