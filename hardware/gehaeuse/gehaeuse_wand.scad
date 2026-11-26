// =====================================================================
//  AstroOyInk – Wandgehaeuse
// ---------------------------------------------------------------------
//  Flacher Rahmen fuer die Wand. Das Glas liegt vorne an der Blende an,
//  Rippen am Rueckdeckel druecken von hinten auf den inaktiven Rand.
//  Die komplette Elektronik sitzt auf dem Deckel. Ohne Kleber.
//  Einziger Anschluss: USB-C des TC4056 an der Unterkante.
//
//  Koordinaten: x nach rechts, y nach oben, z in die Tiefe (0 = Front).
//
//  Teile (Variable teil):
//    "rahmen"  – Frontrahmen, Druck mit der Sichtseite auf dem Bett
//    "deckel"  – Rueckdeckel mit Aufnahmen, Druck mit der Aussenseite auf dem Bett
//    "ansicht" – Zusammenbau fuer die Vorschau (haengend)
//    "schnitt" – Deckel mit Elektronik (Vorschau)
//
//  Export:  openscad -D 'teil="rahmen"' -o rahmen.stl gehaeuse_wand.scad
// =====================================================================

include <common.scad>

teil = "ansicht";
// Fuer tools/make_gehaeuse_mockups.py
gruppe   = "rahmen";
cut_x    = -1;
cut_y    = -1;
cut_z    = -1;
aa_maske = false;

// --- Parameter ---------------------------------------------------------
rand     = 5;          // Wandstaerke seitlich und unten/oben
kinn     = 6;          // Freiraum unter dem Glas (Flexkabel)
oben     = 6;          // Freiraum ueber dem Glas (Schraubdome)
front_t  = 2;          // Blende
r_aussen = 6;
fase_v   = 1.2;        // Fase an der Frontkante
deckel_t = 2.5;
schraube_d = 2.6;      // M3 selbstschneidend
D        = front_t + pnl_t + 0.8 + drv_stift + 3 + drv_t + drv_oben;  // Rahmentiefe (~21)

W = pnl_w + 2 * tol + 2 * rand;
H = rand + kinn + pnl_h + 2 * tol + oben + rand;
pnl_x = rand + tol;                  // Glas links unten
pnl_y = rand + kinn + tol;
glas_hinten = front_t + pnl_t;       // Rueckseite Glas

// Sichtfenster = aktive Flaeche + 0,3 mm
fen = [pnl_x + aa_side - 0.3, pnl_y + aa_bot - 0.3, aa_w + 0.6, aa_h + 0.6];

dome = [[rand + 3.5, rand + 3], [W - rand - 3.5, rand + 3],
        [rand + 3.5, H - rand - 3], [W - rand - 3.5, H - rand - 3]];
schluessel = [[W / 2 - 50, H - 24], [W / 2 + 50, H - 24]];

// Elektronik (Deckel-Koordinaten: z' = Abstand von der Deckel-Innenseite)
lipo_pos = [100, 40];
drv_pos  = [20, 50];
drv_h    = drv_stift + 3;            // Platinenunterseite ueber dem Deckel
ada_pos  = [W / 2 - ada_l / 2, 14];
ada_h_st = 12;                        // Abstandshalter Adapter
tc_pos   = [120, rand + 0.5];         // USB-C nach unten
buck_pos = [138, 10];
bme_kam  = [rand, rand, 31, 16];      // x, y, Breite, Hoehe der Kammer
bme_pos  = [12, 7.5];                 // Sensor steht senkrecht, 13 mm entlang x

// --- Frontrahmen --------------------------------------------------------
module rahmen_aussen() {
  hull() {
    translate([fase_v, fase_v, 0]) rrect(W - 2 * fase_v, H - 2 * fase_v, 0.01, r_aussen - fase_v);
    translate([0, 0, fase_v]) rrect(W, H, D - fase_v, r_aussen);
  }
}

module fenster() {
  hull() {   // 45-Grad-Fase nach vorne
    translate([fen[0], fen[1], front_t - 0.01]) cube([fen[2], fen[3], 0.02]);
    translate([fen[0] - front_t, fen[1] - front_t, -0.01]) cube([fen[2] + 2 * front_t, fen[3] + 2 * front_t, 0.02]);
  }
  translate([fen[0], fen[1], -1]) cube([fen[2], fen[3], front_t + 2]);
}

