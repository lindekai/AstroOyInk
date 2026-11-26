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

> **Aktueller Status:** Release-Kandidat `v1.0.0-rc1`. Roadmap-Punkte
> 1–5 und 8 abgeschlossen, Punkt 6 (ISS) pausiert, weil die Datenquelle
> abgeschaltet wurde. Punkt 7 (SD-Screenshot) wurde bewusst gestrichen.
> WEATHER-Modus und SESSION-Modus laufen vollständig.
> Die State-Machine erkennt N.I.N.A. + laufende Sequenz und wechselt
> automatisch zwischen den Modi.

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
| Stromversorgung | USB 5V **oder** Akku-Referenzaufbau wie DoorSign: TC4056 → LiPo 3500 mAh (EEMB LP104567) → TPS62827-Buck (Iq ~4 µA) → 3,3 V auf **3V3-Pin** (nicht VIN), 1500 µF Puffer-Elko |
| Akkuanzeige | Teiler 100k/100k Akku+ → GPIO34; optional USB-Sense 100k/100k an GPIO35 (`BATT_USB_SENSE_PIN`) |

## Pinbelegung (`config.h`)

```
BUSY=25  RST=26  DC=27  CS=15  SCK=13  MOSI=14
```

Das sind die Defaults (passen zum Waveshare ESP32 e-Paper Driver Board).
Fuer den Aufbau auf **einem** Steckbrett (`docs/images/breadboard.svg`)
liegen alle Signale auf einer DevKitC-Stiftleiste, dort gilt abweichend:
`CS=33  BUSY=39  SDA=32  SCL=25` (in `config.h` eintragen). Schaltplan und
Steckbrett-Bild erzeugt `tools/make_wiring_svgs.py` — bei Pinaenderungen
dort nachziehen und neu erzeugen.

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
│   │   ├── ota_updater.h/.cpp     # ArduinoOTA-Fenster nach WLAN-Connect
│   │   ├── local_sensor.h/.cpp    # BME280 I2C, Innentemperatur/Feuchte/Druck
│   │   └── scan_window.h          # Zeitfenster der Auto-Discovery (Host-Test)
│   │
│   ├── data_sources/              # External APIs / Datenquellen
│   │   ├── weather_api.h/.cpp     # Open-Meteo HTTPS-Client
│   │   ├── seeing_api.h/.cpp      # 7Timer! HTTP-Client, Seeing & Transparenz
│   │   ├── iss_api.h/.cpp         # open-notify.org, ISS-Passes (ISS_ENABLED 0:
│   │   │                          # Dienst liefert 404, Abruf aus)
│   │   ├── nina_api.h/.cpp        # N.I.N.A. Advanced API v2 (SESSION-Modus)
│   │   ├── nina_discovery.h/.cpp  # Auto-Discovery: /24-Scan nach Port 1888
│   │   └── nina_sequence.h/.cpp   # sequence/state-Baum auswerten (Host-Test)
│   │
│   ├── modes/
│   │   └── mode_state_machine.h/.cpp   # detectCurrentMode() - aktuell Stub
│   │
│   └── rendering/
│       └── display_base.h/.cpp    # GxEPD2-Init, Page-Loop, manueller BUSY-Wait
│                                  # PLUS: Wetter-Dashboard + Session-Dashboard.
│                                  # Koennte spaeter aufgeteilt werden in:
│                                  #   - display_base.cpp (nur Basis-Helpers)
│                                  #   - view_weather.cpp (Wetter-Dashboard)
│                                  #   - view_session.cpp (Session-Dashboard)
│
├── tests/                         # Host-Tests (g++, kein ESP32 noetig)
│   ├── test_astro.cpp
│   ├── test_moon.cpp
│   ├── test_scan_window.cpp
│   ├── test_nina_sequence.cpp     # braucht ArduinoJson (ARDUINOJSON_DIR)
│   ├── fixtures/                  # echte sequence/state-Antworten
│   ├── Arduino.h                  # Stub fuer Host-Kompilierung
│   └── build_and_run.sh
│
├── tools/
│   ├── make_readme_svgs.py        # README-Grafiken (stilisiert, NICHT pixelgenau)
│   ├── make_gehaeuse_mockups.py   # Gehaeuse-Hero + Schnittzeichnungen (OpenSCAD)
│   ├── make_wiring_svgs.py        # Schaltplan + Steckbrett-Bild (Pins aus config.h
│   │                              # von Hand gespiegelt – bei Pinaenderung neu erzeugen)
│   ├── make_bohrschablone.py      # A4-PDF 1:1 fuers Wandgehaeuse (Masse von Hand aus
│   │                              # gehaeuse_wand.scad gespiegelt – bei Aenderung neu erzeugen)
│   └── preview/                   # Pixelgenaue Host-Vorschau des Displays
│       ├── build_preview.sh       # → tools/preview/out/*.png
│       ├── preview_main.cpp       # Beispieldaten fuer alle Ansichten
│       └── stubs/                 # Arduino/Print/SPI/GxEPD2-Ersatz
│
├── hardware/gehaeuse/             # 3D-Druck-Gehaeuse (OpenSCAD, ohne Kleber)
│   ├── common.scad                # Bauteilmasse (nachgemessen), Helfer, Rastnasen
│   ├── gehaeuse_tisch.scad        # Sockel + Bodenplatte, Display im 15°-Schlitz
│   ├── gehaeuse_wand.scad         # Frontrahmen + Rueckdeckel, Schluesselloecher
│   ├── export_stl.sh              # → stl/*.stl
│   ├── pruefe_kollision.sh        # Tisch: alle Teile paarweise schneiden, vor jedem Druck
│   └── README.md                  # Teileliste, Druck, Montage
│
├── docs/images/                   # README-Grafiken + Vorschau-PNGs,
│                                  # schaltplan.svg, breadboard.svg
│                                  # (display_*_preview.png aus tools/preview)
│
├── README.md
├── INSTALL.md
├── CHANGELOG.md
├── CONTRIBUTING.md
└── CLAUDE.md                      # diese Datei
```

### Wichtig zur `src/`-Struktur

Die Arduino IDE kompiliert **alle** `.cpp`-Dateien unter `src/` automatisch
rekursiv, nimmt die Unterordner aber **nicht** in den Include-Pfad auf.
Projekt-Includes sind deshalb **relativ zur einbindenden Datei**:

- im Sketch: `#include "src/common/data_types.h"`
- gleicher Ordner: `#include "time_utils.h"`
- anderer Unterordner: `#include "../common/time_utils.h"`
- `config.h` aus `src/<x>/`: `#include "../../config.h"`

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
3b. otaSetup() + otaWaitWindow(30s)  → ArduinoOTA-Fenster, NUR beim Kaltstart
                                        (esp_reset_reason != DEEPSLEEP) oder DEBUG_MODE
