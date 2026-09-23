#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FC="$ROOT/third_party/FreeCAD"
FC_BUILD="$FC/build/relWithDebInfo"
EXPECTED="df7dd2eefe5afa54e86b65b5a7a1ce721907c721"
echo "FreeShape architecture-spike doctor"
uname -srmo
if [[ -e "$FC/.git" ]]; then
  ACTUAL="$(git -C "$FC" rev-parse HEAD)"; echo "FreeCAD: $ACTUAL"; [[ "$ACTUAL" == "$EXPECTED" ]] || echo "WARNING expected $EXPECTED"
else echo "FreeCAD: NOT INITIALIZED"; fi
for t in git pixi cmake ninja; do command -v "$t" >/dev/null && echo "$t: $(command -v "$t")" || echo "$t: MISSING"; done
for l in FreeCADBase FreeCADApp FreeCADGui; do compgen -G "$FC_BUILD/lib/lib$l.so*" >/dev/null && echo "$l: present" || echo "$l: not built"; done
[[ -x "$FC_BUILD/bin/FreeShape" ]] && echo "FreeShape: built" || echo "FreeShape: not built"
