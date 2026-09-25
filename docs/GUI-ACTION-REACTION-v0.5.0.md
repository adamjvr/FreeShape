# FreeShape GUI / Action-Reaction Full Pass v0.5.0

This pass is based on the clean v0.4.0 run diagnostic and the 2026-09-25
screencast.

## What the screencast exposed

The v0.4 shell has the right broad layout, but it still feels like an
integration prototype:

- toolbar glyphs are too small at normal desktop resolution;
- most tools cannot be identified without reading tiny text;
- native tooltips are inconsistent and delayed;
- the semantic reference-plane labels can clamp to unrelated viewport edges,
  making them look like random tooltips;
- an active sketch tool does not visibly latch in the toolbar;
- pressing the same sketch shortcut does not behave like a true tool toggle;
- Escape can cancel too much state at once;
- viewport plane picks arrive from FreeCAD as `Body / Origin.XY_Plane.` rather
  than a direct `XY_Plane` object, so the FreeShape sketch-plane workflow must
  normalize nested subobject picks;
- hover/preselection should drive commands such as Hide and Normal-to;
- feature dialogs need to react to the current selection rather than act like
  disconnected forms.

## Toolbar interaction contract

FreeShape toolbars now use real 32px vector-like Qt-generated icon artwork,
rendered at 25px inside 40x36 tool buttons.

Every icon-only button gets a FreeShape hover bubble after a short dwell:

```text
┌─────────────────────┐
│ Center point circle  C
│ Click center, then radius
└─────────────────────┘
```

This avoids cramming text into the toolbar while keeping tools discoverable.

Toolbar groups follow the Onshape workflow:

### Part Studio

- Sketch
- Extrude / Revolve / Sweep / Loft
- Hole / Fillet / Chamfer / Shell / Draft
- Pattern / Mirror / Boolean / Transform

### Sketch

- Extrude / Revolve remain directly accessible
- Line / Circle / Corner rectangle / Center rectangle
- Arc / Spline
- Dimension / Construction / Trim / Use / Offset / Constraints

## Active tool reaction

Sketch tools now behave like tools rather than fire-and-forget commands:

- click a tool → it latches visibly;
- click the same tool again → exit it;
- press the same shortcut again → exit it;
- choose another tool → switch directly;
- Escape exits the active sketch tool before it cancels the Sketch command.

This matches the documented Onshape shortcut behavior.

## Reference-plane reaction

FreeCAD can report a visible datum-plane click as:

```text
object = Body
sub    = Origin.XY_Plane.
```

FreeShape now normalizes those nested picks to its semantic reference geometry:

```text
Origin.XY_Plane. → Top
Origin.XZ_Plane. → Front
Origin.YZ_Plane. → Right
```

That normalization is used by:

- Sketch plane selection;
- preselection HUD;
- View normal to;
- context menus;
- hover Hide.

Off-screen semantic plane labels are hidden rather than clamped to arbitrary
viewport edges.

## Selection-driven dialogs

When a Fillet/feature dialog is open, the selection field now follows the live
selection set. Preselect-first and command-first workflows can therefore
converge on the same interaction architecture.

## Context reaction

Right-clicking a default datum plane now offers plane-specific behavior:

- New sketch on Top/Front/Right;
- View normal to that plane;
- Hide;
- Show reference planes;
- Fit;
- Select Other.

Blank/model context retains the general Part Studio commands.

## Onshape documentation mapped into this pass

Public Onshape documentation explicitly establishes:

- toolbars change based on workflow;
- Sketch has its own toolbar;
- Extrude/Revolve remain available while sketching;
- toolbar tools behave as toggles and Escape exits a selected tool;
- selection is additive/toggle by ordinary click;
- hover/preselection is meaningful input;
- `Y` hides the entity under the cursor;
- dialogs react to selections and support keyboard acceptance;
- Search Tools and tool descriptions are discoverability mechanisms;
- toolbars are organized into tool sets rather than text menus.

FreeShape continues to use FreeCAD only for CAD/view services beneath that
interaction contract.
