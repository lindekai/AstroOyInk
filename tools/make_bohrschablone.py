#!/usr/bin/env python3
"""Erzeugt die Bohrschablone fuer das Wandgehaeuse als A4-PDF (Massstab 1:1).

Aufruf:  python3 tools/make_bohrschablone.py [hardware/gehaeuse/bohrschablone_wand.pdf]

Masse von Hand aus hardware/gehaeuse/gehaeuse_wand.scad + common.scad
gespiegelt (W, H, schluessel, tc_pos). Bei Aenderungen dort hier nachziehen
und neu erzeugen. Keine Abhaengigkeiten ausser Python 3 (PDF wird direkt
geschrieben).
"""
import sys

OUT = sys.argv[1] if len(sys.argv) > 1 else "hardware/gehaeuse/bohrschablone_wand.pdf"

# --- Masse aus den SCAD-Dateien (mm) --------------------------------------
tol, rand, kinn, oben = 0.3, 5, 6, 6
pnl_w, pnl_h = 170.2, 111.2
W = pnl_w + 2 * tol + 2 * rand                       # 180,8
H = rand + kinn + pnl_h + 2 * tol + oben + rand      # 133,8
R_AUSSEN = 6
KEY_DX = 50            # schluessel = [W/2 -+ 50, H - 24]
KEY_Y = H - 24         # Mitte der Einhaengebohrung (Kopf rein)
KEY_SLOT = 9           # Langloch nach oben, Schraube sitzt am oberen Ende
KEY_HEAD_D, KEY_SLOT_W = 9, 4.5
USB_X = 120 + 12 / 2   # tc_pos[0] + tc_b / 2, Frontansicht
SCREW_Y = KEY_Y + KEY_SLOT   # Lage der Schraube, wenn das Geraet haengt

# --- Blatt ----------------------------------------------------------------
PW, PH = 210.0, 297.0
OX = (PW - W) / 2            # Gehaeuse links unten auf dem Blatt
OY = PH - 54 - H             # Platz fuer Titel und Bemassung
PT = 72 / 25.4


