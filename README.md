# AstroOyInk 🌌🔭

> **E-Ink Astro-Display für Astrofotografen.**
> Ein Gerät, zwei Modi: tagsüber Astro-Wetter-Dashboard, nachts Live-
> Telemetrie deiner N.I.N.A.-Aufnahmesession. Wechsel automatisch, je nach
> dem, ob N.I.N.A. im Netz erreichbar ist.

[![Host Tests](https://github.com/lindekai/AstroOyInk/actions/workflows/test.yml/badge.svg)](https://github.com/lindekai/AstroOyInk/actions/workflows/test.yml)
[![Arduino Build](https://github.com/lindekai/AstroOyInk/actions/workflows/arduino-compile.yml/badge.svg)](https://github.com/lindekai/AstroOyInk/actions/workflows/arduino-compile.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Warum „AstroOyInk“?

AstroOyInk entstand aus einer ungewöhnlich ehrlichen Beschreibung moderner Astrofotografie.

Der Name kombiniert den bekannten jiddischen Ausdruck Oy Vey — sinngemäß:

„Oh nein. Nicht schon wieder.“

…mit „Ink“ als Verweis auf die verwendete eInk/ePaper-Technologie.

Der Ausdruck „Oy Vey“ beschreibt dabei erstaunlich präzise den emotionalen Zustand vieler Astrofotografen:

> meist kurz nach erfolgreichem Polar Alignment, 
> während des Meridian Flips oder genau in dem Moment,
> 6h nach Sessionstart, wenn Du merkst, dass Du die Tauheizung vergessen hast,  
> in dem trotz perfekter Wettervorhersage Wolken aufziehen.

Zusammen steht AstroOyInk für die besondere Mischung aus Technik, Hoffnung, Schlafmangel und leichtem Wahnsinn, die Astrofotografie so einzigartig macht.

Oder einfacher gesagt:

**AstroOyInk**
Ein kleines ePaper-Astrodisplay für Menschen, die „klarer Himmel“ eher als theoretisches Konzept betrachten.

---

## Die zwei Modi

```
┌─────────────────────────────────────────────────────┐
│  EIN ESP32 + EIN E-Ink Display                      │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Modus „Wetter"           Modus „Session"           │
│  ┌──────────────┐         ┌──────────────┐          │
│  │ Astro-Wetter │  ←────→ │ Session-Live │          │
│  │ Mond, Wolken │ AUTO    │ Kamera, Mount│          │
│  │ Open-Meteo   │ DETECT  │ ninaAPI      │          │
│  └──────────────┘         └──────────────┘          │
│         ↑                        ↑                  │
│    Default-Modus         Aktiv wenn N.I.N.A.        │
│    immer wenn keine      im Netzwerk erreichbar     │
│    Session läuft         und eine Sequenz läuft     │
│                                                     │
└─────────────────────────────────────────────────────┘
```

**Aktueller Stand:** WEATHER-Modus und SESSION-Modus sind vollständig implementiert
(siehe Roadmap). Automatischer Wechsel je nach N.I.N.A.-Erreichbarkeit.

---

## Inhaltsverzeichnis

1. [Warum „AstroOyInk"?](#warum-astrooyink)
2. [Was es kann](#was-es-kann)
3. [Hardware](#hardware)
4. [Display-Layout im WEATHER-Modus](#display-layout-im-weather-modus)
5. [Installation](#installation)
6. [Projektstruktur](#projektstruktur)
7. [Konfiguration](#konfiguration)
8. [Tests](#tests)
9. [Roadmap](#roadmap)
10. [Vorgeschichte: AstroWeatherEPD](#vorgeschichte-astroweatherepd)
11. [Credits](#credits)
12. [Lizenz](#lizenz)

---

## Was es kann

### Heute (WEATHER-Modus)

- ✅ Stündliches Update von Open-Meteo (kein API-Key)
- 🌙 Mondberechnung lokal nach Meeus (Phase, Illumination, Höhe, Himmelsaufhellung)
- ⭐ Astro-Score aus gewichteter Wolkenbedeckung (Low 0.5 / Mid 0.3 / High 0.2)
- 🕘 Echtes Nachtfenster (Sonnenuntergang +2h … Sonnenaufgang −2h)
- ⚠️ Wind- und Tau-Warnungen (>25 km/h, >90 % Feuchte)
- 📊 Dashboard mit Nacht-Verlauf, Stundenwerten, 24h-Charts, 5-Tage-Vorschau
- 💤 Deep-Sleep zwischen Updates
- 🎨 3-Color-Design ohne Grün (Sterne + Outline statt grüner Ampelfarbe)
- 🔋 Batterieanzeige im Header (LiPo-Ladestand, USB-Erkennung, rot bei < 20 %)
- 📶 WiFiManager: Captive Portal bei fehlendem/falschem WLAN — alle Parameter (Ort, Lat/Lon, Zeitzone, Intervall, N.I.N.A.) ohne Re-Flash konfigurierbar
- 📤 OTA-Updates: 30-s-Flashfenster nach jedem WLAN-Connect — Firmware-Update aus der Arduino IDE ohne USB-Kabel
- 🌡️ Lokaler BME280-Sensor: Innentemperatur, Feuchte und Luftdruck direkt am Gerät als „IN:"-Zeile im Dashboard
- 🪐 7Timer! Seeing & Transparenz: astronomisches Seeing + Durchsicht als rote Kurven im Nacht-Wolken-Chart
- 🛰️ ISS-Überflüge: nächster Pass rechts in der FAZIT-Zeile (open-notify.org, kein Key)

### SESSION-Modus (N.I.N.A. Advanced API)

- 🔭 Automatischer Wechsel WEATHER → SESSION wenn N.I.N.A. läuft
- 📋 Sequenzfortschritt: Target i/n, Sub i/n, Restzeit, Status
- 🎯 Objekt-Höhe und Azimut (aus N.I.N.A.)
- 📷 Kamera: Name, Filter, Belichtungszeit, HFR
- 🔭 Guiding: RMS RA/Dec/Total, Warnungen ab 1,0″
- 🌐 Mount: Name, RA/Dec, Pier-Seite, Meridian-Flip-Countdown

## Hardware

| Komponente | Modell |
|---|---|
| MCU | ESP32 (Standard-DevKit oder Waveshare e-Paper Driver Board) |
| Display | Waveshare **7.5″ E-Paper V3** (B/W/R, 800×480) |
| Stromversorgung | USB 5 V (kurzfristig bis 200 mA) |
| Optional | 7×5″ Bilderrahmen für die Wand |

## Display-Layout im WEATHER-Modus

```
┌─── Saarmund ─── 01:18 ─── [████  ] 72%  Update 01:18 ──┐
│ AKTUELL          │ NACHT-BEWERTUNG    │ MOND            │
│  🌙  3.7°C       │  ┌──┐   GUT        │ 🌒 Zun. Sichel  │
│      6 km/h     │  │0%│  ★★★★★      │    25 % beleucht │
│      78 %       │  └──┘             │    42° u. Hor.   │
│      Tau 0.2 C  │  Nacht 22:17-03:54│    Himmel: dunkel│
├──────────────────┴────────────────────┴──────────────────┤
│ NACHT - WOLKEN VERLAUF (%)             Gesamt — Low      │
│ 100, 75, 50, 25, 0 ─────────────       Mid ··· High      │
├──────────────────────────────────────────────────────────┤
│ Stundenwerte Nacht  / Mini-Charts (24h)                  │
├──────────────────────────────────────────────────────────┤
│ Vorhersage   Mi  Do  Fr  Sa  So                          │
├──────────────────────────────────────────────────────────┤
│ FAZIT:  Klare Nacht erwartet — Teleskop bereitstellen!   │
└──────────────────────────────────────────────────────────┘
```

## Installation

Siehe **[INSTALL.md](INSTALL.md)** für die ausführliche Schritt-für-Schritt-Anleitung.

Kurzversion:
1. Arduino IDE 2.x installieren
2. ESP32-Boardpaket installieren (Version ≥2.0.14)
3. Bibliotheken: GxEPD2, Adafruit GFX, **ArduinoJson 7.x**
4. `config.h` editieren (WLAN, Lat/Lon)
5. Board „ESP32 Dev Module" auswählen
6. Flashen → fertig

## Projektstruktur

```
AstroOyInk/
├── AstroOyInk.ino                 # Hauptsketch + State-Machine-Hook
├── config.h                       # WLAN, Standort, Pins, Schwellwerte
│
├── src/                           # automatisch rekursiv kompiliert
│   ├── common/                    # querschnittlich
│   │   ├── data_types.h
│   │   ├── time_utils.h/.cpp
│   │   ├── astro_logic.h/.cpp
│   │   ├── moon_calc.h/.cpp
│   │   ├── battery.h/.cpp         # Batteriemessung (Roadmap #1)
│   │   ├── app_config.h/.cpp      # NVS-Konfiguration (Roadmap #2)
│   │   ├── ota_updater.h/.cpp     # OTA-Flashfenster (Roadmap #3)
│   │   └── local_sensor.h/.cpp   # BME280-Innensensor (Roadmap #4)
│   │
│   ├── data_sources/
│   │   ├── weather_api.h/.cpp     # Open-Meteo
│   │   ├── seeing_api.h/.cpp      # 7Timer! Seeing & Transparenz (Roadmap #5)
│   │   └── iss_api.h/.cpp         # ISS-Überflüge open-notify (Roadmap #6)
│   │
│   ├── modes/
│   │   └── mode_state_machine.h/.cpp  # WEATHER ↔ SESSION (Skelett)
│   │
│   └── rendering/
│       └── display_base.h/.cpp    # GxEPD2-Init + Wetter-Render
│
├── tests/                         # Host-Tests (g++)
├── .github/                       # CI + Issue-Templates
├── CLAUDE.md                      # Projektkontext für Claude Code
├── INSTALL.md
├── CHANGELOG.md
├── CONTRIBUTING.md
└── SETUP_NEW_MACHINE.md
```

## Konfiguration

Alles in `config.h`:

```cpp
#define WIFI_SSID       "MeinWLAN"
#define WIFI_PASSWORD   "geheim"

#define LOCATION_NAME   "Berlin"
#define LOCATION_LAT    52.5200f
#define LOCATION_LON    13.4050f

#define TIMEZONE_POSIX  "CET-1CEST,M3.5.0,M10.5.0/3"
#define REFRESH_INTERVAL_S   3600UL
#define DEBUG_MODE      1

// Batterieanzeige (optionaler LiPo-Akku)
#define BATT_ENABLED         1       // 0 = deaktiviert
#define BATT_ADC_PIN         34      // GPIO34, input-only
#define BATT_VOLTAGE_DIVIDER 2.0f    // 100k + 100k Spannungsteiler
```

## Tests

```bash
cd tests
./build_and_run.sh
```

Validiert Astro-Bewertung (8 Szenarien) und Mondberechnung gegen
bekannte Mondereignisse 2026.

## Roadmap

In dieser Reihenfolge geplant:

1. ~~🔋 Batterieanzeige im Header (ADC)~~ ✅
2. ~~📶 WiFiManager (Captive Portal)~~ ✅
3. ~~📤 OTA-Updates~~ ✅
4. ~~🌡️ Lokaler Sensor (BME280)~~ ✅
5. ~~🪐 7Timer! Seeing & Transparenz~~ ✅
6. ~~🛰️ ISS-Überflüge~~ ✅
7. ~~📷 Screenshot-Export auf SD~~ *(entfällt)*
8. ~~🔭 **SESSION-Modus** (N.I.N.A. Advanced API)~~ ✅

Siehe [CHANGELOG.md](CHANGELOG.md) für die Versionshistorie.

## Vorgeschichte: AstroWeatherEPD

AstroOyInk geht aus dem Vorgängerprojekt **AstroWeatherEPD v1.0** hervor.
Dort wurde der WEATHER-Modus ursprünglich entwickelt. Wenn dich der Weg
dorthin interessiert: siehe `CHANGELOG.md`. Das frühere Repo wurde nicht
veröffentlicht — AstroOyInk ist die offizielle Linie.

## Credits

**Autor:** Kai Linde  
**Community:** Dark Matters Discord

---

## Lizenz

MIT — siehe [LICENSE](LICENSE).
Open-Meteo-Daten unterliegen den Bedingungen von <https://open-meteo.com>.
