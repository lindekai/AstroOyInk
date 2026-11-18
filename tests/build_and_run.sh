#!/usr/bin/env bash
# =====================================================================
#  tests/build_and_run.sh
#  Baut und startet die Host-Tests (Linux/macOS).
#  Voraussetzung: g++ mit C++14
# =====================================================================
set -e
cd "$(dirname "$0")"

mkdir -p build

echo "=== Building test_astro ==="
g++ -std=c++14 -Wall -Wextra -o build/test_astro test_astro.cpp

echo "=== Building test_moon ==="
# moon_calc.cpp/h inkludiert Arduino.h. Der Stub liegt in tests/Arduino.h
# und wird vor den echten gefunden, weil tests/ vor src/common/ im
# Include-Pfad steht.
g++ -std=c++14 -Wall -Wextra \
    -I. -I../src/common \
    -o build/test_moon \
    test_moon.cpp ../src/common/moon_calc.cpp

echo "=== Building test_scan_window ==="
g++ -std=c++14 -Wall -Wextra -o build/test_scan_window test_scan_window.cpp

# test_nina_sequence braucht ArduinoJson (header-only). Standard: Arduino-
# Library-Ordner; anders per ARDUINOJSON_DIR=/pfad/zu/ArduinoJson/src
ARDUINOJSON_DIR="${ARDUINOJSON_DIR:-$HOME/Documents/Arduino/libraries/ArduinoJson/src}"
HAVE_AJ=0
if [ -f "$ARDUINOJSON_DIR/ArduinoJson.h" ]; then
  HAVE_AJ=1
  echo "=== Building test_nina_sequence ==="
  g++ -std=c++14 -Wall -Wextra \
      -I. -I"$ARDUINOJSON_DIR" \
      -o build/test_nina_sequence \
      test_nina_sequence.cpp ../src/data_sources/nina_sequence.cpp
else
  echo "=== SKIP test_nina_sequence (ArduinoJson nicht gefunden: $ARDUINOJSON_DIR) ==="
fi

echo
echo "=== Running test_astro ==="
build/test_astro

echo
echo "=== Running test_moon ==="
build/test_moon

echo
echo "=== Running test_scan_window ==="
build/test_scan_window

if [ "$HAVE_AJ" = 1 ]; then
  echo
  echo "=== Running test_nina_sequence ==="
  build/test_nina_sequence
fi

echo
echo "All tests done."
