#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "============================================================"
echo "FREESHAPE UX BIG PASS 3"
echo "============================================================"
echo
echo "New interaction checks:"
echo "  Alt+C      command search"
echo "  S          context-sensitive shortcut toolbar"
echo "  Z          zoom out"
echo "  Shift+Z    zoom in"
echo "  blank Part Studio planes labeled Top / Front / Right"
echo "  larger default reference-plane presentation"
echo
echo "Regression path:"
echo "  Shift+S -> Top -> C -> center -> radius -> diameter"
echo "  Shift+E -> depth -> Enter"
echo

"$ROOT/scripts/build-iteration.sh" build
"$ROOT/scripts/build-iteration.sh" run
