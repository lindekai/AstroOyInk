#!/usr/bin/env python3
"""Erzeugt Schaltplan und Steckbrett-Bild (Breadboard) fuer AstroOyInk.

Aufruf:  python3 tools/make_wiring_svgs.py docs/images
Ausgabe: schaltplan.svg, breadboard.svg

Pinbelegung entspricht config.h (EPD_PIN_*, BATT_*, LOCAL_SENSOR_*).
Bei Aenderungen dort hier nachziehen und neu erzeugen.
"""
import os, sys

OUT = sys.argv[1] if len(sys.argv) > 1 else "docs/images"
FONT = "ui-sans-serif, -apple-system, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif"
MONO = "ui-monospace, 'SF Mono', Menlo, Consolas, monospace"
INK, RED, GREY, MUTED = "#1B1B1B", "#C8102E", "#8A8F99", "#5B6472"


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


class Svg:
    def __init__(self, w, h):
        self.w, self.h, self.p = w, h, []

    def a(self, s):
        self.p.append(s)

    def text(self, x, y, s, size=12, anchor="start", fill=INK, weight=400, rot=None, mono=False, italic=False,
             halo=None):
        ex = f' font-family="{MONO}"' if mono else ""
        if italic:
            ex += ' font-style="italic"'
        if halo:
            ex += f' stroke="{halo}" stroke-width="3" paint-order="stroke"'
        if rot is not None:
            ex += f' transform="rotate({rot} {x} {y})"'
        self.a(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" '
               f'fill="{fill}" font-weight="{weight}"{ex}>{esc(s)}</text>')

    def line(self, x1, y1, x2, y2, stroke=INK, w=2, dash=None):
        d = f' stroke-dasharray="{dash}"' if dash else ""
        self.a(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{stroke}" stroke-width="{w}"{d} stroke-linecap="round"/>')

    def poly(self, pts, stroke=INK, w=2, dash=None):
        d = f' stroke-dasharray="{dash}"' if dash else ""
        pp = " ".join(f"{x},{y}" for x, y in pts)
        self.a(f'<polyline points="{pp}" fill="none" stroke="{stroke}" stroke-width="{w}"{d} stroke-linejoin="round" stroke-linecap="round"/>')

    def rect(self, x, y, w, h, fill="none", stroke=INK, sw=2, rx=0, dash=None):
        d = f' stroke-dasharray="{dash}"' if dash else ""
        self.a(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" '
               f'stroke="{stroke}" stroke-width="{sw}"{d}/>')

    def circle(self, x, y, r, fill="none", stroke=INK, sw=2):
        self.a(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')

    def out(self):
        head = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {self.w} {self.h}" '
                f'width="{self.w}" height="{self.h}" font-family="{FONT}">')
        return head + "\n" + "\n".join(self.p) + "\n</svg>\n"


# =====================================================================
#  Schaltplan
# =====================================================================

def schematic():
    s = Svg(1600, 1420)
    s.rect(0, 0, 1600, 1420, fill="#FFFFFF", stroke="none", sw=0)

    def frame(x, y, w, h, title, right=False, opt=False):
        # Rahmen direkt hinter den Hintergrund einsortieren, damit Leitungen
        # aus Nachbarbloecken nicht verdeckt werden
        n = len(s.p)
        s.rect(x, y, w, h, fill="#F7F8FA", stroke="#C9CED8", sw=1.5, rx=10, dash="6 4" if opt else None)
        if right:
            s.text(x + w - 14, y + 22, title, 13, "end", MUTED, 700)
        else:
            s.text(x + 14, y + 22, title, 13, "start", MUTED, 700)
        new = s.p[n:]
        del s.p[n:]
        s.p[1:1] = new

    def dot(x, y):
        s.circle(x, y, 4, fill=INK, stroke="none", sw=0)

    def gnd(x, y):
        # DIN-Masse: Stich + dicker Balken
        s.line(x, y, x, y + 12)
        s.line(x - 12, y + 12, x + 12, y + 12, w=4)

    def nc(x, y):
        s.line(x - 6, y - 6, x + 6, y + 6)
        s.line(x - 6, y + 6, x + 6, y - 6)

    def net(x, y, name, d="left", dash=None):
        w = 9 * len(name) + 12
        if d == "left":
            pts = [(x, y), (x - 9, y - 10), (x - 9 - w, y - 10), (x - 9 - w, y + 10), (x - 9, y + 10), (x, y)]
            tx, ty = x - 9 - w / 2, y + 4
        elif d == "right":
            pts = [(x, y), (x + 9, y - 10), (x + 9 + w, y - 10), (x + 9 + w, y + 10), (x + 9, y + 10), (x, y)]
            tx, ty = x + 9 + w / 2, y + 4
        else:  # up
            pts = [(x, y), (x - w / 2, y - 9), (x - w / 2, y - 29), (x + w / 2, y - 29), (x + w / 2, y - 9), (x, y)]
            tx, ty = x, y - 15
        pp = " ".join(f"{a},{b}" for a, b in pts)
        dd = f' stroke-dasharray="{dash}"' if dash else ""
        s.a(f'<polygon points="{pp}" fill="#FFFFFF" stroke="{INK}" stroke-width="1.5"{dd}/>')
        s.text(tx, ty, name, 11, "middle", INK, 700, mono=True)

    def res_v(x, y1, y2, ref, val, dash=None):
        # Widerstand senkrecht (DIN-Rechteck), Anschluss oben y1 / unten y2
        m = (y1 + y2) / 2
        s.line(x, y1, x, m - 25, dash=dash)
        s.line(x, m + 25, x, y2, dash=dash)
        s.rect(x - 8, m - 25, 16, 50, fill="#FFFFFF", dash=dash)
        s.text(x + 16, m - 3, ref, 12, weight=700)
        s.text(x + 16, m + 13, val, 12)

    def marker(x, y, n):
        s.circle(x, y, 11, fill=RED, stroke="none", sw=0)
        s.text(x, y + 4.5, str(n), 12, "middle", "#FFFFFF", 800)

    def comment(x, y, lines, size=12, anchor="start"):
        for i, ln in enumerate(lines):
            s.text(x, y + i * 17, ln, size, anchor, MUTED, italic=True)

    # --- Titel -------------------------------------------------------
    s.text(30, 44, "AstroOyInk – Schaltplan", 26, weight=800)
    s.text(30, 70, "ESP32-DevKit (WROOM-32) · Waveshare 7,5″ e-Paper V3 B/W/R · optional: LiPo-Akku, "
                   "Akkuanzeige, USB-Erkennung, BME280 · Pinbelegung laut config.h", 13, fill=MUTED)
    s.text(1570, 44, "v1.0.0-rc1", 13, "end", MUTED, 700, mono=True)

    # --- A: Stromversorgung ------------------------------------------
    frame(30, 95, 1540, 320, "A   STROMVERSORGUNG – Akkubetrieb (optional)", opt=True)

    # U1 TC4056
    s.rect(220, 150, 180, 150, fill="#FFFFFF")
    s.text(310, 175, "U1  TC4056", 14, "middle", weight=800)
    s.text(310, 193, "Lademodul + DW01", 11, "middle", MUTED)
    s.rect(236, 210, 46, 22, fill="#E6E8EC", stroke=INK, sw=1.5, rx=6)
    s.text(259, 225, "USB-C", 9, "middle", weight=700)
    for y, nm in ((180, "IN+"), (270, "IN−")):
        s.line(200, y, 220, y)
        s.text(226, y + 4, nm, 11, weight=700, mono=True)
    for y, nm in ((180, "OUT+"), (270, "OUT−")):
        s.line(400, y, 420, y)
        s.text(394, y + 4, nm, 11, "end", weight=700, mono=True)
    for x, nm in ((280, "B+"), (340, "B−")):
        s.line(x, 300, x, 330)
        s.text(x, 294, nm, 11, "middle", weight=700, mono=True)
    # IN+ -> USB_5V, IN- -> GND
    s.line(200, 180, 180, 180)
    net(180, 180, "USB_5V", "left", dash="4 3")
    s.line(200, 270, 170, 270)
    gnd(170, 270)

    # BT1 LiPo
    s.poly([(280, 330), (280, 365), (302, 365)])
    s.poly([(318, 365), (340, 365), (340, 330)])
    s.line(302, 345, 302, 385, w=2)
    s.line(316, 353, 316, 377, w=5)
    s.text(296, 350, "+", 13, "end", weight=800)
    s.text(360, 362, "BT1  LiPo 3,7 V, 3500 mAh", 12, weight=700)
    s.text(360, 379, "EEMB LP104567 (1 Zelle)", 12, fill=MUTED)

    # OUT+ -> VBAT -> U2 VIN
    s.line(420, 180, 640, 180)
    dot(520, 180)
    s.line(520, 180, 520, 150)
    net(520, 150, "VBAT", "up")
    s.line(420, 270, 470, 270)
    gnd(470, 270)

    # U2 TPS62827
    s.rect(660, 150, 180, 150, fill="#FFFFFF")
    s.text(750, 175, "U2  TPS62827", 14, "middle", weight=800)
    s.text(750, 193, "Buck 3,3 V / 2 A", 11, "middle", MUTED)
    s.text(750, 209, "(Adafruit 4920)", 11, "middle", MUTED)
    s.line(640, 180, 660, 180)
    s.text(666, 184, "VIN", 11, weight=700, mono=True)
    s.line(640, 270, 660, 270)
    s.text(666, 274, "GND", 11, weight=700, mono=True)
    s.line(620, 270, 640, 270)
    gnd(620, 270)
    s.line(840, 180, 860, 180)
    s.text(834, 184, "VOUT", 11, "end", weight=700, mono=True)
    s.line(860, 180, 930, 180)
    net(930, 180, "3V3", "right")

    marker(436, 306, 1)
    marker(750, 128, 2)
    marker(1010, 158, 3)
    comment(44, 330, ["Lädt mit 1 A bis 4,2 V,", "Schutz trennt bei ~2,5 V"], 11)
    comment(670, 325, ["Ruhestrom ≈ 4 µA. Unter ~3,4 V", "Durchgang: VOUT folgt der Zelle."], 11)
    comment(866, 214, ["Geht auf den ESP32-Pin 3V3,", "NICHT auf VIN/5V (Board-LDO)."], 11)

    # Netzteil-Alternative
    s.rect(1090, 140, 460, 250, fill="#FFFFFF", stroke="#C9CED8", sw=1.5, rx=8)
    s.text(1110, 166, "Alternative: Netzteilbetrieb", 13, weight=800)
    s.rect(1110, 190, 120, 50, fill="#F1F2F5", rx=6)
    s.text(1170, 212, "USB-Netzteil", 12, "middle", weight=700)
    s.text(1170, 229, "5 V", 11, "middle", MUTED)
    s.line(1230, 215, 1330, 215, w=2.5)
    s.poly([(1322, 209), (1330, 215), (1322, 221)])
    s.rect(1330, 190, 200, 50, fill="#F1F2F5", rx=6)
    s.text(1430, 212, "Micro-USB am ESP32", 12, "middle", weight=700)
    s.text(1430, 229, "(Board-LDO macht 3V3)", 11, "middle", MUTED)
    comment(1110, 266, ["Block A und Block B entfallen komplett.",
                        "In config.h: BATT_ENABLED 0 – kein Akku-Symbol.",
                        "Nur Display (D) und optional BME280 (E)",
                        "anschließen. Für Dauerbetrieb an der Wand",
                        "ist das die einfachste Variante."], 12)

    # --- B: Akkumessung ----------------------------------------------
    frame(30, 435, 540, 575, "B   AKKUMESSUNG (ADC)")
    # Teiler 1: VBAT -> GPIO34
    net(200, 505, "VBAT", "up")
    res_v(200, 505, 625, "R1", "100 kΩ")
    s.line(200, 625, 200, 660)
    dot(200, 640)
    s.line(200, 640, 720, 640)
    s.text(232, 632, "BATT_ADC  (VBAT / 2)", 11, fill=MUTED, mono=True)
    res_v(200, 660, 760, "R2", "100 kΩ")
    gnd(200, 760)
    marker(140, 560, 4)
    # Teiler 2 (optional): USB_5V -> GPIO35
    D = "6 4"
    net(420, 695, "USB_5V", "up", dash="4 3")
    res_v(420, 695, 800, "R3", "100 kΩ", dash=D)
    dot(420, 800)
    s.line(420, 800, 720, 800, dash=D)
    s.text(436, 792, "USB_SENSE", 11, fill=MUTED, mono=True)
    res_v(420, 800, 910, "R4", "100 kΩ", dash=D)
    gnd(420, 910)
    s.text(470, 720, "optional", 11, fill=RED, weight=700)
    marker(360, 740, 5)
    comment(50, 812, ["Teiler 1:2 → 4,2 V Akku = 2,1 V am ADC",
                      "(config.h: BATT_VOLTAGE_DIVIDER 2.0f).",
                      "Querstrom 4,2 V / 200 kΩ ≈ 21 µA, dauernd.",
                      "GPIO34/35 sind reine Eingänge an ADC1",
                      "→ messbar auch bei aktivem WLAN.",
                      "R3/R4 nur für „USB“ im Header:",
                      "BATT_USB_SENSE_PIN 35 (sonst -1)."], 12)

    # --- C: Mikrocontroller ------------------------------------------
    frame(585, 435, 385, 575, "C   MIKROCONTROLLER")
    s.rect(740, 485, 180, 490, fill="#FFFFFF", sw=2.5)
    s.text(830, 474, "U3  ESP32-WROOM-32", 13, "middle", weight=800)
    s.text(830, 735, "ESP32", 22, "middle", "#D5D8DE", 800)
    s.text(830, 757, "DevKitC / Waveshare", 10, "middle", "#B5BAC5", 700)
    s.text(830, 771, "e-Paper Driver Board", 10, "middle", "#B5BAC5", 700)
    left = [(510, "3V3"), (640, "GPIO34"), (800, "GPIO35"), (870, "5V/VIN"), (945, "GND")]
    for y, nm in left:
        s.line(720, y, 740, y)
        s.text(746, y + 4, nm, 11, weight=700, mono=True)
    right = [(535, "GPIO14"), (565, "GPIO13"), (595, "GPIO15"), (625, "GPIO27"), (655, "GPIO26"),
             (685, "GPIO25"), (845, "GPIO22"), (875, "GPIO21")]
    for y, nm in right:
        s.line(920, y, 940, y)
        s.text(914, y + 4, nm, 11, "end", weight=700, mono=True)
    # 3V3 + C1
    net(612, 510, "3V3", "left")
    s.line(612, 510, 720, 510)
    dot(650, 510)
    s.line(650, 510, 650, 538)
    s.rect(640, 538, 20, 6, fill="#FFFFFF", sw=1.8)
    s.rect(640, 550, 20, 6, fill=INK, sw=1.8)
    s.text(636, 545, "+", 12, "end", weight=800)
    s.line(650, 556, 650, 578)
    gnd(650, 578)
    s.text(668, 548, "C1", 12, weight=700)
    s.text(668, 564, "1500 µF", 11)
    s.text(668, 578, "≥ 6,3 V", 11, fill=MUTED)
    marker(612, 560, 6)
    nc(720, 870)
    s.text(704, 874, "frei", 11, "end", MUTED, italic=True)
    marker(668, 890, 7)
    s.line(720, 945, 700, 945)
    gnd(700, 945)
    comment(600, 990, ["Alle GND gemeinsam (Sternpunkt am ESP32)."], 11)

    # --- D: Display ---------------------------------------------------
    frame(985, 435, 585, 315, "D   DISPLAY", right=True)
    s.rect(1100, 460, 120, 250, fill="#FFFFFF")
    s.text(1160, 452, "J1  e-Paper HAT", 12, "middle", weight=800)
    dpins = [(475, "VCC"), (505, "GND"), (535, "DIN"), (565, "CLK"), (595, "CS"), (625, "DC"),
             (655, "RST"), (685, "BUSY")]
    for i, (y, nm) in enumerate(dpins):
        s.line(1080, y, 1100, y)
        s.text(1106, y + 4, nm, 11, weight=700, mono=True)
        s.text(1214, y + 4, str(i + 1), 10, "end", GREY, mono=True)
    s.line(1080, 475, 1066, 475)
    net(1066, 475, "3V3", "left")
    s.line(1080, 505, 1066, 505)
    net(1066, 505, "GND", "left")
    sig = {535: "MOSI", 565: "SCK", 595: "CS", 625: "DC", 655: "RST", 685: "BUSY"}
    for y, nm in sig.items():
        s.line(940, y, 1080, y)
        s.text(1010, y - 5, nm, 10, "middle", MUTED, mono=True)
    # FPC + Panel
    for k in range(6):
        s.line(1220, 560 + k * 6, 1300, 560 + k * 6, stroke="#B88A2E", w=1.5)
    s.text(1260, 552, "24-pol. FPC", 10, "middle", MUTED)
    s.rect(1300, 468, 250, 192, fill="#ECEBE4", sw=2.5, rx=6)
    s.rect(1314, 482, 222, 150, fill="#FFFFFF", stroke="#B5BAC5", sw=1)
    s.text(1425, 535, "Waveshare 7,5″ V3", 14, "middle", weight=800)
    s.text(1425, 555, "800 × 480 px", 11, "middle", MUTED)
    s.text(1425, 571, "Schwarz / Weiß / Rot", 11, "middle", MUTED)
    s.text(1425, 595, "GxEPD2_750c_Z08", 11, "middle", INK, 700, mono=True)
    s.text(1425, 650, "Panel", 10, "middle", MUTED)
    marker(1310, 700, 8)
    comment(1330, 690, ["VCC nur 3,3 V! SPI-Pins frei", "wählbar: config.h EPD_PIN_*"], 11)
    marker(962, 610, 9)

    # --- E: BME280 ----------------------------------------------------
    frame(985, 760, 585, 250, "E   INNENSENSOR BME280 (optional)", right=True, opt=True)
    s.rect(1100, 770, 120, 180, fill="#FFFFFF", dash="6 4")
    s.text(1160, 966, "U4  BME280", 12, "middle", weight=800)
    bpins = [(785, "VCC"), (815, "GND"), (845, "SCL"), (875, "SDA"), (905, "CSB"), (935, "SDO")]
    for y, nm in bpins:
        s.line(1080, y, 1100, y)
        s.text(1106, y + 4, nm, 11, weight=700, mono=True)
    s.line(1080, 785, 1066, 785)
    net(1066, 785, "3V3", "left")
    s.line(1080, 815, 1066, 815)
    net(1066, 815, "GND", "left")
    s.line(940, 845, 1080, 845, dash=D)
    s.text(1010, 840, "SCL", 10, "middle", MUTED, mono=True)
    s.line(940, 875, 1080, 875, dash=D)
    s.text(1010, 870, "SDA", 10, "middle", MUTED, mono=True)
    s.line(1080, 905, 1066, 905)
    net(1066, 905, "3V3", "left", dash="4 3")
    s.line(1080, 935, 1066, 935)
    net(1066, 935, "GND", "left")
    marker(1250, 800, 10)
    comment(1270, 805, ["I²C, Adresse 0x76 (SDO → GND).",
                        "SDO → 3V3 ergibt 0x77, dann",
                        "LOCAL_SENSOR_ADDR 0x77.",
                        "CSB HIGH = I²C-Modus (auf den",
                        "meisten Breakouts schon gesetzt).",
                        "Pull-ups für SDA/SCL sitzen",
                        "auf dem Breakout.",
                        "config.h: LOCAL_SENSOR_ENABLED 1"], 11)

    # --- Kommentare + Legende ----------------------------------------
    frame(30, 1025, 1540, 375, "KOMMENTARE")
    notes = [
        "TC4056-Modul: B+ und OUT+ sind dasselbe Netz (VBAT). Der DW01-Schutz-FET sitzt im Minuszweig (B− ↔ OUT−), "
        "deshalb ist OUT− die System-Masse – nicht B−.",
        "TPS62827 (oder TPS62840 / TPS63802): Iq im einstelligen µA-Bereich ist entscheidend. "
        "KEIN LM2596-Modul (~5 mA Eigenverbrauch) und kein Boost auf 5 V.",
        "3V3 direkt auf den 3V3-Pin. Über VIN/5V liefe alles durch den AMS1117-LDO des Boards (Verlust + Ruhestrom). "
        "Zum Flashen per USB den Buck-Ausgang abklemmen oder OTA nutzen.",
        "R1/R2 messen die Zellspannung. Die Firmware liest analogReadMilliVolts() (werkskalibriert) und rechnet mit "
        "Faktor 2; 3,0 V = 0 %, 4,2 V = 100 %.",
        "R3/R4 (optional) liegen am 5-V-Eingang des TC4056: steckt USB, sieht GPIO35 ~2,5 V und der Header zeigt „USB“ "
        "statt Prozent.",
        "C1 möglichst kurz an 3V3/GND des ESP32 (Polung!). Fängt WLAN-Sendespitzen (~500 mA) ab – "
        "ohne ihn: „Brownout detector was triggered“.",
        "5V/VIN bleibt im Akkubetrieb frei. Im Netzteilbetrieb kommt die Versorgung über die Micro-USB-Buchse des Boards.",
        "Beim Waveshare ESP32 e-Paper Driver Board ist J1 schon auf der Platine verdrahtet – nur das FPC-Kabel "
        "einstecken. Beim DevKit 8 Dupont-Leitungen wie gezeigt.",
        "GPIO15 ist ein Strapping-Pin (steuert nur Boot-Meldungen) und als CS unkritisch. "
        "Meiden: GPIO 0, 2, 12 sowie 6–11 (Flash).",
        "BME280 nur mit 3,3 V betreiben. Steht kein Sensor, LOCAL_SENSOR_ENABLED 0 lassen – die INNEN-Zeile entfällt.",
        "Gezeigt sind die config.h-Defaults. Für den Aufbau auf einem Steckbrett (breadboard.svg): "
        "CS→GPIO33, BUSY→GPIO39, SDA→GPIO32, SCL→GPIO25.",
    ]
    y = 1068
    for i, n in enumerate(notes):
        marker(58, y - 4, i + 1)
        s.text(80, y + 1, n, 12.5)
        y += 28
    s.line(1060, 1382, 1100, 1382)
    s.text(1108, 1386, "Pflicht", 11, fill=MUTED)
    s.line(1170, 1382, 1210, 1382, dash=D)
    s.text(1218, 1386, "optional", 11, fill=MUTED)
    dot(1300, 1382)
    s.text(1312, 1386, "Verbindung", 11, fill=MUTED)
    nc(1400, 1382)
    s.text(1414, 1386, "nicht belegt", 11, fill=MUTED)
    return s.out()


# =====================================================================
#  Steckbrett (Breadboard)
# =====================================================================
#  Ein Brett (830 Kontakte). Das DevKitC hat 1,0″ Reihenabstand: Leiste
#  J2 steckt in Reihe b, J3 in Reihe j. Nur Reihe a bleibt frei, deshalb
#  liegen alle Signale auf J2 – mit eigenen Pins (BB_PINS, in config.h
#  einzutragen).

P = 14                     # Rastermass 2,54 mm
BX = 100                   # linke Kante Steckbrett
COLS = 60
YA = 210                   # Oberkante Brett
ROWS_UP = "abcde"
ROWS_DN = "fghij"

# Steckbrett-Pins, die von den config.h-Defaults abweichen
BB_PINS = [("EPD_PIN_CS", 33, 15), ("EPD_PIN_BUSY", 39, 25),
           ("LOCAL_SENSOR_SDA", 32, 21), ("LOCAL_SENSOR_SCL", 25, 22)]


def X(c):
    return BX + 24 + (c - 1) * P


R = {ch: YA + 58 + i * P for i, ch in enumerate(ROWS_UP)}
R.update({ch: YA + 58 + 7 * P + i * P for i, ch in enumerate(ROWS_DN)})
R["+"], R["-"] = YA + 16, YA + 30            # obere Schienen
R["-2"], R["+2"] = R["j"] + 28, R["j"] + 42  # untere Schienen (frei)


def H(row, c):
    return (X(c), R[row])


def breadboard():
    W, HH = 1440, 840
    s = Svg(W, HH)
    s.rect(0, 0, W, HH, fill="#FFFFFF", stroke="none", sw=0)
    s.text(30, 44, "AstroOyInk – Steckbrett-Aufbau", 26, weight=800)
    s.text(30, 66, "ESP32-DevKitC (38 Pins) auf einem Steckbrett (830 Kontakte) · alle Signale auf einer "
                   "Stiftleiste · Steckbrett-Pins in config.h eintragen (Kasten unten)", 13, fill=MUTED)

    # --- Brett --------------------------------------------------------
    s.rect(BX, YA, X(COLS) + 24 - BX, R["+2"] + 16 - YA, fill="#F4F2EC", stroke="#D6D2C6", sw=1.5, rx=8)
    my = (R["e"] + R["f"]) / 2
    s.rect(BX + 6, my - 6, X(COLS) + 18 - BX - 6, 12, fill="#E6E2D6", stroke="none", sw=0, rx=3)
    for ch in ROWS_UP + ROWS_DN:
        for c in range(1, COLS + 1):
            s.rect(X(c) - 2.5, R[ch] - 2.5, 5, 5, fill="#3F3D38", stroke="none", sw=0, rx=1)
        s.text(BX + 10, R[ch] + 3.5, ch, 9, "middle", "#9C978A", 700)
        s.text(X(COLS) + 14, R[ch] + 3.5, ch, 9, "middle", "#9C978A", 700)
    for c in (1, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60):
        s.text(X(c), R["a"] - 9, str(c), 8, "middle", "#9C978A")
        s.text(X(c), R["j"] + 15, str(c), 8, "middle", "#9C978A")
    for key, sym, off in (("+", "+", -7), ("-", "−", 7), ("-2", "−", -7), ("+2", "+", 7)):
        y = R[key]
        col = RED if sym == "+" else "#2F6FDE"
        s.line(X(1) - 6, y + off, X(COLS) + 6, y + off, stroke=col, w=1.5)
        for c in range(1, COLS + 1):
            if c % 6 == 0:
                continue
            s.rect(X(c) - 2.5, y - 2.5, 5, 5, fill="#3F3D38", stroke="none", sw=0, rx=1)
        s.text(BX + 10, y + 4, sym, 12, "middle", col, 800)
        s.text(X(COLS) + 14, y + 4, sym, 12, "middle", col, 800)

    def wire(pts, color, dash=None, opacity=1.0, r=10):
        # Leitung mit runden Ecken; Endstecker als kleine Quadrate
        d = f"M{pts[0][0]},{pts[0][1]}"
        for i in range(1, len(pts) - 1):
            (x0, y0), (x1, y1), (x2, y2) = pts[i - 1], pts[i], pts[i + 1]
            l1 = max(1e-6, ((x1 - x0) ** 2 + (y1 - y0) ** 2) ** .5)
            l2 = max(1e-6, ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** .5)
            rr = min(r, l1 / 2, l2 / 2)
            ax, ay = x1 - (x1 - x0) / l1 * rr, y1 - (y1 - y0) / l1 * rr
            bx, by = x1 + (x2 - x1) / l2 * rr, y1 + (y2 - y1) / l2 * rr
            d += f" L{ax:.1f},{ay:.1f} Q{x1},{y1} {bx:.1f},{by:.1f}"
        d += f" L{pts[-1][0]},{pts[-1][1]}"
        dd = f' stroke-dasharray="{dash}"' if dash else ""
        g = f'<g opacity="{opacity}">'
        g += f'<path d="{d}" fill="none" stroke="#00000040" stroke-width="6.5" stroke-linecap="round" stroke-linejoin="round"{dd}/>'
        g += f'<path d="{d}" fill="none" stroke="{color}" stroke-width="4" stroke-linecap="round" stroke-linejoin="round"{dd}/>'
        for (x, y) in (pts[0], pts[-1]):
            g += f'<rect x="{x-3.5}" y="{y-3.5}" width="7" height="7" rx="1" fill="#2B2B2B"/>'
        g += "</g>"
        s.a(g)

    def resistor(p1, p2, ref, opacity=1.0, label_dy=-12):
        (x1, y1), (x2, y2) = p1, p2
        g = f'<g opacity="{opacity}">'
        g += f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="#9A9A9A" stroke-width="2"/>'
        mx, my = (x1 + x2) / 2, (y1 + y2) / 2
        g += f'<rect x="{mx-15}" y="{my-5.5}" width="30" height="11" rx="5" fill="#D9C39A" stroke="#A8916A" stroke-width="1"/>'
        # 100 kOhm: braun schwarz gelb gold
        for k, col in enumerate(("#7A4A21", "#111111", "#E6C21A")):
            g += f'<rect x="{mx-10+k*5}" y="{my-5.5}" width="2.6" height="11" fill="{col}"/>'
        g += f'<rect x="{mx+8}" y="{my-5.5}" width="2.6" height="11" fill="#C9A23A"/>'
        for (x, y) in (p1, p2):
            g += f'<circle cx="{x}" cy="{y}" r="2.2" fill="#8A8A8A"/>'
        g += "</g>"
        s.a(g)
        s.text(mx, my + label_dy, ref, 10, "middle", INK, 700, halo="#F4F2EC")

    def marker(x, y, n):
        s.circle(x, y, 11, fill=RED, stroke="#FFFFFF", sw=2)
        s.text(x, y + 4.5, str(n), 12, "middle", "#FFFFFF", 800)

    def module(x, y, w, h, title, sub, color):
        s.rect(x, y, w, h, fill=color, stroke="#00000055", sw=1.5, rx=5)
        s.text(x + w / 2, y + h / 2 - 2, title, 12, "middle", "#FFFFFF", 800)
        s.text(x + w / 2, y + h / 2 + 13, sub, 9.5, "middle", "#FFFFFFCC")

    def pad(x, y, w=8, h=6):
        s.rect(x - w / 2, y - h / 2, w, h, fill="#E4C25A", stroke="none", sw=0, rx=1)

    # --- ESP32-DevKitC ------------------------------------------------
    # gedreht: USB links, Antenne rechts. J2 (3V3 ... 5V) in Reihe b,
    # J3 in Reihe j. Reihe a ist das einzige freie Loch je Pin.
    top_pins = ["5V", "CMD", "SD3", "SD2", "13", "GND", "12", "14", "27", "26", "25", "33", "32",
                "35", "34", "VN", "VP", "EN", "3V3"]
    bot_pins = ["CLK", "SD0", "SD1", "15", "2", "0", "4", "16", "17", "5", "18", "19", "GND",
                "21", "RX", "TX", "22", "23", "GND"]
    C0 = 20
    used = {"3V3", "GND", "13", "14", "27", "26", "25", "33", "32", "35", "34", "VN"}
    ex0, ex1 = X(C0) - 22, X(C0 + 18) + 34
    ey0, ey1 = R["b"] - 9, R["j"] + 9
    s.rect(ex0, ey0, ex1 - ex0, ey1 - ey0, fill="#1E1E22", stroke="#000000", sw=1.5, rx=4)
    s.rect(X(31) - 4, ey0 + 34, X(C0 + 18) + 8 - X(31), ey1 - ey0 - 62, fill="#2A5C3A", stroke="none", sw=0, rx=2)
    s.rect(X(31) + 2, ey0 + 40, X(37) - X(31), ey1 - ey0 - 74, fill="#C9CCD1", stroke="#9DA2A9", sw=1, rx=2)
    s.text((X(31) + X(37)) / 2 + 2, (ey0 + ey1) / 2, "ESP32-WROOM-32", 7.5, "middle", "#4A4F57", 800)
    s.text((X(31) + X(37)) / 2 + 2, (ey0 + ey1) / 2 + 13, "Schirmblech", 8, "middle", "#6A7079")
    ax = X(37) + 8
    s.poly([(ax, ey0 + 44), (ax + 8, ey0 + 44), (ax + 8, ey0 + 62), (ax, ey0 + 62), (ax, ey0 + 80),
            (ax + 8, ey0 + 80), (ax + 8, ey0 + 98), (ax, ey0 + 98), (ax, ey0 + 116), (ax + 8, ey0 + 116)],
           stroke="#D4A93C", w=2)
    s.rect(ex0 - 14, (ey0 + ey1) / 2 - 16, 26, 32, fill="#C9CCD1", stroke="#8A8F99", sw=1, rx=2)
    s.text(ex0 + 2, (ey0 + ey1) / 2 + 30, "USB", 8, "middle", "#C9CCD1", 700)
    for k, nm in enumerate(("EN", "BOOT")):
        bx = X(C0) + 6 + k * 40
        s.rect(bx, ey1 - 44, 14, 12, fill="#8A8F99", stroke="none", sw=0, rx=2)
        s.circle(bx + 7, ey1 - 38, 3.5, fill="#2B2B2B", stroke="none", sw=0)
        s.text(bx + 7, ey1 - 48, nm, 7, "middle", "#C9CCD1", 700)
    s.text(X(25), (ey0 + ey1) / 2 - 2, "ESP32-DevKitC", 13, "middle", "#FFFFFF", 800)
    s.text(X(25), (ey0 + ey1) / 2 + 13, "38 Pins · USB links", 9, "middle", "#AEB3BC")
    for i, nm in enumerate(top_pins):
        x, y = H("b", C0 + i)
        hl = nm in used
        s.rect(x - 4, y - 4, 8, 8, fill="#E4C25A" if hl else "#9C8A50", stroke="#00000066", sw=1, rx=1)
        s.text(x + 3, y + 10, nm, 7.5, "end", "#FFFFFF" if hl else "#8A8F99", 800 if hl else 400, rot=-90)
    for i, nm in enumerate(bot_pins):
        x, y = H("j", C0 + i)
        s.rect(x - 4, y - 4, 8, 8, fill="#9C8A50", stroke="#00000066", sw=1, rx=1)
        s.text(x + 3, y - 9, nm, 7.5, "start", "#8A8F99", 400, rot=-90)

    def pc(name):
        return C0 + top_pins.index(name)

    # --- Elko C1 auf den Schienen neben dem 3V3-Pin -------------------
    cx = X(44)
    cy = (R["+"] + R["-"]) / 2 - 22
    s.line(cx, R["+"], cx, R["-"], stroke="#9A9A9A", w=2)
    s.circle(cx, cy, 15, fill="#2E3A7A", stroke="#1A2350", sw=1.5)
    s.a(f'<path d="M{cx+5},{cy-13} A15 15 0 0 1 {cx+5},{cy+13}" fill="none" stroke="#D9DCE6" stroke-width="5"/>')
    s.text(cx - 2, cy + 4, "−", 11, "middle", "#D9DCE6", 800)
    s.text(cx - 22, R["+"] - 28, "C1 1500 µF", 10, "end", INK, 700)
    s.text(cx - 22, R["+"] - 16, "Streifen (−) → GND", 9, "end", MUTED)

    # --- ESP-Versorgung -----------------------------------------------
    wire([H("a", pc("3V3")), (X(pc("3V3")), R["+"])], RED)
    wire([H("a", pc("GND")), (X(pc("GND")), R["-"])], "#1B1B1B")

    # --- Akkuteiler R1/R2 ---------------------------------------------
    lane1, lane2 = R["a"] - 13, R["a"] - 20
    resistor(H("c", 50), H("c", 46), "R1 100k")
    resistor(H("d", 46), H("d", 42), "R2 100k", label_dy=17)
    wire([H("b", 42), (X(42), R["-"])], "#1B1B1B")
    wire([H("e", 46), (X(46) + 9, R["e"]), (X(46) + 9, lane1), (X(pc("34")), lane1), H("a", pc("34"))],
         "#F2C200", r=6)
    # optional USB-Erkennung R3/R4
    resistor(H("c", 60), H("c", 56), "R3 100k", opacity=0.55)
    resistor(H("d", 56), H("d", 52), "R4 100k", opacity=0.55, label_dy=17)
    wire([H("b", 52), (X(52), R["-"])], "#1B1B1B", opacity=0.55)
    wire([H("e", 56), (X(56) + 9, R["e"]), (X(56) + 9, lane2), (X(pc("35")), lane2), H("a", pc("35"))],
         "#9B59D0", dash="7 5", opacity=0.8, r=6)

    # --- Display-Adapter (8-pol. HAT-Kabel) ---------------------------
    dx0, dy0 = X(15), 84
    dw = 22 * P + 30
    s.rect(dx0, dy0, dw, 62, fill="#2C2C30", stroke="#000", sw=1.5, rx=5)
    s.text(dx0 + 12, dy0 + 18, "e-Paper HAT / Kabel → Waveshare 7,5″ V3", 11, "start", "#FFFFFF", 800)
    dnames = ["VCC", "GND", "DIN", "CLK", "CS", "DC", "RST", "BUSY"]
    dcol = [RED, "#1B1B1B", "#2F6FDE", "#F2C200", "#2BA84A", "#F07F1A", "#FFFFFF", "#8A5A2B"]
    pxs = [dx0 + 24 + i * 34 for i in range(8)]
    for i, nm in enumerate(dnames):
        s.text(pxs[i], dy0 + 40, nm, 9, "middle", "#E4C25A", 700)
        pad(pxs[i], dy0 + 58, 8, 12)
    yb = dy0 + 64
    wire([(pxs[0], yb), (pxs[0], yb + 12), (X(17), yb + 22), (X(17), R["+"])], dcol[0])
    wire([(pxs[1], yb), (pxs[1], yb + 12), (X(19), yb + 26), (X(19), R["-"])], dcol[1])
    tgt = {"DIN": "14", "CLK": "13", "CS": "33", "DC": "27", "RST": "26", "BUSY": "VN"}
    for i, nm in enumerate(dnames):
        if nm in tgt:
            c = pc(tgt[nm])
            jog = yb + 14 + i * 3
            wire([(pxs[i], yb), (pxs[i], jog), (X(c), jog + 14), H("a", c)], dcol[i])

    # --- BME280 oben links --------------------------------------------
    bx0, by0 = X(1) - 10, 84
    s.rect(bx0, by0, 12 * P + 8, 62, fill="#6B2C8F", stroke="#00000055", sw=1.5, rx=5, dash="6 3")
    s.text(bx0 + 6 * P + 4, by0 + 18, "BME280 (optional)", 11, "middle", "#FFFFFF", 800)
    bnames = ["VCC", "GND", "SCL", "SDA", "CSB", "SDO"]
    bxs = {nm: X(2 + 2 * k) for k, nm in enumerate(bnames)}
    for nm, x in bxs.items():
        s.text(x, by0 + 40, nm, 9, "middle", "#FFFFFF", 700)
        pad(x, by0 + 58, 8, 12)
    byb = by0 + 64
    lane_scl, lane_sda = YA - 18, YA - 10
    wire([(bxs["VCC"], byb), (bxs["VCC"], R["+"])], RED, opacity=0.85)
    wire([(bxs["GND"], byb), (bxs["GND"], R["-"])], "#1B1B1B", opacity=0.85)
    wire([(bxs["SDO"], byb), (bxs["SDO"], R["-"])], "#1B1B1B", opacity=0.85)
    wire([(bxs["SCL"], byb), (bxs["SCL"], lane_scl), (X(pc("25")), lane_scl), H("a", pc("25"))],
         "#2F6FDE", opacity=0.85, r=6)
    wire([(bxs["SDA"], byb), (bxs["SDA"], lane_sda), (X(pc("32")), lane_sda), H("a", pc("32"))],
         "#2BA84A", opacity=0.85, r=6)

    # --- Stromversorgung rechts ---------------------------------------
    # U2 TPS62827: links VOUT/GND auf Hoehe der Schienen, rechts VIN/GND
    tx0, ty0 = 1010, 200
    module(tx0, ty0, 120, 96, "TPS62827", "Buck 3,3 V", "#1D4E9A")
    for nm, y in (("VOUT", R["+"]), ("GND", R["-"])):
        pad(tx0 - 4, y)
        s.text(tx0 + 4, y + 3, nm, 8, "start", "#FFFFFFCC", 700)
    for nm, y in (("VIN", ty0 + 16), ("GND", ty0 + 80)):
        pad(tx0 + 124, y)
        s.text(tx0 + 116, y + 3, nm, 8, "end", "#FFFFFFCC", 700)
    wire([(tx0 - 8, R["+"]), (X(59), R["+"])], RED)
    wire([(tx0 - 8, R["-"]), (X(59), R["-"])], "#1B1B1B")
    # U1 TC4056 (Pads wie auf den gaengigen Modulen: OUT+, B+, B-, OUT-)
    cx0, cy0 = 1190, 200
    module(cx0, cy0, 180, 96, "TC4056", "Lademodul, USB-C", "#2252A8")
    s.rect(cx0 + 180 - 8, cy0 + 32, 22, 32, fill="#C9CCD1", stroke="#8A8F99", sw=1, rx=4)
    pads = {"OUT+": cy0 + 16, "B+": cy0 + 38, "B−": cy0 + 58, "OUT−": cy0 + 80}
    for nm, y in pads.items():
        pad(cx0 + 4, y, 12, 10)
        s.text(cx0 + 14, y + 3, nm, 8, "start", "#FFFFFFCC", 700)
    for nm, x in (("IN+", cx0 + 130), ("IN−", cx0 + 152)):
        pad(x, cy0 + 3, 10, 10)
        s.text(x, cy0 + 20, nm, 8, "middle", "#FFFFFFCC", 700)
    wire([(cx0, pads["OUT+"]), (tx0 + 128, ty0 + 16)], RED)
    wire([(cx0, pads["OUT−"]), (tx0 + 128, ty0 + 80)], "#1B1B1B")
    # VBAT-Abgriff -> R1 (Spalte 50)
    wire([(cx0 + 4, pads["OUT+"]), (cx0 + 4, 178), (X(50), 178), H("a", 50)], "#F07F1A")
    # USB_5V (optional) -> R3 (Spalte 60)
    wire([(cx0 + 130, cy0 - 2), (cx0 + 130, 166), (X(60), 166), H("a", 60)], "#E4007C",
         dash="7 5", opacity=0.75)
    # LiPo
    ly0 = 340
    s.rect(cx0 - 10, ly0, 210, 110, fill="#5C6E86", stroke="#3A4659", sw=1.5, rx=10)
    s.text(cx0 + 95, ly0 + 46, "LiPo 3,7 V · 3500 mAh", 13, "middle", "#FFFFFF", 800)
    s.text(cx0 + 95, ly0 + 64, "EEMB LP104567", 10, "middle", "#DDE3EC")
    s.text(cx0 + 95, ly0 + 86, "Polung vor dem Anstecken prüfen!", 10, "middle", "#FFD5DA", 700)
    wire([(cx0, pads["B+"]), (cx0 - 22, pads["B+"]), (cx0 - 22, ly0 + 24), (cx0 - 10, ly0 + 24)], RED, r=8)
    wire([(cx0, pads["B−"]), (cx0 - 36, pads["B−"]), (cx0 - 36, ly0 + 44), (cx0 - 10, ly0 + 44)], "#1B1B1B", r=8)

    # --- Marker -------------------------------------------------------
    marker(1160, 322, 1)
    marker(X(48), R["e"] + 16, 2)
    marker(cx + 26, R["+"] - 24, 3)
    marker(978, R["+"] - 16, 4)
    marker(dx0 + dw + 18, dy0 + 30, 5)
    marker(ex0 - 34, R["b"], 6)
    marker(bx0 - 16, by0 + 30, 7)
    marker(X(57), 150, 8)

    # --- Schritte + config.h ------------------------------------------
    ly = 560
    s.rect(30, ly - 30, W - 60, HH - ly + 10, fill="#F7F8FA", stroke="#C9CED8", sw=1.5, rx=10)
    s.text(48, ly - 8, "SCHRITTE & HINWEISE", 13, "start", MUTED, 700)
    steps = [
        "TC4056 + LiPo: B+/B− an die Zelle, OUT+/OUT− an VIN/GND des Buck. OUT+ ist zugleich VBAT (orange).",
        "R1/R2 (2 × 100 kΩ): VBAT → R1 → Knoten → R2 → GND-Schiene; Knoten (gelb) zu GPIO34.",
        "C1 1500 µF direkt in die Schienen neben 3V3: langes Bein (+) rot, Streifenseite (−) blau.",
        "Buck-VOUT (3,3 V) auf die rote Schiene, von dort zum 3V3-Pin – nie auf 5V/VIN.",
        "Display: VCC/GND auf die Schienen, DIN→14, CLK→13, CS→33, DC→27, RST→26, BUSY→VN (39).",
        "DevKit mit der Leiste 3V3 … 5V in Reihe b stecken (andere Leiste in j, unbenutzt): Reihe a bleibt je Pin frei.",
        "BME280 (optional): SCL→25, SDA→32, VCC/GND an die Schienen, SDO→GND (Adresse 0x76).",
        "USB-Erkennung (optional, gestrichelt): TC4056 IN+ → R3/R4 → GPIO35, BATT_USB_SENSE_PIN 35.",
    ]
    for i, t in enumerate(steps):
        y = ly + 22 + i * 26
        s.circle(60, y - 4, 11, fill=RED, stroke="none", sw=0)
        s.text(60, y + 0.5, str(i + 1), 12, "middle", "#FFFFFF", 800)
        s.text(82, y, t, 12.5)
    s.text(48, ly + 242, "Ohne Akku: TC4056, LiPo, Buck, R1–R4 und C1 weglassen, Board per Micro-USB versorgen, "
                         "BATT_ENABLED 0. Das Waveshare ESP32 e-Paper Driver Board braucht kein Steckbrett.",
           12, fill=MUTED, italic=True)
    # config.h-Kasten
    kx, ky = 900, ly + 2
    s.rect(kx, ky, 490, 190, fill="#1E2230", stroke="none", sw=0, rx=8)
    s.text(kx + 18, ky + 26, "config.h für diesen Aufbau", 12, "start", "#FFFFFF", 800)
    s.text(kx + 18, ky + 44, "Pins weichen vom Default (Waveshare Driver Board) ab:", 10.5, "start", "#AEB3BC")
    for i, (nm, v, dflt) in enumerate(BB_PINS):
        y = ky + 74 + i * 22
        s.text(kx + 18, y, "#define", 12, "start", "#C792EA", mono=True)
        s.text(kx + 84, y, nm, 12, "start", "#E6E8EE", mono=True)
        s.text(kx + 250, y, str(v), 12, "start", "#F78C6C", 700, mono=True)
        s.text(kx + 282, y, f"// Default {dflt}", 12, "start", "#7F8798", mono=True)
    s.text(kx + 18, ky + 174, "Alle übrigen Pins bleiben wie ausgeliefert.", 10.5, "start", "#AEB3BC")
    return s.out()


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    open(os.path.join(OUT, "schaltplan.svg"), "w", encoding="utf-8").write(schematic())
    open(os.path.join(OUT, "breadboard.svg"), "w", encoding="utf-8").write(breadboard())
    print("ok:", OUT)