4. timeSyncNTP(tz)         → configTzTime + 3 NTP-Server
5. resolveNinaHost()       → fester Portal-Host ODER Auto-Discovery:
                              gemerkte Adresse (NVS "nha") pruefen, sonst
                              /24-Scan — nur sunset-1h … sunrise+1h
                              (Sonnenzeiten aus letztem Wetterabruf, RTC-RAM)
   detectCurrentMode()     → MODE_SESSION wenn N.I.N.A. erreichbar + Sequenz laeuft
6+7. switch(mode):
   - MODE_WEATHER:
       fetchWeather/Seeing()     → Open-Meteo (7 Tage), 7Timer!
                                   (ISS nur mit ISS_ENABLED=1)
       disconnectWiFi()          → WLAN aus VOR dem 30-s-Refresh (Akku)
       computeAstroSummary()     → Cloud-Score, Verdict, Warnungen
       computeMoon() x2          → "jetzt" + "Mitte der Nacht"
       computeMoonTrack()        → Mondhoehe je Nachtstunde (Stundentabelle)
       renderDashboard()         → setFullWindow + firstPage/nextPage Loop
                                   + waitBusyManual()
   - MODE_SESSION:
       ninaFetchAll()            → Sequenz, Kamera, Guider, Mount
       disconnectWiFi()
       computeMoon(jetzt)        → Mond-Zeile (lokal, kein Netz)
       renderSessionDashboard()  → + Sonnenaufgang aus RTC (sunriseTodS)
