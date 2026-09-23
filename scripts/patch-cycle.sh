#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MESSAGE="${1:-}"
RUNNER="${2:-./scripts/ux-big-pass-3.sh}"

if [[ -z "$MESSAGE" ]]; then
    echo "Usage: $0 \"commit message\" [runner]" >&2
    exit 2
fi

cd "$ROOT"

echo "============================================================"
echo "FREESHAPE PATCH CYCLE"
echo "============================================================"
echo "message : $MESSAGE"
echo "runner  : $RUNNER"
echo

BRANCH="$(git branch --show-current)"
if [[ "$BRANCH" != "main" ]]; then
    echo "REFUSING: patch cycles currently publish only from main; current=$BRANCH" >&2
    exit 3
fi

git remote get-url origin >/dev/null 2>&1 || {
    echo "REFUSING: Git remote 'origin' is not configured." >&2
    exit 3
}

git fetch origin main

LOCAL_BASE="$(git rev-parse HEAD)"
REMOTE_BASE="$(git rev-parse origin/main)"

if [[ "$LOCAL_BASE" != "$REMOTE_BASE" ]]; then
    echo "REFUSING: local main and origin/main diverged before this patch." >&2
    echo "local : $LOCAL_BASE" >&2
    echo "remote: $REMOTE_BASE" >&2
    echo "Reconcile explicitly before publishing another patch." >&2
    exit 4
fi

if [[ -d third_party/FreeCAD ]]; then
    FC_DIRTY="$(git -C third_party/FreeCAD status --porcelain)"
    if [[ -n "$FC_DIRTY" ]]; then
        echo "REFUSING: pinned FreeCAD submodule contains local modifications:" >&2
        echo "$FC_DIRTY" >&2
        exit 5
    fi
fi

git diff --check

echo
echo "=== BUILD + INTERACTIVE RUNTIME GATE ==="
bash -lc "cd '$ROOT' && $RUNNER"

echo
echo "=== POST-RUN SOURCE CHECK ==="
git diff --check

if [[ -d third_party/FreeCAD ]]; then
    FC_DIRTY="$(git -C third_party/FreeCAD status --porcelain)"
    if [[ -n "$FC_DIRTY" ]]; then
        echo "REFUSING TO COMMIT: FreeCAD submodule became dirty during the run." >&2
        echo "$FC_DIRTY" >&2
        exit 6
    fi
fi

git add -A
git diff --cached --check

if git diff --cached --quiet; then
    echo "No repository changes to commit."
else
    git commit -m "$MESSAGE"
fi

NEW_HEAD="$(git rev-parse HEAD)"

echo
echo "=== PUSH ==="
git push origin main

REMOTE_AFTER="$(git ls-remote --heads origin main | awk '{print $1}')"

echo
echo "local main : $NEW_HEAD"
echo "remote main: $REMOTE_AFTER"

if [[ "$NEW_HEAD" != "$REMOTE_AFTER" ]]; then
    echo "PUSH VERIFICATION FAILED" >&2
    exit 7
fi

echo
echo "============================================================"
echo "FREESHAPE PATCH: BUILD / RUN / COMMIT / PUSH PASS"
echo "============================================================"
git status -sb
git log -1 --oneline --decorate
