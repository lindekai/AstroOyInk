// =====================================================================
//  tests/test_moon.cpp
//
//  Validiert moon_calc.cpp gegen bekannte Mondereignisse.
//  Wird vom build_and_run.sh aufgerufen.
// =====================================================================
#include <cstdio>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>

#include "../src/common/moon_calc.h"

static int failures = 0;

static void expectClose(const char* label, float actual, float expected, float tol) {
  float diff = std::fabs(actual - expected);
  bool ok = diff <= tol;
  printf("  %s: actual=%.3f expected=%.3f tol=%.3f %s\n",
         label, actual, expected, tol, ok ? "OK" : "FAIL");
  if (!ok) failures++;
}

static time_t makeUtc(int Y, int M, int D, int h = 0, int m = 0) {
  struct tm t = {};
  t.tm_year = Y - 1900;
  t.tm_mon  = M - 1;
  t.tm_mday = D;
  t.tm_hour = h;
  t.tm_min  = m;
  return timegm(&t);
}

int main() {
  printf("=== moon_calc Tests ===\n\n");

  // Bekannte Mondereignisse 2026 (Quelle: NASA/AA):
  //   Vollmond:  2026-04-02
  //   Neumond:   2026-04-17
  // Wir prüfen Illumination ±5 % und Phasen ±0.03

  printf("Vollmond 2026-04-02:\n");
  MoonInfo m = computeMoon(makeUtc(2026, 4, 2, 12), 52.52f, 13.405f);
  expectClose("illum",  m.illumination,  1.00f, 0.05f);
  expectClose("phase",  m.phase,         0.50f, 0.03f);

  printf("\nNeumond 2026-04-17:\n");
  m = computeMoon(makeUtc(2026, 4, 17, 12), 52.52f, 13.405f);
  expectClose("illum",  m.illumination,  0.00f, 0.05f);

  printf("\nZun. Sichel 2026-04-22:\n");
  m = computeMoon(makeUtc(2026, 4, 22, 22), 52.52f, 13.405f);
  expectClose("illum",  m.illumination,  0.25f, 0.10f);
  if (std::string(m.phaseName).find("Sichel") == std::string::npos &&
      std::string(m.phaseName).find("Zun")    == std::string::npos) {
    printf("  phaseName: '%s' enthaelt weder 'Sichel' noch 'Zun' FAIL\n", m.phaseName);
    failures++;
  } else {
    printf("  phaseName: '%s' OK\n", m.phaseName);
  }

  // Mond-Höhe Sanity: über 24 h muss sich die Höhe stark ändern
  printf("\nMondhoehe Tageszyklus:\n");
  float minAlt = 1e9, maxAlt = -1e9;
  for (int h = 0; h < 24; h++) {
    m = computeMoon(makeUtc(2026, 4, 22, h), 52.52f, 13.405f);
    if (m.altitudeDeg < minAlt) minAlt = m.altitudeDeg;
    if (m.altitudeDeg > maxAlt) maxAlt = m.altitudeDeg;
  }
  printf("  alt-Range: %.1f .. %.1f Grad %s\n",
         minAlt, maxAlt,
         (maxAlt - minAlt > 60.0f) ? "OK" : "FAIL");
  if (maxAlt - minAlt <= 60.0f) failures++;

  printf("\n=== Result: %s ===\n",
         failures == 0 ? "ALL PASSED" : "SOME FAILED");
  return failures == 0 ? 0 : 1;
}
