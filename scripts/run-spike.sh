#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FC="$ROOT/third_party/FreeCAD"
FC_BUILD="$FC/build/relWithDebInfo"
FS_BUILD="$ROOT/build/freeshape"
EXE="$FC_BUILD/bin/FreeShape"
STAMP="$FS_BUILD/.freeshape-source-fingerprint"

if [[ ! -x "$EXE" ]]; then
    echo "FreeShape is not built. Run ./scripts/build-iteration.sh build first." >&2
    exit 2
fi

CURRENT="$("$ROOT/scripts/source-fingerprint.sh")"

if [[ ! -f "$STAMP" ]]; then
    echo "REFUSING TO RUN: no successful-build fingerprint exists." >&2
    echo "The executable may be stale." >&2
    echo "Run: ./scripts/build-iteration.sh build" >&2
    exit 42
fi

BUILT="$(tr -d '[:space:]' < "$STAMP")"

if [[ "$BUILT" != "$CURRENT" ]]; then
    echo "REFUSING TO RUN STALE FREESHAPE BINARY" >&2
    echo "built fingerprint : $BUILT" >&2
    echo "source fingerprint: $CURRENT" >&2
    echo "Run: ./scripts/build-iteration.sh build" >&2
    exit 42
fi

CONFIG_ROOT="${XDG_CONFIG_HOME:-$HOME/.config}/freeshape"
DATA_ROOT="${XDG_DATA_HOME:-$HOME/.local/share}/freeshape"
CACHE_ROOT="${XDG_CACHE_HOME:-$HOME/.cache}/freeshape"

mkdir -p "$CONFIG_ROOT" "$DATA_ROOT" "$CACHE_ROOT/tmp"

export FREECAD_USER_HOME="$CONFIG_ROOT/"
export FREECAD_USER_DATA="$DATA_ROOT/"
export FREECAD_USER_TEMP="$CACHE_ROOT/tmp/"
export LD_LIBRARY_PATH="$FC_BUILD/lib:${LD_LIBRARY_PATH:-}"
export PYTHONPATH="$FC_BUILD/Mod:$FC_BUILD/lib:${PYTHONPATH:-}"

echo "FreeShape source fingerprint: $CURRENT"
echo "FreeShape executable        : $EXE"

exec "$EXE" "$@"
