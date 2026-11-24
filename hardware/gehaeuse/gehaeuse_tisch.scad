// =====================================================================
//  AstroOyInk – Tischgehaeuse
// ---------------------------------------------------------------------
//  Flacher Sockel mit der kompletten Elektronik. Das Glas des Displays
//  steckt nur unten in einem schraeg gestellten Schlitz, oben ist alles
//  frei. Einziger Anschluss: USB-C des Laderegler hinten.
//  Ohne Kleber: Rastnasen, Niederhalter-Rippen, Presspassung, Schrauben.
//
//  Teile (Variable teil):
//    "sockel"  – Sockelschale, Druck mit der Oberseite auf dem Bett
//    "boden"   – Bodenplatte mit Aufnahmen, Druck flach
//    "ansicht" – Zusammenbau fuer die Vorschau
//    "schnitt" – Ansicht mit offenem Sockel (Elektronik sichtbar)
//    "kollision" – Schnittmenge von gruppe und gruppe2 (leer = passt)
//
//  Export:  openscad -D 'teil="sockel"' -o sockel.stl gehaeuse_tisch.scad
// =====================================================================

include <common.scad>

teil = "ansicht";
// Fuer tools/make_gehaeuse_mockups.py
gruppe   = "sockel";    // Bauteilgruppe fuer teil = "cut" und "kollision"
gruppe2  = "boden";     // zweite Gruppe fuer teil = "kollision"
cut_x    = -1;          // Schnitt senkrecht bei x (Seitenansicht) ...
cut_y    = -1;          // ... quer bei y (Vorderansicht) ...
cut_z    = -1;          // ... oder waagerecht bei z (Draufsicht)
aa_maske = false;       // aktive Flaeche magenta (Bildeinpassung)

// --- Parameter ---------------------------------------------------------
neigung       = 15;     // Display nach hinten geneigt (Grad aus der Senkrechten)
b_w           = 184;    // Sockel Breite (Glas 170,2 + Schlitzwaende)
b_d           = 68;     // Sockel Tiefe
b_h           = 19;     // Hoehe Sockelschale (ohne Bodenplatte)
b_r           = 10;     // Eckradius
fase          = 3;      // Fase an der Oberkante
wand          = 2.4;
decke         = 2.4;
boden_t       = 2.4;
schlitz_y     = 12;     // Abstand Vorderkante -> Schlitz (Oberseite)
schlitz_tiefe = aa_bot - 0.3;   // genau der inaktive Rand (9,3 mm)
schlitz_t     = pnl_t + 0.5;    // Glas + Spiel, lehnt durch die Neigung an
schlitz_l     = pnl_w + 0.8;
schraube_d    = 2.6;    // M3 selbstschneidend in die Dome
dom_d         = 8;
ada_schraube  = 2.2;    // M2,5 selbstschneidend fuer den FPC-Adapter
rand_h        = 2;      // Zentrierrand der Bodenplatte
rand_w        = 1.2;

innen_h = b_h - decke;  // lichte Hoehe ueber der Bodenplatte

// Lage der Elektronik (z = 0 = Oberseite Bodenplatte)
lipo_pos = [6, schlitz_y + 3];                 // links, Kabel an der rechten Kurzseite
ada_pos  = [b_w / 2 - ada_l / 2, schlitz_y + 6];   // mittig direkt hinter dem Schlitz
tc_pos   = [110, b_d - wand - 0.5 - tc_l];     // USB-C nach hinten, Buchse buendig mit der Wand
drv_pos  = [b_w - wand - 3 - drv_l, schlitz_y + 4];   // rechts vorne, USB-C innen
drv_z    = drv_stift + 1;                      // Platinenunterseite
buck_pos = [129, b_d - wand - 4.5 - buck_b];   // hinter dem Driver Board
// BME-Kammer hinten rechts: Innenraum von (x, y) bis zu den Aussenwaenden
bme_kam  = [150, drv_pos[1] + drv_b + 2.6];
bme_pos  = [bme_kam[0] + 3, b_d - wand - 6];   // Halter (Sensor steht parallel zur Rueckwand)

// Dome fuer die Bodenschrauben (hinten links neben dem Akku)
dome = [[6, 6], [b_w - 6, 6], [lipo_pos[0] + lipo_l + 7, b_d - 6], [b_w - 6, b_d - 6]];

