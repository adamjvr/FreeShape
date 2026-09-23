# Phase 0 Big Pass A

This pass closes a diagnostic hole discovered during v0.0.6.

## Stale executable incident

The v0.0.6 build failed because `main.cpp` called
`App::Document::countObjects()` while only seeing the forward declaration from
`App/Application.h`. The missing direct include was:

```cpp
#include <App/Document.h>
```

The subsequent `run` still returned `rc0` because the old v0.0.5 executable was
left in FreeCAD's `bin` directory and `run-spike.sh` had no freshness check.

Therefore the screenshot from that run proved the v0.0.5 viewport seam, but did
**not** prove v0.0.6 FCStd round-trip or selection.

## New invariant

A FreeShape executable may run only when:

```
source fingerprint == successful-build fingerprint
```

The fingerprint includes:

- FreeCAD pinned HEAD
- FreeShape `CMakeLists.txt`
- every C/C++ source/header under `src/`

`run-spike.sh` exits 42 if the executable is missing, unstamped, or stale.

## Runtime gate

A run now returns success only after all of these log markers exist:

- model created
- pre-save verification
- FCStd saved
- document closed
- model reloaded
- FCStd reload passed
- round-trip passed
- at least one Face selection
- at least one Edge selection

A clean GUI exit without all gates returns rc65.

## Diagnostic bundle upgrades

Every iteration now archives:

- complete terminal transcript
- first-class FreeShape markers
- source fingerprint
- successful-build fingerprint
- executable SHA-256/stat/ldd/readelf
- full Git diff/status/submodule state
- compact source snapshot (`freeshape-source-snapshot.tar.gz`)
- actual generated `FreeShapeSpike.FCStd`
- FCStd SHA-256 and archive listing

This makes each uploaded ZIP sufficient to reconstruct what was actually built
and what model was actually exercised.
