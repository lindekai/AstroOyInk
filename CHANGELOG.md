# Changelog

Alle relevanten Änderungen am Projekt.

## [Unreleased]

### Hinzugefügt
- 📐 **Bohrschablone fürs Wandgehäuse** (`hardware/gehaeuse/bohrschablone_wand.pdf`,
  A4, 1:1): Gehäuseumriss, beide Bohrpunkte (100 mm Abstand, 15 mm unter der
  Oberkante), Wasserwaagen-Linie, Kontrollstrecke für den Druckmaßstab und
  Schraubenempfehlung. Erzeugt mit `tools/make_bohrschablone.py`.
- 🏠 **3D-Druck-Gehäuse** in `hardware/gehaeuse/` (OpenSCAD, parametrisch,
  STLs in `stl/`): **Tischgehäuse** – Display steht frei im 15°-Schlitz, nur
  der inaktive Rand steckt im Sockel, Elektronik und Akku im 21 mm flachen
  Sockel – und **Wandgehäuse** – flacher Rahmen mit 2-mm-Blende,
  Schlüssellöcher, Elektronik auf dem Rückdeckel. Beide ohne Kleber
  (Rastnasen, Niederhalter, Presspassung), je ein USB-C-Anschluss zum Laden,
  belüftete BME280-Kammer. Maße der Elektronik nachgemessen.
- 🖼️ Gehäuse-Mockups (`tools/make_gehaeuse_mockups.py`): Hero-Bilder mit
  echter Display-Vorschau und Schnittzeichnungen aus echten Modellschnitten.
- 📖 README ohne aufklappbare Bereiche: Pinbelegung, Schaltplan,
  Ersatzteile, Display-Layout und Vorgeschichte sind jetzt normale
  Abschnitte.
- 🔌 **Schaltplan und Steckbrett-Bild** (`docs/images/schaltplan.svg`,
  `docs/images/breadboard.svg`), vollständig kommentiert: Display,
  TC4056 + LiPo, TPS62827-Buck, Pufferelko, Akkuteiler an GPIO34, optionale
  USB-Erkennung an GPIO35 und BME280. Der Steckbrett-Aufbau passt auf ein
  Brett, alle Signale auf einer Stiftleiste (CS 33, BUSY 39, SDA 32, SCL 25
  in `config.h`; die Defaults bleiben fürs Waveshare Driver Board). Erzeugt
  mit `tools/make_wiring_svgs.py`, eingebunden in README und INSTALL.
- 🌙 **Mondhöhe über die Nacht** — neue Zeile „Mond“ in der Stundentabelle
  (Grad je Nachtstunde, ab 30° rot, unter dem Horizont „-“),
  berechnet mit `computeMoonTrack()`.
- 🖼️ **Pixelgenaue Host-Vorschau** `tools/preview/build_preview.sh` —
  rendert alle Ansichten mit echter Adafruit GFX als PNG, `--time` für
  feste Zeitpunkte. README-Bild ist vorerst diese Vorschau.
- 🔭 **N.I.N.A.-Auto-Discovery** (leerer Host = automatisch, Scan nur
  abends/nachts) und Sequenz-Auswertung über `sequence/state`.

### Geändert (Gehäuse)
- 🔋 **Wanddeckel überarbeitet nach Probedruck:** Ladeelektronik (TC4056 +
  Buck) sitzt jetzt links auf der ESP-Seite, der TC4056 in einer Tasche mit
  Führungen, Rastnasen und Anschlag hinten. USB-C wandert an der Unterkante
  nach links (x = 47), die Rahmenwand vor der Buchse ist außen auf 1 mm
  ausgespart, damit der Stecker ganz einrastet. BME-Kammer: kurze Wand stieß
  an die Glasauflage des Rahmens – jetzt 1 mm darunter, obere Wand 1 mm vor
  dem Glas, Kabeldurchlass Richtung ESP32, Sensorhalter weg vom Schraubdom.
  **Rahmen und Deckel neu drucken.**
- 🧪 `pruefe_kollision.sh wand` prüft jetzt auch das Wandgehäuse.
- 🧩 **Probestücke** fürs Wandgehäuse (`teil="probe_rahmen"` / `"probe_deckel"`,
  `stl/probe_wand_*.stl`): nur die Ecke unten links mit BME-Kammer,
  TC4056-Tasche, Buck und USB-C – Passung testen, bevor die 8-h-Teile laufen.
- 🔩 **Wanddeckel: Senkungen auf die Außenseite** – die M3-Senkungen saßen
  innen, verschraubt wird aber von außen. Jetzt wie bei der Tisch-Bodenplatte
  (Ø 6,6 mm an der Außenfläche). `wand_deckel.stl` neu exportiert.
