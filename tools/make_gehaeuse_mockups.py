#!/usr/bin/env python3
"""Mockups fuer die 3D-Druck-Gehaeuse (hardware/gehaeuse/*.scad).

Aufruf:  python3 tools/make_gehaeuse_mockups.py [tisch] [wand]
Ausgabe: docs/images/gehaeuse_<name>_hero.png   (Rendering mit Display-Inhalt)
         docs/images/gehaeuse_<name>_schnitt.svg (Schnittzeichnung)

Benoetigt OpenSCAD (Pfad per Umgebungsvariable OPENSCAD) sowie PIL + numpy.
Alle Geometrie kommt aus den .scad-Dateien; die Schnitte sind echte
projection(cut=true)-Schnitte, keine nachgezeichneten Skizzen.
"""
import math, os, re, subprocess, sys, tempfile
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CASE = os.path.join(ROOT, "hardware", "gehaeuse")
IMG = os.path.join(ROOT, "docs", "images")
OPENSCAD = os.environ.get("OPENSCAD", "/Applications/OpenSCAD-2021.01.app/Contents/MacOS/OpenSCAD")
TMP = tempfile.mkdtemp(prefix="aoi_case_")

FONT = "ui-sans-serif, -apple-system, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif"
MONO = "ui-monospace, 'SF Mono', Menlo, Consolas, monospace"
INK, RED, MUTED, GREY = "#1B1B1B", "#C8102E", "#5B6472", "#8A8F99"


def scad(scadfile, out, defs, extra=()):
    args = [OPENSCAD, "-o", out]
    for k, v in defs.items():
        args += ["-D", f'{k}="{v}"' if isinstance(v, str) else f"{k}={str(v).lower()}"]
    args += list(extra) + [os.path.join(CASE, scadfile)]
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode != 0 or not os.path.exists(out):
        sys.exit(f"OpenSCAD-Fehler ({out}):\n{r.stderr[-2000:]}")
    return out


# =====================================================================
#  Hero-Bild
# =====================================================================

def font(size, bold=False):
    f = "/System/Library/Fonts/Avenir Next.ttc"
    try:
        return ImageFont.truetype(f, size, index=2 if bold else 0)
    except OSError:
        return ImageFont.load_default()


def perspective_coeffs(dst, src):
    # PIL erwartet die Abbildung Ausgabe -> Eingabe
    m, b = [], []
    for (x, y), (u, v) in zip(dst, src):
        m.append([x, y, 1, 0, 0, 0, -u * x, -u * y]); b.append(u)
        m.append([0, 0, 0, x, y, 1, -v * x, -v * y]); b.append(v)
    return np.linalg.solve(np.array(m, float), np.array(b, float)).tolist()


def render(scadfile, camera, size, defs):
    out = os.path.join(TMP, f"r_{abs(hash((camera, tuple(sorted(defs.items()))))) }.png")
    scad(scadfile, out, defs, ["--camera=" + camera, f"--imgsize={size[0]},{size[1]}",
                               "--colorscheme=Tomorrow", "--projection=p"])
    return Image.open(out).convert("RGB")


