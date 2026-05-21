// =====================================================================
//  seeing_api.h
//  Seeing & Transparenz-Vorhersage via 7Timer! ASTRO-Produkt.
//  Kostenloses HTTP-API (kein Key noetig).
//  Aufloesung: 3-Stunden-Intervalle, bis zu 8 Tage voraus.
//
//  Skala (beide Werte, 1-8):
//    1 = exzellent  (Seeing < 0.5", Transp sehr gut)
//    8 = schlecht   (Seeing > 2.5", Transp miserabel)
//
//  Wenn der API-Aufruf scheitert: valid=false, count=0.
//  Der Rest des Dashboards laeuft weiter normal.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_SEEING_API_H
#define ASTROOYINK_SEEING_API_H

#include <time.h>
#include <stdint.h>

#define MAX_SEEING_POINTS  24   // 24 x 3 h = 72 h Vorhersage

struct SeeingPoint {
  time_t  ts;           // Unix-Zeitstempel (berechnet aus UTC-Init + Timepoint)
  uint8_t seeing;       // 1-8  (1 = <0.5" exzellent, 8 = >2.5" schlecht)
  uint8_t transparency; // 1-8  (1 = exzellent, 8 = schlecht)
};

struct SeeingData {
  bool        valid;
  uint8_t     count;
  SeeingPoint points[MAX_SEEING_POINTS];
};

// Daten von 7Timer! laden. Gibt true zurueck wenn mindestens 1 Datenpunkt
// geparsed werden konnte. Bei Fehler: out.valid = false.
bool fetchSeeing(float lat, float lon, SeeingData& out);

#endif // ASTROOYINK_SEEING_API_H
