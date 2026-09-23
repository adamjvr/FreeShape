# Onshape workflow research notes used for UX Big Pass 1

The following behavior is treated as product-level interaction evidence, not
just visual inspiration.

## Selection

Desktop Onshape selection is toggle/additive: click to select, click the same
entity again to deselect, click additional entities to add them, click empty
graphics space or press Space to clear.  Box direction changes containment
semantics.  Select Other is bound to the grave-accent key and cycles obscured
entities beneath the cursor.

## Navigation

Onshape's default three-button mouse profile uses right-button drag to rotate,
middle-button drag (or Ctrl+right drag) to pan, and the wheel to zoom.  F fits,
N orients normal to a hovered/preselected plane or face, W starts zoom-window,
and Shift+1..7 provide standard views.

## Commands

S opens a shortcut toolbar at the cursor.  Shift+S starts Sketch.  Shift+E
opens Extrude, including while sketching.  Shift+F opens Fillet.  Enter accepts,
Shift+Enter accepts/repeats, Escape cancels, and Tab moves through feature
dialog fields.

## Sketching

Sketching is command-dense and inference-heavy.  Onshape supports direct
shortcuts for line/circle/rectangle/dimension/construction/trim/use and applies
automatic inference while moving geometry.  Several tools allow numeric
dimension entry during creation instead of forcing a separate dimension pass.

## Part Studio

The Feature list is semantic history, not an implementation object tree.  It
contains default geometry, feature icons, a rollback bar, and a separate Parts
area.  Feature/graphics selections cross-highlight and both can supply feature
inputs.  The Part Studio remains active while the feature panel is open.

## Dialogs

Feature dialogs float over the graphics area.  Required geometry fields are
visually distinct selection fields; text fields are keyboard focused.  Preview
is part of the operation, and the checkmark/Enter commits the feature.

## Screencast observation

The supplied screencast reinforces the documentation: plane selection flows
straight into sketching; a circle is produced with a compact tool interaction;
Extrude opens directly from the sketch and produces a live solid preview while
the small floating dialog remains in the upper-left of the graphics area.

FreeShape's visible workflow must converge on this model rather than exposing
FreeCAD workbenches, Combo View, TaskPanels, or explicit recompute choreography.