class Pdf:
    def __init__(self):
        self.c = []

    def a(self, s):
        self.c.append(s)

    def lw(self, w):
        self.a(f"{w:.3f} w")

    def gray(self, g, fill=False):
        self.a(f"{g:.2f} {'g' if fill else 'G'}")

    def red(self, fill=False):
        self.a(f"0.78 0.06 0.18 {'rg' if fill else 'RG'}")

    def dash(self, on=None, off=None):
        self.a(f"[{on} {off}] 0 d" if on else "[] 0 d")

    def line(self, x1, y1, x2, y2):
        self.a(f"{x1:.3f} {y1:.3f} m {x2:.3f} {y2:.3f} l S")

    def circle(self, cx, cy, r, op="S"):
        k = 0.5523 * r
        self.a(f"{cx + r:.3f} {cy:.3f} m "
               f"{cx + r:.3f} {cy + k:.3f} {cx + k:.3f} {cy + r:.3f} {cx:.3f} {cy + r:.3f} c "
               f"{cx - k:.3f} {cy + r:.3f} {cx - r:.3f} {cy + k:.3f} {cx - r:.3f} {cy:.3f} c "
               f"{cx - r:.3f} {cy - k:.3f} {cx - k:.3f} {cy - r:.3f} {cx:.3f} {cy - r:.3f} c "
               f"{cx + k:.3f} {cy - r:.3f} {cx + r:.3f} {cy - k:.3f} {cx + r:.3f} {cy:.3f} c {op}")

    def rrect(self, x, y, w, h, r, op="S"):
        k = 0.5523 * r
        self.a(f"{x + r:.3f} {y:.3f} m {x + w - r:.3f} {y:.3f} l "
               f"{x + w - r + k:.3f} {y:.3f} {x + w:.3f} {y + r - k:.3f} {x + w:.3f} {y + r:.3f} c "
               f"{x + w:.3f} {y + h - r:.3f} l "
               f"{x + w:.3f} {y + h - r + k:.3f} {x + w - r + k:.3f} {y + h:.3f} {x + w - r:.3f} {y + h:.3f} c "
               f"{x + r:.3f} {y + h:.3f} l "
               f"{x + r - k:.3f} {y + h:.3f} {x:.3f} {y + h - r + k:.3f} {x:.3f} {y + h - r:.3f} c "
               f"{x:.3f} {y + r:.3f} l "
               f"{x:.3f} {y + r - k:.3f} {x + r - k:.3f} {y:.3f} {x + r:.3f} {y:.3f} c {op}")

    def slot(self, cx, y1, y2, r, op="S"):
        # senkrechtes Langloch, Kreismitten bei y1 (unten) und y2 (oben)
        k = 0.5523 * r
        self.a(f"{cx + r:.3f} {y1:.3f} m {cx + r:.3f} {y2:.3f} l "
               f"{cx + r:.3f} {y2 + k:.3f} {cx + k:.3f} {y2 + r:.3f} {cx:.3f} {y2 + r:.3f} c "
               f"{cx - k:.3f} {y2 + r:.3f} {cx - r:.3f} {y2 + k:.3f} {cx - r:.3f} {y2:.3f} c "
               f"{cx - r:.3f} {y1:.3f} l "
               f"{cx - r:.3f} {y1 - k:.3f} {cx - k:.3f} {y1 - r:.3f} {cx:.3f} {y1 - r:.3f} c "
               f"{cx + k:.3f} {y1 - r:.3f} {cx + r:.3f} {y1 - k:.3f} {cx + r:.3f} {y1:.3f} c {op}")

    def text(self, x, y, s, size=3.5, bold=False, anchor="start", fill=0.1):
        font = "F2" if bold else "F1"
        w = text_width(s, size, bold)
        if anchor == "middle":
            x -= w / 2
        elif anchor == "end":
            x -= w
        s = s.replace("\\", "\\\\").replace("(", "\\(").replace(")", "\\)")
        self.a(f"{fill:.2f} g BT /{font} {size:.2f} Tf {x:.3f} {y:.3f} Td ({s}) Tj ET")


# Helvetica-Breiten (1/1000 em) fuer zentrierten Text, Naeherung reicht
_HW = {c: 556 for c in "0123456789"}
_HW.update({" ": 278, ",": 278, ".": 278, ":": 278, "-": 333, "(": 333, ")": 333, "/": 278,
            "×": 584, "Ø": 778, "–": 556, "°": 400, "%": 889, "≤": 584})


def text_width(s, size, bold=False):
    tot = 0
    for ch in s:
        if ch in _HW:
            tot += _HW[ch]
        elif ch.isupper():
            tot += 722 if bold else 667
        elif ch in "ilj":
            tot += 278 if bold else 222
        elif ch in "mw":
            tot += 889 if bold else 833
        elif ch in "frt":
            tot += 389 if bold else 333
        else:
            tot += 611 if bold else 556
    return tot / 1000 * size


p = Pdf()
p.a(f"{PT:.6f} 0 0 {PT:.6f} 0 0 cm")
p.a("1 J 1 j")

# --- Titel ------------------------------------------------------------------
p.text(OX, PH - 18, "AstroOyInk – Bohrschablone Wandgehäuse", 6, bold=True)
p.text(OX, PH - 25, "Maßstab 1:1 · Mit „Tatsächliche Größe“ / 100 % drucken, NICHT „An Seite anpassen“.", 3.3)
p.text(OX, PH - 30, "Vor dem Bohren die Kontrollstrecke unten nachmessen. Ansicht von vorn (wie das Gerät an der Wand hängt).", 3.3)

