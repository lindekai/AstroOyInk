// =====================================================================
//  weather_api.cpp
//  Open-Meteo Client.
//
//  Hinweise:
//   - Open-Meteo benötigt KEINEN API-Key.
//   - Wir verwenden HTTP (kein S), um Speicher zu sparen. Open-Meteo
//     leitet trotzdem auf HTTPS um, daher folgen wir Redirects ODER
//     fragen direkt HTTPS an. Hier: HTTPS via WiFiClientSecure ohne
//     Zertifikatsprüfung – das ist akzeptabel, weil die Daten öffentlich
//     sind und nicht sicherheitskritisch.
//   - JSON-Filter reduzieren den RAM-Bedarf erheblich.
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "weather_api.h"
#include "time_utils.h"
#include "config.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <new>            // std::nothrow

// ---------------------------------------------------------------------
//  URL-Builder
// ---------------------------------------------------------------------
String buildOpenMeteoUrl(float lat, float lon, uint8_t days) {
  // Wir benutzen die "klassischen" Variablennamen (cloudcover_*),
  // die Open-Meteo aus Kompatibilitätsgründen weiterhin akzeptiert.
  // Falls dein Account/Service strikt die neuen Namen verlangt, ersetze
  // sie durch cloud_cover_low, relative_humidity_2m, ...
  String url = "https://api.open-meteo.com/v1/forecast";
  url += "?latitude=";  url += String(lat,  4);
  url += "&longitude="; url += String(lon, 4);
  url += "&hourly=temperature_2m,relativehumidity_2m,dewpoint_2m,";
  url += "windspeed_10m,cloudcover,cloudcover_low,cloudcover_mid,";
  url += "cloudcover_high,precipitation,surface_pressure";
  url += "&daily=temperature_2m_max,temperature_2m_min,sunrise,sunset,";
  url += "precipitation_sum";
  url += "&timezone=Europe%2FBerlin";
  url += "&forecast_days="; url += String(days);
  url += "&windspeed_unit=kmh";
  return url;
}

// ---------------------------------------------------------------------
//  Helper: Float aus JsonVariant, NaN-sicher
// ---------------------------------------------------------------------
static inline float jvFloat(JsonVariant v, float fallback = NAN) {
  if (v.isNull()) return fallback;
  return v.as<float>();
}