- 🏠 **Tischgehäuse nach dem ersten Probedruck überarbeitet:** kleiner
  (184 × 68 × 21,4 statt 200 × 84 × 26,4 mm), Schlitz weiter vorn, Akku links,
  Driver Board rechts vorn, BME280-Kammer hinten rechts. Die Kammerwände
  stehen jetzt auf der Bodenplatte, und der Zentrierrand ist dort offen. Vorher
  stießen sie auf den Rand, der Sockel ließ sich nicht aufsetzen. Ebenfalls
  behoben: FPC-Adapter lag auf der Akku-Mulde, TC4056 lag auf dem Zentrierrand.
  Die USB-C-Wand ist von außen auf 1 mm ausgespart, damit Stecker ganz
  einrasten. Neu: `hardware/gehaeuse/pruefe_kollision.sh` prüft alle Teile
  paarweise auf Überschneidungen.
- 🔤 Bodenplatte des Tischgehäuses mit vertiefter Beschriftung auf der
  Unterseite: AstroOyInk, Version, Autor, Community und Lizenz.

### Geändert (Layout)
- 🔭 **SESSION-Ansicht neu geordnet** wie WEATHER: Sequenz-Kasten mit
  großem Targetnamen und Sub-Fortschrittsbalken; KAMERA / GUIDING / MOUNT
  mit je einem großen Leitwert (HFR, RMS gesamt, Zeit bis Flip); neue
  NACHT-Zeile mit Mond (lokal berechnet) und Sonnenaufgang. Gradzeichen bei
  Dec/Alt/Az, Pier-Seite „Ost/West“. README/INSTALL mit Vorschau-Bild.
- 📷 **Kamera-Temperatur** im SESSION-Modus: Sensor ist/soll und
  Kühlerleistung, rot bei > 1,5 K Abweichung oder ausgeschaltetem Kühler.
- 🌐 **Flip-Uhrzeit:** unter dem Countdown „bis Flip ~23:15“.
- Portal- und Fallback-Bildschirm: Titel „AstroOyInk“, Texte ohne fehlende
  Glyphen („ausfuellen“ statt „ausfullen“, Bindestrich statt Gedankenstrich).
- WEATHER-Layout aufgeräumt: Zeilenabstände, Gradzeichen, einheitlich
  GUT/MITTEL/SCHLECHT, ruhigeres Wolken-Chart mit drei 2-px-Linien.
- Transparenz-Linie schwarz gestrichelt — bei gleichen Werten lag sie
  unsichtbar unter der roten Seeing-Linie.
- Vorhersage ohne Überschrift „VORHERSAGE“, jetzt **6 Tage** über die volle
  Breite (`FORECAST_SHOW_DAYS`). Abgerufen werden 7 Tage (`FORECAST_DAYS`),
  damit auch die letzte Nacht bis 04:00 Daten hat — vorher beruhte die
  Bewertung des letzten Tages nur auf 22–23 Uhr.
- Regenbalken mindestens 3 px hoch (0,1 mm war nur ein Strich).
- Log unterscheidet „N.I.N.A. nicht erreichbar“ und „keine laufende Sequenz“.

### Entfernt / pausiert
- 🛰️ **ISS-Überflüge pausiert** — open-notify liefert für die Überflug-
  Vorhersage nur noch HTTP 404. Abruf per `ISS_ENABLED 0` (in `iss_api.h`)
  aus, README/Roadmap angepasst. Der Code bleibt für eine neue Quelle.

### Behoben (Layout)
- Vorhersage zeigte „-0°“ bei Werten wie -0,4 °C.
- Akkuanzeige „0 %“ ohne angeschlossenen Akku — wird jetzt ausgeblendet.
- N.I.N.A. wurde nicht gefunden: Probe-Pfad `/v2/api/version`
  (`application/version` liefert 404).
- Meridian-Flip-Countdown: `TimeToMeridianFlip` kommt in **Stunden**, wurde
  aber als Sekunden gelesen (zeigte „0 min“ in Rot). 24 h = kein Flip
  ansteht → Anzeige „--“.

### Hinzugefügt (Stromversorgung, README)
- 🔋 **Stromversorgung / Akkubetrieb** — Referenzaufbau wie DoorSign:
  TC4056 → LiPo 3500 mAh → TPS62827-Buck (Iq ~4 µA) → 3,3 V auf den
  `3V3`-Pin, 1500-µF-Puffer-Elko, Teiler 2 × 100 kΩ an GPIO34.
  Doku in README (Abschnitt „Stromversorgung“ mit Laufzeit-Hochrechnung),
  INSTALL.md und CLAUDE.md.
