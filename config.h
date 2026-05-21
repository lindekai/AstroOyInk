// =====================================================================
//  config.h
//  Zentrale Konfiguration für AstroOyInk.
//  --------------------------------------------------------------------
//  Hier wird ALLES eingestellt, was der Nutzer anpassen muss:
//   - WLAN
//   - Standort (Lat/Lon)
//   - Anzeige-Ort (Stadtname für den Header)
//   - Pinbelegung des E-Ink-Displays
//   - Update-Intervall
//   - Zeitzone (Europe/Berlin inkl. Sommerzeit)
//   - (später) N.I.N.A.-Host für den Session-Modus
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_CONFIG_H
#define ASTROOYINK_CONFIG_H

// ---------------------------------------------------------------------
//  WLAN-Zugangsdaten
//  ACHTUNG: Niemals echte Credentials in ein öffentliches Repo committen!
//  Beim Veröffentlichen leere Strings einsetzen oder eine secrets.h nutzen.
// ---------------------------------------------------------------------
#define WIFI_SSID       "DEIN_WLAN_SSID"
#define WIFI_PASSWORD   "DEIN_WLAN_PASSWORT"

// Maximaler Verbindungsversuch in ms, danach: Fallback / alte Daten
#define WIFI_TIMEOUT_MS 20000

// ---------------------------------------------------------------------
//  Standort für die Wetterabfrage und Anzeige
// ---------------------------------------------------------------------
#define LOCATION_NAME   "Berlin"
#define LOCATION_LAT    52.5200f
#define LOCATION_LON    13.4050f

// ---------------------------------------------------------------------
//  Zeitzone (POSIX TZ-String)
//  CET-1CEST,M3.5.0,M10.5.0/3  =  Mitteleuropäische Zeit + Sommerzeit
// ---------------------------------------------------------------------
#define TIMEZONE_POSIX  "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER_1    "de.pool.ntp.org"
#define NTP_SERVER_2    "pool.ntp.org"
#define NTP_SERVER_3    "time.google.com"

// ---------------------------------------------------------------------
//  Refresh-Intervall in Sekunden (3600 = 1 Stunde) im WEATHER-Modus.
//  Im SESSION-Modus (geplant) wird ein kürzeres Intervall (~60-120 s)
//  verwendet, weil sich Belichtungs- und Guiding-Daten häufiger ändern.
// ---------------------------------------------------------------------
#define REFRESH_INTERVAL_S  3600UL

// ---------------------------------------------------------------------
//  DEBUG_MODE
//  Auf 1 setzen für Entwicklung / Fehlersuche:
//    - KEIN Deep-Sleep (Serial Monitor bleibt dauerhaft verbunden)
//    - Update alle 30 Sekunden statt 1 Stunde
//    - Mehr Log-Ausgaben
//  Auf 0 setzen für den Produktiv-Betrieb.
// ---------------------------------------------------------------------
#define DEBUG_MODE  1

// ---------------------------------------------------------------------
//  PIN-Belegung für das Waveshare 7.5" E-Paper (B/W/R)
//  Default-Mapping passt zum offiziellen Waveshare ESP32 Driver Board.
// ---------------------------------------------------------------------
#define EPD_PIN_BUSY    25
#define EPD_PIN_RST     26
#define EPD_PIN_DC      27
#define EPD_PIN_CS      15
#define EPD_PIN_SCK     13
#define EPD_PIN_MOSI    14

// ---------------------------------------------------------------------
//  Astro-Schwellwerte (Cloud-Score in %, Wind in km/h, Feuchte in %)
// ---------------------------------------------------------------------
#define ASTRO_CLOUD_GOOD       20.0f
#define ASTRO_CLOUD_MODERATE   45.0f
#define ASTRO_WIND_WARN        25.0f
#define ASTRO_HUMIDITY_WARN    90.0f

// ---------------------------------------------------------------------
//  Sonstiges
// ---------------------------------------------------------------------
#define FORECAST_DAYS          5
#define HTTP_USER_AGENT        "AstroOyInk/0.1 (ESP32)"

