#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
exec "$ROOT/scripts/patch-cycle.sh" \
  "GUI action-reaction v0.5.0: tool discoverability and contextual UX" \
  "./scripts/gui-action-reaction-v050.sh"
