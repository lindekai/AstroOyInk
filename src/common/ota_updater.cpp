// =====================================================================
//  ota_updater.cpp
//  OTA-Update-Fenster via ArduinoOTA.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "ota_updater.h"
#include <Arduino.h>
#include <ArduinoOTA.h>

void otaSetup(const char* hostname, const char* password) {
  ArduinoOTA.setHostname(hostname);
  if (password && password[0] != '\0') {
    ArduinoOTA.setPassword(password);
  }

  ArduinoOTA.onStart([]() {
    Serial.println("[OTA] Update-Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Update abgeschlossen — Neustart...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] %3u%%\r", progress * 100 / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Fehler[%u]: ", error);
    if      (error == OTA_AUTH_ERROR)    Serial.println("Authentifizierung fehlgeschlagen");
    else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin fehlgeschlagen");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Verbindung fehlgeschlagen");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Empfang fehlgeschlagen");
    else if (error == OTA_END_ERROR)     Serial.println("Abschluss fehlgeschlagen");
    else                                 Serial.println("Unbekannter Fehler");
  });

  ArduinoOTA.begin();
  Serial.printf("[OTA] Bereit — Hostname: '%s'\n", hostname);
}

void otaWaitWindow(uint32_t windowMs) {
  Serial.printf("[OTA] Wartenfenster %lu ms ...\n", (unsigned long)windowMs);
  uint32_t start = millis();
  while ((millis() - start) < windowMs) {
    ArduinoOTA.handle();
    delay(10);
  }
  Serial.println("[OTA] Fenster geschlossen");
}
