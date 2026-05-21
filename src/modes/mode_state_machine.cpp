// =====================================================================
//  src/modes/mode_state_machine.cpp
//  Modus-Erkennung: WEATHER ↔ SESSION.
//
//  Logik:
//    1. ninaHost leer → sofort MODE_WEATHER
//    2. N.I.N.A. nicht erreichbar → MODE_WEATHER
//    3. Keine Sequenz aktiv → MODE_WEATHER
//    4. Sequenz laeuft → MODE_SESSION
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "mode_state_machine.h"
#include "nina_api.h"

DisplayMode detectCurrentMode(const char* ninaHost, uint16_t ninaPort) {
  if (!ninaHost || ninaHost[0] == '\0') {
    Serial.println("[MODE] N.I.N.A.-Host nicht konfiguriert → WEATHER");
    return MODE_WEATHER;
  }

  Serial.printf("[MODE] Pruefe N.I.N.A. %s:%u ...\n", ninaHost, ninaPort);
  if (!ninaIsSequenceRunning(ninaHost, ninaPort)) {
    Serial.println("[MODE] N.I.N.A. nicht erreichbar oder keine Sequenz → WEATHER");
    return MODE_WEATHER;
  }

  Serial.println("[MODE] N.I.N.A.-Sequenz laeuft → SESSION");
  return MODE_SESSION;
}

const char* modeName(DisplayMode m) {
  switch (m) {
    case MODE_WEATHER: return "WEATHER";
    case MODE_SESSION: return "SESSION";
    default:           return "UNKNOWN";
  }
}
