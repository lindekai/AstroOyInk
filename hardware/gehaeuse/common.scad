// =====================================================================
//  AstroOyInk – gemeinsame Masse fuer Tisch- und Wandgehaeuse (mm)
// ---------------------------------------------------------------------
//  Alle Bauteilmasse sind Annahmen nach Datenblatt bzw. gaengigen
//  Modulen. Vor dem Druck mit den eigenen Teilen nachmessen!
// =====================================================================

$fn = 48;
tol = 0.3;                     // Spiel fuer Passungen

// --- Waveshare 7,5" e-Paper V3 (B/W/R), Rohpanel -----------------------
pnl_w = 170.2;                 // Aussenmass Breite
pnl_h = 111.2;                 // Aussenmass Hoehe
pnl_t = 1.2;                   // Dicke (Glas)
aa_w  = 163.2;                 // aktive Flaeche
aa_h  = 97.92;
aa_side = (pnl_w - aa_w) / 2;  // Rand links/rechts
aa_top  = 3.65;                // Rand oben (Datenblatt pruefen)
aa_bot  = pnl_h - aa_h - aa_top;   // Rand unten, FPC-Seite (~9,6)
fpc_w   = 20;                  // Flexkabel 15 mm + Luft

rueck_t = 2.0;                 // Ruecken-/Andruckplatte
tape_t  = 0.3;                 // doppelseitiges Klebeband

// --- Elektronik (nachgemessen am 19.11.2026) ---------------------------
// Waveshare ESP32 e-Paper Driver Board Rev 3
drv_l = 48.3; drv_b = 29.5; drv_t = 1.6;
drv_oben  = 3.4;               // Bauteile oben (ESP32-Modul)
drv_stift = 9;                 // Stiftleisten unten (bleiben dran)
// FPC-Verlaengerung (Adapterplatine + 200-mm-Flachbandkabel)
ada_l = 32; ada_b = 18; ada_h = 4;
ada_loch = [25, 11]; ada_loch_d = 3;
// LiPo EEMB LP104567, Kabel (JST-PH) an der kurzen Kante
lipo_l = 68; lipo_b = 46; lipo_t = 10;
// TC4056 USB-C, Pads gegenueber der Buchse
tc_l = 30; tc_b = 12; tc_t = 1.6;
usbc_w = 9.5; usbc_h = 4;      // Buchsenhoehe ueber der Platine
// TPS62827-Buck, direkt angeloetet
buck_l = 10; buck_b = 12; buck_h = 3;
// BME280-Breakout, 4 Pins an der kurzen Kante
bme_l = 13; bme_b = 11;

// --- Helfer ------------------------------------------------------------
// Abgerundetes Rechteck, Ursprung links vorne unten
module rrect(w, d, h, r) {
  r2 = max(0.01, min(r, w / 2 - 0.01, d / 2 - 0.01));
  hull() for (x = [r2, w - r2], y = [r2, d - r2])
    translate([x, y, 0]) cylinder(r = r2, h = h);
}

// Langloch entlang x
module langloch(l, d, h) {
  hull() {
    cylinder(d = d, h = h);
    translate([l, 0, 0]) cylinder(d = d, h = h);
  }
}

// Aufnahme fuer eine Platine/Zelle: Innenmass l x b, Wandhoehe h,
// optional Auflagepads (Hoehe pad) in den Ecken
module rahmen(l, b, h = 3, w = 1.2, pad = 0) {
  difference() {
    translate([-w - tol, -w - tol, 0]) cube([l + 2 * (w + tol), b + 2 * (w + tol), h]);
    translate([-tol, -tol, -1]) cube([l + 2 * tol, b + 2 * tol, h + 2]);
    // Kabeldurchlass mittig an einer Langseite
    translate([l / 2 - 4, b - 1, pad + 0.01]) cube([8, w + 3, h]);
  }
  if (pad > 0)
    for (x = [0, l - 4], y = [0, b - 4]) translate([x, y, 0]) cube([4, 4, pad]);
}

// --- Rastnase (Kragarm mit Haken), Haken zeigt nach +y -----------------
module rastnase(h, breite = 6) {
  translate([-breite / 2, -1.4, 0]) cube([breite, 1.4, h + 1.4]);
  hull() {
    translate([-breite / 2, -0.01, h]) cube([breite, 0.7, 0.01]);
    translate([-breite / 2, -0.01, h + 1.3]) cube([breite, 0.01, 0.01]);
  }
}

// Platinenaufnahme ohne Kleber: Stirnwaende + zwei Rastnasen an den Langseiten.
// l entlang x, b entlang y, Platine liegt auf Pads der Hoehe pad.
module rastrahmen(l, b, pad, pcb = 1.6, stirn = true) {
  h = pad + pcb;
  for (x = [0, l - 4], y = [0, b - 4]) translate([x, y, 0]) cube([4, 4, pad]);
  if (stirn) for (x = [-1.2 - tol, l + tol]) translate([x, b / 2 - 4, 0]) cube([1.2, 8, h + 0.6]);
  translate([l / 2, -tol, 0]) rastnase(h);
  translate([l / 2, b + tol, 0]) mirror([0, 1, 0]) rastnase(h);
}

// Senkkopf M3 (Loch von unten), Senkung an z = 0
module senkung_m3(h = 10) {
  translate([0, 0, -0.01]) cylinder(d = 3.4, h = h);
  translate([0, 0, -0.01]) cylinder(d1 = 6.6, d2 = 3.4, h = 1.7);
}

// Platzhalter fuer die Vorschau
module dummy(l, b, h, c = "SteelBlue") {
  color(c, 0.9) cube([l, b, h]);
}
