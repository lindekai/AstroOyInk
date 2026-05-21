// =====================================================================
//  seeing_api.cpp
//  7Timer! ASTRO-API — Seeing & Transparenz-Vorhersage.
//
//  API-Endpoint (HTTP, kein Key):
//    http://www.7timer.info/bin/astro.php?lon=<lon>&lat=<lat>&
//           ac=0&unit=metric&output=json&tzshift=0
//
//  JSON-Struktur (gekuerzt):
//    {
//      "init": "YYYYMMDDHH",   // UTC-Zeitpunkt der Modellinit
//      "dataseries": [
//        { "timepoint": 1, "seeing": 5, "transparency": 4, ... },
//        { "timepoint": 4, "seeing": 3, "transparency": 3, ... },
//        ...
//      ]
//    }
//  Wir lesen nur "init", "timepoint", "seeing" und "transparency".
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "seeing_api.h"
#include "config.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------------
//  Hilfsfkt: "YYYYMMDDHH" (UTC) → Unix-Zeitstempel
// ---------------------------------------------------------------------
static time_t parseInit7Timer(const char* s) {
  if (!s || strlen(s) < 10) return 0;

  int Y = (s[0] - '0') * 1000 + (s[1] - '0') * 100 +
          (s[2] - '0') * 10  + (s[3] - '0');
  int M = (s[4] - '0') * 10 + (s[5] - '0');
  int D = (s[6] - '0') * 10 + (s[7] - '0');
  int H = (s[8] - '0') * 10 + (s[9] - '0');

  static const uint16_t mdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

  long days = 0;
  for (int y = 1970; y < Y; y++) {
    bool leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
    days += leap ? 366 : 365;
  }
  bool isLeap = (Y % 4 == 0 && (Y % 100 != 0 || Y % 400 == 0));
  for (int m = 1; m < M; m++) {
    days += mdays[m - 1];
    if (m == 2 && isLeap) days++;
  }
  days += D - 1;
  return (time_t)(days * 86400LL + (long)H * 3600LL);
}

// ---------------------------------------------------------------------
//  fetchSeeing
// ---------------------------------------------------------------------
bool fetchSeeing(float lat, float lon, SeeingData& out) {
  out.valid = false;
  out.count = 0;

  char url[200];
  snprintf(url, sizeof(url),
           "http://www.7timer.info/bin/astro.php"
           "?lon=%.4f&lat=%.4f&ac=0&unit=metric&output=json&tzshift=0",
           lon, lat);

  Serial.printf("[7TIMER] URL: %s\n", url);

  WiFiClient    client;
  HTTPClient    http;
  if (!http.begin(client, url)) {
    Serial.println("[7TIMER] http.begin fehlgeschlagen");
    return false;
  }
  http.setTimeout(10000);
  http.setUserAgent(HTTP_USER_AGENT);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[7TIMER] HTTP %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  Serial.printf("[7TIMER] payload bytes: %u\n", (unsigned)payload.length());

  // Filter: nur die Felder die wir brauchen
  JsonDocument filter;
  filter["init"]                          = true;
  filter["dataseries"][0]["timepoint"]    = true;
  filter["dataseries"][0]["seeing"]       = true;
  filter["dataseries"][0]["transparency"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(
    doc, payload, DeserializationOption::Filter(filter));

  if (err) {
    Serial.printf("[7TIMER] JSON parse error: %s\n", err.c_str());
    return false;
  }

  const char* initStr = doc["init"] | "";
  time_t initTs = parseInit7Timer(initStr);
  if (initTs == 0) {
    Serial.println("[7TIMER] Ungueltige Init-Zeit");
    return false;
  }
  Serial.printf("[7TIMER] init=%s → ts=%ld\n", initStr, (long)initTs);

  JsonArray series = doc["dataseries"].as<JsonArray>();
  if (series.isNull()) {
    Serial.println("[7TIMER] dataseries fehlt");
    return false;
  }

  for (JsonObject pt : series) {
    if (out.count >= MAX_SEEING_POINTS) break;

    int tp    = pt["timepoint"] | -1;
    int see   = pt["seeing"]    | 0;
    int transp= pt["transparency"] | 0;

    if (tp < 0 || see < 1 || see > 8 || transp < 1 || transp > 8) continue;

    SeeingPoint& sp  = out.points[out.count++];
    sp.ts            = initTs + (time_t)tp * 3600L;
    sp.seeing        = (uint8_t)see;
    sp.transparency  = (uint8_t)transp;
  }

  out.valid = (out.count > 0);
  Serial.printf("[7TIMER] OK: %u Datenpunkte\n", out.count);
  return out.valid;
}
