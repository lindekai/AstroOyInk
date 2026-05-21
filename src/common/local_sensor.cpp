// =====================================================================
//  local_sensor.cpp
//  Lokaler BME280-Sensor fuer Innentemperatur, Feuchte und Luftdruck.
//
//  Benoetigt: "Adafruit BME280 Library" + "Adafruit Unified Sensor"
//  (beide via Arduino Library Manager installieren).
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "local_sensor.h"
#include "config.h"
#include <Arduino.h>

#if LOCAL_SENSOR_ENABLED

#include <Wire.h>
#include <Adafruit_BME280.h>

static Adafruit_BME280 bme;
static bool            g_bmeOk = false;

bool localSensorInit() {
  Wire.begin(LOCAL_SENSOR_SDA, LOCAL_SENSOR_SCL);
  g_bmeOk = bme.begin(LOCAL_SENSOR_ADDR);
  if (g_bmeOk) {
    Serial.printf("[SENSOR] BME280 OK (addr=0x%02X, SDA=%d, SCL=%d)\n",
                  LOCAL_SENSOR_ADDR, LOCAL_SENSOR_SDA, LOCAL_SENSOR_SCL);
  } else {
    Serial.printf("[SENSOR] BME280 nicht gefunden (addr=0x%02X) — "
                  "SDO-Pin pruefen oder Adresse in config.h aendern\n",
                  LOCAL_SENSOR_ADDR);
  }
  return g_bmeOk;
}

LocalSensorData localSensorRead() {
  LocalSensorData d;
  d.tempC       = NAN;
  d.humidity    = NAN;
  d.pressureHPa = NAN;
  d.valid       = false;

  if (!g_bmeOk) return d;

  d.tempC       = bme.readTemperature();
  d.humidity    = bme.readHumidity();
  d.pressureHPa = bme.readPressure() / 100.0f;  // Pa → hPa

  d.valid = !isnan(d.tempC) && !isnan(d.humidity) && !isnan(d.pressureHPa);
  if (d.valid) {
    Serial.printf("[SENSOR] T=%.1fC  H=%.0f%%  P=%.0fhPa\n",
                  d.tempC, d.humidity, d.pressureHPa);
  } else {
    Serial.println("[SENSOR] Lesefehler — NAN zurueckgegeben");
  }
  return d;
}

#else  // LOCAL_SENSOR_ENABLED == 0

bool localSensorInit() { return false; }

LocalSensorData localSensorRead() {
  LocalSensorData d;
  d.tempC       = NAN;
  d.humidity    = NAN;
  d.pressureHPa = NAN;
  d.valid       = false;
  return d;
}

#endif // LOCAL_SENSOR_ENABLED
