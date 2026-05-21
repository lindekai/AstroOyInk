// =====================================================================
//  src/common/app_config.cpp
//  NVS-Lese- und Schreibzugriffe fuer AppConfig.
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "app_config.h"
#include "config.h"
#include <Preferences.h>

static const char* kNS = "aoi";  // NVS-Namespace (max 15 Zeichen)

AppConfig loadConfig() {
  AppConfig cfg;
  cfg.fromNvs = false;

  // Defaults aus config.h
  strlcpy(cfg.locationName, LOCATION_NAME, sizeof(cfg.locationName));
  cfg.lat              = LOCATION_LAT;
  cfg.lon              = LOCATION_LON;
  strlcpy(cfg.timezone, TIMEZONE_POSIX,   sizeof(cfg.timezone));
  cfg.refreshIntervalS = REFRESH_INTERVAL_S;
  memset(cfg.ninaHost, 0, sizeof(cfg.ninaHost));
  cfg.ninaPort         = 1888;

  Preferences prefs;
  if (!prefs.begin(kNS, /*readOnly=*/true)) return cfg;

  if (prefs.isKey("loc")) {
    prefs.getString("loc", cfg.locationName, sizeof(cfg.locationName));
    cfg.lat              = prefs.getFloat("lat", cfg.lat);
    cfg.lon              = prefs.getFloat("lon", cfg.lon);
    prefs.getString("tz",  cfg.timezone,   sizeof(cfg.timezone));
    cfg.refreshIntervalS = prefs.getUInt("iv", cfg.refreshIntervalS);
    prefs.getString("nh",  cfg.ninaHost,   sizeof(cfg.ninaHost));
    cfg.ninaPort         = (uint16_t)prefs.getUInt("np", cfg.ninaPort);
    cfg.fromNvs          = true;
  }
  prefs.end();

  Serial.printf("[CFG] Ort=%s Lat=%.4f Lon=%.4f IV=%us fromNvs=%d\n",
                cfg.locationName, cfg.lat, cfg.lon,
                cfg.refreshIntervalS, (int)cfg.fromNvs);
  return cfg;
}

void saveConfig(const AppConfig& cfg) {
  Preferences prefs;
  if (!prefs.begin(kNS, /*readOnly=*/false)) {
    Serial.println("[CFG] NVS open FAILED");
    return;
  }
  prefs.putString("loc", cfg.locationName);
  prefs.putFloat( "lat", cfg.lat);
  prefs.putFloat( "lon", cfg.lon);
  prefs.putString("tz",  cfg.timezone);
  prefs.putUInt(  "iv",  cfg.refreshIntervalS);
  prefs.putString("nh",  cfg.ninaHost);
  prefs.putUInt(  "np",  cfg.ninaPort);
  prefs.end();
  Serial.println("[CFG] NVS gespeichert");
}

void clearConfig() {
  Preferences prefs;
  if (prefs.begin(kNS, /*readOnly=*/false)) {
    prefs.clear();
    prefs.end();
    Serial.println("[CFG] NVS geloescht");
  }
}