module bme_lueftung() {
  // Unterkante: 5 Schlitze unter der Kammer
  for (i = [0 : 4]) translate([10 + i * 4, -1, 7]) cube([2, rand + 2, 11]);
  // linke Seite: 3 Schlitze
  for (y = [8, 12, 16]) translate([-1, y, 7]) cube([rand + 2, 1.8, 11]);
}

module usb_loch() {
  translate([tc_pos[0] + tc_b / 2, -1, D - 1 - tc_t - usbc_h / 2]) rotate([-90, 0, 0])
    hull() for (dx = [-(usbc_w + 1) / 2 + 2.4, (usbc_w + 1) / 2 - 2.4]) translate([dx, 0, 0])
      cylinder(r = 2.4, h = rand + 2);
}

module rahmen() {
  difference() {
    union() {
      difference() {
        rahmen_aussen();
        translate([rand, rand, front_t]) rrect(W - 2 * rand, H - 2 * rand, D, 2);
      }
      for (p = dome) translate([p[0], p[1], front_t]) cylinder(d = 6, h = D - front_t);
      // Auflagen fuer die Glas-Unterkante, links und rechts vom Flexkabel
      for (x = [[rand, W / 2 - fpc_w / 2], [W / 2 + fpc_w / 2, W - rand]])
        translate([x[0], pnl_y - tol - kinn + 0.01, front_t]) cube([x[1] - x[0], kinn - 0.01, pnl_t + 0.8]);
      // Anschlaege oben
      for (x = [W * 0.25, W * 0.75]) translate([x - 8, pnl_y + pnl_h + tol, front_t]) cube([16, oben, pnl_t + 0.8]);
    }
    fenster();
    for (p = dome) translate([p[0], p[1], front_t + 2]) cylinder(d = schraube_d, h = D);
    bme_lueftung();
    usb_loch();
  }
}

// --- Rueckdeckel ---------------------------------------------------------
// Aufbau in Deckel-Koordinaten (z' = 0 Innenseite, waechst Richtung Glas),
// eingebaut wird er gespiegelt bei z = D.
module schluesselloch_schnitt() {
  for (p = schluessel) translate([p[0], p[1], 0]) {
    translate([0, 0, -deckel_t - 1]) cylinder(d = 9, h = deckel_t + 2);
    translate([0, 0, -deckel_t - 1]) rotate([0, 0, 90]) langloch(9, 4.5, deckel_t + 2);
    translate([0, 0, -0.01]) rotate([0, 0, 90]) langloch(9, 9.5, 3);
  }
}

