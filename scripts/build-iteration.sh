#!/usr/bin/env bash
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FC="$ROOT/third_party/FreeCAD"
FC_BUILD="$FC/build/relWithDebInfo"
FS_BUILD="$ROOT/build/freeshape"
MODE="${1:-build}"

STAMP="$(date +%Y%m%d-%H%M%S)"
OUTDIR="${FREESHAPE_DIAG_DIR:-$HOME/Downloads}"
WORK="$(mktemp -d "/tmp/freeshape-diag-${STAMP}-XXXXXX")"
LOG="$WORK/terminal.log"

mkdir -p "$OUTDIR"

cleanup() { rm -rf "$WORK"; }
trap cleanup EXIT

section() {
    printf '\n============================================================\n'
    printf '%s\n' "$1"
    printf '============================================================\n'
}

safe_cmd() {
    local outfile="$1"
    shift
    {
        printf '$'
        printf ' %q' "$@"
        printf '\n'
        "$@"
    } >"$outfile" 2>&1 || true
}

current_fingerprint() {
    "$ROOT/scripts/source-fingerprint.sh" 2>/dev/null || echo unavailable
}

write_successful_build_stamp() {
    mkdir -p "$FS_BUILD"
    current_fingerprint > "$FS_BUILD/.freeshape-source-fingerprint"
}

collect_pre() {
    safe_cmd "$WORK/date.txt" date --iso-8601=seconds
    safe_cmd "$WORK/uname.txt" uname -a
    safe_cmd "$WORK/os-release.txt" cat /etc/os-release
    safe_cmd "$WORK/lscpu.txt" lscpu
    safe_cmd "$WORK/memory.txt" free -h
    safe_cmd "$WORK/disk.txt" df -h "$ROOT" "$HOME"
    safe_cmd "$WORK/git-version.txt" git --version
    safe_cmd "$WORK/pixi-version.txt" pixi --version
    safe_cmd "$WORK/cmake-version.txt" cmake --version
    safe_cmd "$WORK/ninja-version.txt" ninja --version
    safe_cmd "$WORK/git-status-before.txt" git -C "$ROOT" status -sb
    safe_cmd "$WORK/git-log-before.txt" git -C "$ROOT" log -10 --oneline --decorate
    safe_cmd "$WORK/git-diff-before.txt" git -C "$ROOT" diff --stat
    safe_cmd "$WORK/git-diff-full-before.txt" git -C "$ROOT" diff
    safe_cmd "$WORK/untracked-before.txt" git -C "$ROOT" ls-files --others --exclude-standard
    safe_cmd "$WORK/submodules-before.txt" git -C "$ROOT" submodule status --recursive

    current_fingerprint > "$WORK/source-fingerprint-before.txt"

    if [[ -f "$FS_BUILD/.freeshape-source-fingerprint" ]]; then
        cp "$FS_BUILD/.freeshape-source-fingerprint" "$WORK/build-stamp-before.txt"
    else
        printf 'MISSING\n' > "$WORK/build-stamp-before.txt"
    fi

    if [[ -d "$FC/.git" || -f "$FC/.git" ]]; then
        safe_cmd "$WORK/freecad-head.txt" git -C "$FC" rev-parse HEAD
        safe_cmd "$WORK/freecad-status.txt" git -C "$FC" status -sb
    fi

    {
        for v in PATH CC CXX CMAKE_PREFIX_PATH LD_LIBRARY_PATH \
                 QT_QPA_PLATFORM WAYLAND_DISPLAY DISPLAY XDG_SESSION_TYPE \
                 PIXI_ENVIRONMENT_NAME FREESHAPE_PHASE0_STRICT; do
            printf '%s=%q\n' "$v" "${!v-}"
        done
    } >"$WORK/environment-allowlist.txt"

    if [[ -f "$FC_BUILD/CMakeCache.txt" ]]; then
        cp "$FC_BUILD/CMakeCache.txt" "$WORK/FreeCAD-CMakeCache.txt"
    fi
}

