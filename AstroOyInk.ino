// =====================================================================
//  AstroOyInk.ino
//  Astro-Display fuer Astrofotografen.  Zwei Modi:
//    - WEATHER:  Astro-Wetter-Dashboard (Open-Meteo + Mond)
//    - SESSION:  N.I.N.A.-Live-Anzeige  (geplant, siehe Roadmap)
//
//  Ablauf pro Zyklus:
//    1) Display-Init
//    2) Konfiguration aus NVS laden (oder config.h-Defaults)
//    3) WiFi via WiFiManager verbinden (bei Fehler: Captive Portal)
//    3b) OTA-Fenster (30 s) fuer Firmware-Updates per Netzwerk
//    4) NTP-Zeit synchronisieren
//    5) Modus ermitteln (detectCurrentMode)
//    6) Modus-spezifisch: Daten holen + rendern
//    7) Display in Hibernate
//
//  Produktiv (DEBUG_MODE=0):  Zyklus in setup(), danach Deep-Sleep.
//  Debug    (DEBUG_MODE=1):  Zyklus in setup() UND in loop() alle 30 s,
//                            kein Deep-Sleep → Serial Monitor bleibt an.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "config.h"

// common
#include "data_types.h"
#include "time_utils.h"
#include "astro_logic.h"
#include "moon_calc.h"
#include "battery.h"
#include "app_config.h"
#include "ota_updater.h"
#include "local_sensor.h"

// data sources
#include "weather_api.h"

// modes
#include "mode_state_machine.h"

// rendering
#include "display_base.h"

// ---------------------------------------------------------------------
//  Persistente Daten ueber Deep-Sleep hinweg (RTC-RAM)
// ---------------------------------------------------------------------
RTC_DATA_ATTR uint32_t bootCount       = 0;
RTC_DATA_ATTR bool     hasEverRendered = false;

// Laufzeit-Konfiguration (aus NVS oder config.h-Defaults)
static AppConfig g_cfg;

// ---------------------------------------------------------------------
//  WiFi + Captive Portal (WiFiManager)
// ---------------------------------------------------------------------
static bool connectWithPortal() {
  WiFiManager wm;
#if !DEBUG_MODE
  wm.setDebugOutput(false);
#endif

  // Aktuelle Werte als Voreinstellung im Portal-Formular
  char latStr[16], lonStr[16], ivStr[12], portStr[8];
  snprintf(latStr,  sizeof(latStr),  "%.6f", g_cfg.lat);
  snprintf(lonStr,  sizeof(lonStr),  "%.6f", g_cfg.lon);
  snprintf(ivStr,   sizeof(ivStr),   "%u",   g_cfg.refreshIntervalS);
  snprintf(portStr, sizeof(portStr), "%u",   g_cfg.ninaPort);

  WiFiManagerParameter p_loc("loc", "Ort",                   g_cfg.locationName, 31);
  WiFiManagerParameter p_lat("lat", "Breitengrad",           latStr,             15);
  WiFiManagerParameter p_lon("lon", "Laengengrad",           lonStr,             15);
  WiFiManagerParameter p_tz( "tz",  "Zeitzone (POSIX)",      g_cfg.timezone,     47);
  WiFiManagerParameter p_iv( "iv",  "Update-Intervall (s)",  ivStr,              11);
  WiFiManagerParameter p_nh( "nh",  "N.I.N.A. Host (opt.)", g_cfg.ninaHost,     63);
  WiFiManagerParameter p_np( "np",  "N.I.N.A. Port",        portStr,             5);

  wm.addParameter(&p_loc);
  wm.addParameter(&p_lat);
  wm.addParameter(&p_lon);
  wm.addParameter(&p_tz);
  wm.addParameter(&p_iv);
  wm.addParameter(&p_nh);
  wm.addParameter(&p_np);

  // AP-Name: AstroOyInk-XXXXXX mit den letzten 3 Bytes der MAC
  char apName[28];
  snprintf(apName, sizeof(apName), "AstroOyInk-%06X",
           (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF));

  // Portal-Anzeige auf dem E-Paper wenn AP gestartet wird
  wm.setAPCallback([&](WiFiManager*) {
    Serial.printf("[WIFI] Portal offen: AP=%s\n", apName);
    renderPortalInfo(apName);
  });

  bool paramsUpdated = false;
  wm.setSaveParamsCallback([&]() { paramsUpdated = true; });

  wm.setConfigPortalTimeout(WIFI_MANAGER_TIMEOUT_S);
  wm.setTitle("AstroOyInk");

  Serial.printf("[WIFI] autoConnect AP=\"%s\" timeout=%ds\n",
                apName, WIFI_MANAGER_TIMEOUT_S);
  bool connected = wm.autoConnect(apName);

  if (connected) {
    Serial.printf("[WIFI] Verbunden. SSID=%s\n", WiFi.SSID().c_str());
    if (paramsUpdated) {
      // Neue Werte aus dem Portal-Formular uebernehmen
      strlcpy(g_cfg.locationName, p_loc.getValue(), sizeof(g_cfg.locationName));
      g_cfg.lat = atof(p_lat.getValue());
      g_cfg.lon = atof(p_lon.getValue());
      strlcpy(g_cfg.timezone, p_tz.getValue(), sizeof(g_cfg.timezone));
      uint32_t iv = (uint32_t)atol(p_iv.getValue());
      g_cfg.refreshIntervalS = (iv >= 60) ? iv : REFRESH_INTERVAL_S;
      strlcpy(g_cfg.ninaHost, p_nh.getValue(), sizeof(g_cfg.ninaHost));
      g_cfg.ninaPort = (uint16_t)atoi(p_np.getValue());
      saveConfig(g_cfg);
    }
  } else {
    Serial.println("[WIFI] Timeout — kein WLAN konfiguriert.");
  }
  return connected;
}

