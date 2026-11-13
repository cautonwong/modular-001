#!/usr/bin/env bash
# The one CI entry point. It discovers and verifies every target under targets/.
set -euo pipefail
root="$(cd "$(dirname "$0")" && pwd)"
exec python3 "$root/tools/ci_matrix.py" --root "$root"
