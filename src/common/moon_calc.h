// =====================================================================
//  moon_calc.h
//  Berechnung von Mondphase, Illumination und Mondhöhe.
//  Nach J. Meeus "Astronomical Algorithms" (vereinfacht).
//  Genauigkeit: Phase ±0.1 %, Höhe ±1° – reicht für Astro-Planung locker.
// =====================================================================
#ifndef ASTRO_MOON_CALC_H
#define ASTRO_MOON_CALC_H

#include <Arduino.h>

// Ergebnisstruktur einer Mondberechnung
struct MoonInfo {
  float       phase;        // 0..1   0=Neumond, 0.25=1.Viertel, 0.5=Voll, 0.75=L.Viertel
  float       illumination; // 0..1   beleuchteter Anteil
  float       altitudeDeg;  // Höhe über Horizont in Grad (neg. = unter Horizont)
  float       skyImpact;    // 0..1   grober "Himmelsaufhellungs-Index"
  const char* phaseName;    // "Neumond", "Zun. Sichel", "Vollmond" ...
};

// Mondberechnung für einen bestimmten Zeitpunkt (UTC-Unixzeit) und Ort.
MoonInfo computeMoon(time_t utcTime, float latitudeDeg, float longitudeDeg);

#endif // ASTRO_MOON_CALC_H