collect_source_snapshot() {
    local snapshot="$WORK/freeshape-source-snapshot.tar.gz"
    local items=()

    for item in CMakeLists.txt README.md .gitmodules .gitignore src scripts docs; do
        [[ -e "$ROOT/$item" ]] && items+=("$item")
    done

    if (( ${#items[@]} > 0 )); then
        tar -czf "$snapshot" -C "$ROOT" "${items[@]}" 2>/dev/null || true
    fi
}

collect_fcstd() {
    local cache_root="${XDG_CACHE_HOME:-$HOME/.cache}/freeshape"
    local fcstd="$cache_root/tmp/freeshape-spike/FreeShapeSpike.FCStd"

    if [[ -f "$fcstd" ]]; then
        mkdir -p "$WORK/artifacts"
        cp "$fcstd" "$WORK/artifacts/FreeShapeSpike.FCStd"
        safe_cmd "$WORK/artifacts/FreeShapeSpike.sha256.txt" sha256sum "$fcstd"
        safe_cmd "$WORK/artifacts/FreeShapeSpike-unzip-list.txt" unzip -l "$fcstd"
        safe_cmd "$WORK/artifacts/FreeShapeSpike-file.txt" file "$fcstd"
    fi
}

collect_post() {
    safe_cmd "$WORK/git-status-after.txt" git -C "$ROOT" status -sb
    safe_cmd "$WORK/git-diff-after.txt" git -C "$ROOT" diff --stat
    safe_cmd "$WORK/git-diff-full-after.txt" git -C "$ROOT" diff
    safe_cmd "$WORK/untracked-after.txt" git -C "$ROOT" ls-files --others --exclude-standard
    safe_cmd "$WORK/submodules-after.txt" git -C "$ROOT" submodule status --recursive

    current_fingerprint > "$WORK/source-fingerprint-after.txt"
    if [[ -f "$FS_BUILD/.freeshape-source-fingerprint" ]]; then
        cp "$FS_BUILD/.freeshape-source-fingerprint" "$WORK/build-stamp-after.txt"
    else
        printf 'MISSING\n' > "$WORK/build-stamp-after.txt"
    fi

    [[ -f "$FS_BUILD/CMakeCache.txt" ]] && cp "$FS_BUILD/CMakeCache.txt" "$WORK/FreeShape-CMakeCache.txt"

    if [[ -f "$LOG" ]]; then
        tail -n 1200 "$LOG" >"$WORK/terminal-tail-1200.txt" || true
        grep -nEi \
            'CMake Error|FAILED:|fatal error:|undefined reference|collect2: error|ld: error|ninja: build stopped|segmentation fault|assertion.*failed|Traceback|Exception' \
            "$LOG" >"$WORK/errors-extracted.txt" || true
        grep -nE \
            'FREESHAPE_(MODEL_VERIFY|SAVE|CLOSE|RELOAD|ROUNDTRIP|SELECTION)' \
            "$LOG" >"$WORK/freeshape-markers.txt" || true
        grep -cEi 'warning:' "$LOG" >"$WORK/warning-count.txt" || true
        grep -cEi 'error:|CMake Error|FAILED:|undefined reference' "$LOG" >"$WORK/error-count.txt" || true
    fi

    local exe="$FC_BUILD/bin/FreeShape"
    if [[ -x "$exe" ]]; then
        safe_cmd "$WORK/freeshape-file.txt" file "$exe"
        safe_cmd "$WORK/freeshape-ldd.txt" ldd "$exe"
        safe_cmd "$WORK/freeshape-readelf-dynamic.txt" readelf -d "$exe"
        safe_cmd "$WORK/freeshape-sha256.txt" sha256sum "$exe"
        safe_cmd "$WORK/freeshape-stat.txt" stat "$exe"
    fi

    collect_fcstd
    collect_source_snapshot
}

run_iteration() {
    case "$MODE" in
        bootstrap) "$ROOT/scripts/bootstrap.sh" ;;
        build)
            "$ROOT/scripts/configure.sh" &&
            pixi run --manifest-path "$FC/pixi.toml" \
                cmake --build "$FS_BUILD" --parallel "$(nproc)"
            ;;
        configure) "$ROOT/scripts/configure.sh" ;;
        doctor) "$ROOT/scripts/doctor.sh" ;;
        run) "$ROOT/scripts/run-spike.sh" ;;
        *)
            echo "Usage: $0 {bootstrap|build|configure|doctor|run}" >&2
            return 64
            ;;
    esac
}