// --- Grundform ----------------------------------------------------------
module aussen(h) {
  hull() {
    rrect(b_w, b_d, h - fase, b_r);
    translate([fase, fase, 0]) rrect(b_w - 2 * fase, b_d - 2 * fase, h, b_r - fase);
  }
}

// Innenraum des Sockels, o = zusaetzlicher Abstand zu den Waenden
module innen(o = 0) {
  hull() {
    translate([wand + o, wand + o, -1]) rrect(b_w - 2 * (wand + o), b_d - 2 * (wand + o), innen_h - fase + 1, b_r - wand - o);
    translate([wand + fase + o, wand + fase + o, -1])
      rrect(b_w - 2 * (wand + fase + o), b_d - 2 * (wand + fase + o), innen_h + 1, b_r - wand - fase - o);
  }
}

// Koordinatensystem des Schlitzes: x laengs, y Dicke (0 = Vorderseite),
// z entlang der Displayhoehe (0 = Sockeloberseite)
module schlitz_trafo() {
  translate([b_w / 2, schlitz_y, b_h]) rotate([-neigung, 0, 0]) children();
}

module schlitz_koerper() {
  translate([-schlitz_l / 2 - wand, -wand, -schlitz_tiefe - wand])
    cube([schlitz_l + 2 * wand, schlitz_t + 2 * wand, schlitz_tiefe + wand + 4]);
}

module schlitz_aussparung() {
  translate([-schlitz_l / 2, 0, -schlitz_tiefe]) cube([schlitz_l, schlitz_t, schlitz_tiefe + 10]);
  hull() {  // Einfuehrfase
    translate([-schlitz_l / 2, 0, -1]) cube([schlitz_l, schlitz_t, 0.01]);
    translate([-schlitz_l / 2 - 1, -1, 1.5]) cube([schlitz_l + 2, schlitz_t + 2, 0.01]);
  }
  // Durchlass fuer das Panel-Flexkabel im Schlitzboden; das Glas liegt
  // links und rechts davon auf, das Kabel traegt nichts
  translate([-fpc_w / 2, 0, -schlitz_tiefe - wand - 1]) cube([fpc_w, schlitz_t + 1.0, wand + 2]);
  // flache Mulde hinter der Klebestelle des Flexkabels
  translate([-fpc_w / 2, schlitz_t - 0.01, -schlitz_tiefe - 0.01]) cube([fpc_w, 1.0, 7]);
}

// --- BME-Kammer ---------------------------------------------------------
// Die Trennwaende stehen auf der Bodenplatte (flach gedruckt) und enden
// 0,3 mm unter der Decke; der Zentrierrand ist im Kammerbereich offen.
module bme_waende() {
  x = bme_kam[0]; y = bme_kam[1];
  intersection() {
    difference() {
      union() {
        translate([x - 1.6, y - 1.6, 0]) cube([1.6, b_d - y + 1.6, innen_h - 0.3]);   // links
        translate([x - 1.6, y - 1.6, 0]) cube([b_w - x + 1.6, 1.6, innen_h - 0.3]);   // vorne
      }
      translate([x - 3, y + 4, -1]) cube([4, 5, 5]);    // Kabeldurchlass unten
    }
    innen(tol);
  }
}

// Lueftung: Rueckwand (vor dem Sensor) und rechte Seitenwand
module bme_schlitze() {
  for (z = [4, 8, 12]) {
    translate([bme_pos[0] + 1, b_d - wand - 1, z]) cube([bme_l, wand + 2, 1.8]);
    translate([b_w - wand - 1, bme_kam[1] + 2, z]) cube([wand + 2, 6, 1.8]);
  }
}

// --- Oeffnungen ---------------------------------------------------------
module langloch_y(w, h, l) {   // abgerundetes Loch in y-Richtung, Mitte (0, 0)
  r = h / 2;
  hull() for (dx = [-(w / 2 - r), w / 2 - r]) translate([dx, 0, 0]) rotate([90, 0, 0]) cylinder(r = r, h = l);
}