def hero(scadfile, camera, content_png, out_png, title, subtitle, chips, size=(1600, 1000), zoom=2,
         box=(600, 150, 1540, 900), schatten="boden", bg="#F4F5F8"):
    rs = (size[0] * zoom, size[1] * zoom)
    base = render(scadfile, camera, rs, {"teil": "ansicht"})
    mask = render(scadfile, camera, rs, {"teil": "ansicht", "aa_maske": True})
    a = np.asarray(base).astype(int)
    render_bg = a[2, 2]
    alpha = (np.abs(a - render_bg).sum(axis=2) > 6).astype(np.uint8) * 255
    m = np.asarray(mask).astype(int)
    # Magenta am Farbton erkennen (Helligkeit haengt von der Beleuchtung ab)
    r_, g_, b_ = m[:, :, 0], m[:, :, 1], m[:, :, 2]
    mag = (r_ > 2 * g_ + 30) & (b_ > 2 * g_ + 30) & (np.abs(r_ - b_) < 40)
    ys, xs = np.nonzero(mag)
    s, d = xs + ys, xs - ys
    quad = [(xs[s.argmin()], ys[s.argmin()]), (xs[d.argmax()], ys[d.argmax()]),
            (xs[s.argmax()], ys[s.argmax()]), (xs[d.argmin()], ys[d.argmin()])]
    content = Image.open(content_png).convert("RGB")
    # e-Paper-Look: Papierweiss statt reinem Weiss
    c = np.asarray(content).astype(float)
    paper = np.array([236, 235, 228]) / 255.0
    c = (c / 255.0) * paper * 255
    content = Image.fromarray(c.clip(0, 255).astype(np.uint8))
    cw, ch = content.size
    coeffs = perspective_coeffs(quad, [(0, 0), (cw, 0), (cw, ch), (0, ch)])
    warped = content.transform(rs, Image.PERSPECTIVE, coeffs, Image.BICUBIC)
    obj = base.copy()
    obj.paste(warped, (0, 0), Image.fromarray(mag.astype(np.uint8) * 255))
    obj = obj.convert("RGBA")
    obj.putalpha(Image.fromarray(alpha))
    obj = obj.crop(obj.getbbox())
    bx0, by0, bx1, by1 = box
    f = min((bx1 - bx0) / obj.width, (by1 - by0) / obj.height)
    obj = obj.resize((int(obj.width * f), int(obj.height * f)), Image.LANCZOS)
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    layer.paste(obj, (bx0 + (bx1 - bx0 - obj.width) // 2, by1 - obj.height), obj)
    obj = layer

    # Hintergrund + weicher Schatten
    W, H = size
    canvas = Image.new("RGB", size, bg)
    grad = Image.linear_gradient("L").resize(size)
    top = Image.new("RGB", size, "#FFFFFF")
    canvas = Image.composite(canvas, top, grad)
    al = np.asarray(obj.split()[3])
    yy, xx = np.nonzero(al > 0)
    if schatten == "wand":
        # Schlagschatten auf die Wand, Licht von links oben
        m = Image.fromarray((al > 0).astype(np.uint8) * 110)
        sh = Image.new("L", size, 0)
        sh.paste(m, (22, 30))
        sh = sh.filter(ImageFilter.GaussianBlur(26))
    else:
        sh = Image.new("L", size, 0)
        ImageDraw.Draw(sh).ellipse([xx.min() + 30, yy.max() - 26, xx.max() - 30, yy.max() + 22], fill=120)
        sh = sh.filter(ImageFilter.GaussianBlur(28))
    canvas.paste(Image.new("RGB", size, "#6E7486"), (0, 0), sh)
    canvas.paste(obj, (0, 0), obj)

    dr = ImageDraw.Draw(canvas)
    dr.text((80, 150), "AstroOyInk", font=font(26, True), fill=RED)
    dr.text((80, 184), title, font=font(60, True), fill="#111522")
    y = 280
    for ln in subtitle:
        dr.text((82, y), ln, font=font(24), fill="#4E586F")
        y += 36
    y += 30
    for chip in chips:
        f = font(19, True)
        w = dr.textlength(chip, font=f)
        dr.rounded_rectangle([82, y, 82 + w + 28, y + 36], radius=18, fill="#FFFFFF", outline="#D3D8E4", width=2)
        dr.text((96, y + 7), chip, font=f, fill="#27304A")
        y += 50
    dr.text((W - 70, H - 46), "Mockup – Elektronik nachgemessen, Display nach Datenblatt", font=font(18),
            fill="#8A93A8", anchor="ra")
    canvas.save(out_png, optimize=True)
    print("ok:", out_png)


# =====================================================================
#  Schnittzeichnung
# =====================================================================

def cut_paths(scadfile, gruppe, **cut):
    out = os.path.join(TMP, f"c_{gruppe}_{'_'.join(f'{k}{v}' for k, v in cut.items())}.svg")
    defs = {"teil": "cut", "gruppe": gruppe}
    defs.update(cut)
    scad(scadfile, out, defs)
    txt = open(out).read()
    return re.findall(r'd="([^"]+)"', txt)


class Sheet:
    def __init__(self, w, h):
        self.w, self.h, self.p = w, h, []

    def a(self, s):
        self.p.append(s)

    def text(self, x, y, s, size=12, anchor="start", fill=INK, weight=400, mono=False, italic=False):
        ex = f' font-family="{MONO}"' if mono else ""
        if italic:
            ex += ' font-style="italic"'
        s = s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        self.a(f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" text-anchor="{anchor}" fill="{fill}" '
               f'font-weight="{weight}"{ex}>{s}</text>')

    def line(self, x1, y1, x2, y2, stroke=INK, w=1.2, dash=None, marker=None):
        d = f' stroke-dasharray="{dash}"' if dash else ""
        m = f' marker-end="url(#{marker})"' if marker else ""
        self.a(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{stroke}" '
               f'stroke-width="{w}"{d}{m} stroke-linecap="round"/>')

    def out(self):
        return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {self.w} {self.h}" width="{self.w}" '
                f'height="{self.h}" font-family="{FONT}">\n' + "\n".join(self.p) + "\n</svg>\n")


DEFS = """<defs>
<pattern id="hatchA" width="6" height="6" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">
  <rect width="6" height="6" fill="#E3E1DA"/><line x1="0" y1="0" x2="0" y2="6" stroke="#8C8778" stroke-width="1.1"/></pattern>
<pattern id="hatchB" width="6" height="6" patternUnits="userSpaceOnUse" patternTransform="rotate(-45)">
  <rect width="6" height="6" fill="#EEECE6"/><line x1="0" y1="0" x2="0" y2="6" stroke="#A29C8C" stroke-width="1"/></pattern>
<marker id="pfeil" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
  <path d="M0,0 L10,5 L0,10 z" fill="#2F6FDE"/></marker>
<marker id="pfeilR" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
  <path d="M0,0 L10,5 L0,10 z" fill="#C8102E"/></marker>
<marker id="mass" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
  <path d="M0,1 L10,5 L0,9" fill="none" stroke="#5B6472" stroke-width="1.6"/></marker>
</defs>"""

STYLE = {
    "sockel":  dict(fill="url(#hatchA)", stroke="#3D3A33", sw=0.35),
    "boden":   dict(fill="url(#hatchB)", stroke="#3D3A33", sw=0.35),
    "rahmen":  dict(fill="url(#hatchA)", stroke="#3D3A33", sw=0.35),
    "deckel":  dict(fill="url(#hatchB)", stroke="#3D3A33", sw=0.35),
    "panel":   dict(fill="#1E1E22", stroke="#1E1E22", sw=0.2),
    "ruecken": dict(fill="#7A8090", stroke="#3D4250", sw=0.25),
    "ada":     dict(fill="#1F4FA0", stroke="#123468", sw=0.3),
    "drv":     dict(fill="#2C6E49", stroke="#1C4A30", sw=0.3),
    "lipo":    dict(fill="#B8BEC8", stroke="#6E7686", sw=0.3),
    "tc":      dict(fill="#2252A8", stroke="#173A78", sw=0.3),
    "buck":    dict(fill="#1D4E9A", stroke="#123468", sw=0.3),
    "bme":     dict(fill="#6B2C8F", stroke="#4A1E63", sw=0.3),
}


class View:
    """Eine Ansicht auf dem Blatt: mm-Koordinaten -> Pixel."""

    def __init__(self, sheet, ox, oy, k, clip=None):
        self.s, self.ox, self.oy, self.k, self.clip = sheet, ox, oy, k, clip

    def P(self, u, v):
        return self.ox + u * self.k, self.oy - v * self.k

    def draw(self, scadfile, groups, **cut):
        cid = ""
        if self.clip:
            u0, v0, u1, v1 = self.clip
            x0, y0 = self.P(u0, v1)
            x1, y1 = self.P(u1, v0)
            cid = f"clip{len(self.s.p)}"
            self.s.a(f'<clipPath id="{cid}"><rect x="{x0:.1f}" y="{y0:.1f}" width="{x1-x0:.1f}" '
                     f'height="{y1-y0:.1f}"/></clipPath>')
        g = f'<g clip-path="url(#{cid})">' if cid else "<g>"
        g += f'<g transform="translate({self.ox:.1f},{self.oy:.1f}) scale({self.k})">'
        for grp in groups:
            st = STYLE[grp]
            for d in cut_paths(scadfile, grp, **cut):
                g += (f'<path d="{d}" fill="{st["fill"]}" stroke="{st["stroke"]}" '
                      f'stroke-width="{st["sw"]}" fill-rule="evenodd" stroke-linejoin="round"/>')
        g += "</g></g>"
        self.s.a(g)

    def path(self, pts, stroke, w=2.0, dash=None, fill="none", marker=None):
        d = "M" + " L".join(f"{self.P(u, v)[0]:.1f},{self.P(u, v)[1]:.1f}" for u, v in pts)
        dd = f' stroke-dasharray="{dash}"' if dash else ""
        m = f' marker-end="url(#{marker})"' if marker else ""
        self.s.a(f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="{w}"{dd}{m} '
                 f'stroke-linecap="round" stroke-linejoin="round"/>')

    def callout(self, n, u, v, lx, ly, side="right"):
        # Fuehrungslinie vom Bauteil (mm) zur nummerierten Marke (px relativ zur Ansicht)
        x, y = self.P(u, v)
        mx, my = x + lx, y + ly
        self.s.a(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="2.6" fill="{INK}"/>')
        self.s.line(x, y, mx, my, stroke=INK, w=1)
        marker(self.s, mx, my, n)

    def dim(self, u1, v1, u2, v2, txt, off=(0, 0), tpos=None):
        x1, y1 = self.P(u1, v1)
        x2, y2 = self.P(u2, v2)
        x1 += off[0]; x2 += off[0]; y1 += off[1]; y2 += off[1]
        self.s.a(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{MUTED}" '
                 f'stroke-width="1" marker-start="url(#mass)" marker-end="url(#mass)"/>')
        tx, ty = tpos if tpos else ((x1 + x2) / 2, (y1 + y2) / 2 - 6)
        self.s.text(tx, ty, txt, 11, "middle", MUTED, 700, mono=True)


def marker(s, x, y, n):
    s.a(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="11" fill="{RED}"/>')
    s.text(x, y + 4.5, str(n), 12, "middle", "#FFFFFF", 800)


def title_block(s, x, y, label, sub):
    s.text(x, y, label, 15, "start", INK, 800)
    s.text(x, y + 18, sub, 11.5, "start", MUTED)


# =====================================================================
#  Tischgehaeuse
# =====================================================================

T = dict(neigung=15, b_w=184, b_d=68, b_h=19, boden_t=2.4, schlitz_y=12, schlitz_tiefe=111.2 - 97.92 - 3.65 - 0.3,
         wand=2.4, pnl_t=1.2, tape_t=0.3, rueck_t=2.0, aa_bot=111.2 - 97.92 - 3.65, pnl_h=111.2,
         ada_y=18, drv_x=130.3, drv_y=16, drv_b=29.5)


def slot_world(yl, zl):
    """Schlitz-Koordinaten (Dicke yl, Hoehe zl) -> Welt (y, z)."""
    a = math.radians(T["neigung"])
    return (T["schlitz_y"] + yl * math.cos(a) + zl * math.sin(a),
            T["b_h"] - yl * math.sin(a) + zl * math.cos(a))


TISCH_LEGENDE = [
    "Display (Rohpanel 170,2 × 111,2 × 1,2) – der inaktive untere Rand (~9,6 mm) verschwindet im Schlitz.",
    "Glas steht direkt im Schlitz auf zwei Auflagen links/rechts vom Flexkabel – keine Rückenplatte.",
    "Schlitz 15° geneigt, 9,3 mm tief, 1,7 mm breit, mit Einführfase – oben bleibt alles frei.",
    "Durchlass für das Flexkabel (15 × 25 mm), Mulde hinter der Klebestelle – das Kabel trägt nichts.",
    "Driver Board Rev 3 auf zwei Stegen, USB-C innen (Flashen: Bodenplatte ab oder OTA).",
    "Bodenplatte, 4 × M3-Senkkopf von unten, Mulden für Gummifüße – oben keine Schraube sichtbar.",
    "LiPo 68 × 46 × 10 links, JST-Kabel nach rechts zum Laderegler.",
    "Akku-Mulde mit 0,5 mm Luft rundum – die Zelle wird nicht geklemmt.",
    "Niederhalter-Rippen in der Decke, 0,6 mm über dem Akku.",
    "BME280 (13 × 11) steht senkrecht im Halter vor der Rückwand (Presspassung).",
    "Lüftungsschlitze in Rückwand und rechter Seitenwand, je 3 × waagerecht.",
    "Lüftungsschlitze in der Bodenplatte, 4 ×.",
    "Trennwände stehen auf der Bodenplatte, unten Durchlass für die 4 Sensorleitungen.",
    "TC4056 im Rastrahmen mit Anschlag, USB-C hinten – Wand außen auf 1 mm ausgespart.",
    "TPS62827-Buck im Rastrahmen, direkt verdrahtet.",
    "FPC-Adapter auf 4 Domen (M2,5), 100-mm-Flachbandkabel gefaltet zum Board.",
    "Stiftleisten bleiben dran: Platine 10 mm über dem Boden, 1 mm Luft unter den Stiften.",
    "Niederhalter-Rippe auf dem Blech des ESP32-Moduls.",
]


def tisch_schnitt(out):
    f = "gehaeuse_tisch.scad"
    W, H = 1600, 1330
    s = Sheet(W, H)
    s.a(f'<rect width="{W}" height="{H}" fill="#FFFFFF"/>')
    s.a(DEFS)
    s.text(40, 52, "AstroOyInk – Tischgehäuse · Schnittzeichnung", 26, weight=800)
    s.text(40, 78, "Einbaulage der Elektronik nach gemessenen Teilen · Schnitte 2,6 : 1, Draufsicht 3 : 1 · "
                   "alle Maße in mm · ohne Kleber montiert", 13, fill=MUTED)
    s.text(W - 40, 52, "Mockup", 13, "end", RED, 800)
    k, base_y = 2.6, 560

    # --- A-A: Schlitz, Flexkabel, FPC-Adapter ------------------------
    A = View(s, 70, base_y, k)
    title_block(s, 50, 120, "Schnitt A–A", "Mitte: Display, Flexkabel, FPC-Adapter (x = 92)")
    A.draw(f, ["boden", "ada", "sockel", "panel"], cut_x=92)
    p0 = slot_world(T["pnl_t"] + 0.6, -T["schlitz_tiefe"] + 3)
    p1 = slot_world(T["pnl_t"] + 0.6, -T["schlitz_tiefe"] - 3)
    A.path([p0, p1, (p1[0] + 4, 7.5), (T["ada_y"] + 2.5, 6.2)], "#D9822B", 2.2)
    base = slot_world(0, 0)
    A.path([base, (base[0], base[1] + 62)], GREY, 1, dash="4 3")
    arc = [(base[0] + 42 * math.cos(t), base[1] + 42 * math.sin(t))
           for t in np.linspace(math.radians(90), math.radians(90 - T["neigung"]), 16)]
    A.path(arc, RED, 1.4)
    tx, ty = A.P(base[0] + 5, base[1] + 47)
    s.text(tx, ty, f"{T['neigung']}°", 14, "start", RED, 800)
    A.dim(0, -T["boden_t"], T["b_d"], -T["boden_t"], f"{T['b_d']}", off=(0, 24))
    A.dim(T["b_d"], -T["boden_t"], T["b_d"], T["b_h"], f"{T['b_h'] + T['boden_t']:.1f}", off=(20, 0),
          tpos=(A.P(T["b_d"], 11)[0] + 26, A.P(0, 11)[1] + 4))
    A.callout(1, *slot_world(0.6, 62), -95, -10)
    A.callout(2, *slot_world(0.85, -T["schlitz_tiefe"] + 0.5), 70, -70)
    A.callout(3, *slot_world(-0.4, -1), -60, -36)
    A.callout(4, *p1, -50, 44)
    A.callout(16, 30, 5.5, 50, -60)
    A.callout(6, 50, -1.2, 10, 42)

    # --- B-B: Akku ------------------------------------------------------
    B = View(s, 350, base_y, k)
    title_block(s, 330, 120, "Schnitt B–B", "links: Akku und Niederhalter (x = 40)")
    B.draw(f, ["boden", "lipo", "sockel", "panel"], cut_x=40)
    B.callout(7, 40, 5, 40, -70)
    B.callout(8, 62.3, 2, 30, 50)
    B.callout(9, 51, 14, 10, -110)

    # --- E-E: Driver Board ---------------------------------------------
    E = View(s, 630, base_y, k)
    title_block(s, 610, 120, "Schnitt E–E", "rechts: Driver Board (x = 141)")
    E.draw(f, ["boden", "drv", "sockel", "panel"], cut_x=141)
    E.callout(5, 40, 10.8, 40, -80)
    E.callout(17, 43, 5, 30, 56)
    E.callout(18, 30, 16, 30, -130)

    # --- Detail C: BME-Kammer ------------------------------------------
    C = View(s, 935 - 44 * 7.0, 400, 7.0, clip=(44, -4, 70, 19))
    title_block(s, 915, 120, "Detail C – BME280-Kammer", "Seitenschnitt durch die Kammer (x = 160), 7 : 1")
    C.draw(f, ["boden", "bme", "sockel"], cut_x=160)
    for z in (4.9, 8.9, 12.9):
        C.path([(74, z), (67, z)], "#2F6FDE", 1.6, marker="pfeil")
    for y in (52, 56):
        C.path([(y, -8), (y, -1.5)], "#2F6FDE", 1.6, marker="pfeil")
    C.path([(54, 1), (58, 4), (60.6, 6)], "#2F6FDE", 1.2, dash="3 3")
    C.callout(10, 62, 9, -60, -95)
    C.callout(11, 66.8, 12.9, 40, -70)
    C.callout(12, 54, -1.2, -90, 40)
    C.callout(13, 47.3, 13, -50, -60)
    s.text(935, 520, "Frischluft strömt von unten und von der Seite ein, die Trennwand hält", 12, fill=MUTED,
           italic=True)
    s.text(935, 537, "die Abwärme von ESP32, Laderegler und Akku vom Sensor fern.", 12, fill=MUTED, italic=True)

    # --- Draufsicht -----------------------------------------------------
    D = View(s, 80, 0, 3.0)
    title_block(s, 60, 640, "Draufsicht", "Sockel waagerecht geschnitten (z = 8), Display-Lage gestrichelt · 3 : 1")
    D.oy = 705 + T["b_d"] * 3.0
    D.draw(f, ["boden"], cut_z=1.5)
    for g, z in (("lipo", 5), ("ada", 3.8), ("tc", 2), ("buck", 1.5), ("drv", 12.8), ("bme", 6)):
        D.draw(f, [g], cut_z=z)
    D.draw(f, ["sockel"], cut_z=8)
    D.path([(7, 12.5), (177, 12.5)], INK, 1.2, dash="6 4")
    D.path([(92, 12.5), (92, 18)], "#D9822B", 2.2)
    D.path([(108, 30), (124, 30), (132, 34)], "#B9BEC8", 3.4, dash="2 2")
    x = 116
    D.path([(x, T["b_d"] + 1), (x, T["b_d"] + 9)], RED, 1.6, marker="pfeilR")
    tx, ty = D.P(x, T["b_d"] + 6)
    s.text(tx + 8, ty, "USB-C Laden", 11, "start", RED, 700)
    for (u, v, n) in ((40, 38, 7), (92, 27, 16), (116, 50, 14), (134, 55, 15), (160, 30, 5), (160, 62, 10)):
        marker(s, *D.P(u, v), n)
    D.dim(0, 0, T["b_w"], 0, f"{T['b_w']}", off=(0, 26))
    D.dim(0, 0, 0, T["b_d"], f"{T['b_d']}", off=(-22, 0), tpos=(D.P(0, T["b_d"] / 2)[0] - 36, D.P(0, T["b_d"] / 2)[1]))
    s.text(80, D.oy + 70, "Graues gestricheltes Band: Flachbandkabel vom Adapter zum Board, Rest flach "
                          "gefaltet zwischen Adapter und Board.", 12, fill=MUTED, italic=True)

    # --- Legende ----------------------------------------------------------
    lx, ly = 760, 650
    s.a(f'<rect x="{lx}" y="{ly - 28}" width="800" height="{len(TISCH_LEGENDE) * 31 + 90}" rx="10" '
        f'fill="#F7F8FA" stroke="#C9CED8"/>')
    s.text(lx + 20, ly, "LEGENDE & EINBAUHINWEISE", 13, "start", MUTED, 700)
    for i, t in enumerate(TISCH_LEGENDE):
        y = ly + 30 + i * 31
        marker(s, lx + 34, y - 4, i + 1)
        s.text(lx + 56, y, t, 12.5)
    s.text(lx + 20, ly + 30 + len(TISCH_LEGENDE) * 31 + 12,
           "Druck: Sockel mit der Oberseite aufs Bett, Bodenplatte flach · PETG oder PLA, 0,2 mm · nur 2 Druckteile.",
           12, fill=MUTED, italic=True)
    open(out, "w", encoding="utf-8").write(s.out())
    print("ok:", out)


def tisch():
    hero("gehaeuse_tisch.scad", "92,25,50,68,0,24,410",
         os.path.join(IMG, "display_weather_preview.png"),
         os.path.join(IMG, "gehaeuse_tisch_hero.png"),
         "Tischgehäuse",
         ["Das Display steht frei und ist nur", "unten eingefasst – die Elektronik", "verschwindet im Sockel."],
         ["15° Neigung", "Akku im Sockel", "BME280-Lüftungskammer", "ohne Kleber montiert", "184 × 68 × 21 mm"])
    tisch_schnitt(os.path.join(IMG, "gehaeuse_tisch_schnitt.svg"))


# =====================================================================
#  Wandgehaeuse
# =====================================================================

WD = dict(W=180.8, H=133.8, D=21.0, deckel_t=2.5, rand=5, pnl_y=11.3, front_t=2.0, pnl_t=1.2)

WAND_LEGENDE = [
    "Blende 2 mm, Sichtfenster = aktive Fläche + 0,3 mm, 45°-Fase nach vorne.",
    "Glas liegt unten auf zwei Auflagen links/rechts vom Flexkabel, oben zwei Anschläge.",
    "Andrückrippen am Deckel auf dem inaktiven Glasrand (0,3 mm Luft) – kein Kleber.",
    "Flexkabel läuft in den Freiraum unter dem Glas und biegt nach hinten zum Adapter.",
    "FPC-Adapter auf 4 Abstandshaltern (M2,5), 200-mm-Kabel gefaltet zum Board.",
    "Driver Board auf zwei Stegen, Rastnasen an den Stirnseiten, Stiftleisten frei.",
    "LiPo 68 × 46 × 10 in Mulde, zwei Rastnasenpaare halten die Zelle ohne Druck.",
    "TC4056 links in einer Tasche mit Anschlag, USB-C unten – Wand außen auf 1 mm ausgespart.",
    "TPS62827-Buck im Rastrahmen über dem TC4056 – kurze Wege zum ESP32.",
    "BME280 steht senkrecht im Halter direkt über den Schlitzen der Unterkante.",
    "Lüftungsschlitze: 5 × Unterkante, 3 × linke Seite – warme Luft zieht seitlich ab.",
    "Trennwände enden 1 mm vor Glas bzw. Glasauflage, Kabeldurchlass Richtung ESP32.",
    "Zwei Schlüssellöcher (Abstand 100 mm) mit Polster für den Schraubenkopf.",
    "4 × M3-Senkkopf durch den Deckel in die Dome – von vorne keine Schraube sichtbar.",
]


def wand_schnitt(out):
    f = "gehaeuse_wand.scad"
    Wp, Hp = 1600, 1330
    s = Sheet(Wp, Hp)
    s.a(f'<rect width="{Wp}" height="{Hp}" fill="#FFFFFF"/>')
    s.a(DEFS)
    s.text(40, 52, "AstroOyInk – Wandgehäuse · Schnittzeichnung", 26, weight=800)
    s.text(40, 78, "Einbaulage der Elektronik nach gemessenen Teilen · Schnitte 2,6 : 1, Durchsicht 3 : 1 · "
                   "alle Maße in mm · ohne Kleber montiert", 13, fill=MUTED)
    s.text(Wp - 40, 52, "Mockup", 13, "end", RED, 800)
    k, base_y = 2.6, 520
    D, Wd, Hd = WD["D"], WD["W"], WD["H"]

    # --- A-A Mitte ------------------------------------------------------
    A = View(s, 90, base_y, k)
    title_block(s, 50, 120, "Schnitt A–A", "Mitte: Glas, Flexkabel, FPC-Adapter (x = 90)")
    A.draw(f, ["deckel", "ada", "rahmen", "panel"], cut_x=90.4)
    A.path([(2.9, 11.3), (3.0, 8.2), (4.6, 6.4), (6.6, 6.8), (6.6, 15.5)], "#D9822B", 2.2)
    A.dim(0, -4, D + WD["deckel_t"], -4, f"{D + WD['deckel_t']:.1f}", off=(0, 18))
    A.dim(-4, 0, -4, Hd, f"{Hd:.1f}", off=(-12, 0), tpos=(A.P(-4, 70)[0] - 40, A.P(-4, 70)[1]))
    A.callout(1, 1, 90, -60, -20)
    A.callout(2, 2.6, 10, -60, 40)
    A.callout(3, 6, 121, 60, -40)
    A.callout(4, 4.5, 6.6, 60, 40)
    A.callout(5, 8.4, 24, 60, 10)

    # --- B-B TC4056 + Akku ------------------------------------------------
    B = View(s, 330, base_y, k)
    title_block(s, 300, 120, "Schnitt B–B", "TC4056 und USB-C (x = 47)")
    B.draw(f, ["deckel", "tc", "rahmen", "panel"], cut_x=47)
    B.path([(D - 4.6, -2), (D - 4.6, -12)], RED, 1.6, marker="pfeilR")
    tx, ty = B.P(D - 4.6, -15)
    s.text(tx, ty, "USB-C", 11, "middle", RED, 700)
    B.callout(8, 19.5, 20, 60, 30)

    # --- E-E Board + Schluesselloch --------------------------------------
    E = View(s, 570, base_y, k)
    title_block(s, 540, 120, "Schnitt E–E", "Driver Board und Schlüsselloch (x = 40)")
    E.draw(f, ["deckel", "drv", "rahmen", "panel"], cut_x=40.4)
    E.callout(6, 8.4, 66, 60, -40)
    E.callout(13, D + 1, 114, 50, -40)
    E.callout(14, D + 1, 8.5, 50, 30)

    # --- Detail C BME ------------------------------------------------------
    C = View(s, 870, 420, 6.0, clip=(-3, -6, 26, 30))
    title_block(s, 840, 120, "Detail C – BME280-Kammer", "Seitenschnitt durch die Kammer (x = 19), 6 : 1")
    C.draw(f, ["deckel", "bme", "rahmen", "panel"], cut_x=19)
    for zz in (9, 12.5, 16):
        C.path([(zz, -8), (zz, -1)], "#2F6FDE", 1.6, marker="pfeil")
    C.path([(12, 4), (12, 16), (7, 21)], "#2F6FDE", 1.2, dash="3 3")
    C.callout(10, 14, 8.3, 110, -60)
    C.callout(11, 12, 2.5, 120, 40)
    C.callout(12, 12, 22, 110, -40)
    s.text(840, 560, "Frischluft kommt durch die Unterkante, die warme Luft zieht über die Seitenschlitze ab.", 12,
           fill=MUTED, italic=True)
    s.text(840, 577, "Die Trennwände halten die Abwärme von ESP32, Laderegler und Akku fern.", 12, fill=MUTED,
           italic=True)

    # --- Durchsicht ---------------------------------------------------------
    V = View(s, 70, 0, 3.0)
    title_block(s, 50, 640, "Durchsicht von vorne", "Glas ausgeblendet, Bauteile auf dem Deckel · 3 : 1")
    V.oy = 690 + Hd * 3.0
    V.draw(f, ["deckel"], cut_z=D - 1.0)
    for g, z in (("lipo", 16), ("drv", 8.4), ("ada", 8.4), ("tc", 19.4), ("buck", 19.7), ("bme", 15)):
        V.draw(f, [g], cut_z=z)
    V.draw(f, ["rahmen"], cut_z=12)
    V.path([(90.4, 11.3), (90.4, 16)], "#D9822B", 2.2)
    V.path([(106.4, 23), (112, 30), (112, 44), (68.3, 64)], "#B9BEC8", 3.4, dash="2 2")
    V.path([(47, -1), (47, -9)], RED, 1.6, marker="pfeilR")
    tx, ty = V.P(47, -6)
    s.text(tx + 8, ty + 10, "USB-C Laden", 11, "start", RED, 700)
    for (u, v, n) in ((44, 65, 6), (134, 63, 7), (47, 22, 8), (63, 30, 9), (90.4, 23, 5), (19, 9, 10),
                      (40.4, 114, 13), (140.4, 114, 13)):
        marker(s, *V.P(u, v), n)
    V.dim(0, 0, Wd, 0, f"{Wd:.1f}", off=(0, 30))

    # --- Legende -------------------------------------------------------------
    lx, ly = 700, 650
    s.a(f'<rect x="{lx}" y="{ly - 28}" width="860" height="{len(WAND_LEGENDE) * 31 + 90}" rx="10" '
        f'fill="#F7F8FA" stroke="#C9CED8"/>')
    s.text(lx + 20, ly, "LEGENDE & EINBAUHINWEISE", 13, "start", MUTED, 700)
    for i, t in enumerate(WAND_LEGENDE):
        y = ly + 30 + i * 31
        marker(s, lx + 34, y - 4, i + 1)
        s.text(lx + 56, y, t, 12.5)
    s.text(lx + 20, ly + 30 + len(WAND_LEGENDE) * 31 + 12,
           "Druck: Rahmen mit der Sichtseite aufs Bett, Deckel mit der Außenseite aufs Bett · PETG oder PLA · "
           "2 Druckteile.", 12, fill=MUTED, italic=True)
    open(out, "w", encoding="utf-8").write(s.out())
    print("ok:", out)


def wand():
    hero("gehaeuse_wand.scad", "90,0,67,86,0,160,480",
         os.path.join(IMG, "display_session_preview.png"),
         os.path.join(IMG, "gehaeuse_wand_hero.png"),
         "Wandgehäuse",
         ["Flach wie ein Bilderrahmen – schmale", "Blende, Elektronik und Akku", "unsichtbar im Rücken."],
         ["Schlüssellöcher", "Akku im Rahmen", "BME280-Lüftungskammer", "ohne Kleber montiert",
          "181 × 134 × 24 mm"], schatten="wand", bg="#EEEDE9")
    wand_schnitt(os.path.join(IMG, "gehaeuse_wand_schnitt.svg"))



if __name__ == "__main__":
    wahl = sys.argv[1:] or ["tisch"]
    if "tisch" in wahl:
        tisch()
    if "wand" in wahl:
        wand()
