// =====================================================================
//  local_sensor.h
//  Lokaler BME280-Sensor fuer Innentemperatur, Feuchte und Luftdruck.
//  Angeschlossen via I2C an LOCAL_SENSOR_SDA / LOCAL_SENSOR_SCL
//  (konfigurierbar in config.h).
//
//  Das Struct ist immer definiert, auch wenn LOCAL_SENSOR_ENABLED=0,
//  damit Aufrufstellen kompilieren ohne #if-Guards ueberall.
//  Wenn der Sensor deaktiviert ist, liefert localSensorRead() ein Struct
//  mit valid=false und NAN-Werten — das Display zeigt dann keine INNEN-Zeile.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_LOCAL_SENSOR_H
#define ASTROOYINK_LOCAL_SENSOR_H

#include <stdint.h>
#include <math.h>   // NAN

struct LocalSensorData {
  float tempC;       // Innentemperatur in Grad Celsius
  float humidity;    // Relative Feuchte in %
  float pressureHPa; // Luftdruck in hPa
  bool  valid;       // false wenn Sensor nicht gefunden oder Lesefehler
};

// Sensor initialisieren. Muss einmal pro Zyklus nach dem Aufwachen
// aufgerufen werden (I2C-Zustand geht im Deep-Sleep verloren).
// Gibt true zurueck wenn der BME280 antwortet.
bool localSensorInit();

// Alle drei Werte lesen und zurueckgeben.
// Bei deaktiviertem Sensor oder Fehler: valid=false, Werte=NAN.
LocalSensorData localSensorRead();

#endif // ASTROOYINK_LOCAL_SENSOR_H
