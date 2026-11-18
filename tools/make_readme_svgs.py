#!/usr/bin/env python3
"""Erzeugt Hero-Banner (hell/dunkel) und Modus-Grafik fuer das AstroOyInk-README."""
import random, sys, os

OUT = sys.argv[1]
FONT = "ui-sans-serif, -apple-system, 'SF Pro Display', 'Segoe UI', Roboto, Helvetica, Arial, sans-serif"
MONO = "ui-monospace, 'SF Mono', Menlo, Consolas, monospace"

THEMES = {
    "dark": dict(bg="#0B1020", bg2="#141B33", grid="#18203A", star="#E9ECF5", title="#F1F3F8",
                 sub="#9AA4C0", eyebrow="#FF5A5F", chip="#151C33", chipline="#2A3458",
                 frame="#2B2F38", bezel="#3A3F4A", shadow="#000000"),
    "light": dict(bg="#F4F5F8", bg2="#E6E9F1", grid="#E3E6EE", star="#9AA3BC", title="#111522",
                  sub="#4E586F", eyebrow="#C8102E", chip="#FFFFFF", chipline="#D3D8E4",
                  frame="#C9CDD6", bezel="#B5BAC5", shadow="#7A8194"),
}
PAPER, INK, RED, GREY = "#ECEBE4", "#1B1B1B", "#C8102E", "#8A8A84"


