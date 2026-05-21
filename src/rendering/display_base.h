// =====================================================================
//  src/rendering/display_base.h
//
//  Basis des E-Ink-Renderings für AstroOyInk.
//  Verantwortlich für:
//    - SPI- und Panel-Initialisierung (GxEPD2)
//    - Page-Loop und manueller BUSY-Wait
//    - Bisher AUCH: das komplette Wetter-Dashboard (siehe unten)
//
//  ROADMAP-Hinweis:
//    Der konkrete Wetter-Render-Code wird in einem späteren Schritt nach
//    src/rendering/view_weather.cpp wandern. Der Session-Renderer wird
//    in view_session.cpp dazu kommen. Diese Datei behält dann nur die
//    Basis-Funktionen (displayInit, displaySleep, waitBusyManual, gemein-
//    same Zeichen-Helper). Aktuell bleibt alles hier zusammen, um das
//    funktionierende v1.0-Verhalten nicht zu brechen.
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_DISPLAY_BASE_H
#define ASTROOYINK_DISPLAY_BASE_H

#include "data_types.h"
#include "battery.h"
#include "local_sensor.h"
#include "seeing_api.h"
#include "iss_api.h"

// Initialisiert das Display (SPI, RST, etc.). Einmal in setup() aufrufen.
void displayInit();

// Schaltet das Panel in Tiefschlaf (ohne Buffer löschen).
void displaySleep();

// Komplettes Wetter-Dashboard zeichnen (volles Refresh).
//   wd.valid == false       →  Fallback-Bildschirm wird gezeichnet.
//   batt.status == BATT_UNKNOWN → kein Batterie-Widget im Header.
//   sensor.valid == false   →  INNEN-Zeile wird nicht gezeichnet.
void renderDashboard(const WeatherData& wd, BatteryInfo batt,
                     const LocalSensorData& sensor,
                     const SeeingData& seeing,
                     const ISSData& iss);

// Reine Fallback-Anzeige (Erststart, kein WLAN, kein API).
void renderFallback(const char* msg);

// Portal-Info: zeigt AP-Name und Verbindungshinweis waehrend der
// WiFiManager das Captive Portal betreibt.
void renderPortalInfo(const char* apName);

#endif // ASTROOYINK_DISPLAY_BASE_H
