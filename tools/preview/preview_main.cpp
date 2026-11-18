// =====================================================================
//  tools/preview/preview_main.cpp
//  Pixelgenaue Vorschau: ruft die echten Render-Funktionen aus
//  src/rendering/display_base.cpp mit Beispieldaten auf und schreibt
//  je Ansicht ein PNG (800x480, Panel-Farben).
//
//  Aufruf: tools/preview/build_preview.sh [ausgabe-ordner] [--scale N]
//                                         [--time UNIXZEIT]
// =====================================================================
#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <string>

#include "../../config.h"
#include "../../src/common/astro_logic.h"
#include "../../src/common/moon_calc.h"
#include "../../src/common/time_utils.h"
#include "../../src/rendering/display_base.h"

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Wetter fuer heute + Folgetage, angelehnt an eine wechselhafte Herbstnacht
static void buildWeather(WeatherData& wd, time_t now) {
  struct tm lt;
  localtime_r(&now, &lt);
  lt.tm_hour = 0; lt.tm_min = 0; lt.tm_sec = 0; lt.tm_isdst = -1;
  time_t day0 = mktime(&lt);

  wd.valid        = true;
  wd.fetchedAt    = now;
  wd.locationName = LOCATION_NAME;
  wd.hourlyCount  = MAX_HOURLY;
  for (uint16_t i = 0; i < wd.hourlyCount; i++) {
    HourlyData& h = wd.hourly[i];
    float t = (float)i;
    h.ts          = day0 + (time_t)i * 3600;
    h.tempC       = 8.0f + 3.0f * sinf((t - 9.0f) / 24.0f * 2.0f * (float)M_PI);
    h.humidity    = clampf(78.0f - 14.0f * sinf((t - 9.0f) / 24.0f * 2.0f * (float)M_PI), 0, 100);
    h.dewpointC   = h.tempC - (100.0f - h.humidity) / 5.0f;
    h.windKmh     = clampf(18.0f + 10.0f * sinf(t / 7.0f), 2, 60);
    h.cloudLow    = clampf(45.0f + 40.0f * sinf(t / 3.1f), 0, 100);
    h.cloudMid    = clampf(55.0f + 35.0f * sinf(t / 4.3f + 1.0f), 0, 100);
    h.cloudHigh   = clampf(40.0f + 45.0f * sinf(t / 5.7f + 2.0f), 0, 100);
    h.cloudTotal  = clampf(fmaxf(h.cloudLow, fmaxf(h.cloudMid, h.cloudHigh)), 0, 100);
    h.precipMm    = (i % 24 >= 14 && i % 24 <= 17 && i < 48) ? 0.4f * (float)(i % 3 + 1) : 0.0f;
    h.pressureHpa = 978.0f + 4.0f * sinf(t / 30.0f);
  }

  wd.dailyCount = FORECAST_DAYS < MAX_DAILY ? FORECAST_DAYS : MAX_DAILY;
  for (uint8_t d = 0; d < wd.dailyCount; d++) {
    DailyData& dd = wd.daily[d];
    dd.date        = day0 + (time_t)d * 86400;
    dd.tMinC       = 6.0f - 1.5f * d;
    dd.tMaxC       = 12.0f - 1.5f * d;
    dd.precipSumMm = (d % 2) ? 0.0f : 2.0f;
    dd.sunrise     = dd.date + 7 * 3600 + 30 * 60;
    dd.sunset      = dd.date + 16 * 3600 + 10 * 60;
  }

  // wie runWeatherMode() im Sketch
  computeAstroSummary(wd);
  wd.moonNow = computeMoon(now, LOCATION_LAT, LOCATION_LON);
  time_t nightMid = (wd.summary.nightStart > 0 && wd.summary.nightEnd > 0)
                        ? (wd.summary.nightStart + wd.summary.nightEnd) / 2
                        : now + 6 * 3600;
  wd.moonNight = computeMoon(nightMid, LOCATION_LAT, LOCATION_LON);
  computeMoonTrack(wd, LOCATION_LAT, LOCATION_LON);
}

// Werte einer echten 7Timer-Antwort (Berlin, 18.11.2026): ab der zweiten
// Nachthaelfte ist Seeing == Transparenz, die Linien liegen aufeinander
static void buildSeeing(SeeingData& sd, time_t day0) {
  static const uint8_t see[]  = {4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 4, 5, 5, 5, 5, 5};
  static const uint8_t tra[]  = {3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 6, 8, 4, 4, 6};
  sd.valid = true;
  sd.count = MAX_SEEING_POINTS;
  for (uint8_t k = 0; k < sd.count; k++) {
    sd.points[k].ts           = day0 + (time_t)k * 3 * 3600;
    sd.points[k].seeing       = see[k % 16];
    sd.points[k].transparency = tra[k % 16];
  }
}

