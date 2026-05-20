// =====================================================================
//  AstroOyInk.ino
//  Astro-Display für Astrofotografen.  Zwei Modi:
//    - WEATHER:  Astro-Wetter-Dashboard (Open-Meteo + Mond)
//    - SESSION:  N.I.N.A.-Live-Anzeige  (geplant, siehe Roadmap)
//
//  Ablauf pro Zyklus:
//    1) Display-Init
//    2) WLAN verbinden
//    3) NTP-Zeit synchronisieren
//    4) Modus ermitteln (detectCurrentMode)
//    5) Modus-spezifisch: Daten holen + rendern
//    6) Display in Hibernate
//
//  Produktiv (DEBUG_MODE=0):  Zyklus in setup(), danach Deep-Sleep.
//  Debug    (DEBUG_MODE=1):  Zyklus in setup() UND in loop() alle 30 s,
//                            kein Deep-Sleep → Serial Monitor bleibt an.
// =====================================================================

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"

// common
#include "data_types.h"
#include "time_utils.h"
#include "astro_logic.h"
#include "moon_calc.h"
#include "battery.h"

// data sources
#include "weather_api.h"

// modes
#include "mode_state_machine.h"

// rendering
#include "display_base.h"

// ---------------------------------------------------------------------
//  Persistente Daten über Deep-Sleep hinweg (RTC-RAM)
// ---------------------------------------------------------------------
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR bool     hasEverRendered = false;

// ---------------------------------------------------------------------
//  WLAN verbinden mit Reconnect-Logik
// ---------------------------------------------------------------------
static bool connectWiFi() {
  Serial.printf("[WIFI] Connecting to %s ...\n", WIFI_SSID);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WIFI] OK, IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  Serial.println("[WIFI] FAILED");
  return false;
}

static void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// ---------------------------------------------------------------------
//  Tiefschlaf bis zum nächsten Update (nur im Produktivmodus)
// ---------------------------------------------------------------------
static void goDeepSleep() {
  uint64_t sleepUs = (uint64_t)REFRESH_INTERVAL_S * 1000000ULL;

  time_t now = time(nullptr);
  if (now > 1700000000) {
    struct tm lt; localtime_r(&now, &lt);
    uint32_t toNextHour = (uint32_t)((59 - lt.tm_min) * 60 + (60 - lt.tm_sec));
    if (toNextHour < 60) toNextHour += 3600;
    if (toNextHour < REFRESH_INTERVAL_S * 2) {
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
  // Akku einmal pro Zyklus lesen (vor dem Render, kein WLAN noetig)
  BatteryInfo batt = readBattery();

  // WeatherData liegt static, damit es den 8 KB Stack nicht sprengt
  static WeatherData wd;
  wd.locationName = LOCATION_NAME;

  bool apiOk = false;
  if (wifiOk && timeOk) {
    Serial.println("[STEP 4] Fetch Open-Meteo ...");
    apiOk = fetchWeather(LOCATION_LAT, LOCATION_LON, FORECAST_DAYS, wd);
  }

  if (apiOk) {
    Serial.println("[STEP 5] Compute Astro-Summary ...");
    computeAstroSummary(wd);
    Serial.printf("         verdict=%d  avgCloudScore=%.1f  nightHours=%u\n",
                  (int)wd.summary.verdict,
                  wd.summary.avgCloudScore,
                  wd.summary.nightHourCount);

    Serial.println("[STEP 5b] Compute Moon ...");
    wd.moonNow = computeMoon(time(nullptr), LOCATION_LAT, LOCATION_LON);
    time_t nightMid = 0;
    if (wd.summary.nightStart > 0 && wd.summary.nightEnd > 0) {
      nightMid = (wd.summary.nightStart + wd.summary.nightEnd) / 2;
    } else {
      nightMid = time(nullptr) + 6 * 3600;
    }
    wd.moonNight = computeMoon(nightMid, LOCATION_LAT, LOCATION_LON);
    Serial.printf("         Moon now:    %s  illum=%.0f%%  alt=%.1f°\n",
                  wd.moonNow.phaseName,
                  wd.moonNow.illumination * 100.0f,
                  wd.moonNow.altitudeDeg);
    Serial.printf("         Moon Mitte:  %s  illum=%.0f%%  alt=%.1f°\n",
                  wd.moonNight.phaseName,
                  wd.moonNight.illumination * 100.0f,
                  wd.moonNight.altitudeDeg);
  }

  if (apiOk) {
    Serial.println("[STEP 6] Render Dashboard ...");
    Serial.flush();
    uint32_t t0 = millis();
    renderDashboard(wd, batt);
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
//  Roadmap: nach Abschluss der Features 1-7 (Batterie, WiFiManager,
//  OTA, BME280, 7Timer!, ISS, SD).
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

  // 2) WLAN
  Serial.println("[STEP 2] WiFi ...");
  bool wifiOk = connectWiFi();

  // 3) NTP
  bool timeOk = false;
  if (wifiOk) {
    Serial.println("[STEP 3] NTP ...");
    timeOk = timeSyncNTP();
  }

  // 4) Modus ermitteln (aktuell: immer WEATHER)
  DisplayMode mode = detectCurrentMode();
  Serial.printf("[STEP 4] Modus = %s\n", modeName(mode));

  // 5+6) Modus-spezifisch
  switch (mode) {
    case MODE_SESSION: runSessionMode(wifiOk, timeOk); break;
    case MODE_WEATHER:
    default:           runWeatherMode(wifiOk, timeOk); break;
  }

  // 7) Aufräumen
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
  goDeepSleep();
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
