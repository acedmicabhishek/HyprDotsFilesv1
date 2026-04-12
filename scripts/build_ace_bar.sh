#!/usr/bin/env bash
set -euo pipefail
ROOT="$HOME/.config/hypr"
SRC_DIR="$ROOT/scripts/ace_bar_src"

cd "$SRC_DIR"

if ! command -v pkg-config >/dev/null 2>&1; then
  echo "pkg-config is required to build AceBar." >&2
  exit 1
fi

if ! pkg-config --exists gtk4 gtk4-layer-shell-0; then
  echo "GTK4 and gtk4-layer-shell are required to build AceBar." >&2
  exit 1
fi

make
