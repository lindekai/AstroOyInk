// =====================================================================
//  src/data_sources/nina_api.cpp
//  N.I.N.A. Advanced API v2 — REST-Client.
//  Alle Abfragen sind HTTP (kein TLS), Timeout 4 s pro Request.
//
//  Feldzuordnung (N.I.N.A. 3.x Advanced API Plugin):
//    Response-Wrapper:  {"Success": true/false, "Response": {...}}
//    Sequence:   sequence/state liefert einen Baum (Status je Knoten,
//                Target{TargetName}, Iterations, Conditions) —
//                Auswertung in nina_sequence.cpp
//    Camera:     Connected, Name
//    ImageHistory: [ {Filter, ExposureTime, HFR}, ... ]
//    Mount:      Connected, Name, RightAscension, Declination,
//                Altitude, Azimuth, SideOfPier, TimeToMeridianFlip,
//                Slewing
//    Guider:     Connected, RMSError{RA,Dec,Total}, State
//
//  Sollte N.I.N.A. einzelne Felder nicht liefern, bleibt der
//  entsprechende Struct-Wert auf 0 / false / "--".
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================

#include "nina_api.h"
#include "nina_sequence.h"
#include "../../config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------------
//  Interner HTTP-GET-Helfer
// ---------------------------------------------------------------------
static bool ninaGet(const char* host, uint16_t port, const char* path,
                    JsonDocument& doc) {
  char url[192];
  snprintf(url, sizeof(url), "http://%s:%u%s", host, port, path);

  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);
  http.setTimeout(4000);
  http.addHeader("Accept", "application/json");

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[NINA] GET %s -> HTTP %d\n", path, code);
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  DeserializationError err = deserializeJson(
      doc, body, DeserializationOption::NestingLimit(NINA_SEQ_NESTING_LIMIT));
  if (err) {
    Serial.printf("[NINA] JSON-Fehler bei %s: %s\n", path, err.c_str());
    return false;
  }
  if (!doc["Success"].as<bool>()) {
    Serial.printf("[NINA] Success=false bei %s\n", path);
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------
bool ninaProbe(const char* host, uint16_t port) {
  if (!host || host[0] == '\0') return false;
  JsonDocument doc;
  return ninaGet(host, port, "/v2/api/version", doc);
}

bool ninaIsSequenceRunning(const char* host, uint16_t port) {
  if (!host || host[0] == '\0') return false;
  JsonDocument doc;
  if (!ninaGet(host, port, "/v2/api/sequence/state", doc)) {
    Serial.println("[NINA] sequence/state nicht abrufbar (N.I.N.A. nicht erreichbar?)");
    return false;
  }
  bool running = ninaSequenceTreeRunning(doc["Response"]);
  if (!running) Serial.println("[NINA] Erreichbar, aber keine Sequenz mit Status RUNNING");
  return running;
}

// ---------------------------------------------------------------------
bool ninaFetchAll(const char* host, uint16_t port, NinaData& out) {
  memset(&out, 0, sizeof(out));
  out.mount.flipEtaS = -1;
  out.camera.tempC     = NAN;
  out.camera.setpointC = NAN;
  // Standardwerte fuer Strings damit das Display "--" zeigt
  strlcpy(out.camera.filterName, "--", sizeof(out.camera.filterName));
  strlcpy(out.guiding.state,     "---", sizeof(out.guiding.state));

  if (!ninaProbe(host, port)) return false;

  JsonDocument doc;

  // --- Sequenz (Baum aus sequence/state) ---
  strlcpy(out.sequence.status, "Idle", sizeof(out.sequence.status));
  if (ninaGet(host, port, "/v2/api/sequence/state", doc)) {
    ninaParseSequenceState(doc["Response"], out.sequence);
  }

  // --- Kamera ---
  doc.clear();
  if (ninaGet(host, port, "/v2/api/equipment/camera/info", doc)) {
    JsonObject resp = doc["Response"].as<JsonObject>();
    out.camera.connected = resp["Connected"] | false;
    strlcpy(out.camera.name, resp["Name"] | "--", sizeof(out.camera.name));

    // Kuehlung: TemperatureSetPoint ist der Wert im Treiber und steht bei
    // ausgeschaltetem Kuehler auf einem Default (z. B. 10). TargetTemp ist
    // das in N.I.N.A. eingestellte Ziel - das ist ohne Kuehler das "Soll".
    out.camera.canCool        = resp["CanSetTemperature"] | false;
    out.camera.coolerOn       = resp["CoolerOn"] | false;
    out.camera.tempC          = resp["Temperature"] | NAN;
    out.camera.coolerPowerPct = resp["CoolerPower"] | 0.0f;
    out.camera.setpointC      = out.camera.coolerOn
                                    ? (resp["TemperatureSetPoint"] | NAN)
                                    : (resp["TargetTemp"] | NAN);
  }

  // --- Letztes Bild (Filter, Belichtung, HFR) ---
  doc.clear();
  // Response ist eine Liste; index=0 = neuestes Bild
  if (ninaGet(host, port, "/v2/api/image-history?index=0", doc)) {
    JsonVariant r = doc["Response"];
    JsonObject resp = r.is<JsonArray>() ? r[0].as<JsonObject>() : r.as<JsonObject>();
    const char* filt = resp["Filter"] | nullptr;
    if (filt && filt[0] != '\0') {
      strlcpy(out.camera.filterName, filt, sizeof(out.camera.filterName));
    }
    out.camera.exposureS = resp["ExposureTime"] | 0.0f;
    out.camera.hfr       = resp["HFR"]         | 0.0f;
  }

  // --- Montierung ---
  doc.clear();
  if (ninaGet(host, port, "/v2/api/equipment/mount/info", doc)) {
    JsonObject resp = doc["Response"].as<JsonObject>();
    out.mount.connected = resp["Connected"] | false;
    strlcpy(out.mount.name, resp["Name"] | "--", sizeof(out.mount.name));

    out.mount.raH    = resp["RightAscension"] | 0.0f;
    out.mount.decDeg = resp["Declination"]    | 0.0f;
    out.mount.altDeg = resp["Altitude"]       | 0.0f;
    out.mount.azDeg  = resp["Azimuth"]        | 0.0f;

    // Pier-Seite
    const char* pier = resp["SideOfPier"] | "Unknown";
    if      (strcmp(pier, "West") == 0 || strcmp(pier, "0") == 0) out.mount.pierSide = 0;
    else if (strcmp(pier, "East") == 0 || strcmp(pier, "1") == 0) out.mount.pierSide = 1;
    else                                                           out.mount.pierSide = 2;

    // Meridian-Flip: TimeToMeridianFlip kommt in STUNDEN (double).
    // 24 h meldet N.I.N.A., wenn kein Flip ansteht (z. B. Tracking aus).
    JsonVariant flip = resp["TimeToMeridianFlip"];
    if (!flip.isNull()) {
      float h = flip.as<float>();
      if (h >= 0.0f && h < 23.9f) out.mount.flipEtaS = (int32_t)(h * 3600.0f);
    }

    // Slewing ueberschreibt den abgeleiteten Status
    if (resp["Slewing"] | false) {
      strlcpy(out.sequence.status, "Slewing", sizeof(out.sequence.status));
    }
  }

  // --- Guiding ---
  doc.clear();
  if (ninaGet(host, port, "/v2/api/equipment/guider/info", doc)) {
    JsonObject resp = doc["Response"].as<JsonObject>();
    out.guiding.connected = resp["Connected"] | false;
    out.guiding.rmsRA     = resp["RMSError"]["RA"]    | 0.0f;
    out.guiding.rmsDec    = resp["RMSError"]["Dec"]   | 0.0f;
    out.guiding.rmsTotal  = resp["RMSError"]["Total"] | 0.0f;
    const char* st = resp["State"] | nullptr;
    if (st && st[0] != '\0') {
      strlcpy(out.guiding.state, st, sizeof(out.guiding.state));
    }
  }

  out.valid = true;
  Serial.printf("[NINA] Fetch ok: seq=%s running=%d sub=%d/%d\n",
                out.sequence.targetName, (int)out.sequence.running,
                out.sequence.subsDone, out.sequence.subsTotal);
  return true;
}
