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

echo
echo "=== Running test_astro ==="
build/test_astro

echo
echo "=== Running test_moon ==="
build/test_moon

echo
echo "All tests done."
