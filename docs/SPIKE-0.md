# Spike 0 — FreeCAD embedding seam

## Current status

### PASS: build/ABI seam

- FreeCAD `main` builds independently.
- FreeShape consumes it through `FreeShape::FreeCADBridge`.
- C++23, Qt, Python, Coin, PyCXX and FastSignals compile contracts match.
- FreeShape compiles and links with the pinned FreeCAD engine.

### PASS: visible application ownership

- The only visible top-level application shell is FreeShape's `QMainWindow`.
- A hidden FreeCAD `Gui::MainWindow` remains temporarily as compatibility
  infrastructure for current global `getMainWindow()` assumptions.

### PASS: embedded viewport

- `Gui::Document::createView(View3DInventor, Clone)` creates a populated
  FreeCAD View3DInventor without adding it to FreeCAD's MDI UI.
- The resulting view can be reparented into FreeShape.
- The PartDesign Pad renders correctly.
- FreeCAD navigation/camera UI remains functional inside the embedded viewport.

### Next gates in v0.0.6

1. Create Body -> Sketch -> Pad.
2. Save the real document to FCStd.
3. Verify a non-empty FCStd file exists.
4. Close the document through `App::Application`.
5. Reopen that FCStd through `App::Application`.
6. Recompute and verify Body, Sketch and Pad still exist.
7. Create the visible FreeShape viewport from the *reloaded* document.
8. Listen to FreeCAD's selection singleton.
9. Click a face/edge and prove object + subelement + picked point reach
   FreeShape without using FreeCAD's tree/task UI.

Expected diagnostic markers:

```
FREESHAPE_MODEL_VERIFY PASS stage=created
FREESHAPE_MODEL_VERIFY PASS stage=pre-save
FREESHAPE_SAVE PASS ...
FREESHAPE_CLOSE PASS
FREESHAPE_MODEL_VERIFY PASS stage=reloaded
FREESHAPE_RELOAD PASS ...
FREESHAPE_ROUNDTRIP PASS ...
FREESHAPE_SELECTION doc=... object=Pad sub=Face1 ... picked=true ...
```

## Remaining compatibility debt

The runtime currently emits:

```
QObject::connect: No such signal Gui::GUIApplication::messageReceived(...)
```

It is nonfatal in the current spike and does not prevent document creation,
round-trip I/O, rendering or clean shutdown.  Do not hide it yet; keep it
visible until the startup compatibility shell is audited.
