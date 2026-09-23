#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FC="$ROOT/third_party/FreeCAD"
BUILD="$ROOT/build/freeshape"
[[ "$(uname -s)" == Linux ]] || { echo "Linux only." >&2; exit 2; }
[[ "$(uname -m)" == x86_64 ]] || { echo "Phase 0 is Linux x86_64 only." >&2; exit 2; }
command -v pixi >/dev/null || { echo "pixi is required: https://pixi.sh" >&2; exit 2; }

git -C "$ROOT" submodule update --init third_party/FreeCAD
git -C "$FC" submodule update --init --recursive
printf 'FreeCAD pin: '; git -C "$FC" rev-parse HEAD

pixi run --manifest-path "$FC/pixi.toml" configure-rel-with-deb-info
pixi run --manifest-path "$FC/pixi.toml" build-rel-with-deb-info
"$ROOT/scripts/configure.sh"
pixi run --manifest-path "$FC/pixi.toml" cmake --build "$BUILD" --parallel "$(nproc)"

echo
echo "Bootstrap complete. Run: ./scripts/run-spike.sh"