// ---------------------------------------------------------------------
//  WiFiManager (Feature 2)
//  Captive Portal oeffnet sich automatisch wenn kein WLAN gespeichert
//  oder die Verbindung fehlschlaegt.
//  AP-Name: AstroOyInk-XXXXXX (letzte 3 Bytes der Chip-MAC)
// ---------------------------------------------------------------------
#define WIFI_MANAGER_TIMEOUT_S  180    // Portal schliesst nach 3 Minuten

// ---------------------------------------------------------------------
//  SESSION-Modus — N.I.N.A. Advanced API (Roadmap #8)
//  Host/Port werden auch im WiFiManager-Portal konfiguriert (NVS).
//  Diese Defaults gelten nur wenn NVS leer ist.
//  Leer lassen → SESSION-Modus deaktiviert, immer WEATHER.
// ---------------------------------------------------------------------
// #define NINA_HOST_DEFAULT   "nina-pc.local"  // oder "192.168.1.42"
// #define NINA_PORT_DEFAULT   1888

// Aktualisierungsintervall im SESSION-Modus (Geraet bleibt wach)
#define SESSION_REFRESH_INTERVAL_S  300UL   // 5 Minuten

// ---------------------------------------------------------------------
//  Display-Farb-Modus
//  1 = BWR (Schwarz / Weiss / Rot) — Standard Waveshare 7.5" V3
//  0 = BW  (nur Schwarz / Weiss)  — z.B. Waveshare 7.5" mono
//  Bei BW: rote Elemente werden durch invertierte Bloecke (weiss auf
//  schwarz) ersetzt, damit Warnungen trotzdem hervorstechen.
// ---------------------------------------------------------------------
#define DISPLAY_HAS_RED  1

// ---------------------------------------------------------------------
//  Guiding-Warnschwelle (SESSION-Modus)
//  RMS Total >= dieser Wert → [Guiding WARN] in Rot / invertiert.
// ---------------------------------------------------------------------
#define GUIDING_WARN_RMS_ARCSEC  1.0f


// ---------------------------------------------------------------------
//  Batterieanzeige (Feature 1)
//  Spannungsteiler: 100k + 100k zwischen Akku-Plus und GPIO34 (1:2).
//  Empfehlung Aufbau: Akku+ → 100k → GPIO34 → 100k → GND
// ---------------------------------------------------------------------
#define BATT_ENABLED         1
#define BATT_ADC_PIN         34
#define BATT_VOLTAGE_DIVIDER 2.0f
//
// ---------------------------------------------------------------------
//  Lokaler Umgebungssensor (Feature 4, BME280 via I2C)
//  Misst Innentemperatur, Feuchte und Luftdruck direkt am Geraet.
//  Benoetigt Bibliotheken: "Adafruit BME280 Library" +
//                          "Adafruit Unified Sensor" (Library Manager).
//
//  Verdrahtung:
//    BME280 VCC → 3V3      BME280 GND → GND
//    BME280 SDA → GPIO 21  BME280 SCL → GPIO 22
//    BME280 SDO → GND  (I2C-Adresse 0x76)
//           SDO → VCC  (I2C-Adresse 0x77, dann SENSOR_ADDR aendern)
// ---------------------------------------------------------------------
#define LOCAL_SENSOR_ENABLED  0       // 0 = deaktiviert, 1 = aktiv
#define LOCAL_SENSOR_SDA      21      // Standard-I2C-SDA (kein Konflikt mit Display)
#define LOCAL_SENSOR_SCL      22      // Standard-I2C-SCL (kein Konflikt mit Display)
#define LOCAL_SENSOR_ADDR     0x76    // SDO an GND → 0x76; SDO an VCC → 0x77

// ---------------------------------------------------------------------
//  OTA-Updates (Feature 3)
//  Hostname: mDNS-Name unter dem das Geraet im Netzwerk erreichbar ist
//            (z. B. flash via Arduino IDE → "astrooyink.local").
//  Password: leer lassen fuer kein Passwort (nicht empfohlen im
//            Heimnetz mit fremden Gaesten).
//  Fenster:  Millisekunden nach dem WLAN-Connect, in denen ein
//            Update aus der Arduino IDE eingespielt werden kann.
// ---------------------------------------------------------------------
#define OTA_HOSTNAME    "astrooyink"
#define OTA_PASSWORD    ""
#define OTA_WINDOW_MS   30000UL

#endif // ASTROOYINK_CONFIG_H
