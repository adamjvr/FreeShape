# UX Big Pass 2 v0.2.2 — Direct Coin Link Repair

The v0.2.1 source compiles completely and fails only at the final link step.

`SketchController::screenToXY()` now directly calls Coin APIs:

- `SoRenderManager::getCamera()`
- `SbViewVolume`
- `SbLine`
- `SbPlane`

FreeCADGui itself depends on Coin, but that does not make Coin's symbols part
of FreeCADGui's link interface for an external executable. FreeShape therefore
must directly link the bundled Coin library.

The pinned FreeCAD build configures its bundled Coin subproject with
`CMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/lib`, so the exact Coin
library for this FreeCAD build lives in the same `build/relWithDebInfo/lib`
directory as FreeCADGui/App/Base.

The `FreeShape::FreeCADBridge` now explicitly links that exact library.

No source behavior from UX Big Pass 2 is changed.
