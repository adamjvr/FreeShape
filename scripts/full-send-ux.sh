#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "============================================================"
echo "FREESHAPE FULL SEND UX PASS v0.4.0"
echo "============================================================"
echo
echo "This is not a single-feature smoke. Exercise the interaction layer:"
echo "  1. hover Top/Front/Right and model geometry"
echo "  2. ordinary additive/toggle selection + Space clear"
echo "  3. L→R blue window box selection"
echo "  4. R→L yellow crossing box selection"
echo "  5. Ctrl+box subtraction"
echo "  6. grave / Shift+grave Select Other + Enter"
echo "  7. automatic measurement HUD"
echo "  8. move + resize the Sketch/Extrude dialog"
echo "  9. Shift+S -> Top -> C circle"
echo " 10. L line and G corner rectangle in sketch"
echo " 11. hold Shift near axis/origin to suppress inference"
echo " 12. Shift+E -> live depth -> Enter"
echo

"$ROOT/scripts/build-iteration.sh" build
"$ROOT/scripts/build-iteration.sh" run