7. displaySleep() + disconnectWiFi()
8. esp_deep_sleep_start()  → WEATHER: bis zur vollen Stunde,
                              SESSION: SESSION_REFRESH_INTERVAL_S (5 min)
                              Kein Wach-Bleiben mehr im SESSION-Modus (Akku).
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
| `data_types.h: No such file or directory` | Flacher Include, `src/`-Unterordner sind nicht im Include-Pfad | Relativ includen (siehe „Wichtig zur `src/`-Struktur") |
| `Textbereich überschreitet den verfügbaren Platz` | ESP32-Core 3.x: Firmware ~1,34 MB > Default-Partition 1,25 MB | Partition Scheme „Minimal SPIFFS (1.9MB APP with OTA)" |

## Roadmap

In dieser Reihenfolge geplant (Reihenfolge vom Nutzer gewählt):

1. ~~🔋 **Batterieanzeige** im Header (ADC-Spannungsteiler)~~ ✅
2. ~~📶 **WiFiManager** (Captive Portal statt hardcoded SSID)~~ ✅
3. ~~📤 **OTA-Updates** (ArduinoOTA)~~ ✅
4. ~~🌡️ **Lokaler Sensor** (BME280, I2C) — Innen-Temp/Feuchte/Druck~~ ✅
5. ~~🪐 **7Timer! Seeing & Transparenz** — rote Kurven im Nacht-Chart~~ ✅
6. 🛰️ **ISS-Überflüge** — ⏸️ pausiert: open-notify hat `iss-pass.json`
   abgeschaltet (HTTP 404). Code bleibt, `ISS_ENABLED 0` in `iss_api.h`.
7. ~~📷 **Screenshot-Export** auf SD-Karte~~ *(entfällt — bewusst gestrichen)*
8. ~~🔭 **SESSION-Modus**: N.I.N.A. Advanced API~~ ✅

> **Workflow für Claude Code:** Pro Roadmap-Punkt erst Detailfragen mit
> dem Nutzer klären, *dann* implementieren. Nicht voreilig anfangen.

## Hinweise zur Arbeit mit Claude Code

- **Vor größeren Änderungen** immer kurz die Architektur in dieser Datei
  prüfen — speziell die „Architektur-Prinzipien".
- **Beim Debugging** zuerst `DEBUG_MODE=1` setzen, dann mit den `[STEP …]`-
  Logs lokalisieren, in welcher Phase es klemmt.
- **Layout-Änderungen:** Erst mit `tools/preview/build_preview.sh` rendern
  und das PNG prüfen, dann flashen. Die Vorschau kompiliert den
  unveränderten `display_base.cpp` mit echter Adafruit GFX + Fonts; nur
  GxEPD2 ist durch eine Leinwand ersetzt (gleiche Farblogik). Die
  README-SVGs sind dagegen nur Mockups. Mit `--time UNIXZEIT` rendert
  die Vorschau einen festen Zeitpunkt (z. B. eine Nacht mit hohem Mond).
- **Text auf dem Display:** Grad-Zeichen als Backtick schreiben
  (`"12`C"`), die Helfer zeichnen daraus `°`. Zeilenabstand bei 9pt
  mindestens 20 px. Breite mit `textWidth()` prüfen, bei Platzmangel
  `fitFont()` statt Text über den Rand laufen lassen.
- **Tests:** Host-Tests in `tests/`. Mond- und Astro-Berechnungen
  validieren — laufen ohne ESP32-Hardware.
- **Library-Updates:** Wenn ArduinoJson 8 erscheint, brechen womöglich
  `JsonDocument` und die Filter-API. Vorher in `weather_api.cpp` testen.
- **SESSION-Modus-Vorbereitung:** Die ninaAPI hat eine REST-API auf Port
  1888 (`/v2/api/...`) UND einen WebSocket (`/v2/socket`). Wir wollen den
  REST-Weg, weil polling-basiert + simpler. Detection-Endpoints:
  - `GET /v2/api/equipment/camera/info` — Kamera connected?
  - `GET /v2/api/version` — Erreichbarkeit / Discovery-Bestätigung
  - `GET /v2/api/sequence/state` — Sequenz als Baum, läuft = ein Knoten
    mit `Status: "RUNNING"` (Auswertung in `nina_sequence.cpp`)
  - `GET /v2/api/image-history?index=0` — letztes Bild (Response ist Liste)

  Getestet gegen Advanced API **2.2.15.3**. Dort liefern
  `/v2/api/application/version`, `/v2/api/sequence` und
  `/v2/api/image-history/0` **404** — nicht verwenden.
