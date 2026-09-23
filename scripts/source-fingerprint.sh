#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FC="$ROOT/third_party/FreeCAD"

if [[ ! -d "$ROOT/src" ]]; then
    echo "FreeShape source tree missing: $ROOT/src" >&2
    exit 2
fi

FC_HEAD="$(git -C "$FC" rev-parse HEAD 2>/dev/null || echo unknown)"

{
    printf 'freecad=%s\n' "$FC_HEAD"

    if [[ -f "$ROOT/CMakeLists.txt" ]]; then
        sha256sum "$ROOT/CMakeLists.txt"
    fi

    find "$ROOT/src" \
        -type f \
        \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \
           -o -name '*.h' -o -name '*.hpp' -o -name '*.inl' \) \
        -print0 \
        | sort -z \
        | xargs -0 -r sha256sum
} | sha256sum | awk '{print $1}'