# --- Gehaeuseumriss -------------------------------------------------------
p.gray(0.1)
p.lw(0.35)
p.rrect(OX, OY, W, H, R_AUSSEN)
# Sichtfenster grob angedeutet (aktive Flaeche), nur zur Orientierung
aa_w, aa_h = 163.2, 97.92
aa_x = OX + rand + tol + (pnl_w - aa_w) / 2
aa_y = OY + rand + kinn + tol + (pnl_h - aa_h - 3.65)
p.gray(0.75)
p.lw(0.25)
p.dash(1.5, 1.5)
p.a(f"{aa_x:.3f} {aa_y:.3f} {aa_w:.3f} {aa_h:.3f} re S")
p.dash()
p.text(OX + W / 2, aa_y + aa_h / 2 - 22, "Sichtfenster (Display)", 3.2, anchor="middle", fill=0.6)

# Mittellinie senkrecht
cx = OX + W / 2
p.gray(0.55)
p.lw(0.2)
p.a("[4 1.5 0.8 1.5] 0 d")
p.line(cx, OY - 6, cx, OY + H + 6)
p.dash()
p.text(cx + 1.5, OY + H + 3, "Mitte", 2.6, fill=0.45)

# Waagerechte durch die Bohrpunkte (fuer die Wasserwaage)
sy = OY + SCREW_Y
p.red()
p.lw(0.25)
p.dash(3, 1.5)
p.line(OX - 5, sy, OX + W + 5, sy)
p.dash()
p.text(cx - 25, sy + 2.2, "Wasserwaage hier anlegen", 3, anchor="middle", fill=0.35)

# Bohrpunkte
for sx in (cx - KEY_DX, cx + KEY_DX):
    # Schluesselloch im Deckel (Lage, wenn das Geraet haengt)
    p.gray(0.6)
    p.lw(0.2)
    p.dash(1, 1)
    p.circle(sx, sy - KEY_SLOT, KEY_HEAD_D / 2)
    p.slot(sx, sy - KEY_SLOT, sy, KEY_SLOT_W / 2)
    p.dash()
    # Fadenkreuz + Bohrkreis
    p.red()
    p.lw(0.3)
    p.line(sx - 9, sy, sx + 9, sy)
    p.line(sx, sy - 4, sx, sy + 9)
    p.circle(sx, sy, 3.0)
    p.red(fill=True)
    p.circle(sx, sy, 0.45, "f")
    p.text(sx, sy + 11, "BOHREN", 3.4, bold=True, anchor="middle", fill=0.1)

# Bemassung Abstand 100 mm
dy = OY + H + 10
p.gray(0.2)
p.lw(0.2)
for sx in (cx - KEY_DX, cx + KEY_DX):
    p.line(sx, sy + 15, sx, dy + 1.5)
p.line(cx - KEY_DX, dy, cx + KEY_DX, dy)
for sx, d in ((cx - KEY_DX, 1), (cx + KEY_DX, -1)):
    p.a(f"{sx:.3f} {dy:.3f} m {sx + d * 2.5:.3f} {dy + 0.9:.3f} l {sx + d * 2.5:.3f} {dy - 0.9:.3f} l f")
p.a("0.2 g")
p.text(cx - 25, dy + 1.4, "100 mm", 3.2, anchor="middle")

# Abstand Oberkante -> Bohrpunkt
mx = OX + W + 3
p.line(OX + W - 2, OY + H, mx + 1.5, OY + H)
p.line(OX + W + 5, sy, mx + 1.5, sy)
p.line(mx, sy, mx, OY + H)
p.text(mx - 1, OY + H + 2.5, f"{H - SCREW_Y:.0f} mm", 3.0, anchor="end")
p.text(OX + 2, OY + H + 2.5, f"Oberkante Gehäuse · {W:.1f} × {H:.1f} mm".replace(".", ","), 2.8, fill=0.35)

