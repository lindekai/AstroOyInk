# Changelog

Alle relevanten Änderungen am Projekt.

## [Unreleased]

### Geplant (in dieser Reihenfolge)
- 🔋 Batterieanzeige im Header (ADC)
- 📶 WiFiManager / Captive-Portal-Konfiguration
- 📤 OTA-Updates
- 🌡️ Lokaler Sensor (BME280 / DHT22)
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
