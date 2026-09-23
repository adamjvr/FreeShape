# FreeShape v0.2.3 — Reference Plane Visibility

## Root cause

The v0.2.2 blank Part Studio correctly created a PartDesign Body and its
Origin/XY/XZ/YZ objects, but the viewport remained empty.

FreeCAD's PartDesign Body claims its `App::Origin` ViewProvider, and the Origin
claims the datum-plane ViewProviders. The Origin is hidden by default. Setting
`XY_Plane`, `XZ_Plane`, and `YZ_Plane` visible does not bypass the hidden
parent's scene-graph visibility switch.

This is why the left panel could select Top/Front/Right while nothing appeared
in the viewport.

## Fix

FreeShape now treats reference geometry as an explicit product-level visual
state:

- Body visible
- Origin parent visible
- X/Y/Z axes hidden
- XY/XZ/YZ planes visible
- plane labels forced visible
- datum plane display scale increased modestly
- `P` toggles the complete Origin+plane visibility state
- starting Sketch restores the planes for support selection
- choosing a support plane hides the default planes and enters sketch mode
- cancelling Sketch restores the reference planes

The planes remain real FreeCAD `App::Plane` / `Gui::ViewProviderPlane`
entities, so picking them feeds the same engine-backed support references used
by Sketcher.