# USB-C und BME-Schlitze an der Unterkante zur Orientierung
ux = OX + USB_X
p.gray(0.3)
p.lw(0.25)
p.rrect(ux - 5.25, OY + 0.8, 10.5, 2.4, 1.2)
p.text(ux, OY + 5, "USB-C (Ladekabel)", 2.8, anchor="middle", fill=0.35)
p.gray(0.6)
for i in range(5):
    p.a(f"{OX + 10 + i * 4:.3f} {OY + 0.8:.3f} 2 2 re S")
p.text(OX + 8, OY + 5, "Lüftung BME280", 2.8, fill=0.5)

# --- Kontrollmasse -----------------------------------------------------------
ky = 62
p.gray(0.1)
p.text(OX, ky + 8, "Kontrolle Druckmaßstab", 4, bold=True)
p.lw(0.3)
p.line(OX, ky, OX + 100, ky)
for i in range(0, 101, 10):
    h = 3 if i % 50 == 0 else 1.8
    p.line(OX + i, ky, OX + i, ky - h)
for i in range(0, 101, 1):
    if i % 10:
        p.line(OX + i, ky, OX + i, ky - 0.9)
for i in (0, 50, 100):
    p.text(OX + i, ky - 6.5, f"{i}", 2.8, anchor="middle")
p.text(OX + 104, ky - 1.2, "muss genau 100 mm sein", 3.0, fill=0.35)
p.a(f"{OX + 150:.3f} {ky - 10:.3f} 30 30 re S")
p.text(OX + 165, ky + 4, "30 × 30 mm", 3.0, anchor="middle", fill=0.35)

# --- Anleitung -------------------------------------------------------------------
y = 42
lines = [
    ("Schrauben:", "2 × Rund-/Linsenkopf, Schaft Ø 3,5–4 mm, Kopf Ø max. 8,5 mm (kein Senkkopf), z. B. 4 × 30 + 5er-Dübel."),
    ("Bohren:", "Schablone mit der Wasserwaage ausrichten, ankleben, beide Punkte durchstechen oder ankörnen."),
    ("Eindrehen:", "Kopf-Unterseite ca. 3 mm vor der Wand stehen lassen (Deckel 2,5 mm + Luft)."),
    ("Einhängen:", "Gerät mit den runden Öffnungen (gestrichelt, 9 mm tiefer) über die Köpfe, nach unten rutschen lassen."),
]
for label, s in lines:
    p.text(OX, y, label, 3.2, bold=True)
    p.text(OX + 21, y, s, 3.0)
    y -= 6
p.text(OX, 12, "Erzeugt mit tools/make_bohrschablone.py aus gehaeuse_wand.scad", 2.5, fill=0.55)

# --- PDF schreiben ---------------------------------------------------------------
content = "\n".join(p.c).encode("cp1252")
objs = [
    b"<< /Type /Catalog /Pages 2 0 R >>",
    b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
    f"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 {PW * PT:.2f} {PH * PT:.2f}] "
    f"/Resources << /Font << /F1 5 0 R /F2 6 0 R >> >> /Contents 4 0 R >>".encode(),
    b"<< /Length " + str(len(content)).encode() + b" >>\nstream\n" + content + b"\nendstream",
    b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>",
    b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>",
]
out = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
offs = []
for i, o in enumerate(objs, 1):
    offs.append(len(out))
    out += f"{i} 0 obj\n".encode() + o + b"\nendobj\n"
xref = len(out)
out += f"xref\n0 {len(objs) + 1}\n0000000000 65535 f \n".encode()
for o in offs:
    out += f"{o:010d} 00000 n \n".encode()
out += f"trailer\n<< /Size {len(objs) + 1} /Root 1 0 R >>\nstartxref\n{xref}\n%%EOF\n".encode()
with open(OUT, "wb") as f:
    f.write(out)
print(f"{OUT}: Gehaeuse {W:.1f} x {H:.1f} mm, Bohrpunkte {2 * KEY_DX} mm Abstand, "
      f"{H - SCREW_Y:.1f} mm unter der Oberkante")
