# FreeShape UX Big Pass 2 v0.2.1 Repair

Repairs the v0.2.0 generated-source compile break.

## Source fix

Two FreeCAD diagnostic strings in `FreeShapeWindow::finishExtrudePreview()`
were accidentally emitted with physical newlines inside normal C++ string
literals. They now use escaped `\n` sequences.

## Harness fix

The apply script also hardens the existing `scripts/build-iteration.sh` in-place
so the final diagnostic summary does not try to redirect input from a missing
successful-build fingerprint after a failed build.

The source fingerprint is invalidated so the stale-binary guard remains strict.

No UX Big Pass 2 feature scope is removed.