static void buildSession(NinaData& nd) {
  memset(&nd, 0, sizeof(nd));
  nd.valid = true;
  NinaSequenceInfo& s = nd.sequence;
  s.running     = true;
  strlcpy(s.targetName, "M42 Orionnebel", sizeof(s.targetName));
  s.targetIndex = 0;
  s.targetTotal = 2;
  s.subsDone    = 12;
  s.subsTotal   = 30;
  strlcpy(s.status, "Imaging", sizeof(s.status));

  nd.camera.connected = true;
  strlcpy(nd.camera.name, "ATR2600C", sizeof(nd.camera.name));
  strlcpy(nd.camera.filterName, "L-eXtreme", sizeof(nd.camera.filterName));
  nd.camera.exposureS = 180.0f;
  nd.camera.hfr       = 2.31f;

  nd.guiding.connected = true;
  nd.guiding.rmsRA     = 0.42f;
  nd.guiding.rmsDec    = 0.38f;
  nd.guiding.rmsTotal  = 0.57f;
  strlcpy(nd.guiding.state, "Guiding", sizeof(nd.guiding.state));

  nd.mount.connected = true;
  strlcpy(nd.mount.name, "EQ6-R Pro", sizeof(nd.mount.name));
  nd.mount.raH      = 5.588f;
  nd.mount.decDeg   = -5.39f;
  nd.mount.altDeg   = 38.2f;
  nd.mount.azDeg    = 152.0f;
  nd.mount.pierSide = 1;
  nd.mount.flipEtaS = 2700;
}

int main(int argc, char** argv) {
  std::string outDir = ".";
  time_t fixedNow = 0;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--scale" && i + 1 < argc) g_previewScale = atoi(argv[++i]);
    else if (a == "--time" && i + 1 < argc) fixedNow = (time_t)atoll(argv[++i]);
    else outDir = a;
  }
  if (g_previewScale < 1) g_previewScale = 1;

  configTzTime(TIMEZONE_POSIX, "");
  previewNowRef() = fixedNow;
  time_t now = time(nullptr);

  displayInit();

  BatteryInfo batt = {3.92f, 76, BATT_OK};

  // --- WEATHER ---
  static WeatherData wd;
  buildWeather(wd, now);
  SeeingData seeing;
  buildSeeing(seeing, wd.hourly[0].ts);
  ISSData iss = {};
  iss.valid = true;
  iss.count = 1;
  iss.passes[0].riseTime = wd.hourly[0].ts + 20 * 3600 + 15 * 60;
  iss.passes[0].duration = 300;
  LocalSensorData sensor = {21.4f, 48.0f, 1012.0f, true};

  std::string p = outDir + "/weather.png";
  g_previewOut = p.c_str();
  renderDashboard(wd, batt, sensor, seeing, iss);

  // --- WEATHER, klare ruhige Nacht ohne ISS, Sensor und Seeing ---
  for (uint16_t i = 0; i < wd.hourlyCount; i++) {
    HourlyData& h = wd.hourly[i];
    h.cloudLow   *= 0.1f;
    h.cloudMid   *= 0.15f;
    h.cloudHigh  *= 0.2f;
    h.cloudTotal *= 0.2f;
    h.windKmh     = 8.0f;
    h.humidity    = 70.0f;
  }
  computeAstroSummary(wd);
  computeMoonTrack(wd, LOCATION_LAT, LOCATION_LON);
  SeeingData noSeeing = {};
  ISSData noIss = {};
  LocalSensorData noSensor = {NAN, NAN, NAN, false};
  std::string pg = outDir + "/weather-clear.png";
  g_previewOut = pg.c_str();
  renderDashboard(wd, batt, noSensor, noSeeing, noIss);

  // --- SESSION ---
  static NinaData nd;
  buildSession(nd);
  std::string ps = outDir + "/session.png";
  g_previewOut = ps.c_str();
  renderSessionDashboard(nd, batt, LOCATION_NAME);

  // --- Fallback + Portal ---
  std::string pf = outDir + "/fallback.png";
  g_previewOut = pf.c_str();
  renderFallback("Kein WLAN");

  std::string pp = outDir + "/portal.png";
  g_previewOut = pp.c_str();
  renderPortalInfo("AstroOyInk-3A7F21");

  return 0;
}
