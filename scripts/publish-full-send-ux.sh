#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
exec "$ROOT/scripts/patch-cycle.sh" \
  "Full Send UX v0.4.0: selection, measurement, dialogs, and sketch tools" \
  "./scripts/full-send-ux.sh"
