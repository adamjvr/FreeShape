# FreeShape UX Specification

## Product metric

For common modeling tasks, FreeShape should require no more deliberate user
actions than the equivalent documented Onshape workflow unless an engine
constraint makes that impossible.

Backend capability alone is not completion.

## Part Studio shell

The normal Part Studio presents:

- document/navigation bar;
- context-sensitive feature/sketch toolbar;
- Feature + Parts panel;
- default Origin / Top / Front / Right geometry;
- semantic feature history and rollback control;
- large graphics area;
- floating feature dialogs;
- view controls;
- automatic measurement HUD;
- document tabs.

No Workbench selector, Combo View or FreeCAD TaskPanel belongs in the visible
product.

## Selection contract

Target behavior:

- click unselected entity → add;
- click selected entity → remove;
- click empty graphics area → clear;
- Space → clear;
- hover → preselection;
- left→right box → fully enclosed only;
- right→left box → crossing;
- Ctrl+box → subtract;
- grave → Select Other;
- Shift+grave → previous Select Other candidate;
- Enter → accept candidate;
- feature list and graphics area cross-highlight;
- selections automatically flow into the active feature-dialog selection field.

## Navigation contract

Default Onshape profile:

- RMB drag → orbit;
- MMB drag / Ctrl+RMB drag → pan;
- wheel → zoom;
- F → fit;
- N → normal to hovered/selected plane/face;
- N again → inverse normal;
- Z / Shift+Z → zoom out/in;
- W → zoom window;
- Arrow → 15° rotate;
- Shift+Arrow → 90°;
- Ctrl+Arrow → 5°;
- Ctrl+Shift+Arrow → pan;
- Shift+1…7 → standard views.

## Command access

- S → context-sensitive cursor-local shortcut toolbar;
- Alt+C → Search Tools;
- context menu is composed from context-aware command predicates;
- shortcuts operate directly without requiring toolbar focus.

## Dialog contract

- solid-blue field = geometry selection input;
- blue-outline field = keyboard/numeric input;
- preselection seeds appropriate selection inputs;
- Tab advances input;
- arrows navigate dropdowns;
- Enter accepts;
- Shift+Enter accepts and repeats;
- Escape cancels;
- dialogs are movable/resizable;
- preview is live through document transactions.

Editing an older feature rolls history to that point. Preview opacity and a
Final-state toggle are product requirements.

## Sketch contract

Sketcher is the solver, not the UX.

The FreeShape controller owns:

- active tool;
- previews;
- mouse-to-sketch transform;
- inference;
- direct numeric entry;
- auto constraints;
- command chaining.

Core keyboard vocabulary follows the documented Onshape-style workflow:
`L`, `C`, `R`, `G`, `A`, `D`, `Q`, `M`, `U`, `O`, `X` plus constraint keys.
Shift suppresses automatic inference while placing geometry.

Canonical benchmark:

```text
Shift+S → Top → C → center → radius → diameter → Enter
→ Shift+E → depth → Enter
```

## Visibility

- P → default reference planes;
- Shift+H → sketches;
- Y → hide selected/hovered;
- Shift+Y → show hidden workflow;
- future: Shift+I Isolate; Shift+T transparency.

## Measurement

Relevant measurements appear automatically at bottom-right whenever entities
are selected. `[` opens the detailed measurement panel.

## History

The Feature list is semantic history, not the raw engine object tree.

Required behavior:

- unique feature icons;
- cross-highlighting;
- drag reorder;
- true rollback bar;
- feature/part filters;
- suppression/error/warning state;
- independently sized Features and Parts regions.


## v0.4 implementation checkpoint

The interaction contract now has concrete implementation for hover/preselection,
selection-count feedback, directional box selection, Ctrl-box subtraction,
Select Other candidate UI, measurement HUD, movable/resizable dialogs, and the
first Line/Corner Rectangle sketch tools. Remaining work should deepen these
systems rather than bypassing them with FreeCAD-visible commands.


## Toolbar discoverability

Icon-only toolbar controls must not depend on memorized glyphs. Every tool icon
has a short-delay hover bubble containing its canonical name, shortcut when
present, and a one-line interaction hint. Icons target roughly 25px artwork
inside a 40x36px hit area on desktop.

## Tool state

A sketch tool is a persistent mode. Activating the same tool again exits it.
Escape exits the active tool before cancelling the containing Sketch command.
The toolbar visibly indicates which tool is latched.

## Semantic engine selections

Engine selection paths are normalized before they reach product UX. For
example, a FreeCAD nested pick `Body / Origin.XY_Plane.` is exposed as `Top`,
not as an implementation path.
