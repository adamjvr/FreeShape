# FreeShape Testing Strategy

## Build gate

Every patch must build FreeShape against the exact pinned FreeCAD build with
source/build fingerprints matching.

## Runtime engine gate

Current required smoke markers:

- model created;
- pre-save verification;
- FCStd saved;
- document closed;
- model reloaded;
- FCStd reopened;
- round-trip passed.

Selection markers remain optional until dedicated automated picking exists.

## Product interaction gates

Each UX subsystem gains a reproducible manual acceptance path before it is
considered stable.

Examples:

### Sketch → Extrude

```text
Shift+S
Top
C
center
radius
diameter
Enter
Shift+E
depth
Enter
```

Expected:

- one Sketch feature;
- one Extrude feature;
- one Part;
- no TaskPanel/workbench interaction;
- save/reload preserves the result.

### Selection

Future gate:

- click Face1;
- click Face2 without Ctrl;
- both selected;
- click Face1 again;
- Face1 removed;
- Space clears;
- directional box semantics verified;
- Select Other candidate UI verified.

## Golden models

The mature suite should compare:

- recompute success;
- solid count;
- volume;
- bounding box;
- semantic feature order;
- expected reference relationships;
- STEP round-trip where relevant.

## Upstream FreeCAD update gate

No gitlink update is accepted until engine build, adapter tests, golden models
and interactive smoke pass against the candidate SHA.
