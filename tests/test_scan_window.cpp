// =====================================================================
//  tests/test_scan_window.cpp
//  Validiert das Zeitfenster der N.I.N.A.-Auto-Discovery.
// =====================================================================
#include <cstdio>
#include "../src/common/scan_window.h"

static int failures = 0;
static void expect(const char* label, bool got, bool want) {
  bool ok = got == want;
  printf("  %-36s got=%-5s want=%-5s %s\n", label,
         got ? "true" : "false", want ? "true" : "false", ok ? "OK" : "FAIL");
  if (!ok) failures++;
}

static int32_t hm(int h, int m) { return h * 3600 + m * 60; }

int main() {
  printf("=== Scan-Fenster Tests ===\n\n");

  // Berlin Sommer: Aufgang 04:45, Untergang 21:30 → Fenster 20:30 .. 05:45
  const int32_t sr = hm(4, 45), ss = hm(21, 30), m = 3600;
  expect("Sommer 12:00 (Tag)",       inNinaScanWindow(hm(12, 0),  sr, ss, m), false);
  expect("Sommer 20:29 (kurz davor)", inNinaScanWindow(hm(20, 29), sr, ss, m), false);
  expect("Sommer 20:30 (Start)",     inNinaScanWindow(hm(20, 30), sr, ss, m), true);
  expect("Sommer 23:59",             inNinaScanWindow(hm(23, 59), sr, ss, m), true);
  expect("Sommer 00:00",             inNinaScanWindow(0,          sr, ss, m), true);
  expect("Sommer 05:45 (Ende)",      inNinaScanWindow(hm(5, 45),  sr, ss, m), true);
  expect("Sommer 05:46 (danach)",    inNinaScanWindow(hm(5, 46),  sr, ss, m), false);

  // Berlin Winter: Aufgang 08:15, Untergang 15:55 → Fenster 14:55 .. 09:15
  const int32_t wr = hm(8, 15), ws = hm(15, 55);
  expect("Winter 12:00 (Tag)",       inNinaScanWindow(hm(12, 0),  wr, ws, m), false);
  expect("Winter 15:00",             inNinaScanWindow(hm(15, 0),  wr, ws, m), true);
  expect("Winter 09:00",             inNinaScanWindow(hm(9, 0),   wr, ws, m), true);

  // Unbekannte Sonnenzeiten (Kaltstart) → immer suchen
  expect("unbekannt 12:00",          inNinaScanWindow(hm(12, 0),  -1, -1, m), true);

  // Sehr kurzer Tag (< 2 h) → Puffer ueberlappen, immer suchen
  expect("kurzer Tag 11:00",         inNinaScanWindow(hm(11, 0),  hm(10, 0), hm(11, 30), m), true);

  printf("\n=== Result: %s ===\n", failures == 0 ? "ALL PASSED" : "SOME FAILED");
  return failures == 0 ? 0 : 1;
}
