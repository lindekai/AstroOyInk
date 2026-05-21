// =====================================================================
//  src/data_sources/nina_api.cpp
//  N.I.N.A. Advanced API v2 — REST-Client.
//  Alle Abfragen sind HTTP (kein TLS), Timeout 4 s pro Request.
//
//  Feldzuordnung (N.I.N.A. 3.x Advanced API Plugin):
//    Response-Wrapper:  {"Success": true/false, "Response": {...}}
//    Sequence:   IsRunning, IsPaused, SequenceName, SelectedTarget{Name,
//                CapturedFrames, TotalFrames}, ActiveTargetIndex,
//                TotalTargets, EstimatedEndTime, CurrentInstruction
//    Camera:     Connected, Name
//    ImageHistory: Filter, ExposureTime, HFR
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
#include "config.h"
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

  DeserializationError err = deserializeJson(doc, body);
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
  return ninaGet(host, port, "/v2/api/application/version", doc);
}

bool ninaIsSequenceRunning(const char* host, uint16_t port) {
  if (!host || host[0] == '\0') return false;
  JsonDocument doc;
  if (!ninaGet(host, port, "/v2/api/sequence", doc)) return false;
  return doc["Response"]["IsRunning"].as<bool>();
}

// ---------------------------------------------------------------------
bool ninaFetchAll(const char* host, uint16_t port, NinaData& out) {
  memset(&out, 0, sizeof(out));
  out.mount.flipEtaS = -1;
  // Standardwerte fuer Strings damit das Display "--" zeigt
  strlcpy(out.camera.filterName, "--", sizeof(out.camera.filterName));
  strlcpy(out.guiding.state,     "---", sizeof(out.guiding.state));

  if (!ninaProbe(host, port)) return false;

  JsonDocument doc;

  // --- Sequenz ---
  if (ninaGet(host, port, "/v2/api/sequence", doc)) {
    JsonObject resp = doc["Response"].as<JsonObject>();

    out.sequence.running = resp["IsRunning"] | false;
    out.sequence.paused  = resp["IsPaused"]  | false;

    strlcpy(out.sequence.sequenceName,
            resp["SequenceName"] | "", sizeof(out.sequence.sequenceName));

    // Target-Infos (N.I.N.A. 3.x Struktur)
    JsonObject tgt = resp["SelectedTarget"].as<JsonObject>();
    if (!tgt.isNull()) {
      strlcpy(out.sequence.targetName,
              tgt["Name"] | "", sizeof(out.sequence.targetName));
      out.sequence.subsDone  = (int16_t)(tgt["CapturedFrames"] | 0);
      out.sequence.subsTotal = (int16_t)(tgt["TotalFrames"]    | 0);
    }
    out.sequence.targetIndex = (int16_t)(resp["ActiveTargetIndex"] | -1);
    out.sequence.targetTotal = (int16_t)(resp["TotalTargets"]      | 0);

    // Geschaetzte Endzeit: absoluter Unix-Timestamp oder relative Sekunden
    JsonVariant etaVar = resp["EstimatedEndTime"];
    if (!etaVar.isNull()) {
      uint32_t etaRaw = etaVar.as<uint32_t>();
      time_t now = time(nullptr);
      if (etaRaw > 1700000000UL) {
        out.sequence.etaS = (etaRaw > (uint32_t)now) ? etaRaw - (uint32_t)now : 0;
      } else {
        out.sequence.etaS = etaRaw;
      }
    }

    // Status ableiten
    if (out.sequence.paused) {
      strlcpy(out.sequence.status, "Pausiert", sizeof(out.sequence.status));
    } else if (out.sequence.running) {
      const char* step = resp["CurrentInstruction"] | "";
      if (strstr(step, "Focus") || strstr(step, "Autofocus")) {
        strlcpy(out.sequence.status, "Focusing", sizeof(out.sequence.status));
      } else {
        strlcpy(out.sequence.status, "Imaging",  sizeof(out.sequence.status));
      }
    } else {
      strlcpy(out.sequence.status, "Idle", sizeof(out.sequence.status));
    }
  }

  // --- Kamera ---
  doc.clear();
  if (ninaGet(host, port, "/v2/api/equipment/camera/info", doc)) {
    JsonObject resp = doc["Response"].as<JsonObject>();
    out.camera.connected = resp["Connected"] | false;
    strlcpy(out.camera.name, resp["Name"] | "--", sizeof(out.camera.name));
  }

  // --- Letztes Bild (Filter, Belichtung, HFR) ---
  doc.clear();
  if (ninaGet(host, port, "/v2/api/image-history/0", doc)) {
    JsonObject resp = doc["Response"].as<JsonObject>();
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

    // Meridian-Flip ETA
    JsonVariant flip = resp["TimeToMeridianFlip"];
    out.mount.flipEtaS = flip.isNull() ? -1L : (int32_t)flip.as<long>();

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
