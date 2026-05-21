# Claude Code — Projektkontext für AstroOyInk

Diese Datei wird von Claude Code beim Start automatisch gelesen und enthält
alles Wissen, das Claude für sinnvolle Mitarbeit am Projekt braucht.

> **Wenn du an diesem Projekt arbeitest und Claude Code Antworten gibt, die
> nicht zur Realität passen, ist meistens diese Datei nicht aktuell. Bitte
> aktualisiere sie, wenn sich Architektur oder Konventionen ändern.**

---

## Was ist AstroOyInk?

Ein E-Ink-Astro-Display (Waveshare 7.5″ 800×480, 3 Farben B/W/R) auf einem
ESP32, das **automatisch zwischen zwei Modi wechselt**:

- **MODE_WEATHER** — Astro-Wetter-Dashboard
  Wenn keine N.I.N.A.-Session läuft. Zeigt Cloud-Score der Nacht, Mond,
  Forecast etc. (Open-Meteo + Meeus-Mondberechnung lokal).

- **MODE_SESSION** — Live-Telemetrie deiner laufenden N.I.N.A.-Session
  Erkannt automatisch durch Erreichbarkeit der N.I.N.A. Advanced API +
  Indikator einer aktiven Sequenz. Zeigt aktuelles Target, Sub-Status,
  HFR, Guiding-RMS, Mount-Status, Sequenz-Fortschritt etc.

Beide Modi laufen auf **dem gleichen Gerät**. Der Modus wird zu Beginn
jedes Zyklus neu ermittelt (siehe `src/modes/mode_state_machine.cpp`).

> **Aktueller Status:** Roadmap-Punkte 1–3 abgeschlossen (Batterieanzeige,
> WiFiManager, OTA). Der WEATHER-Modus läuft komplett. Die State-Machine ist
> als Skelett vorbereitet, liefert aber immer `MODE_WEATHER`. Der SESSION-Modus
> wird nach Roadmap-Punkten 4–7 implementiert.

## Vorgeschichte

Das Projekt ist aus **AstroWeatherEPD** hervorgegangen. Der dortige v1.0-Code
ist mit kosmetischer Restrukturierung (in `src/`-Unterordner) übernommen
worden — keine funktionale Änderung. Wer den Verlauf vom Wetter-Display
zu AstroOyInk nachvollziehen will, findet die Iterationen in `CHANGELOG.md`.

## Hardware

| Komponente | Modell |
|---|---|
| MCU | ESP32 (WROOM-32, getestet mit Waveshare ESP32 e-Paper Driver Board) |
| Display | Waveshare 7.5″ E-Paper **V3** (B/W/R, 800×480), Treiber `GxEPD2_750c_Z08` |
| Stromversorgung | USB 5V (Refresh zieht kurz bis 200 mA) |

## Pinbelegung (`config.h`)

```
BUSY=25  RST=26  DC=27  CS=15  SCK=13  MOSI=14
```

## Build-System

**Arduino IDE 2.x — kein PlatformIO.**

Begründung: niedrige Einstiegshürde, einfacher Library Manager, gewollte
Beschränkung. Bibliotheken (alle über Library Manager):

| Lib | Version | Zweck |
|---|---|---|
| `GxEPD2`            | ≥1.5.x | E-Paper Treiber |
| `Adafruit GFX`      | ≥1.11  | Schriften, Primitive |
| `ArduinoJson`       | **7.x**  | API-JSON |
| `WiFiManager`       | ≥2.0 (tzapu) | Captive Portal |
| WiFi / HTTPClient / WiFiClientSecure / ArduinoOTA | im ESP32-Core | HTTPS + OTA |

ESP32-Boardpaket: Espressif, ≥2.0.14.

## Quellbaum

