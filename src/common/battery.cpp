// =====================================================================
//  src/common/battery.cpp
//  ADC-Lesung und LiPo-Entladekurve fuer die Batterieanzeige.
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "battery.h"
#include "config.h"

#if BATT_ENABLED

// LiPo-Stuetzpunkte: Spannung (V) absteigend → Kapazitaet (%)
static const float kVolt[] = {4.20f, 4.05f, 3.90f, 3.75f, 3.60f, 3.45f, 3.30f, 3.00f};
static const float kPct[]  = {100.f,  90.f,  75.f,  60.f,  40.f,  20.f,  10.f,   0.f};
static const uint8_t kN    = sizeof(kVolt) / sizeof(kVolt[0]);

// Mittelwert in Millivolt am ADC-Pin. analogReadMilliVolts() nutzt die
// eFuse-Kalibrierung des ESP32 und ist unabhaengig von der Versorgungs-
// spannung — wichtig, weil der Buck-Wandler bei leerem Akku keine
// stabilen 3,3 V mehr liefert.
static float readPinVoltage(uint8_t pin) {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += analogReadMilliVolts(pin);
  }
  return ((float)sum / 16.0f) / 1000.0f;
}

static uint8_t voltToPercent(float v) {
  if (v >= kVolt[0])     return 100;
  if (v <= kVolt[kN - 1]) return 0;
  for (uint8_t i = 0; i < kN - 1; i++) {
    if (v <= kVolt[i] && v > kVolt[i + 1]) {
      float t = (v - kVolt[i + 1]) / (kVolt[i] - kVolt[i + 1]);
      return (uint8_t)(kPct[i + 1] + t * (kPct[i] - kPct[i + 1]) + 0.5f);
    }
  }
  return 0;
}

// USB am Lademodul angesteckt? Nur mit optionalem zweiten Teiler messbar,
// die Akkuspannung allein verraet es nicht (TC4056 laedt bis 4,2 V).
static bool usbPresent() {
#if BATT_USB_SENSE_PIN >= 0
  float v = readPinVoltage(BATT_USB_SENSE_PIN);
  Serial.printf("[BATT] USB-Sense=%.3f V\n", v);
  return v > 1.5f;  // 5 V / 2 = 2,5 V, ohne USB 0 V
#else
  return false;
#endif
}

BatteryInfo readBattery() {
  float v = readPinVoltage(BATT_ADC_PIN) * BATT_VOLTAGE_DIVIDER;
  Serial.printf("[BATT] V=%.3f\n", v);

  BatteryInfo b;
  b.voltageV = v;
  b.percent  = voltToPercent(v);

  if (usbPresent()) {
    b.status = BATT_USB;
    return b;
  }

  if      (b.percent <  5) b.status = BATT_CRITICAL;
  else if (b.percent < 20) b.status = BATT_LOW;
  else                     b.status = BATT_OK;
  return b;
}

#else  // BATT_ENABLED == 0

BatteryInfo readBattery() {
  BatteryInfo b;
  b.voltageV = 0.0f;
  b.percent  = 0;
  b.status   = BATT_UNKNOWN;
  return b;
}

#endif // BATT_ENABLED
