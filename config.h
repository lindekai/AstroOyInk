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

// =====================================================================
//  GEPLANTE BLÖCKE  (kommen mit späteren Features, vorerst auskommentiert)
// =====================================================================
//
// // ---------------------------------------------------------------------
// //  N.I.N.A.-Anbindung (Session-Modus, kommt nach Roadmap-Feature 7)
// //  Du kannst entweder eine IP-Adresse oder einen Hostnamen eintragen.
// //  Hostnamen funktionieren in den meisten Heimnetzen via mDNS.
// // ---------------------------------------------------------------------
// #define NINA_HOST       "nina-pc.local"   // oder z.B. "192.168.1.42"
// #define NINA_PORT       1888
// #define NINA_PROBE_INTERVAL_S    300      // alle 5 min im Wetter-Modus pingen
//
// ---------------------------------------------------------------------
//  Batterieanzeige (Feature 1)
//  Spannungsteiler: 100k + 100k zwischen Akku-Plus und GPIO34 (1:2).
//  Empfehlung Aufbau: Akku+ → 100k → GPIO34 → 100k → GND
// ---------------------------------------------------------------------
#define BATT_ENABLED         1
#define BATT_ADC_PIN         34
#define BATT_VOLTAGE_DIVIDER 2.0f
//
// // ---------------------------------------------------------------------
// //  Lokaler Umgebungssensor (Feature 4)
// // ---------------------------------------------------------------------
// #define LOCAL_SENSOR_ENABLED 0
// #define LOCAL_SENSOR_TYPE    "BME280"   // oder "DHT22"

#endif // ASTROOYINK_CONFIG_H
