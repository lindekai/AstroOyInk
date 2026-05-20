// =====================================================================
//  src/modes/mode_state_machine.cpp
//
//  Stub-Implementierung. Wird mit der Session-Integration scharf
//  geschaltet (Roadmap-Punkt nach Feature 1-7).
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "mode_state_machine.h"

DisplayMode detectCurrentMode() {
  // TODO: Sobald nina_api.h existiert, hier:
  //   1. Prüfen, ob NINA_HOST erreichbar ist (TCP-Connect auf Port 1888)
  //   2. Wenn ja: GET /v2/api/sequence/state oder /v2/api/image-history
  //   3. Aktive Session → MODE_SESSION, sonst MODE_WEATHER
  return MODE_WEATHER;
}

const char* modeName(DisplayMode m) {
  switch (m) {
    case MODE_WEATHER: return "WEATHER";
    case MODE_SESSION: return "SESSION";
    default:           return "UNKNOWN";
  }
}