// ---------------------------------------------------------------------
//  Hauptfunktion
//  RAM-Strategie:
//   - deserializeJson() liest DIREKT aus dem HTTP-Stream (kein
//     Zwischen-String → spart ~30 KB Peak-RAM).
//   - 'tmp WeatherData' liegt auf dem Heap (sizeof ≈ 8 KB), damit der
//     Stack (8 KB auf ESP32!) nicht überläuft.
//   - JsonDocument verwaltet seinen Pool intern auf dem Heap.
// ---------------------------------------------------------------------
bool fetchWeather(float lat, float lon, uint8_t days, WeatherData& out) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API] WiFi nicht verbunden");
    return false;
  }

  String url = buildOpenMeteoUrl(lat, lon, days);
  Serial.print("[API] URL: ");
  Serial.println(url);

  // --------------------- HTTPS-Verbindung aufbauen ---------------------
  WiFiClientSecure client;
  client.setInsecure();          // Open-Meteo nutzt LE-Zert., wir prüfen nicht.

  HTTPClient http;
  http.setUserAgent(HTTP_USER_AGENT);
  http.setTimeout(15000);
  if (!http.begin(client, url)) {
    Serial.println("[HTTP] begin() failed");
    return false;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[HTTP] GET failed: %d\n", code);
    http.end();
    return false;
  }
  Serial.printf("[HTTP] Content-Length: %d\n", http.getSize());

  // --------------------- Body komplett einlesen ---------------------
  //  Wir lesen den Body in einen String. Das ist robuster als direktes
  //  Stream-Parsen, wenn der Server Chunked Transfer Encoding verwendet
  //  (Content-Length: -1). Mit Paged-Display haben wir genug RAM.
  String payload = http.getString();
  http.end();
  Serial.printf("[HTTP] payload bytes: %u\n", (unsigned)payload.length());

  if (payload.length() < 200) {
    // Verdächtig kurz - Problem sofort sichtbar machen
    Serial.println("[HTTP] WARN: payload ist sehr klein!");
    Serial.println("------------------ Payload ------------------");
    Serial.println(payload);
    Serial.println("---------------------------------------------");
    return false;
  }

  // Zur Kontrolle die ersten ~200 Zeichen loggen
  Serial.print("[HTTP] payload head: ");
  Serial.println(payload.substring(0, 200));

  // -------- JSON-Filter: nur Felder, die wir wirklich brauchen --------
  JsonDocument filter;
  JsonObject hf = filter["hourly"].to<JsonObject>();
  hf["time"]               = true;
  hf["temperature_2m"]     = true;
  hf["relativehumidity_2m"]= true;
  hf["dewpoint_2m"]        = true;
  hf["windspeed_10m"]      = true;
  hf["cloudcover"]         = true;
  hf["cloudcover_low"]     = true;
  hf["cloudcover_mid"]     = true;
  hf["cloudcover_high"]    = true;
  hf["precipitation"]      = true;
  hf["surface_pressure"]   = true;

  JsonObject df = filter["daily"].to<JsonObject>();
  df["time"]                = true;
  df["temperature_2m_max"]  = true;
  df["temperature_2m_min"]  = true;
  df["sunrise"]             = true;
  df["sunset"]              = true;
  df["precipitation_sum"]   = true;

  // --------------------- Parsen aus String ---------------------
  JsonDocument doc;
  DeserializationError err = deserializeJson(
      doc, payload, DeserializationOption::Filter(filter));

  if (err) {
    Serial.print("[API] JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  // Diagnose: welche Top-Level-Keys hat das Doc nach dem Filter?
  Serial.print("[API] top-level keys:");
  for (JsonPair kv : doc.as<JsonObject>()) {
    Serial.print(" ");
    Serial.print(kv.key().c_str());
  }
  Serial.println();

  // ---------------- Hourly auslesen ----------------
  JsonArray jsTime  = doc["hourly"]["time"].as<JsonArray>();
  JsonArray jsTemp  = doc["hourly"]["temperature_2m"].as<JsonArray>();
  JsonArray jsHum   = doc["hourly"]["relativehumidity_2m"].as<JsonArray>();
  JsonArray jsDew   = doc["hourly"]["dewpoint_2m"].as<JsonArray>();
  JsonArray jsWind  = doc["hourly"]["windspeed_10m"].as<JsonArray>();
  JsonArray jsCl    = doc["hourly"]["cloudcover"].as<JsonArray>();
  JsonArray jsClL   = doc["hourly"]["cloudcover_low"].as<JsonArray>();
  JsonArray jsClM   = doc["hourly"]["cloudcover_mid"].as<JsonArray>();
  JsonArray jsClH   = doc["hourly"]["cloudcover_high"].as<JsonArray>();
  JsonArray jsPre   = doc["hourly"]["precipitation"].as<JsonArray>();
  JsonArray jsPress = doc["hourly"]["surface_pressure"].as<JsonArray>();

  if (jsTime.isNull() || jsTime.size() == 0) {
    Serial.println("[API] hourly leer");
    return false;
  }

  uint16_t n = jsTime.size();
  if (n > MAX_HOURLY) n = MAX_HOURLY;

  // ---- Wir schreiben in eine TEMPORÄRE Struktur und kopieren erst am
  //      Ende. Damit bleiben die alten 'out'-Daten bei einem Fehler erhalten.
  //      Die Struktur liegt auf dem HEAP (ca. 8 KB), weil der ESP32-Stack
  //      nur 8 KB groß ist und schon für WiFi/HTTPS beansprucht wird.
  WeatherData* tmp = new (std::nothrow) WeatherData();
  if (!tmp) {
    Serial.println("[API] kein Heap-Speicher fuer WeatherData");
    return false;
  }
  // new WeatherData() führt Value-Initialisierung aus:
  // POD-Felder auf 0, String-Member leer – KEIN memset() hier, sonst
  // würden die konstruierten Strings zerstört (undefined behavior).
  tmp->locationName = LOCATION_NAME;
  tmp->fetchedAt    = time(nullptr);
  tmp->hourlyCount  = n;

  for (uint16_t i = 0; i < n; i++) {
    HourlyData& h = tmp->hourly[i];
    h.ts          = parseIsoLocal(jsTime[i].as<const char*>());
    h.tempC       = jvFloat(jsTemp[i]);
    h.humidity    = jvFloat(jsHum[i]);
    h.dewpointC   = jvFloat(jsDew[i]);
    h.windKmh     = jvFloat(jsWind[i]);
    h.cloudTotal  = jvFloat(jsCl[i]);
    h.cloudLow    = jvFloat(jsClL[i]);
    h.cloudMid    = jvFloat(jsClM[i]);
    h.cloudHigh   = jvFloat(jsClH[i]);
    h.precipMm    = jvFloat(jsPre[i], 0.0f);
    h.pressureHpa = jvFloat(jsPress[i], NAN);
  }

  // ---------------- Daily auslesen ----------------
  JsonArray dTime = doc["daily"]["time"].as<JsonArray>();
  JsonArray dMax  = doc["daily"]["temperature_2m_max"].as<JsonArray>();
  JsonArray dMin  = doc["daily"]["temperature_2m_min"].as<JsonArray>();
  JsonArray dSr   = doc["daily"]["sunrise"].as<JsonArray>();
  JsonArray dSs   = doc["daily"]["sunset"].as<JsonArray>();
  JsonArray dPr   = doc["daily"]["precipitation_sum"].as<JsonArray>();

  uint8_t dn = dTime.size();
  if (dn > MAX_DAILY) dn = MAX_DAILY;
  tmp->dailyCount = dn;

  for (uint8_t i = 0; i < dn; i++) {
    DailyData& d = tmp->daily[i];
    d.date        = parseIsoLocal((String(dTime[i].as<const char*>()) + "T00:00").c_str());
    d.tMaxC       = jvFloat(dMax[i]);
    d.tMinC       = jvFloat(dMin[i]);
    d.sunrise     = parseIsoLocal(dSr[i].as<const char*>());
    d.sunset      = parseIsoLocal(dSs[i].as<const char*>());
    d.precipSumMm = jvFloat(dPr[i], 0.0f);
  }

  tmp->valid = true;
  out = *tmp;        // erst jetzt übernehmen → Atomarität bei Fehlern
  delete tmp;
  Serial.printf("[API] OK: %u hourly, %u daily\n", n, dn);
  return true;
}