def panel(x, y, w, h):
    """Stilisiertes E-Paper-Panel im WEATHER-Modus (nur B/W/R)."""
    s = []
    a = s.append
    a(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="4" fill="{PAPER}"/>')
    # Header
    a(f'<rect x="{x+10}" y="{y+10}" width="{w-20}" height="26" fill="{INK}"/>')
    a(f'<text x="{x+20}" y="{y+29}" font-size="14" font-weight="700" fill="{PAPER}">Saarmund</text>')
    a(f'<text x="{x+w/2}" y="{y+29}" text-anchor="middle" font-size="14" font-weight="700" fill="{PAPER}">22:41</text>')
    a(f'<text x="{x+w-20}" y="{y+29}" text-anchor="end" font-size="11" fill="{PAPER}">Update 22:41</text>')
    # Drei Boxen
    bw = (w - 20 - 12) / 3
    by, bh = y + 44, 96
    for i, title in enumerate(["AKTUELL", "NACHT", "MOND"]):
        bx = x + 10 + i * (bw + 6)
        a(f'<rect x="{bx}" y="{by}" width="{bw}" height="{bh}" fill="none" stroke="{INK}" stroke-width="1.5"/>')
        a(f'<text x="{bx+8}" y="{by+16}" font-size="10" font-weight="700" fill="{INK}">{title}</text>')
    b0 = x + 10
    a(f'<text x="{b0+10}" y="{by+62}" font-size="30" font-weight="800" fill="{INK}">3.7°</text>')
    a(f'<text x="{b0+bw-10}" y="{by+48}" text-anchor="end" font-size="10" fill="{INK}">6 km/h</text>')
    a(f'<text x="{b0+bw-10}" y="{by+64}" text-anchor="end" font-size="10" fill="{RED}">78 %</text>')
    a(f'<text x="{b0+bw-10}" y="{by+80}" text-anchor="end" font-size="10" fill="{INK}">1012 hPa</text>')
    b1 = x + 10 + bw + 6
    a(f'<circle cx="{b1+36}" cy="{by+56}" r="25" fill="none" stroke="{INK}" stroke-width="2"/>')
    a(f'<text x="{b1+36}" y="{by+61}" text-anchor="middle" font-size="14" font-weight="800" fill="{INK}">8%</text>')
    a(f'<text x="{b1+70}" y="{by+50}" font-size="20" font-weight="800" fill="{INK}">GUT</text>')
    a(f'<text x="{b1+70}" y="{by+70}" font-size="13" fill="{RED}">★★★★★</text>')
    b2 = x + 10 + 2 * (bw + 6)
    mx, my = b2 + 34, by + 56
    a(f'<circle cx="{mx}" cy="{my}" r="22" fill="{INK}"/>')
    a(f'<path d="M{mx} {my-22} A22 22 0 0 1 {mx} {my+22} A10 22 0 0 0 {mx} {my-22}Z" fill="{PAPER}"/>')
    a(f'<circle cx="{mx}" cy="{my}" r="22" fill="none" stroke="{INK}" stroke-width="1.5"/>')
    a(f'<text x="{b2+64}" y="{by+48}" font-size="11" font-weight="700" fill="{INK}">Zun. Sichel</text>')
    a(f'<text x="{b2+64}" y="{by+64}" font-size="10" fill="{INK}">25 % beleuchtet</text>')
    a(f'<text x="{b2+64}" y="{by+80}" font-size="10" font-weight="700" fill="{INK}">Himmel: dunkel</text>')
    # Wolken-Chart
    cy0, ch = by + bh + 8, 86
    a(f'<rect x="{x+10}" y="{cy0}" width="{w-20}" height="{ch}" fill="none" stroke="{INK}" stroke-width="1.5"/>')
    a(f'<text x="{x+18}" y="{cy0+15}" font-size="10" font-weight="700" fill="{INK}">NACHT - WOLKEN VERLAUF (%)</text>')
    px0, px1, py0, py1 = x + 24, x + w - 24, cy0 + 24, cy0 + ch - 12
    for k in range(5):
        gy = py0 + k * (py1 - py0) / 4
        a(f'<line x1="{px0}" y1="{gy:.1f}" x2="{px1}" y2="{gy:.1f}" stroke="{GREY}" stroke-width="0.8" stroke-dasharray="3 4"/>')
    tot = [70, 55, 30, 12, 6, 4, 8, 5, 10, 22, 35]
    low = [55, 40, 18, 6, 2, 2, 4, 2, 6, 14, 25]
    def pts(vals):
        n = len(vals) - 1
        return " ".join(f"{px0 + i*(px1-px0)/n:.1f},{py1 - v/100*(py1-py0):.1f}" for i, v in enumerate(vals))
    a(f'<polyline points="{pts(low)}" fill="none" stroke="{RED}" stroke-width="1.6"/>')
    a(f'<polyline points="{pts(tot)}" fill="none" stroke="{INK}" stroke-width="2.4" stroke-linejoin="round"/>')
    # Stundentabelle
    ty = cy0 + ch + 8
    a(f'<rect x="{x+10}" y="{ty}" width="{w-20}" height="44" fill="none" stroke="{INK}" stroke-width="1.5"/>')
    hours = ["21", "22", "23", "00", "01", "02", "03", "04"]
    vals = ["40", "18", "9", "4", "6", "5", "12", "30"]
    for i, (hh, vv) in enumerate(zip(hours, vals)):
        hx = x + 40 + i * (w - 70) / 7
        a(f'<text x="{hx:.1f}" y="{ty+18}" text-anchor="middle" font-size="11" font-weight="700" fill="{INK}">{hh}</text>')
        col = RED if int(vv) >= 30 else INK
        a(f'<text x="{hx:.1f}" y="{ty+35}" text-anchor="middle" font-size="11" fill="{col}">{vv}</text>')
    a(f'<text x="{x+40+3*(w-70)/7+16:.1f}" y="{ty+18}" font-size="8" font-weight="700" fill="{RED}">ISS</text>')
    # Forecast
    fy = ty + 52
    fh = y + h - 10 - fy
    a(f'<rect x="{x+10}" y="{fy}" width="{w-20}" height="{fh}" fill="none" stroke="{INK}" stroke-width="1.5"/>')
    days = [("Mi", "GUT", INK), ("Do", "MITTEL", INK), ("Fr", "BAD", RED), ("Sa", "BAD", RED), ("So", "GUT", INK)]
    for i, (d, v, c) in enumerate(days):
        dx = x + 50 + i * (w - 100) / 4
        a(f'<text x="{dx:.1f}" y="{fy+18}" text-anchor="middle" font-size="11" font-weight="700" fill="{INK}">{d}</text>')
        a(f'<text x="{dx:.1f}" y="{fy+34}" text-anchor="middle" font-size="11" font-weight="800" fill="{c}">{v}</text>')
    return "\n    ".join(s)


def hero(theme):
    t = THEMES[theme]
    W, H = 1280, 440
    rnd = random.Random(42)
    stars = []
    for _ in range(90):
        sx, sy = rnd.uniform(0, W), rnd.uniform(0, H)
        if 640 < sx < 1240 and 30 < sy < 410:
            continue
        r = rnd.choice([0.6, 0.8, 1.0, 1.2, 1.6])
        op = rnd.uniform(0.25, 0.9)
        stars.append(f'<circle cx="{sx:.0f}" cy="{sy:.0f}" r="{r}" fill="{t["star"]}" opacity="{op:.2f}"/>')
    # Panel mit Rahmen
    px, py, pw, ph = 690, 48, 520, 344
    chips = [("☁  Astro-Wetter", 168), ("🔭  N.I.N.A.-Session", 202), ("⇄  Auto-Wechsel", 170)]
    chip_svg, cx = [], 72
    for label, cw in chips:
        chip_svg.append(f'<rect x="{cx}" y="314" width="{cw}" height="40" rx="20" fill="{t["chip"]}" stroke="{t["chipline"]}"/>'
                        f'<text x="{cx+cw/2}" y="340" text-anchor="middle" font-size="16" font-weight="600" fill="{t["title"]}">{label}</text>')
        cx += cw + 12
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" role="img" aria-labelledby="t d">
  <title id="t">AstroOyInk</title>
  <desc id="d">E-Ink-Astro-Display: Astro-Wetter-Dashboard und N.I.N.A.-Session-Telemetrie auf einem Waveshare 7,5-Zoll-E-Paper mit ESP32.</desc>
  <defs>
    <radialGradient id="glow" cx="0.78" cy="0.5" r="0.6"><stop offset="0" stop-color="{t["bg2"]}"/><stop offset="1" stop-color="{t["bg"]}"/></radialGradient>
    <filter id="sh" x="-10%" y="-10%" width="120%" height="130%"><feDropShadow dx="0" dy="10" stdDeviation="14" flood-color="{t["shadow"]}" flood-opacity="0.35"/></filter>
  </defs>
  <rect width="{W}" height="{H}" rx="24" fill="url(#glow)"/>
  <g>
    {chr(10).join("    " + s for s in stars).strip()}
  </g>
  <!-- Mondsichel -->
  <g transform="translate(560 92)">
    <circle r="26" fill="{t["eyebrow"]}" opacity="0.9"/>
    <circle cx="11" cy="-7" r="23" fill="{t["bg"]}"/>
  </g>
  <g font-family="{FONT}">
    <text x="72" y="118" font-size="15" font-weight="700" letter-spacing="2.4" fill="{t["eyebrow"]}">ESP32 · WAVESHARE 7,5″ E-PAPER · B/W/R</text>
    <text x="70" y="190" font-size="68" font-weight="800" letter-spacing="-1.8" fill="{t["title"]}">Astro<tspan fill="{t["eyebrow"]}">Oy</tspan>Ink</text>
    <text x="72" y="238" font-size="22" fill="{t["sub"]}">Tagsüber Astro-Wetter, nachts Live-Telemetrie</text>
    <text x="72" y="268" font-size="22" fill="{t["sub"]}">deiner N.I.N.A.-Session – auf E-Ink.</text>
    {"".join(chip_svg)}
  </g>
  <g filter="url(#sh)">
    <rect x="{px-14}" y="{py-14}" width="{pw+28}" height="{ph+28}" rx="10" fill="{t["frame"]}" stroke="{t["bezel"]}"/>
  </g>
  <g font-family="{FONT}">
    {panel(px, py, pw, ph)}
  </g>
</svg>
'''


def modes(theme):
    t = THEMES[theme]
    W, H = 1280, 360
    def card(x, title, accent, lines, foot):
        out = [f'<rect x="{x}" y="60" width="440" height="240" rx="18" fill="{t["chip"]}" stroke="{t["chipline"]}" stroke-width="1.5"/>',
               f'<rect x="{x+32}" y="84" width="56" height="5" rx="2.5" fill="{accent}"/>',
               f'<text x="{x+32}" y="126" font-size="26" font-weight="800" fill="{t["title"]}">{title}</text>']
        for i, ln in enumerate(lines):
            out.append(f'<text x="{x+32}" y="{164+i*30}" font-size="18" fill="{t["sub"]}">{ln}</text>')
        out.append(f'<text x="{x+32}" y="276" font-size="14" font-weight="700" letter-spacing="1.6" fill="{accent}">{foot}</text>')
        return "\n    ".join(out)
    ink = t["title"]
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" role="img" aria-labelledby="t d">
  <title id="t">Die zwei Modi von AstroOyInk</title>
  <desc id="d">WEATHER-Modus und SESSION-Modus, automatischer Wechsel je nachdem, ob N.I.N.A. erreichbar ist und eine Sequenz läuft.</desc>
  <rect width="{W}" height="{H}" rx="24" fill="{t["bg"]}"/>
  <g font-family="{FONT}">
    {card(80, "☁  WEATHER", ink, ["Open-Meteo + 7Timer! Seeing", "Mond nach Meeus, Nacht-Bewertung", "5-Tage-Vorschau, ISS-Überflüge"], "STANDARD · DEEP-SLEEP · 1×/STUNDE")}
    {card(760, "🔭  SESSION", t["eyebrow"], ["Target, Subs, Restzeit, Status", "Kamera, HFR, Guiding-RMS", "Mount, Pier-Seite, Meridian-Flip"], "N.I.N.A. AKTIV · WACH · ALLE 5 MIN")}
    <g stroke="{t["sub"]}" stroke-width="2.5" fill="none" stroke-linecap="round" stroke-linejoin="round">
      <path d="M548 150 H732"/><path d="M720 140 L734 150 L720 160"/>
      <path d="M732 210 H548"/><path d="M560 200 L546 210 L560 220"/>
    </g>
    <text x="640" y="136" text-anchor="middle" font-size="14" fill="{t["sub"]}">Sequenz läuft</text>
    <text x="640" y="244" text-anchor="middle" font-size="14" fill="{t["sub"]}">N.I.N.A. weg</text>
    <text x="640" y="185" text-anchor="middle" font-size="13" font-weight="700" letter-spacing="1.6" fill="{t["eyebrow"]}">AUTO</text>
  </g>
</svg>
'''


os.makedirs(OUT, exist_ok=True)
for th in THEMES:
    open(os.path.join(OUT, f"hero-{th}.svg"), "w").write(hero(th))
    open(os.path.join(OUT, f"modi-{th}.svg"), "w").write(modes(th))
print("ok")
