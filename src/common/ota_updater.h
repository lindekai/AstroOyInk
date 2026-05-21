// =====================================================================
//  ota_updater.h
//  OTA-Update-Fenster via ArduinoOTA.
//  Nach jedem WLAN-Connect wird ein kurzes Zeitfenster geoeffnet, in
//  dem ein Flash-Update aus der Arduino IDE per Netzwerk moeglich ist.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_OTA_UPDATER_H
#define ASTROOYINK_OTA_UPDATER_H

#include <stdint.h>

// ArduinoOTA initialisieren (Hostname + optionales Passwort setzen,
// Callbacks registrieren, begin() aufrufen).
void otaSetup(const char* hostname, const char* password);

// windowMs Millisekunden lang ArduinoOTA.handle() pollen, damit ein
// Update aus der Arduino IDE eingespielt werden kann.
void otaWaitWindow(uint32_t windowMs);

#endif // ASTROOYINK_OTA_UPDATER_H
