# Contributing zu AstroOyInk

Vielen Dank für dein Interesse! Dieses Dokument fasst zusammen, wie du
beitragen kannst.

## Setup

```bash
git clone https://github.com/<dein-user>/AstroOyInk.git
cd AstroOyInk
cp secrets.h.example secrets.h   # nicht committen
# secrets.h editieren mit eigenen WLAN-Daten
```

Arduino IDE 2.x öffnen, `AstroOyInk.ino` auswählen, kompilieren.
Details siehe [INSTALL.md](INSTALL.md).

## Architektur

Bitte vor größeren Änderungen **[CLAUDE.md](CLAUDE.md)** lesen. Dort
stehen die Architektur-Prinzipien:

1. Strikte Modulgrenzen (`display_*` kennt kein WLAN, `*_api` kennt kein Display)
2. Static / Heap statt Stack für große Structs (`WeatherData`, `NinaData`)
3. Paged Rendering (`HEIGHT/4` Buffer wegen DRAM-Limit)
4. Keine grüne Farbe — Display kann nur B/W/R; bei `DISPLAY_HAS_RED=0` auch nur B/W
5. Manueller BUSY-Wait für V3-Panels

## Tests

Host-Tests laufen auf jedem Linux/macOS mit g++:

```bash
cd tests
./build_and_run.sh
```

Sie validieren:
- Astro-Verdict in 8 Szenarien (klar, Cirrus, Stratus, overcast, +Wind, +Tau, ...)
- Mondberechnung gegen 2026er Voll-/Neumonde, Höhe-Tageszyklus

GitHub Actions baut zusätzlich den Arduino-Sketch.

## Pull Requests

1. Branch erstellen: `git checkout -b feat/batterie-anzeige`
2. Änderungen + Tests anpassen
3. `./tests/build_and_run.sh` muss grün sein
4. Falls möglich: Foto vom Display anhängen
5. PR mit Beschreibung "Was" + "Warum"

## Style

- 2 Leerzeichen Einrückung, K&R-Klammern
- `snake_case` lokal, `camelCase` Funktionen, `PascalCase` Typen
- Umlaute im Code vermeiden (Fonts haben keine)
- Log-Prefix in eckigen Klammern: `[WIFI]`, `[API]`, `[EPD]`

## Roadmap

Alle 8 Roadmap-Punkte sind abgeschlossen. Weiterentwicklungen (neue Features,
Bugfixes, Layout-Verbesserungen) als Issue mit Label `enhancement` willkommen.

Bekannte Erweiterungsideen:
- Partieller B/W-Refresh im SESSION-Modus für schnellere Updates
- Aufteilen von `display_base.cpp` in `view_weather.cpp` + `view_session.cpp`
- Starlink-Überflüge ergänzen
