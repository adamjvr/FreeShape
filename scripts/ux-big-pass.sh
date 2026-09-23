#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "============================================================"
echo "FREESHAPE UX BIG PASS 1"
echo "============================================================"
echo
echo "This run exercises:"
echo "  - Onshape-style shell"
echo "  - RMB orbit / MMB pan / wheel zoom"
echo "  - additive toggle selection"
echo "  - S shortcut palette"
echo "  - Shift+S Sketch shell"
echo "  - Shift+E live Extrude dialog"
echo "  - Shift+F Fillet shell"
echo "  - Space clear selection"
echo "  - grave accent Select Other"
echo "  - P planes / Shift+H sketches / Y hide"
echo "  - Shift+1..7 standard views"
echo "  - FCStd round-trip"
echo
echo "Build first, then run the exact successful binary."
echo

"$ROOT/scripts/build-iteration.sh" build
"$ROOT/scripts/build-iteration.sh" run
