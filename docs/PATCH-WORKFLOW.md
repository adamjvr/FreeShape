# Patch / Commit / Push Workflow

## Policy

Every FreeShape patch is published only after the exact patched source builds
and its runtime gate completes successfully.

Canonical command:

```bash
./scripts/patch-cycle.sh "Commit message" ./scripts/ux-big-pass-3.sh
```

The cycle:

1. verifies branch `main`;
2. fetches `origin/main`;
3. refuses to continue if local and remote bases differ;
4. refuses a dirty FreeCAD submodule;
5. runs `git diff --check`;
6. runs the requested build/runtime gate;
7. runs source/submodule checks again;
8. stages the complete repository change;
9. commits;
10. pushes `origin main`;
11. verifies the remote SHA equals local `HEAD`.

A compile/runtime failure therefore leaves the patch local and uncommitted.
That is intentional: Git history should represent tested checkpoints.

## First publication

The repository originally contained only an initial README commit while the
bootstrap work remained local. The first successful patch cycle after this
policy lands will therefore commit the accumulated foundation in one large
checkpoint. Subsequent patches should be much smaller and map one-to-one to
tested development passes.

## FreeCAD submodule

Never make mystery edits inside `third_party/FreeCAD`.

If upstream modification becomes unavoidable:

1. try an external adapter first;
2. prefer an upstream contribution;
3. otherwise keep an explicit patch/fork;
4. document it;
5. never let `patch-cycle.sh` publish a dirty submodule.

## Diagnostics

Every build/run continues to emit a ZIP under `~/Downloads`. Upload the newest
ZIP when a cycle fails. The ZIP includes source snapshot, CMake state, terminal
log, fingerprints and the FCStd smoke artifact.
