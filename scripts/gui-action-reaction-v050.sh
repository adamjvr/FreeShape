#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "============================================================"
echo "FREESHAPE GUI / ACTION-REACTION FULL PASS v0.5.0"
echo "============================================================"
echo
echo "Please exercise:"
echo "  - hover every toolbar icon; verify name bubbles"
echo "  - click Line/Circle/Rectangle twice; verify toggle state"
echo "  - press L/C/G/R twice; verify toggle state"
echo "  - Escape with an active sketch tool; verify tool exits first"
echo "  - Shift+S then pick Top directly in the viewport"
echo "  - hover Top/Front/Right; verify semantic names"
echo "  - Y while hovering a reference plane"
echo "  - N while hovering/selecting a reference plane"
echo "  - right-click a reference plane for plane-specific commands"
echo "  - preselect edges then open Fillet; selection field should react"
echo "  - regression: box select, Select Other, Measure HUD"
echo

"$ROOT/scripts/build-iteration.sh" build
"$ROOT/scripts/build-iteration.sh" run