module deckel_inhalt() {
  // Andrueckrippen auf den inaktiven Glasrand (0,3 mm Luft)
  rh = D - glas_hinten - 0.3;
  translate([pnl_x + 0.6, bme_kam[1] + bme_kam[3] + 2, 0]) cube([1.5, pnl_h - 20, rh]);              // links
  translate([pnl_x + pnl_w - 2.1, pnl_y + 6, 0]) cube([1.5, pnl_h - 12, rh]);                         // rechts
  translate([pnl_x + 12, pnl_y + pnl_h - 2.4, 0]) cube([pnl_w - 24, 1.5, rh]);                        // oben
  for (x = [[bme_kam[2] + 8, W / 2 - 20], [W / 2 + 20, tc_pos[0] - 4], [buck_pos[0] + buck_l + 5, pnl_x + pnl_w - 12]])
    translate([x[0], pnl_y + 2, 0]) cube([x[1] - x[0], 1.5, rh]);                                     // unten
  // Zentrierrand (in den Rahmen), an Domen und USB unterbrochen
  difference() {
    translate([rand + tol, rand + tol, 0]) rrect(W - 2 * (rand + tol), H - 2 * (rand + tol), 2, 1.5);
    translate([rand + tol + 1.2, rand + tol + 1.2, -1]) rrect(W - 2 * (rand + tol + 1.2), H - 2 * (rand + tol + 1.2), 4, 1);
    for (p = dome) translate([p[0], p[1], -1]) cylinder(d = 7, h = 4);
    translate([tc_pos[0] - 2, 0, -1]) cube([tc_b + 4, rand + 3, 4]);
    translate([0, 0, -1]) cube([bme_kam[2] + 8, bme_kam[3] + 8, 4]);
  }
  // Akku: Mulde mit Rastnasen an den Langseiten
  translate([lipo_pos[0], lipo_pos[1], 0]) {
    difference() {
      translate([-0.5 - 1.6, -0.5 - 1.6, 0]) cube([lipo_l + 1 + 3.2, lipo_b + 1 + 3.2, 5]);
      translate([-0.5, -0.5, -1]) cube([lipo_l + 1, lipo_b + 1, 7]);
      translate([-3, lipo_b - 12, 1]) cube([5, 10, 6]);    // Kabelausgang links
    }
    for (x = [lipo_l * 0.3, lipo_l * 0.7]) {
      translate([x, -0.5, 0]) rastnase(lipo_t + 0.5, 8);
      translate([x, lipo_b + 0.5, 0]) mirror([0, 1, 0]) rastnase(lipo_t + 0.5, 8);
    }
  }
  // Driver Board: Stege unter den Stirnseiten, Rastnasen an den Stirnseiten
  translate([drv_pos[0], drv_pos[1], 0]) {
    for (x = [0, drv_l - 3]) translate([x, 5, 0]) cube([3, drv_b - 10, drv_h]);
    translate([-tol, drv_b / 2, 0]) rotate([0, 0, -90]) rastnase(drv_h + drv_t, 8);
    translate([drv_l + tol, drv_b / 2, 0]) rotate([0, 0, 90]) rastnase(drv_h + drv_t, 8);
  }
  // FPC-Adapter auf 4 Abstandshaltern (M2,5)
  for (dx = [-1, 1], dy = [-1, 1])
    translate([ada_pos[0] + ada_l / 2 + dx * ada_loch[0] / 2, ada_pos[1] + ada_b / 2 + dy * ada_loch[1] / 2, 0])
      difference() { cylinder(d = 6, h = ada_h_st); translate([0, 0, 2]) cylinder(d = 2.2, h = ada_h_st); }
  // TC4056 (laengs in y, USB-C nach unten) und Buck: Rastrahmen
  translate([tc_pos[0] + tc_b, tc_pos[1], 0]) rotate([0, 0, 90]) rastrahmen(tc_l, tc_b, 1, stirn = false);
  translate([buck_pos[0], buck_pos[1], 0]) rastrahmen(buck_l, buck_b, 0.01, buck_h - 0.6);
  // BME-Kammer: Trennwaende bis kurz vor das Glas, Sensorhalter
  kh = D - glas_hinten - 0.4;
  translate([bme_kam[0] + bme_kam[2], bme_kam[1], 0]) cube([1.6, bme_kam[3] + 1.6, kh]);
  translate([bme_kam[0], bme_kam[1] + bme_kam[3], 0]) cube([bme_kam[2] + 1.6, 1.6, kh]);
  translate([bme_pos[0] - 1, bme_pos[1] - 1.6, 0]) difference() {
    cube([bme_l + 2, 5, 6]);
    translate([1, 1.6, 1]) cube([bme_l + 0.2, 1.7, 6]);
  }
  // Schluessellochpolster
  for (p = schluessel) translate([p[0] - 8, p[1] - 6, 0]) difference() {
    cube([16, 24, 4]);
    translate([8, 6, -0.01]) rotate([0, 0, 90]) langloch(9, 9.5, 3);
  }
}

module deckel_lokal() {
  difference() {
    union() {
      translate([0, 0, -deckel_t]) rrect(W, H, deckel_t, r_aussen);
      deckel_inhalt();
    }
    // Senkung an der Aussenseite (z' = -deckel_t), verschraubt wird von aussen
    for (p = dome) translate([p[0], p[1], -deckel_t]) senkung_m3(deckel_t + 1);
    schluesselloch_schnitt();
  }
}

module deckel() { translate([0, 0, D]) mirror([0, 0, 1]) deckel_lokal(); }