static void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// ---------------------------------------------------------------------
//  Tiefschlaf bis zum naechsten Update (nur im Produktivmodus)
// ---------------------------------------------------------------------
static void goDeepSleep(uint32_t intervalS) {
  uint64_t sleepUs = (uint64_t)intervalS * 1000000ULL;

  time_t now = time(nullptr);
  if (now > 1700000000) {
    struct tm lt; localtime_r(&now, &lt);
    uint32_t toNextHour = (uint32_t)((59 - lt.tm_min) * 60 + (60 - lt.tm_sec));
    if (toNextHour < 60) toNextHour += 3600;
    if (toNextHour < intervalS * 2) {
      sleepUs = (uint64_t)toNextHour * 1000000ULL;
    }
  }

  Serial.printf("[SLEEP] going to sleep for %.0f s ...\n", sleepUs / 1e6);
  Serial.flush();
  esp_sleep_enable_timer_wakeup(sleepUs);
  esp_deep_sleep_start();
}

// ---------------------------------------------------------------------
//  Modus WEATHER: Daten holen, Astro-Logik, Mond, Render
// ---------------------------------------------------------------------
static void runWeatherMode(bool wifiOk, bool timeOk) {
  // Akku + Sensor einmal pro Zyklus lesen (vor dem Render, kein WLAN noetig)
  BatteryInfo      batt   = readBattery();
  LocalSensorData  sensor = localSensorRead();

  // WeatherData liegt static, damit es den 8 KB Stack nicht sprengt
  static WeatherData wd;
  wd.locationName = g_cfg.locationName;

  bool apiOk = false;
  if (wifiOk && timeOk) {
    Serial.println("[STEP 4] Fetch Open-Meteo ...");
    apiOk = fetchWeather(g_cfg.lat, g_cfg.lon, FORECAST_DAYS, wd);
  }

  if (apiOk) {
    Serial.println("[STEP 5] Compute Astro-Summary ...");
    computeAstroSummary(wd);
    Serial.printf("         verdict=%d  avgCloudScore=%.1f  nightHours=%u\n",
                  (int)wd.summary.verdict,
                  wd.summary.avgCloudScore,
                  wd.summary.nightHourCount);

    Serial.println("[STEP 5b] Compute Moon ...");
    wd.moonNow = computeMoon(time(nullptr), g_cfg.lat, g_cfg.lon);
    time_t nightMid = 0;
    if (wd.summary.nightStart > 0 && wd.summary.nightEnd > 0) {
      nightMid = (wd.summary.nightStart + wd.summary.nightEnd) / 2;
    } else {
      nightMid = time(nullptr) + 6 * 3600;
    }
    wd.moonNight = computeMoon(nightMid, g_cfg.lat, g_cfg.lon);
    Serial.printf("         Moon now:    %s  illum=%.0f%%  alt=%.1f\n",
                  wd.moonNow.phaseName,
                  wd.moonNow.illumination * 100.0f,
                  wd.moonNow.altitudeDeg);
    Serial.printf("         Moon Mitte:  %s  illum=%.0f%%  alt=%.1f\n",
                  wd.moonNight.phaseName,
                  wd.moonNight.illumination * 100.0f,
                  wd.moonNight.altitudeDeg);
  }

  if (apiOk) {
    Serial.println("[STEP 6] Render Dashboard ...");
    Serial.flush();
    uint32_t t0 = millis();
    renderDashboard(wd, batt, sensor);
    uint32_t dt = millis() - t0;
    hasEverRendered = true;
    Serial.printf("[STEP 6] Render ok (%lu ms)\n", (unsigned long)dt);
    if (dt < 5000) {
      Serial.println("[STEP 6] WARN: Render <5s - evtl. falsche Display-Klasse");
      Serial.println("        oder Verkabelung pruefen (BUSY-Pin!)");
    }
  } else {
    if (!hasEverRendered) {
      const char* msg = !wifiOk ? "Kein WLAN"
                       : !timeOk ? "Keine NTP-Zeit"
                       : "Keine Wetterdaten";
      Serial.printf("[STEP 6] Fallback: %s\n", msg);
      Serial.flush();
      renderFallback(msg);
      hasEverRendered = true;
    } else {
      Serial.println("[STEP 6] API-Fehler, Anzeige bleibt unveraendert.");
    }
  }
}

