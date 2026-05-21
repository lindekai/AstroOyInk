// =====================================================================
//  iss_api.cpp
//  ISS-Ueberflugvorhersage via open-notify.org.
//
//  API-Endpoint (HTTP, kein Key):
//    http://api.open-notify.org/iss-pass.json?lat=<lat>&lon=<lon>&n=5
//
//  JSON-Struktur:
//    {
//      "message": "success",
//      "response": [
//        { "risetime": 1722349234, "duration": 374 },
//        ...
//      ]
//    }
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "iss_api.h"
#include "config.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool fetchISSPasses(float lat, float lon, ISSData& out) {
  out.valid = false;
  out.count = 0;

  char url[160];
  snprintf(url, sizeof(url),
           "http://api.open-notify.org/iss-pass.json"
           "?lat=%.4f&lon=%.4f&n=%d",
           lat, lon, MAX_ISS_PASSES);

  Serial.printf("[ISS] URL: %s\n", url);

  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, url)) {
    Serial.println("[ISS] http.begin fehlgeschlagen");
    return false;
  }
  http.setTimeout(10000);
  http.setUserAgent(HTTP_USER_AGENT);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[ISS] HTTP %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  Serial.printf("[ISS] payload bytes: %u\n", (unsigned)payload.length());

  JsonDocument filter;
  filter["response"][0]["risetime"] = true;
  filter["response"][0]["duration"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(
    doc, payload, DeserializationOption::Filter(filter));

  if (err) {
    Serial.printf("[ISS] JSON parse error: %s\n", err.c_str());
    return false;
  }

  JsonArray resp = doc["response"].as<JsonArray>();
  if (resp.isNull()) {
    Serial.println("[ISS] response fehlt oder API-Fehler");
    return false;
  }

  for (JsonObject pass : resp) {
    if (out.count >= MAX_ISS_PASSES) break;
    long rt  = pass["risetime"] | 0L;
    int  dur = pass["duration"] | 0;
    if (rt <= 0 || dur <= 0) continue;
    out.passes[out.count].riseTime = (time_t)rt;
    out.passes[out.count].duration = (uint16_t)dur;
    out.count++;
  }

  out.valid = (out.count > 0);
  if (out.valid) {
    Serial.printf("[ISS] OK: %u Passes, naechster risetime=%ld dur=%us\n",
                  out.count,
                  (long)out.passes[0].riseTime,
                  out.passes[0].duration);
  } else {
    Serial.println("[ISS] Keine Passes in der Antwort");
  }
  return out.valid;
}
