# Installationsanleitung — AstroOyInk

Diese Anleitung führt Schritt für Schritt durch den kompletten Aufbau —
auch wenn du **noch nie etwas mit Arduino oder ESP32 gemacht hast**.

> **Geschätzte Dauer:** 30–45 Minuten beim ersten Mal.

---

## Inhaltsverzeichnis

1. [Was du brauchst](#1-was-du-brauchst)
2. [Verkabelung](#2-verkabelung)
3. [Arduino IDE installieren](#3-arduino-ide-installieren)
4. [ESP32-Board-Paket installieren](#4-esp32-board-paket-installieren)
5. [Bibliotheken installieren](#5-bibliotheken-installieren)
6. [Projektdateien öffnen](#6-projektdateien-öffnen)
7. [`config.h` anpassen](#7-configh-anpassen)
8. [Board und Port auswählen](#8-board-und-port-auswählen)
9. [Kompilieren und Flashen](#9-kompilieren-und-flashen)
10. [Erstes Startverhalten](#10-erstes-startverhalten)
11. [Fehlerbehebung](#11-fehlerbehebung)

---

## 1. Was du brauchst

### Hardware

- ESP32 Entwicklerboard
  *(z. B. ESP32-WROOM-32 DevKit V1, Lolin D32, oder das **Waveshare ESP32
  e-Paper Driver Board** – letzteres ist die einfachste Wahl, weil
  Display-Pins und Spannungsregler bereits auf der Platine sitzen)*
- Waveshare **7.5″ E-Paper (B)**, 800 × 480, 3-Farben (Schwarz/Weiß/Rot)
- Verbindungsleitung (HAT-Kabel ist im Waveshare-Lieferumfang dabei)
- USB-Kabel zum Programmieren (meist Micro-USB oder USB-C, **kein reines
  Ladekabel!** — es muss Datenübertragung können)
- Optional: 7 × 5″ Bilderrahmen für die schöne Optik

### Software

- Computer mit Windows / macOS / Linux
- Internetverbindung (für Download und später für die Wetterdaten)
- ~500 MB freier Speicher

---

## 2. Verkabelung

**Beim Waveshare ESP32 e-Paper Driver Board** musst du nichts verkabeln —
das Display wird einfach mit dem 24-Pin-Flachbandkabel angesteckt.
Spring direkt zu Schritt 3.

**Bei einem Standard-ESP32-DevKit** verbindest du wie folgt:

| E-Paper Pin | ESP32 GPIO | Bemerkung |
|---|---|---|
| `VCC`  | `3V3` | **NICHT 5V!** |
| `GND`  | `GND` | |
| `DIN`  | `GPIO 14` | MOSI (in `config.h` als `EPD_PIN_MOSI`) |
| `CLK`  | `GPIO 13` | SCK  (`EPD_PIN_SCK`) |
| `CS`   | `GPIO 15` | Chip-Select (`EPD_PIN_CS`) |
| `DC`   | `GPIO 27` | Data/Command (`EPD_PIN_DC`) |
| `RST`  | `GPIO 26` | Reset (`EPD_PIN_RST`) |
| `BUSY` | `GPIO 25` | Busy-Signal (`EPD_PIN_BUSY`) |

> Du kannst alle GPIOs frei wählen — sie sind in `config.h` konfigurierbar.
> Vermeide nur die **Strapping-Pins** GPIO 0, 2, 12, 15 (15 ist OK als
> CS, weil er nur HIGH-aktiv kurz nach dem Boot ist) und Pins, die mit
> Flash/PSRAM belegt sind (6–11).

### Optional: Batterieanzeige

Wenn du einen LiPo-Akku anschließen und den Ladestand anzeigen willst,
baue einen Spannungsteiler zwischen Akku-Plus und GPIO34:

```
Akku+ ── 100 kΩ ── GPIO34 ── 100 kΩ ── GND
```

| Verbindung | Bemerkung |
|---|---|
| Akku+ → 100 kΩ → GPIO34 | Spannungsteiler-Mitte an GPIO34 |
| GPIO34 → 100 kΩ → GND | Spannungsteiler-Ende |
| Akku+ direkt an ESP32-VBAT / 3V3-Ladepin | je nach deinem Lademodul |

Danach in `config.h` aktivieren:

```cpp
#define BATT_ENABLED         1
#define BATT_ADC_PIN         34
#define BATT_VOLTAGE_DIVIDER 2.0f
```

Ohne Akku: `BATT_ENABLED 0` lassen — dann wird kein Widget gezeichnet.

---

## 3. Arduino IDE installieren

1. Öffne <https://www.arduino.cc/en/software>
2. Lade **Arduino IDE 2.x** für dein Betriebssystem herunter.
3. Installation starten und den Voreinstellungen folgen.
4. IDE öffnen — sie sollte ein leeres `sketch_*.ino` zeigen.

---

## 4. ESP32-Board-Paket installieren

Die Arduino IDE kennt von Haus aus keinen ESP32. Wir fügen ihn hinzu:

1. **Datei → Voreinstellungen** (macOS: *Arduino IDE → Settings*).
2. Im Feld **„Zusätzliche Boardverwalter-URLs"** diesen Eintrag einfügen:

   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

   Falls schon andere URLs dort stehen, mit Komma trennen.
3. **OK** klicken.
4. **Werkzeuge → Board → Boardverwalter…**
5. Im Suchfeld `esp32` eintippen.
6. Eintrag **„esp32 by Espressif Systems"** auswählen → **Installieren**
   (Version **2.0.14 oder neuer** wählen).
7. Warte, bis die Installation fertig ist (kann ~5 Minuten dauern).

---

## 5. Bibliotheken installieren

**Werkzeuge → Bibliotheken verwalten** öffnen und die folgenden vier
Bibliotheken installieren (Suchbegriff → Installieren):

| Suche nach | Autor | Version |
|---|---|---|
| `GxEPD2`        | Jean-Marc Zingg  | aktuelle (≥ 1.5.x) |
| `Adafruit GFX`  | Adafruit         | aktuelle (≥ 1.11.x) |
| `ArduinoJson`   | Benoit Blanchon  | **7.x** (wichtig!) |
| `Adafruit BusIO` | Adafruit        | (wird meist als Abhängigkeit von GFX automatisch vorgeschlagen → annehmen) |

> **WiFi**, **HTTPClient** und **WiFiClientSecure** sind bereits
> Bestandteil des ESP32-Cores und müssen nicht separat installiert werden.

---

## 6. Projektdateien öffnen

1. Lade das Projekt herunter (ZIP von GitHub oder `git clone`).
2. Stelle sicher, dass der **Ordnername** mit dem
   `.ino`-Dateinamen übereinstimmt — also:

   ```
   AstroOyInk/
     ├── AstroOyInk.ino    ← gleicher Name wie Ordner!
     ├── config.h
     ├── data_types.h
     ├── ...
   ```

3. Doppelklick auf `AstroOyInk.ino`.
4. Die Arduino IDE öffnet **alle** `.ino`/`.h`/`.cpp`-Dateien des Ordners
   automatisch in Tabs.

---

## 7. `config.h` anpassen

Im Tab **`config.h`** die folgenden Werte eintragen:

```cpp
#define WIFI_SSID       "MeinWLAN"
#define WIFI_PASSWORD   "meinpasswort123"

#define LOCATION_NAME   "Berlin"          // Anzeigename
#define LOCATION_LAT    52.5200f          // Breitengrad
#define LOCATION_LON    13.4050f          // Längengrad
```

> **Koordinaten finden:** auf <https://www.google.com/maps> einen Rechtsklick
> an deinen Standort machen — die ersten Zahlen sind Lat / Lon.

Falls du einen **anderen Pin** für das Display verwendest, hier ändern:

```cpp
#define EPD_PIN_BUSY    25
#define EPD_PIN_RST     26
// ...
```

> **Niemals echte WLAN-Daten in ein öffentliches Repo committen!**
> Siehe Sicherheits-Hinweis im README.

---

## 8. Board und Port auswählen

1. **Werkzeuge → Board → esp32** → Wähle dein Board:
   - Standard-DevKit → `ESP32 Dev Module`
   - Waveshare Driver Board → `ESP32 Dev Module`
   - Lolin D32 → `WEMOS LOLIN32`
2. **Werkzeuge → Port** → den passenden COM-Port wählen
   *(Windows: `COM3` o. ä.; macOS: `/dev/cu.usbserial-…`;
   Linux: `/dev/ttyUSB0`).*
3. Empfohlene weitere Einstellungen:
   - **Upload Speed:** `921600`
   - **Flash Frequency:** `80 MHz`
   - **Partition Scheme:** `Default 4MB with spiffs`
   - **PSRAM:** *Disabled* (ein Standard-WROOM hat kein PSRAM; bei
     einem WROVER kann es *Enabled* bleiben)

---

## 9. Kompilieren und Flashen

1. Klick auf das **Häkchen-Symbol** oben links (✓) → **Überprüfen**.
   Beim ersten Mal dauert es ~1–2 Minuten. Am Ende sollte unten stehen:

   ```
   Sketch uses XXX bytes (XX%) of program storage space.
   Global variables use YYY bytes (YY%) of dynamic memory.
   ```

2. Falls **„BOOT" / „EN"-Tasten** auf deinem Board sind: ESP32 in den
   Flash-Modus bringen, indem du **BOOT gedrückt hältst** und kurz
   **EN/RST** drückst (manche Boards machen das automatisch).
3. Klick auf das **Pfeil-Symbol** (→) → **Hochladen**.
4. Es erscheint:

   ```
   Connecting....__
   Writing at 0x00010000...
   ...
   Hash of data verified.
   Leaving... Hard resetting via RTS pin...
   ```

5. Fertig — der ESP32 startet automatisch neu.

---

## 10. Erstes Startverhalten

Direkt nach dem Hochladen passiert Folgendes:

1. **Seriellen Monitor** öffnen (Lupe oben rechts oder `Strg/Cmd+Shift+M`),
   Baudrate auf **115200** stellen.
2. Du siehst Ausgaben wie:

   ```
   === AstroOyInk Boot #1 ===
   [WIFI] Connecting to MeinWLAN .....
   [WIFI] OK, IP: 192.168.1.42
   [TIME] NTP sync ok: 1745247600
   [API] URL: https://api.open-meteo.com/v1/forecast?latitude=...
   [HTTP] payload bytes: 28534
   [API] OK: 120 hourly, 5 daily
   [RENDER] Dashboard …
   [SLEEP] going to sleep for 3500 s ...
   ```

3. Während `[RENDER] Dashboard …` brummt das Display ~15–25 Sekunden
   und blinkt mehrfach (Schwarz → Rot → Weiß-Phase). Das ist **normal**
   bei 3-Color-E-Paper.
4. Anschließend siehst du das fertige Dashboard.
5. Die LED auf dem Board geht aus (Deep-Sleep), das Display behält die
   Anzeige stromlos bei.
6. Beim **nächsten Aufwachen** (planmäßig zur nächsten vollen Stunde)
   wiederholt sich der Vorgang.

### Was passiert, wenn etwas fehlschlägt?

- **Kein WLAN beim Erststart** → Fallback-Bildschirm „Erststart / Fehler —
  Kein WLAN", danach Schlafen, in 1 h erneuter Versuch.
- **Kein WLAN beim x-ten Start** → Anzeige bleibt stehen (E-Ink behält
  letztes Bild), Logs zeigen den Fehler, Gerät versucht es in 1 h erneut.
- **Open-Meteo erreichbar, aber JSON-Fehler** → wird im Log gemeldet,
  Anzeige bleibt stehen.

---

## 11. Fehlerbehebung

| Symptom | Mögliche Ursache & Lösung |
|---|---|
| Beim Hochladen: `A fatal error occurred: Failed to connect to ESP32` | BOOT-Taste während des Uploads gedrückt halten, oder USB-Kabel/Port wechseln (manche Kabel sind reine Ladekabel ohne Daten). |
| Display bleibt **komplett weiß** | Pinbelegung in `config.h` prüfen, VCC darf **NICHT 5 V** sein — nur 3.3 V. |
| Display zeigt **Streifen / Geisterbild** | Einmal komplettes Refresh abwarten (15–25 s). Bei dauerhaftem Geisterbild: Display 30 s vom Strom trennen, danach erneut starten. |
| Serielle Ausgabe: `[HTTP] GET failed: -1` | Open-Meteo nicht erreichbar (DNS-Problem? WLAN ohne Internet?). Versuche die URL aus dem Log im Browser. |
| Serielle Ausgabe: `[API] JSON parse error: NoMemory` | ArduinoJson **6.x** statt **7.x** installiert. Im Library Manager auf 7.x updaten. |
| Anzeige steht **stundenlang still**, kein Update | Im seriellen Monitor prüfen (USB anschließen → RST drücken). Gerät kommt aus Deep-Sleep nur über internen Timer; bei stark verbrauchter Batterie ggf. neu laden. |
| Sommer/Winter-Zeit stimmt nicht | `TIMEZONE_POSIX` in `config.h` korrekt gesetzt? Default `CET-1CEST,M3.5.0,M10.5.0/3` ist für Mitteleuropa korrekt. |
| Kompilierfehler `'GxEPD2_750c_Z08' was not declared` | GxEPD2-Bibliothek auf ≥ 1.5.x aktualisieren. Für 800×480 B/W/R ist `Z08` korrekt (in `src/rendering/display_base.cpp`). |
| Kompilierfehler `'JsonDocument' was not declared` | ArduinoJson **7.x** notwendig. Update über Library Manager. |
| Display sieht aus wie gewünscht, aber **Werte sehen falsch aus** | Koordinaten geprüft? Lat/Lon vertauscht? Der erste Test: `LOCATION_LAT=52.52` und `LOCATION_LON=13.405` sollte „Berlin" plausibles Wetter geben. |

---

## Du hast es geschafft 🌌

Hänge das Display ans Fenster, schließe Strom an, und beim nächsten
klaren Wetter siehst du sofort: **GUT** mit Sternen → Teleskop raus!

Bei Fragen / Bugs / Verbesserungsvorschlägen: GitHub-Issue eröffnen.
Clear skies!