// Einziger Anschluss: USB-C des TC4056 hinten. Die Wand ist um die Buchse
// von aussen auf 1 mm ausgespart, damit auch dicke Stecker ganz einrasten.
// Zum Flashen per Kabel die Bodenplatte abnehmen (oder OTA).
usb_x = tc_pos[0] + tc_b / 2;
usb_z = 1 + tc_t + usbc_h / 2;
module oeffnungen() {
  translate([usb_x, b_d + 1, usb_z]) langloch_y(usbc_w + 1, 4.8, wand + 2);
  translate([usb_x, b_d + 1, usb_z]) langloch_y(13, 7.5, 1 + wand - 1.0);
}

// --- Niederhalter-Rippen an der Decke ----------------------------------
module niederhalter() {
  // Akku: zwei Rippen, 0,6 mm Luft ueber der Zelle
  for (y = [lipo_pos[1] + 10, lipo_pos[1] + lipo_b - 10])
    translate([lipo_pos[0] + 8, y - 1, lipo_t + 0.6]) cube([lipo_l - 16, 2, innen_h - lipo_t - 0.6 + 0.1]);
  // Driver Board: Rippe auf das Blech des ESP32-Moduls (Antennenende links)
  translate([drv_pos[0] + 7, drv_pos[1] + 8, drv_z + drv_t + drv_oben + 0.4])
    cube([2, drv_b - 16, innen_h - (drv_z + drv_t + drv_oben + 0.4) + 0.1]);
}

module sockel() {
  difference() {
    union() {
      difference() { aussen(b_h); innen(); }
      intersection() { aussen(b_h); schlitz_trafo() schlitz_koerper(); }
      for (p = dome) translate([p[0], p[1], 0]) cylinder(d = dom_d, h = innen_h + 0.1);
      niederhalter();
    }
    schlitz_trafo() schlitz_aussparung();
    for (p = dome) translate([p[0], p[1], -1]) cylinder(d = schraube_d, h = innen_h - 2);
    oeffnungen();
    bme_schlitze();
  }
}

// Zentrierrand: greift in den Sockel, an Domen, USB und BME-Kammer offen
module zentrierrand() {
  difference() {
    translate([wand + tol, wand + tol, 0]) rrect(b_w - 2 * (wand + tol), b_d - 2 * (wand + tol), rand_h, b_r - wand - tol);
    translate([wand + tol + rand_w, wand + tol + rand_w, -1])
      rrect(b_w - 2 * (wand + tol + rand_w), b_d - 2 * (wand + tol + rand_w), rand_h + 2, b_r - wand - tol - rand_w);
    for (p = dome) translate([p[0], p[1], -1]) cylinder(d = dom_d + 1, h = rand_h + 2);
    translate([tc_pos[0] - 2, b_d - wand - 3, -1]) cube([tc_b + 4, wand + 4, rand_h + 2]);
    translate([bme_kam[0] - 1.6, bme_kam[1] - 1.6, -1]) cube([b_w, b_d, rand_h + 2]);
  }
}

