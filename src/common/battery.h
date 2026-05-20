// =====================================================================
//  src/common/battery.h
//  Liest den LiPo-Ladestand ueber den internen ADC des ESP32.
//  Externer Spannungsteiler: Akku+ → 100k → BATT_ADC_PIN → 100k → GND
// =====================================================================
#ifndef ASTROOYINK_BATTERY_H
#define ASTROOYINK_BATTERY_H

#include <Arduino.h>

enum BatteryStatus : uint8_t {
  BATT_UNKNOWN  = 0,  // Messung deaktiviert (BATT_ENABLED == 0)
  BATT_USB,           // Spannung > 4,3 V → USB-Betrieb / Laden
  BATT_OK,            // >= 20 %
  BATT_LOW,           // < 20 %
  BATT_CRITICAL,      // < 5 %
};

struct BatteryInfo {
  float         voltageV;  // gemessene Akkuspannung in Volt
  uint8_t       percent;   // 0–100
  BatteryStatus status;
};

// Liest den Akku-ADC und liefert eine befuellte BatteryInfo.
// Wenn BATT_ENABLED == 0, wird BATT_UNKNOWN zurueckgegeben.
BatteryInfo readBattery();

#endif // ASTROOYINK_BATTERY_H
