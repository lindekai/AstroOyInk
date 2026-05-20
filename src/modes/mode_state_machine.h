// =====================================================================
//  src/modes/mode_state_machine.h
//
//  Detektion und Verwaltung der Display-Modi.
//
//  Geplante Modi (Roadmap):
//    MODE_WEATHER  – Astro-Wetter-Dashboard (aktuell aktiv, v1.x)
//    MODE_SESSION  – N.I.N.A.-Session-Live-Anzeige (kommt nach Feature 1-7)
//
//  Übergangslogik (geplant):
//    WEATHER → SESSION : N.I.N.A.-Host pingt erreichbar UND eine Sequenz
//                        läuft ODER letztes Bild < 10 min alt
//    SESSION → WEATHER : N.I.N.A. nicht mehr erreichbar ODER 15 min
//                        Inaktivität seit letztem Bild
//
//  Aktueller Status: SKELETT. Die Detection-Funktion liefert immer
//  MODE_WEATHER. Wird in einem späteren Feature scharf geschaltet.
// =====================================================================
#ifndef ASTROOYINK_MODE_STATE_MACHINE_H
#define ASTROOYINK_MODE_STATE_MACHINE_H

#include <Arduino.h>

enum DisplayMode : uint8_t {
  MODE_WEATHER = 0,
  MODE_SESSION = 1
};

// Ermittelt den aktuell passenden Modus.
// Aktuell: liefert IMMER MODE_WEATHER (Stub).
// Später: prüft N.I.N.A.-Erreichbarkeit + Session-Aktivität.
DisplayMode detectCurrentMode();

// Menschenlesbarer Name eines Modus für Logging.
const char* modeName(DisplayMode m);

#endif // ASTROOYINK_MODE_STATE_MACHINE_H
