#!/usr/bin/env bash
# =====================================================================
#  tools/preview/build_preview.sh
#  Baut die pixelgenaue Host-Vorschau und rendert alle Ansichten als PNG.
#  Nutzt den unveraenderten Rendering-Code aus src/ und die echte
#  Adafruit-GFX-Bibliothek inkl. Fonts. Voraussetzung: g++ / clang, zlib.
#
#  Aufruf:  tools/preview/build_preview.sh [ausgabe-ordner] [--scale N]
#  Default-Ausgabe: tools/preview/out/
#  Adafruit GFX anders: GFX_DIR=/pfad/zu/Adafruit_GFX_Library
# =====================================================================
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
GFX_DIR="${GFX_DIR:-$HOME/Documents/Arduino/libraries/Adafruit_GFX_Library}"

if [ ! -f "$GFX_DIR/Adafruit_GFX.h" ]; then
  echo "Adafruit GFX nicht gefunden: $GFX_DIR (GFX_DIR setzen)" >&2
  exit 1
fi

OUT="$HERE/out"
ARGS=()
while [ $# -gt 0 ]; do
  case "$1" in
    --scale) ARGS+=("--scale" "$2"); shift 2 ;;
    *) OUT="$1"; shift ;;
  esac
done
mkdir -p "$OUT" "$HERE/build"

# Reihenfolge der Include-Pfade: Stubs vor allem anderen
g++ -std=c++17 -O1 -w -DARDUINO=100 \
    -I"$HERE/stubs" -I"$GFX_DIR" \
    -o "$HERE/build/preview" \
    "$HERE/preview_main.cpp" \
    "$ROOT/src/rendering/display_base.cpp" \
    "$ROOT/src/common/astro_logic.cpp" \
    "$ROOT/src/common/moon_calc.cpp" \
    "$ROOT/src/common/time_utils.cpp" \
    "$GFX_DIR/Adafruit_GFX.cpp" \
    -lz

"$HERE/build/preview" "$OUT" "${ARGS[@]}"
