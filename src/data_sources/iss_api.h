// =====================================================================
//  iss_api.h
//  ISS-Ueberflugvorhersage via open-notify.org.
//  Kostenloses HTTP-API, kein Key noetig.
//  Liefert die naechsten N ISS-Passes mit Aufgangszeit und Dauer.
//
//  Wenn der API-Aufruf scheitert: valid=false — die Stundentabelle
//  laeuft ohne ISS-Zeile. Derzeit abgeschaltet, siehe ISS_ENABLED.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_ISS_API_H
#define ASTROOYINK_ISS_API_H

#include <time.h>
#include <stdint.h>

#define MAX_ISS_PASSES  5

// open-notify.org hat die Ueberflug-Vorhersage (iss-pass.json) abgeschaltet,
// der Abruf liefert nur noch HTTP 404. Abruf deshalb aus (spart Zeit/Akku).
// Wieder einschalten, sobald es eine neue Quelle gibt.
#ifndef ISS_ENABLED
#define ISS_ENABLED 0
#endif

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