// ---------------------------------------------------------------------
//  Modus SESSION: noch nicht implementiert.
//  Roadmap: nach Abschluss der Features 1-7.
// ---------------------------------------------------------------------
static void runSessionMode(bool /*wifiOk*/, bool /*timeOk*/) {
  Serial.println("[STEP 5/6] SESSION-Mode noch nicht implementiert.");
  renderFallback("SESSION-Mode kommt spaeter");
  hasEverRendered = true;
}

// ---------------------------------------------------------------------
//  Ein kompletter Zyklus mit Modus-Auswahl
// ---------------------------------------------------------------------
static void doCycle() {
  Serial.println();
  Serial.println("----- Cycle start -----");
  Serial.flush();

  // 1) Display initialisieren
  Serial.println("[STEP 1] Display-Init ...");
  Serial.flush();
  displayInit();
  Serial.println("[STEP 1] Display-Init ok");

  // 1b) Lokaler Sensor initialisieren (I2C, kein WLAN noetig)
  localSensorInit();

  // 2) Konfiguration laden (NVS oder config.h-Defaults)
  Serial.println("[STEP 2] Konfiguration laden ...");
  g_cfg = loadConfig();

  // 3) WiFi mit Captive Portal
  Serial.println("[STEP 3] WiFi / Portal ...");
  bool wifiOk = connectWithPortal();

  // 3b) OTA-Fenster: 30 s nach WLAN-Connect fuer Firmware-Updates offenhalten
  if (wifiOk) {
    Serial.println("[STEP 3b] OTA ...");
    otaSetup(OTA_HOSTNAME, OTA_PASSWORD);
    otaWaitWindow(OTA_WINDOW_MS);
  }

  // 4) NTP mit der (ggf. vom Portal aktualisierten) Zeitzone
  bool timeOk = false;
  if (wifiOk) {
    Serial.println("[STEP 4] NTP ...");
    timeOk = timeSyncNTP(g_cfg.timezone);
  }

  // 5) Modus ermitteln (aktuell: immer WEATHER)
  DisplayMode mode = detectCurrentMode();
  Serial.printf("[STEP 5] Modus = %s\n", modeName(mode));

  // 6) Modus-spezifisch
  switch (mode) {
    case MODE_SESSION: runSessionMode(wifiOk, timeOk); break;
    case MODE_WEATHER:
    default:           runWeatherMode(wifiOk, timeOk); break;
  }

  // 7) Aufraumen
  displaySleep();
  disconnectWiFi();

  Serial.printf("----- Cycle end (freeHeap=%u) -----\n", ESP.getFreeHeap());
  Serial.flush();
}

// ---------------------------------------------------------------------
//  setup()
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000);

  for (int i = 0; i < 2; i++) {
    Serial.println();
    Serial.println("===========================================");
    Serial.println(" AstroOyInk  ---  BOOT ALIVE");
    Serial.println("===========================================");
    Serial.flush();
    delay(300);
  }

  bootCount++;
  Serial.printf("Boot #%u  |  Debug=%d  |  SDK=%s\n",
                bootCount, (int)DEBUG_MODE, ESP.getSdkVersion());
  Serial.printf("Free Heap beim Start: %u bytes\n", ESP.getFreeHeap());
  Serial.flush();

  doCycle();

#if !DEBUG_MODE
  goDeepSleep(g_cfg.refreshIntervalS);
#else
  Serial.println("[DEBUG] Kein Deep-Sleep - loop() uebernimmt.");
#endif
}

// ---------------------------------------------------------------------
//  loop() – nur im Debug-Modus aktiv
// ---------------------------------------------------------------------
void loop() {
#if DEBUG_MODE
  const uint32_t intervalMs = 30000;
  Serial.printf("[DEBUG] Warte %lu s bis zum naechsten Zyklus ...\n",
                (unsigned long)(intervalMs / 1000));
  Serial.flush();
  delay(intervalMs);
  doCycle();
#else
  delay(10000);
#endif
}
