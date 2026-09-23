#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FC="$ROOT/third_party/FreeCAD"
FC_BUILD="$FC/build/relWithDebInfo"
BUILD="$ROOT/build/freeshape"
[[ "$(uname -s)" == Linux ]] || { echo "FreeShape currently targets Linux only." >&2; exit 2; }
[[ -f "$FC_BUILD/CMakeCache.txt" ]] || { echo "FreeCAD not configured. Run ./scripts/bootstrap.sh." >&2; exit 2; }
CC_PATH="$(sed -n 's/^CMAKE_C_COMPILER:FILEPATH=//p' "$FC_BUILD/CMakeCache.txt" | head -n1)"
CXX_PATH="$(sed -n 's/^CMAKE_CXX_COMPILER:FILEPATH=//p' "$FC_BUILD/CMakeCache.txt" | head -n1)"
ARGS=(cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DFREESHAPE_FREECAD_SOURCE="$FC" -DFREESHAPE_FREECAD_BUILD="$FC_BUILD")
[[ -z "$CC_PATH" ]] || ARGS+=("-DCMAKE_C_COMPILER=$CC_PATH")
[[ -z "$CXX_PATH" ]] || ARGS+=("-DCMAKE_CXX_COMPILER=$CXX_PATH")
pixi run --manifest-path "$FC/pixi.toml" "${ARGS[@]}"