module boden() {
  difference() {
    union() {
      translate([0, 0, -boden_t]) rrect(b_w, b_d, boden_t, b_r);
      zentrierrand();
      // Akku-Mulde: 0,5 mm Luft rundum; rechts nur hinter dem FPC-Adapter,
      // links nur bis vor die Gehaeuseecke
      translate([lipo_pos[0], lipo_pos[1], 0]) {
        mh = 4;
        translate([-0.5, -2.1, 0]) cube([lipo_l + 1, 1.6, mh]);                 // vorne
        translate([-0.5, lipo_b + 0.5, 0]) cube([lipo_l + 1, 1.6, mh]);         // hinten
        translate([-2.1, -2.1, 0]) cube([1.6, lipo_b - 4, mh]);                 // links
        difference() {                                                           // rechts
          translate([lipo_l + 0.5, ada_pos[1] + ada_b + 0.5 - lipo_pos[1], 0])
            cube([1.6, lipo_b + 2.1 - (ada_pos[1] + ada_b + 0.5 - lipo_pos[1]), mh]);
          translate([lipo_l, lipo_b - 12, 1]) cube([4, 10, mh]);                // Kabelausgang
        }
      }
      // FPC-Adapter auf 4 Domen
      for (dx = [-1, 1], dy = [-1, 1])
        translate([ada_pos[0] + ada_l / 2 + dx * ada_loch[0] / 2, ada_pos[1] + ada_b / 2 + dy * ada_loch[1] / 2, 0])
          cylinder(d = 6, h = 3);
      // TC4056 (laengs in y) im Rastrahmen, Anschlag vorne gegen den Steckdruck
      translate([tc_pos[0] + tc_b, tc_pos[1], 0]) rotate([0, 0, 90]) rastrahmen(tc_l, tc_b, 1, stirn = false);
      translate([tc_pos[0] + 1, tc_pos[1] - 1.2 - tol, 0]) cube([tc_b - 2, 1.2, 1 + tc_t - 0.2]);
      // Buck im Rastrahmen
      translate([buck_pos[0], buck_pos[1], 0]) rastrahmen(buck_l, buck_b, 0.01, buck_h - 0.6);
      // Driver Board: Stege unter den Stirnseiten (zwischen den Stiftreihen)
      for (x = [drv_pos[0], drv_pos[0] + drv_l - 3])
        translate([x, drv_pos[1] + 5, 0]) cube([3, drv_b - 10, drv_z]);
      // Anschlag auf der Antennenseite
      translate([drv_pos[0] - 1.2 - tol, drv_pos[1] + 5, 0]) cube([1.2, drv_b - 10, drv_z + drv_t + 0.8]);
      // BME-Kammer und Sensorhalter (Sensor steht senkrecht, Presspassung)
      bme_waende();
      translate([bme_pos[0], bme_pos[1], 0]) difference() {
        cube([bme_l + 2, 5, 6]);
        translate([1, 1.6, 1]) cube([bme_l + 0.2, 1.7, 6]);
      }
    }
    for (p = dome) translate([p[0], p[1], -boden_t]) senkung_m3(boden_t + 1);
    for (dx = [-1, 1], dy = [-1, 1])
      translate([ada_pos[0] + ada_l / 2 + dx * ada_loch[0] / 2, ada_pos[1] + ada_b / 2 + dy * ada_loch[1] / 2, -1])
        cylinder(d = ada_schraube, h = 5);
    // Lueftung unter der BME-Kammer
    for (i = [0 : 3]) translate([bme_kam[0] + 1.5 + i * 3.6, bme_kam[1] + 2, -boden_t - 1])
      cube([1.8, bme_pos[1] - bme_kam[1] - 3, boden_t + 2]);
    // Mulden fuer Gummifuesse 10 mm
    for (p = fuesse) translate([p[0], p[1], -boden_t - 0.01]) cylinder(d = 10.6, h = 1);
    beschriftung();
  }
}
fuesse = [[14, 12], [b_w - 14, 12], [14, b_d - 12], [b_w - 14, b_d - 12]];

// Beschriftung auf der Unterseite der Bodenplatte, 0,6 mm vertieft.
// Gespiegelt, damit sie lesbar ist, wenn man den Sockel seitlich umdreht.
version   = "v1.0.0-rc1";
schrift   = "Liberation Sans:style=Bold";
text_tief = 0.6;
module beschriftung() {
  zeilen = [   // [Text, Groesse, y]
    ["AstroOyInk",                                        7.0, 46],
    [str(version, " · Tischgehäuse"),                      3.6, 39.5],
    ["Autor: Kai Linde · github.com/lindekai/AstroOyInk",  3.2, 12.5],
    ["Community: Dark Matters Discord · MIT-Lizenz",       3.2, 6]
  ];
  translate([b_w / 2, 0, -boden_t - 0.01]) mirror([1, 0, 0])
    for (z = zeilen) translate([0, z[2], 0]) linear_extrude(text_tief + 0.01)
      text(z[0], size = z[1], font = schrift, halign = "center", valign = "baseline");
}

// --- Vorschau ----------------------------------------------------------
module panel() {
  color("#E4E2DB") cube([pnl_w, pnl_h, pnl_t]);
  color(aa_maske ? "#FF00FF" : "#FFFFFF") translate([aa_side, aa_bot, -0.05]) cube([aa_w, aa_h, 0.05]);
}

module display_im_schlitz() {
  schlitz_trafo() rotate([90, 0, 0]) mirror([0, 0, 1]) {
    // lokal: x laengs, y nach oben, z nach hinten
    translate([-pnl_w / 2, -schlitz_tiefe, 0.25]) panel();
  }
}

