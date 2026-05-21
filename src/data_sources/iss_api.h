// =====================================================================
//  iss_api.h
//  ISS-Ueberflugvorhersage via open-notify.org.
//  Kostenloses HTTP-API, kein Key noetig.
//  Liefert die naechsten N ISS-Passes mit Aufgangszeit und Dauer.
//
//  Wenn der API-Aufruf scheitert: valid=false — FAZIT laeuft normal.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_ISS_API_H
#define ASTROOYINK_ISS_API_H

#include <time.h>
#include <stdint.h>

#define MAX_ISS_PASSES  5

struct ISSPass {
  time_t   riseTime;  // Unix-Zeitstempel des Aufgangs (lokal interpretierbar)
  uint16_t duration;  // Sichtdauer in Sekunden
};

struct ISSData {
  bool     valid;
  uint8_t  count;
  ISSPass  passes[MAX_ISS_PASSES];
};

// Naechste Passes laden. Gibt true zurueck wenn mindestens 1 Pass
// geparsed werden konnte.
bool fetchISSPasses(float lat, float lon, ISSData& out);

#endif // ASTROOYINK_ISS_API_H
