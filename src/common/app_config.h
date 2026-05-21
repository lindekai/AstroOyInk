// =====================================================================
//  src/common/app_config.h
//  Persistente Laufzeit-Konfiguration (NVS / Preferences).
//  Laedt beim Start Einstellungen aus dem Flash; bei fehlenden Werten
//  gelten die Defaults aus config.h. Der WiFiManager schreibt neue
//  Werte nach erfolgreicher Portal-Eingabe zurueck in den NVS.
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_APP_CONFIG_H
#define ASTROOYINK_APP_CONFIG_H

#include <Arduino.h>

struct AppConfig {
  char     locationName[32];
  float    lat;
  float    lon;
  char     timezone[48];
  uint32_t refreshIntervalS;
  char     ninaHost[64];
  uint16_t ninaPort;
  bool     fromNvs;   // true = aus NVS geladen, false = nur config.h-Defaults
};

// Laedt Einstellungen aus NVS. Fehlende Werte werden aus config.h befuellt.
AppConfig loadConfig();

// Speichert alle Einstellungen in NVS.
void saveConfig(const AppConfig& cfg);

// Loescht den gesamten NVS-Namespace (Factory-Reset der Konfiguration).
void clearConfig();

#endif // ASTROOYINK_APP_CONFIG_H
