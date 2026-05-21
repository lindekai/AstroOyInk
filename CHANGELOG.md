# Changelog

Alle relevanten Änderungen am Projekt.

## [Unreleased]

### Hinzugefügt
- 🔋 **Batterieanzeige im Header** — `src/common/battery.h/.cpp`
  LiPo-Lookup-Tabelle (8 Stützpunkte), 16× ADC-Oversampling GPIO34,
  USB-Erkennung (V > 4,3 V). Icon + %, rot bei < 20 %, USB-Label
  bei USB-Betrieb. Spannungsteiler 100k+100k in `config.h` konfigurierbar.
- 📶 **WiFiManager** — Captive Portal (tzapu/WiFiManager) bei fehlgeschlagener
  WLAN-Verbindung. Konfigurierbar: SSID+PW, Ort, Lat/Lon, Zeitzone,
  Refresh-Intervall, N.I.N.A. Host+Port. Einstellungen in NVS gespeichert
  (`src/common/app_config.h/.cpp`). AP-Name `AstroOyInk-XXXXXX`.
  E-Paper zeigt Verbindungsanleitung während Portal läuft.
- 📤 **OTA-Updates** — `src/common/ota_updater.h/.cpp`
  ArduinoOTA-Integration. Nach jedem WLAN-Connect wird ein 30-s-Fenster
  geöffnet, in dem ein Firmware-Update per Netzwerk aus der Arduino IDE
  eingespielt werden kann. Hostname `astrooyink` (erreichbar als
  `astrooyink.local` via mDNS). Optionales Passwort über `OTA_PASSWORD`
  in `config.h` konfigurierbar.
- 📄 `.gitignore` für .DS_Store, Build-Artefakte und `secrets.h`
- 👤 Autor (Kai Linde) und Credits (Dark Matters Discord) in allen Quelldateien

- 🛰️ **ISS-Überflüge** — `src/data_sources/iss_api.h/.cpp`
  open-notify.org HTTP-API, kein Key. Nächster ISS-Pass (innerhalb 24 h)
  wird rechts in der FAZIT-Zeile eingeblendet: „ISS: HH:MM  Xmin".
  Wenn die ISS gerade sichtbar ist: „ISS: JETZT! Xmin" in Rot.
  Bei API-Fehler: FAZIT-Zeile bleibt unverändert.
- 🪐 **7Timer! Seeing & Transparenz** — `src/data_sources/seeing_api.h/.cpp`
  Kostenloses HTTP-API, kein Key nötig. Liefert Seeing und atmosphärische
  Transparenz in 3-Stunden-Intervallen. Beide Kurven werden als rote
  Strich- bzw. Punktlinie in den Nacht-Wolken-Chart eingeblendet (normiert
  auf 0–100 %, 1=exzellent=unten, 8=schlecht=oben — konsistent mit
  Wolkenskala). Legende im Chart um 3. Zeile „Seeing / Transp" ergänzt.
  Bei API-Fehler: kein Dashboard-Blockier, Linien werden weggelassen.
- 🌡️ **Lokaler Sensor (BME280)** — `src/common/local_sensor.h/.cpp`
  I²C-Anbindung an GPIO 21/22 (Standard-ESP32-Pins, kein Konflikt mit
  Display oder Batterie-ADC). Liest Innentemperatur, Feuchte und Luftdruck.
  Anzeige als kompakte „IN: …"-Zeile am unteren Rand des AKTUELL-Blocks.
  Standardmäßig deaktiviert (`LOCAL_SENSOR_ENABLED 0`). I²C-Adresse
  0x76 / 0x77 per `LOCAL_SENSOR_ADDR` in `config.h` wählbar.
  Benötigt: „Adafruit BME280 Library" + „Adafruit Unified Sensor".

### Geplant (verbleibend)
- 🪐 Seeing- & Transparenz-Daten via 7Timer!

- 🛰️ ISS / Starlink-Überflüge
- 📷 Screenshot-Export auf SD-Karte
- 🔭 **SESSION-Modus** über N.I.N.A. Advanced API

---

## [0.1.0] – initial AstroOyInk

### Was ist neu gegenüber AstroWeatherEPD v1.0?

- 🌟 **Neues Projekt** unter eigenem Namen `AstroOyInk`
- 🏗️ **Restrukturierung in `src/` mit Unterordnern**
  (`common/`, `data_sources/`, `modes/`, `rendering/`)
- 🎛️ **State-Machine-Skelett** für den späteren Wechsel zwischen
  WEATHER- und SESSION-Modus (`mode_state_machine.h/.cpp`)
- 📝 **Erweiterte CLAUDE.md** mit dem 2-Modi-Konzept und Hinweisen
  zur N.I.N.A. Advanced API
- 🧪 Tests an die neue Pfadstruktur angepasst, weiterhin grün

### Funktional unverändert

Der WEATHER-Modus läuft 1:1 wie in AstroWeatherEPD v1.0. Keine
Verhaltensänderung im aktuellen Funktionsumfang.

---

## Vorgeschichte: AstroWeatherEPD

AstroOyInk geht aus dem Vorgänger-Projekt AstroWeatherEPD hervor.
Wichtige Meilensteine dort:

### AstroWeatherEPD 1.0.0 (lokaler Prototyp, nie öffentlich gepublished)

#### Features
- Stündliches Update von Open-Meteo (kein API-Key)
- Astro-Bewertung der Nacht (GOOD / MODERATE / BAD)
- Gewichteter Cloud-Score (Low 0.5 / Mid 0.3 / High 0.2)
- Echtes Nachtfenster (Sonnenuntergang +2h … Sonnenaufgang −2h)
- Mondberechnung lokal nach Meeus (Phase, Illumination, Höhe)
- Himmelsaufhellungsindex
- Wind- und Tau-Warnungen
- 5-Tage-Vorschau mit Nacht-Bewertung pro Tag
- 24h-Mini-Charts: Temperatur, Wind, Feuchte, Niederschlag
- Stundenwert-Tabelle
- Deep-Sleep zwischen Updates
- Debug-Modus mit Live-Loop alle 30 s

#### Fixes auf dem Weg
- `configTzTime()` statt `configTime()` für korrekte MESZ
- `http.getString()` statt Stream-Parsing wegen Chunked-Encoding
- `Z08`-Treiber für 800×480 BWR (statt fälschlich `Z90/Z90c`)
- `memset` auf Struct mit `String`-Member entfernt
- Manueller BUSY-Wait für langsame V3-Panels (>20 s Refresh)
- Paged Rendering `HEIGHT/4`, sonst DRAM-Overflow
- Layout-Tuning nach Foto-Feedback (Fonts, Spacing, Forecast-Block)