// --- Elektronik (Huellkoerper, Deckel-Koordinaten) -----------------------
module drv_k()  { translate([0, 0, drv_h]) cube([drv_l, drv_b, drv_t]);
                  translate([drv_l - 21, 2, drv_h + drv_t]) cube([20, drv_b - 4, drv_oben]);
                  for (y = [1.3, drv_b - 3.8]) translate([2, y, drv_h - drv_stift]) cube([drv_l - 4, 2.5, drv_stift]); }
module ada_k()  { translate([0, 0, ada_h_st]) cube([ada_l, ada_b, 1.6]);
                  translate([3, 3, ada_h_st + 1.6]) cube([ada_l - 6, ada_b - 6, ada_h - 1.6]); }
module tc_k()   { translate([0, 0, 1]) cube([tc_b, tc_l, tc_t]);
                  translate([tc_b / 2 - 4.5, -0.5, 1 + tc_t]) cube([9, 7.5, usbc_h - 0.5]); }
module bme_k()  { translate([bme_pos[0], bme_pos[1], 1]) cube([bme_l, 1.6, bme_b]); }

module gruppe_lokal(g) {
  if (g == "drv") translate([drv_pos[0], drv_pos[1], 0]) drv_k();
  if (g == "lipo") translate([lipo_pos[0], lipo_pos[1], 0]) cube([lipo_l, lipo_b, lipo_t]);
  if (g == "ada") translate([ada_pos[0], ada_pos[1], 0]) ada_k();
  if (g == "tc") translate([tc_pos[0], tc_pos[1], 0]) tc_k();
  if (g == "buck") translate([buck_pos[0], buck_pos[1], 0]) cube([buck_l, buck_b, buck_h]);
  if (g == "bme") bme_k();
}

module gruppe_3d(g) {
  if (g == "rahmen") rahmen();
  else if (g == "deckel") deckel();
  else if (g == "panel") translate([pnl_x, pnl_y, front_t]) cube([pnl_w, pnl_h, pnl_t]);
  else translate([0, 0, D]) mirror([0, 0, 1]) gruppe_lokal(g);
}

module panel_vorschau() {
  translate([pnl_x, pnl_y, front_t]) color("#E4E2DB") cube([pnl_w, pnl_h, pnl_t]);
  color(aa_maske ? "#FF00FF" : "#FFFFFF") translate([fen[0], fen[1], front_t - 0.05]) cube([fen[2], fen[3], 0.05]);
}

module elektronik() {
  color("#2C6E49") gruppe_3d("drv");
  color("#C9CDD3") gruppe_3d("lipo");
  color("#1F4FA0") gruppe_3d("ada");
  color("#2252A8") gruppe_3d("tc");
  color("#1D4E9A") gruppe_3d("buck");
  color("#6B2C8F") gruppe_3d("bme");
}

if (teil == "cut") {
  if (cut_x >= 0)
    projection(cut = true) multmatrix([[0, 0, 1, 0], [0, 1, 0, 0], [1, 0, 0, -cut_x], [0, 0, 0, 1]]) gruppe_3d(gruppe);
  else if (cut_y >= 0)
    projection(cut = true) multmatrix([[1, 0, 0, 0], [0, 0, 1, 0], [0, 1, 0, -cut_y], [0, 0, 0, 1]]) gruppe_3d(gruppe);
  else
    projection(cut = true) translate([0, 0, -cut_z]) gruppe_3d(gruppe);
}
else if (teil == "rahmen") rahmen();
else if (teil == "deckel") translate([0, 0, deckel_t]) deckel_lokal();
else if (teil == "schnitt") {
  color("#D9D6CF") deckel_lokal();
  translate([0, 0, 0]) {
    color("#2C6E49") gruppe_lokal("drv");
    color("#C9CDD3") gruppe_lokal("lipo");
    color("#1F4FA0") gruppe_lokal("ada");
    color("#2252A8") gruppe_lokal("tc");
    color("#1D4E9A") gruppe_lokal("buck");
    color("#6B2C8F") gruppe_lokal("bme");
  }
}
else {
  // haengend: Front zeigt nach -y, oben ist +z
  rotate([90, 0, 0]) {
    color("#2B2D31") rahmen();
    color("#2B2D31") deckel();
    panel_vorschau();
  }
}
