#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "============================================================"
echo "FREESHAPE PHASE-0 BIG PASS"
echo "============================================================"
echo
echo "1. Build current source"
echo "2. Stamp successful executable"
echo "3. Run only if executable exactly matches current source"
echo "4. FCStd save/close/reload verification"
echo "5. Embedded viewport"
echo "6. Face + edge selection verification"
echo
echo "During the GUI run: click at least ONE FACE and ONE EDGE, then close FreeShape."
echo

"$ROOT/scripts/build-iteration.sh" build

echo
echo "============================================================"
echo "BUILD PASS — STARTING RUNTIME GATE"
echo "============================================================"
echo

"$ROOT/scripts/build-iteration.sh" run