```
AstroOyInk/
├── AstroOyInk.ino                 # setup() + loop() + doCycle() mit Modus-Switch
├── config.h                       # alle nutzerseitigen Einstellungen
├── secrets.h.example              # Beispiel (echte secrets.h ist in .gitignore)
│
├── src/                           # automatisch von Arduino IDE rekursiv kompiliert
│   ├── common/                    # querschnittliche Logik, modusunabhaengig
│   │   ├── data_types.h           # POD: HourlyData, DailyData, AstroSummary,
│   │   │                          # MoonInfo, WeatherData (Sammelstruktur)
│   │   ├── time_utils.h/.cpp      # NTP via configTzTime, ISO-Parser
│   │   ├── astro_logic.h/.cpp     # Cloud-Score, Nachtfenster, Verdict
│   │   ├── moon_calc.h/.cpp       # Meeus-Mondberechnung
│   │   ├── battery.h/.cpp         # LiPo-Ladestand (ADC + Lookup-Tabelle)
│   │   ├── app_config.h/.cpp      # NVS-Konfiguration (Preferences)
│   │   └── ota_updater.h/.cpp     # ArduinoOTA-Fenster nach WLAN-Connect
│   │
│   ├── data_sources/              # External APIs / Datenquellen
│   │   └── weather_api.h/.cpp     # Open-Meteo Client
│   │   # (NACHHER: nina_api.h/.cpp fuer SESSION-Modus)
│   │
│   ├── modes/
│   │   └── mode_state_machine.h/.cpp   # detectCurrentMode() - aktuell Stub
│   │
│   └── rendering/
│       └── display_base.h/.cpp    # GxEPD2-Init, Page-Loop, manueller BUSY-Wait
│                                  # PLUS: aktuell das komplette Wetter-Dashboard.
│                                  # Wird spaeter aufgeteilt in:
│                                  #   - display_base.cpp (nur Basis-Helpers)
│                                  #   - view_weather.cpp (das Wetter-Dashboard)
│                                  #   - view_session.cpp (Session-Live-Anzeige)
│
├── tests/                         # Host-Tests (g++, kein ESP32 noetig)
│   ├── test_astro.cpp
│   ├── test_moon.cpp
│   ├── Arduino.h                  # Stub fuer Host-Kompilierung
│   └── build_and_run.sh
│
├── .github/
│   ├── workflows/test.yml         # CI: Host-Tests
│   ├── workflows/arduino-compile.yml   # CI: Arduino-Sketch baut
│   └── ISSUE_TEMPLATE/
│
├── README.md
├── INSTALL.md
├── CHANGELOG.md
├── CONTRIBUTING.md
├── SETUP_NEW_MACHINE.md
└── CLAUDE.md                      # diese Datei
```

### Wichtig zur `src/`-Struktur

Die Arduino IDE kompiliert **alle** `.cpp`-Dateien unter `src/` automatisch
rekursiv. Includes bleiben **flach** (z. B. `#include "data_types.h"`), weil
die IDE alle Unterordner zum Include-Path hinzufügt. Voraussetzung:
keine zwei Dateien gleichen Namens in verschiedenen Unterordnern.

## Architektur-Prinzipien

1. **Zwei-Modi-Modell.** Wetter und Session sind separate Code-Pfade.
   Sie teilen sich aber die Module in `src/common/` und `src/rendering/
   display_base`. Beim Hinzufügen von Features klar entscheiden: gehört
   das in Wetter, Session oder beide?
2. **Strikte Modulgrenzen.** `display_*` kennt kein WLAN. `*_api` kennt
   kein Display. `astro_logic`, `moon_calc` kennen weder das eine noch das
   andere.
3. **Static / Heap statt Stack.** `WeatherData` ist ~8 KB und passt nicht
   auf den 8 KB Stack des Arduino-Tasks. Im Sketch `static WeatherData wd;`,
   in `weather_api.cpp` `new WeatherData()`.
4. **Paged Rendering.** Der GxEPD2-Framebuffer ist auf `HEIGHT/4` (= 24 KB)
   begrenzt. Sonst sprengt das Display + WiFi-Stack den DRAM.
5. **3 Farben only.** Display kann Schwarz/Weiß/Rot. **Niemals Grün
   verwenden.** „Gute" Zustände werden durch Sterne, Outline-Boxen, fette
   Schrift signalisiert.
6. **Refresh-Dauer akzeptieren.** Voller 3-Color-Refresh dauert 25-30 s
   auf der V3. Nach dem GxEPD2-Page-Loop wartet `waitBusyManual()` aktiv
   auf BUSY=LOW, weil GxEPD2 intern einen 20-s-Timeout hat.
7. **Arduino-IDE-first.** Nicht PlatformIO einführen ohne expliziten
   User-Wunsch.

## Daten-Fluss pro Zyklus

```
1. displayInit()           → SPI + Panel-Init
2. loadConfig()            → NVS lesen (Preferences) oder config.h-Defaults
3. connectWithPortal()     → WiFiManager autoConnect + Captive Portal
3b. otaSetup() + otaWaitWindow(30s)  → ArduinoOTA-Fenster fuer Firmware-Update
4. timeSyncNTP(tz)         → configTzTime + 3 NTP-Server
5. detectCurrentMode()     → aktuell: immer MODE_WEATHER
6+7. switch(mode):
   - MODE_WEATHER:
       fetchWeather()            → Open-Meteo HTTPS-Client
       computeAstroSummary()     → Cloud-Score, Verdict, Warnungen
       computeMoon() x2          → "jetzt" + "Mitte der Nacht"
       renderDashboard()         → setFullWindow + firstPage/nextPage Loop
                                   + waitBusyManual()
   - MODE_SESSION (geplant):
       ninaProbe()               → /v2/api/equipment/camera/info
       ninaFetchSessionState()   → /v2/api/sequence/state, etc.
       renderSessionDashboard()
7. displaySleep() + disconnectWiFi()
8. esp_deep_sleep_start()  → bis zum naechsten Update
```

Im **DEBUG_MODE** (config.h) wird Schritt 8 übersprungen; `loop()`
ruft `doCycle()` alle 30 s erneut auf, der Serial Monitor bleibt verbunden.

## Astro-Bewertung — Formeln (WEATHER-Modus)

