// =====================================================================
//  astro_logic.h
//  Berechnung des Wolken-Scores und der Nacht-Bewertung.
// =====================================================================
#ifndef ASTRO_LOGIC_H
#define ASTRO_LOGIC_H

#include "data_types.h"

// Berechnet alle astroHourly[i] und 'summary' aus den hourly-Daten.
// Verwendet die Sonnenauf-/-untergänge aus daily[].
//
// Bewertung erfolgt für DIE NÄCHSTE NACHT, also:
//   - "heutiger" Sonnenuntergang  (Tag-Index abhängig von aktueller Uhrzeit)
//   - bis Sonnenaufgang am Folgetag
//   - Fenster: [sunset+2h , sunrise-2h]
//
// Wenn aktuell schon nach Mitternacht aber vor Sonnenaufgang:
// nehmen wir gestrigen Sonnenuntergang + heutigen Sonnenaufgang.
void computeAstroSummary(WeatherData& wd);

// Hilfsfunktion: gewichteter Cloud-Score
inline float weightedCloudScore(float low, float mid, float high) {
  // NaN-Schutz: behandle NaN als 0 (= klar)
  if (isnan(low))  low  = 0;
  if (isnan(mid))  mid  = 0;
  if (isnan(high)) high = 0;
  return 0.5f * low + 0.3f * mid + 0.2f * high;
}

#endif // ASTRO_LOGIC_H