validate_run() {
    local rc=0
    local strict="${FREESHAPE_PHASE0_STRICT:-0}"
    local report="$WORK/runtime-validation.txt"
    : > "$report"

    check_required() {
        local label="$1"; local pattern="$2"
        if grep -Eq "$pattern" "$LOG"; then
            printf 'PASS %s\n' "$label" >>"$report"
        else
            printf 'FAIL %s\n' "$label" >>"$report"
            rc=1
        fi
    }

    check_optional() {
        local label="$1"; local pattern="$2"
        if grep -Eq "$pattern" "$LOG"; then
            printf 'PASS %s\n' "$label" >>"$report"
        elif [[ "$strict" == "1" ]]; then
            printf 'FAIL %s\n' "$label" >>"$report"
            rc=1
        else
            printf 'INFO %s not exercised this run\n' "$label" >>"$report"
        fi
    }

    check_required "model-created" 'FREESHAPE_MODEL_VERIFY PASS stage=created objects=[0-9]+'
    check_required "model-pre-save" 'FREESHAPE_MODEL_VERIFY PASS stage=pre-save objects=[0-9]+'
    check_required "fcstd-save" 'FREESHAPE_SAVE PASS path=.* bytes=[0-9]+'
    check_required "document-close" 'FREESHAPE_CLOSE PASS'
    check_required "model-reloaded" 'FREESHAPE_MODEL_VERIFY PASS stage=reloaded objects=[0-9]+'
    check_required "fcstd-reload" 'FREESHAPE_RELOAD PASS document='
    check_required "roundtrip" 'FREESHAPE_ROUNDTRIP PASS path=.* objects=[0-9]+'
    check_optional "face-selection" 'FREESHAPE_SELECTION .*sub=Face[0-9]+'
    check_optional "edge-selection" 'FREESHAPE_SELECTION .*sub=Edge[0-9]+'

    return "$rc"
}

section "FREESHAPE BUILD ITERATION"
echo "mode      : $MODE"
echo "root      : $ROOT"
echo "timestamp : $STAMP"
echo "diagnostic working dir: $WORK"

collect_pre

set +e
run_iteration 2>&1 | tee "$LOG"
RC=${PIPESTATUS[0]}
set -e

if [[ "$RC" -eq 0 && ( "$MODE" == "build" || "$MODE" == "bootstrap" ) ]]; then
    write_successful_build_stamp
fi

if [[ "$MODE" == "run" && "$RC" -eq 0 ]]; then
    if ! validate_run; then
        echo
        echo "FreeShape runtime exited cleanly, but validation failed."
        RC=65
    fi
fi

collect_post

FS_HEAD="$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)"
FC_HEAD="$(git -C "$FC" rev-parse HEAD 2>/dev/null || echo unknown)"
SOURCE_FP="$(current_fingerprint)"
if [[ -f "$FS_BUILD/.freeshape-source-fingerprint" ]]; then
    BUILD_FP="$(tr -d '[:space:]' < "$FS_BUILD/.freeshape-source-fingerprint")"
else
    BUILD_FP="missing"
fi

cat >"$WORK/summary.txt" <<EOF
FreeShape diagnostic bundle
timestamp=$STAMP
mode=$MODE
exit_code=$RC
freeshape_head=$FS_HEAD
freecad_head=$FC_HEAD
source_fingerprint=$SOURCE_FP
build_fingerprint=$BUILD_FP
host=$(hostname)
kernel=$(uname -sr)
arch=$(uname -m)
EOF

python3 - "$WORK" "$OUTDIR" "$STAMP" "$MODE" "$RC" <<'PY'
import sys, zipfile, pathlib
work, outdir, stamp, mode, rc = sys.argv[1:]
dest = pathlib.Path(outdir) / f"FreeShape-diagnostics-{stamp}-{mode}-rc{rc}.zip"
base = pathlib.Path(work)
with zipfile.ZipFile(dest, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for p in sorted(base.rglob("*")):
        if p.is_file():
            z.write(p, arcname=p.relative_to(base))
print(dest)
PY

ZIP_PATH="$OUTDIR/FreeShape-diagnostics-${STAMP}-${MODE}-rc${RC}.zip"

section "ITERATION COMPLETE"
echo "exit code : $RC"
echo "bundle    : $ZIP_PATH"

if [[ "$MODE" == "run" && -f "$WORK/runtime-validation.txt" ]]; then
    echo
    cat "$WORK/runtime-validation.txt"
fi

echo
echo "Upload that ZIP to the FreeShape chat."
exit "$RC"
