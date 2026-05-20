// =====================================================================
//  astro_logic.cpp
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "astro_logic.h"
#include "config.h"
#include "time_utils.h"

#include <math.h>

// Findet den daily-Index, dessen Sonnenuntergang den Beginn der
// "kommenden Nacht" markiert. Logik:
//   - now < heutiger sunrise  → Nacht von gestern. Index = -1 (vor erstem Eintrag)
//                                Wir benutzen daily[0] für sunrise und … ein
//                                synthetisches sunset aus (sunrise[0] - 12h)
//                                – aber das wird realistisch kaum auftreten,
//                                weil Open-Meteo immer ab "heute" liefert.
//   - now > heutiger sunset    → Nacht beginnt heute → daily[0]
//   - sonst (zwischen sunrise und sunset)  → Nacht beginnt heute → daily[0]
//
// Liefert (nightStart, nightEnd) als Unix-Lokalzeit.
static void resolveNightWindow(const WeatherData& wd, time_t& start, time_t& end) {
  start = 0;
  end   = 0;
  if (wd.dailyCount < 2) return;

  time_t now = time(nullptr);

  // Heuristik: wir suchen den ersten Tag, dessen sunset >= now (dann ist
  // das die nächste oder laufende Nacht). Wenn wir mitten in der Nacht
  // (also vor sunrise[0]) sind, nehmen wir sunset[-1] – den haben wir nicht.
  // Trick: prüfen wir, ob now < sunrise[0]: dann sind wir noch in der Nacht
  // von daily[-1]→daily[0]. Wir nutzen sunset[0] - 86400 als Approximation.

  if (now < wd.daily[0].sunrise) {
    // Wir sind in der Nacht: gestriger Sonnenuntergang ≈ sunset[0] - 24h
    time_t fakeSunset = wd.daily[0].sunset - 86400L;
    start = fakeSunset + 2L * 3600L;
    end   = wd.daily[0].sunrise - 2L * 3600L;
    return;
  }

  // Standardfall: Nacht beginnt mit heutigem Sonnenuntergang
  for (uint8_t i = 0; i < wd.dailyCount - 1; i++) {
    if (now < wd.daily[i].sunset || (now >= wd.daily[i].sunrise && now < wd.daily[i].sunset)) {
      start = wd.daily[i].sunset     + 2L * 3600L;
      end   = wd.daily[i + 1].sunrise - 2L * 3600L;
      return;
    }
    // Wenn wir bereits NACH dem heutigen sunset sind: gleiche Logik anwenden
    if (now >= wd.daily[i].sunset) {
      start = wd.daily[i].sunset     + 2L * 3600L;
      end   = wd.daily[i + 1].sunrise - 2L * 3600L;
      return;
    }
  }
}

// ---------------------------------------------------------------------
//  Hauptfunktion
// ---------------------------------------------------------------------
void computeAstroSummary(WeatherData& wd) {
  // 1) Per-Stunden-Score immer berechnen (auch außerhalb des Nachtfensters,
  //    damit das Diagramm überall sinnvoll aussehen kann)
  for (uint16_t i = 0; i < wd.hourlyCount; i++) {
    AstroHour& a = wd.astroHourly[i];
    a.ts          = wd.hourly[i].ts;
    a.cloudScore  = weightedCloudScore(wd.hourly[i].cloudLow,
                                       wd.hourly[i].cloudMid,
                                       wd.hourly[i].cloudHigh);
    a.windKmh     = wd.hourly[i].windKmh;
    a.humidity    = wd.hourly[i].humidity;
    a.inNightWindow = false;
  }

  // 2) Nachtfenster bestimmen
  time_t nStart, nEnd;
  resolveNightWindow(wd, nStart, nEnd);

  AstroSummary s;
  s.verdict        = ASTRO_NO_NIGHT;
  s.avgCloudScore  = 0;
  s.maxWindKmh     = 0;
  s.maxHumidity    = 0;
  s.nightHourCount = 0;
  s.nightStart     = nStart;
  s.nightEnd       = nEnd;
  s.windWarn       = false;
  s.humidityWarn   = false;
  s.conclusion     = "";

  if (nStart == 0 || nEnd == 0 || nEnd <= nStart) {
    s.conclusion = "Keine Nachtdaten verfuegbar.";
    wd.summary = s;
    return;
  }

  // 3) Stunden im Fenster aggregieren
  double sumCloud = 0;
  uint16_t cnt = 0;
  for (uint16_t i = 0; i < wd.hourlyCount; i++) {
    time_t t = wd.hourly[i].ts;
    if (t >= nStart && t <= nEnd) {
      wd.astroHourly[i].inNightWindow = true;
      sumCloud += wd.astroHourly[i].cloudScore;
      cnt++;
      if (wd.hourly[i].windKmh  > s.maxWindKmh)  s.maxWindKmh  = wd.hourly[i].windKmh;
      if (wd.hourly[i].humidity > s.maxHumidity) s.maxHumidity = wd.hourly[i].humidity;
    }
  }

  if (cnt == 0) {
    s.conclusion = "Keine Stundenwerte im Nachtfenster.";
    wd.summary = s;
    return;
  }

  s.nightHourCount = cnt;
  s.avgCloudScore  = (float)(sumCloud / cnt);

  // 4) Ampel
  if      (s.avgCloudScore <  ASTRO_CLOUD_GOOD)     s.verdict = ASTRO_GOOD;
  else if (s.avgCloudScore <  ASTRO_CLOUD_MODERATE) s.verdict = ASTRO_MODERATE;
  else                                              s.verdict = ASTRO_BAD;

  // 5) Strafzonen: Wind / Feuchte verschlechtern die Bewertung um EINE Stufe
  s.windWarn     = (s.maxWindKmh  > ASTRO_WIND_WARN);
  s.humidityWarn = (s.maxHumidity > ASTRO_HUMIDITY_WARN);

  if (s.windWarn || s.humidityWarn) {
    if      (s.verdict == ASTRO_GOOD)     s.verdict = ASTRO_MODERATE;
    else if (s.verdict == ASTRO_MODERATE) s.verdict = ASTRO_BAD;
    // ASTRO_BAD bleibt
  }

  // 6) Kurzfazit
  switch (s.verdict) {
    case ASTRO_GOOD:
      s.conclusion = "Klare Nacht erwartet – Teleskop bereitstellen!";
      break;
    case ASTRO_MODERATE:
      s.conclusion = "Wechselhaft – Luecken moeglich, Geduld lohnt sich.";
      break;
    case ASTRO_BAD:
      s.conclusion = "Schlecht – stark bewoelkt oder gestoert.";
      break;
    default:
      s.conclusion = "Keine Bewertung moeglich.";
  }

  if (s.windWarn)     s.conclusion += " Achtung Wind.";
  if (s.humidityWarn) s.conclusion += " Tau wahrscheinlich.";

  wd.summary = s;
}
