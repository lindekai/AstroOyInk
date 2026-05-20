// =====================================================================
//  tests/test_astro.cpp
//  Validiert die Astro-Bewertung gegen typische Wolken-Szenarien.
// =====================================================================
#include <cstdio>
#include <cmath>
#include <cstring>

#define ASTRO_CLOUD_GOOD       20.0f
#define ASTRO_CLOUD_MODERATE   45.0f
#define ASTRO_WIND_WARN        25.0f
#define ASTRO_HUMIDITY_WARN    90.0f

enum AstroVerdict { ASTRO_GOOD, ASTRO_MODERATE, ASTRO_BAD, ASTRO_NO_NIGHT };

static float weightedCloudScore(float low, float mid, float high) {
  if (std::isnan(low))  low  = 0;
  if (std::isnan(mid))  mid  = 0;
  if (std::isnan(high)) high = 0;
  return 0.5f * low + 0.3f * mid + 0.2f * high;
}

static AstroVerdict bewerten(float avg, float maxWind, float maxHum) {
  AstroVerdict v;
  if      (avg <  ASTRO_CLOUD_GOOD)     v = ASTRO_GOOD;
  else if (avg <  ASTRO_CLOUD_MODERATE) v = ASTRO_MODERATE;
  else                                  v = ASTRO_BAD;
  if (maxWind > ASTRO_WIND_WARN || maxHum > ASTRO_HUMIDITY_WARN) {
    if      (v == ASTRO_GOOD)     v = ASTRO_MODERATE;
    else if (v == ASTRO_MODERATE) v = ASTRO_BAD;
  }
  return v;
}

static const char* L(AstroVerdict v) {
  switch (v) {
    case ASTRO_GOOD:     return "GOOD";
    case ASTRO_MODERATE: return "MOD";
    case ASTRO_BAD:      return "BAD";
    default:             return "NONE";
  }
}

static int failures = 0;
static void expect(const char* label, const char* got, const char* want) {
  bool ok = std::strcmp(got, want) == 0;
  printf("  %-30s got=%-4s want=%-4s %s\n",
         label, got, want, ok ? "OK" : "FAIL");
  if (!ok) failures++;
}

int main() {
  printf("=== Astro Bewertung Tests ===\n\n");

  // Klare Nacht
  expect("klare Nacht",
         L(bewerten(weightedCloudScore(5, 5, 5), 10, 60)),
         "GOOD");

  // Hohe Cirren, sonst klar
  expect("Cirrus only",
         L(bewerten(weightedCloudScore(0, 0, 80), 10, 60)),
         "GOOD");

  // Tiefe Wolken
  expect("Stratus 80%",
         L(bewerten(weightedCloudScore(80, 0, 0), 10, 60)),
         "MOD");

  // komplett bewoelkt
  expect("Overcast",
         L(bewerten(weightedCloudScore(100, 100, 100), 10, 60)),
         "BAD");

  // Klar aber Wind > 25
  expect("klar + Wind 30",
         L(bewerten(weightedCloudScore(5, 5, 5), 30, 60)),
         "MOD");

  // Klar aber Feuchte > 90
  expect("klar + Hum 95",
         L(bewerten(weightedCloudScore(5, 5, 5), 10, 95)),
         "MOD");

  // Mittel + Wind = Bad
  expect("mittel + Wind 30",
         L(bewerten(35.0f, 30, 60)),
         "BAD");

  // Mittel ohne Wind/Hum bleibt MOD
  expect("mittel only",
         L(bewerten(35.0f, 10, 60)),
         "MOD");

  // Cloud-Score Formel
  float s = weightedCloudScore(80, 0, 0);
  printf("\n  weightedCloudScore(80,0,0) = %.1f (expected 40.0) %s\n",
         s, std::fabs(s - 40.0f) < 0.01f ? "OK" : "FAIL");
  if (std::fabs(s - 40.0f) > 0.01f) failures++;

  printf("\n=== Result: %s ===\n",
         failures == 0 ? "ALL PASSED" : "SOME FAILED");
  return failures == 0 ? 0 : 1;
}