// Huellkoerper der Elektronik (Vorschau und Schnitte)
module drv_koerper() {
  translate([0, 0, drv_z]) cube([drv_l, drv_b, drv_t]);
  translate([1, 2, drv_z + drv_t]) cube([20, drv_b - 4, drv_oben]);   // ESP32-Modul
  for (y = [1.3, drv_b - 3.8]) translate([2, y, drv_z - drv_stift]) cube([drv_l - 4, 2.5, drv_stift]);
}
module ada_koerper() {
  translate([0, 0, 3]) cube([ada_l, ada_b, 1.6]);
  translate([3, 3, 4.6]) cube([ada_l - 6, ada_b - 6, ada_h - 1.6]);
}
module tc_koerper() {
  translate([0, 0, 1]) cube([tc_b, tc_l, tc_t]);
  translate([tc_b / 2 - 4.5, tc_l - 7, 1 + tc_t]) cube([9, 7.5, usbc_h - 0.5]);
}
module buck_koerper() {   // liegt auf 0,01-mm-Pads, Rastkante bei buck_h - 0,6
  translate([0, 0, 0.01]) cube([buck_l, buck_b, buck_h - 0.6]);
  translate([1, 1, 0.01]) cube([buck_l - 2, buck_b - 2, buck_h]);
}
module bme_koerper() { translate([bme_pos[0] + 1.1, bme_pos[1] + 1.6, 1]) cube([bme_l, 1.6, bme_b]); }

module elektronik() {
  translate([drv_pos[0], drv_pos[1], 0]) color("#2C6E49") drv_koerper();
  translate([lipo_pos[0], lipo_pos[1], 0]) dummy(lipo_l, lipo_b, lipo_t, "#C9CDD3");
  translate([ada_pos[0], ada_pos[1], 0]) color("#1F4FA0") ada_koerper();
  translate([tc_pos[0], tc_pos[1], 0]) color("#2252A8") tc_koerper();
  translate([buck_pos[0], buck_pos[1], 0]) color("#1D4E9A") buck_koerper();
  color("#6B2C8F") bme_koerper();
}

module gruppe_3d(g) {
  if (g == "sockel") sockel();
  if (g == "boden") boden();
  if (g == "panel") schlitz_trafo() rotate([90, 0, 0]) mirror([0, 0, 1])
    translate([-pnl_w / 2, -schlitz_tiefe, 0.25]) cube([pnl_w, pnl_h, pnl_t]);
  if (g == "drv") translate([drv_pos[0], drv_pos[1], 0]) drv_koerper();
  if (g == "lipo") translate([lipo_pos[0], lipo_pos[1], 0]) cube([lipo_l, lipo_b, lipo_t]);
  if (g == "ada") translate([ada_pos[0], ada_pos[1], 0]) ada_koerper();
  if (g == "tc") translate([tc_pos[0], tc_pos[1], 0]) tc_koerper();
  if (g == "buck") translate([buck_pos[0], buck_pos[1], 0]) buck_koerper();
  if (g == "bme") bme_koerper();
}

if (teil == "cut") {
  if (cut_x >= 0)
    projection(cut = true) multmatrix([[0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, -cut_x], [0, 0, 0, 1]]) gruppe_3d(gruppe);
  else if (cut_y >= 0)
    projection(cut = true) multmatrix([[1, 0, 0, 0], [0, 0, 1, 0], [0, 1, 0, -cut_y], [0, 0, 0, 1]]) gruppe_3d(gruppe);
  else
    projection(cut = true) translate([0, 0, -cut_z]) gruppe_3d(gruppe);
}
else if (teil == "sockel") rotate([180, 0, 0]) translate([0, 0, -b_h]) sockel();
else if (teil == "boden") translate([0, 0, boden_t]) boden();
else if (teil == "schnitt") {
  color("#D9D6CF") difference() { sockel(); translate([-1, -1, innen_h - 0.5]) cube([b_w + 2, b_d + 2, 20]); }
  color("#BDB9B0") boden();
  elektronik();
}
else if (teil == "kollision") intersection() { gruppe_3d(gruppe); gruppe_3d(gruppe2); }
else {
  color("#2B2D31") sockel();
  color("#2B2D31") boden();
  display_im_schlitz();
}