```
cloud_score = 0.5*cloudcover_low + 0.3*cloudcover_mid + 0.2*cloudcover_high

avg = Mittel über Stunden im Nachtfenster (sunset+2h … sunrise-2h)

verdict:
  avg <  20%  → GOOD
  avg <  45%  → MODERATE
  avg ≥  45%  → BAD

Penalty: Wind > 25 km/h ODER humidity > 90 % → eine Stufe schlechter
```

## Konventionen

- **Sprache:** alle Kommentare und Variablennamen auf Deutsch sind ok,
  Code-Identifier (Funktions-/Typnamen) in Englisch.
- **Umlaute im Code:** Vermeiden — die Standard-GFX-Fonts haben kein
  ä/ö/ü/ß. Statt „für" → „fuer". In Markdown-Doku können Umlaute bleiben.
- **Style:** 2 Leerzeichen Einrückung, K&R-Klammern, snake_case für
  lokale Variablen, camelCase für Funktionen, PascalCase für Typen.
- **Logging:** `Serial.printf("[MODUL] ...")` mit Prefix, z. B. `[WIFI]`,
  `[API]`, `[EPD]`, `[STEP 5b]`, `[NINA]`.
- **Geheimnisse:** Niemals echte WiFi-Credentials in `config.h` committen.
  Beim Veröffentlichen Platzhalter `DEIN_WLAN_SSID` zurück setzen.
  `secrets.h` liegt in `.gitignore`.

## Was häufig schief geht (Lessons learned)

| Symptom | Ursache | Fix |
|---|---|---|
| `'GxEPD2_750c_Z90c' not declared` | Klasse heißt `GxEPD2_750c_Z90` (ohne c). Für 800×480 BWR aber immer `Z08`. | `Z08` verwenden, auch bei V3 |
| `dram0_0_seg overflowed` | Volles 96 KB Framebuffer + WiFi → DRAM voll | Paged Mode: `<…, HEIGHT/4>` |
| `[API] hourly leer` trotz `payload bytes: 28534` | Stream-Parsing + Chunked Encoding | `http.getString()` benutzen |
| Uhr zeigt UTC statt MESZ | `configTime(0,0,…)` setzt TZ auf UTC | `configTzTime(POSIX_TZ, …)` |
| `Busy Timeout!` im Render | V3 braucht 25-30 s, GxEPD2 default 20 s | `waitBusyManual()` nach Page-Loop |
| Display zeigt nichts, Render < 1 s | Falsche Display-Klasse | `Z08` für 800×480 |
| Arduino-IDE findet `.cpp` in Unterordner nicht | Nur `src/` wird rekursiv kompiliert | Unter `src/` legen, nicht z. B. `modes/` direkt im Root |

## Roadmap

In dieser Reihenfolge geplant (Reihenfolge vom Nutzer gewählt):

1. ~~🔋 **Batterieanzeige** im Header (ADC-Spannungsteiler)~~ ✅
2. ~~📶 **WiFiManager** (Captive Portal statt hardcoded SSID)~~ ✅
3. ~~📤 **OTA-Updates** (ArduinoOTA)~~ ✅
4. 🌡️ **Lokaler Sensor** (BME280 / DHT22) für Tau-Berechnung am Standort
5. 🪐 **7Timer! Seeing & Transparenz** zusätzlich zu Open-Meteo
6. 🛰️ **ISS / Starlink Überflüge** für die Session-Vorschau
7. 📷 **Screenshot-Export** auf SD-Karte
8. 🔭 **SESSION-Modus**: N.I.N.A. Advanced API anzapfen (Punkt 8 = der grosse Brocken)

> **Workflow für Claude Code:** Pro Roadmap-Punkt erst Detailfragen mit
> dem Nutzer klären, *dann* implementieren. Nicht voreilig anfangen.

## Hinweise zur Arbeit mit Claude Code

- **Vor größeren Änderungen** immer kurz die Architektur in dieser Datei
  prüfen — speziell die „Architektur-Prinzipien".
- **Beim Debugging** zuerst `DEBUG_MODE=1` setzen, dann mit den `[STEP …]`-
  Logs lokalisieren, in welcher Phase es klemmt.
- **Tests:** Host-Tests in `tests/`. Mond- und Astro-Berechnungen
  validieren — laufen ohne ESP32-Hardware.
- **Library-Updates:** Wenn ArduinoJson 8 erscheint, brechen womöglich
  `JsonDocument` und die Filter-API. Vorher in `weather_api.cpp` testen.
- **SESSION-Modus-Vorbereitung:** Die ninaAPI hat eine REST-API auf Port
  1888 (`/v2/api/...`) UND einen WebSocket (`/v2/socket`). Wir wollen den
  REST-Weg, weil polling-basiert + simpler. Detection-Endpoints:
  - `GET /v2/api/equipment/camera/info` — Kamera connected?
  - `GET /v2/api/sequence/state` — Sequenz läuft?
  - `GET /v2/api/image-history/0` — letztes Bild (für „Aktivität seit X")
