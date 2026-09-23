#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "============================================================"
echo "FREESHAPE UX BIG PASS 2 — REAL SKETCH → EXTRUDE"
echo "============================================================"
echo
echo "This build starts the visible document as a BLANK Part Studio."
echo "The engine persistence smoke test runs separately and stays hidden."
echo
echo "Primary acceptance path:"
echo
echo "  Shift+S"
echo "  click Top in Default geometry"
echo "  press C"
echo "  click circle center"
echo "  click circle radius"
echo "  type a diameter and press Enter"
echo "  press Shift+E"
echo "  change depth in the floating Extrude dialog"
echo "  press Enter"
echo
echo "Expected result: Part 1 appears with Sketch 1 + Extrude 1 history."
echo

"$ROOT/scripts/build-iteration.sh" build
"$ROOT/scripts/build-iteration.sh" run
