// =====================================================================
//  src/common/scan_window.h
//  Zeitfenster fuer die N.I.N.A.-Auto-Discovery.
//  Reine Logik ohne Arduino-Abhaengigkeit (Host-Test: test_scan_window).
//
//  Fenster: [Sonnenuntergang - margin, Sonnenaufgang + margin], ueber
//  Mitternacht hinweg. Alle Zeiten in Sekunden seit lokaler Mitternacht.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_SCAN_WINDOW_H
#define ASTROOYINK_SCAN_WINDOW_H

#include <stdint.h>

static const int32_t kSecondsPerDay = 86400;

// true, wenn todS im Scan-Fenster liegt.
// Unbekannte Sonnenzeiten (< 0, z. B. nach Kaltstart) → true:
// lieber einmal zu oft suchen als N.I.N.A. nie finden.
inline bool inNinaScanWindow(int32_t todS, int32_t sunriseTodS,
                             int32_t sunsetTodS, int32_t marginS) {
  if (sunriseTodS < 0 || sunsetTodS < 0) return true;

  // Tag kuerzer als beide Puffer zusammen → Fenster deckt alles ab
  if (sunsetTodS - sunriseTodS <= 2 * marginS) return true;

  int32_t from = ((sunsetTodS - marginS) % kSecondsPerDay + kSecondsPerDay) % kSecondsPerDay;
  int32_t to   = (sunriseTodS + marginS) % kSecondsPerDay;

  if (from > to) return todS >= from || todS <= to;   // ueber Mitternacht
  return todS >= from && todS <= to;
}

#endif // ASTROOYINK_SCAN_WINDOW_H