- ⚙️ **`BATT_USB_SENSE_PIN`** in `config.h` — optionale USB-Erkennung über
  einen zweiten Teiler am 5-V-Eingang des TC4056 (Standard `-1` = aus).
- 📸 **Foto vom echten Display** im README (`docs/images/display_weather.jpg`,
  WEATHER-Modus) — inzwischen durch die pixelgenauen Vorschau-Bilder ersetzt,
  weil es noch das alte Layout zeigte.
- 🎨 **README überarbeitet:** Hero-Banner und Modus-Grafik (hell/dunkel, SVG),
  Badges, Navigation, Funktionsübersicht, Layout-Skizzen für beide Modi.
  Generator unter `tools/make_readme_svgs.py`.

### Geändert
- 💤 **SESSION-Modus schläft zwischen den Updates** (Deep-Sleep, 5 min)
  statt dauerhaft wach zu bleiben. Eine Session-Nacht kostet damit
  rechnerisch ~60 mAh statt mehrerer Hundert.
- 📡 **WLAN aus vor dem Display-Refresh** — die Daten sind geholt, der
  30-s-Refresh läuft ohne Funk.
- 📤 **OTA-Fenster nur noch beim Kaltstart** (Reset-Taste / Strom an) bzw.
  im `DEBUG_MODE`, nicht mehr nach jedem Deep-Sleep-Wake (~1 mAh je Fenster).
- 🔋 **Akkumessung mit `analogReadMilliVolts()`** — nutzt die
  Werkskalibrierung und bleibt korrekt, wenn der Buck bei leerem Akku
  keine 3,3 V mehr liefert.

### Behoben
- Akkuanzeige: „USB“ wurde erst ab 4,3 V Akkuspannung erkannt — hinter dem
  TC4056 (Ladeschluss 4,2 V) nie erreicht. Jetzt über `BATT_USB_SENSE_PIN`.
- README: Layout-Skizzen auf reines ASCII umgestellt (verrutschte Rahmen).
- README: tote CI-Badges, nicht existierende `SETUP_NEW_MACHINE.md` und die
  entfernte FAZIT-Zeile aus der Doku genommen.
- 🔭 **SESSION-Modus** — `src/data_sources/nina_api.h/.cpp` +
  `renderSessionDashboard()` in `display_base.cpp`
  N.I.N.A. Advanced API v2, Port 1888, kein Key. Automatischer Wechsel
  WEATHER → SESSION wenn N.I.N.A. erreichbar **und** Sequenz läuft.
  Zeigt: Sequenzname, Target i/n, Sub i/n, Restzeit, Status
  (Imaging/Focusing/Slewing/Pausiert), Objekt-Höhe/Azimut, Kameraname,
  Filter, Belichtungszeit, HFR, Guiding RMS RA/Dec/Total + Status-Label
  ([Guiding OK] / [Guiding WARN] / [Guiding LOST]), Mountname,
  RA/Dec-Koordinaten, Pier-Seite, Meridian-Flip-Countdown.
  Warnungen (RMS ≥ 1,0″, Flip < 10 min, Slewing/Pausiert):
  rot (BWR) oder invertierter Block (BW).
  SESSION-Modus: Gerät bleibt wach, Update alle 5 min (Wake-Loop);
  bei Verbindungsverlust automatisch zurück zu WEATHER + Deep-Sleep.
- 🎨 **`DISPLAY_HAS_RED`** in `config.h` — steuert ob Rot-Kanal
  genutzt wird (1 = BWR Standard, 0 = B/W-Display). Alle roten
  Elemente (WEATHER + SESSION) reagieren auf diese Einstellung.
  Makros `DISPLAY_RED` und `DISPLAY_RED_BG` in `display_base.cpp`.
- ⚙️ **`GUIDING_WARN_RMS_ARCSEC`** in `config.h` — Schwellwert für
  Guiding-Warnung (Standard 1,0″).
- ⚙️ **`SESSION_REFRESH_INTERVAL_S`** in `config.h` — Aktualisierungs-
  intervall im SESSION-Modus (Standard 300 s / 5 min).

### Geplant (verbleibend)
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

### Geändert
- 🗑️ **FAZIT-Zeile** aus dem WEATHER-Dashboard entfernt.
  ISS-Empfehlung war redundant — Beobachter sehen den Himmel selbst.
- 🛰️ **ISS-Symbol** im Stundenblock: statt eigener FAZIT-Zeile erscheint
  „ISS" (rot) in der Stundenspalte, in der der nächste Überflug beginnt.

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
