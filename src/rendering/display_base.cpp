// =====================================================================
//  display_renderer.cpp
//  Astro-Wetter-Dashboard für Waveshare 7.5" E-Paper (B/W/R, 800x480)
//  via GxEPD2.
//
//  Farb-Strategie (Display kann nur B / W / R):
//    - Schwarz : Standardtext, Rahmen, neutrale Symbole
//    - Rot     : Warnungen, schlechte Bedingungen
//    - "Gut"   : durchgehend Schwarz, optisch betont durch
//                * Rahmen / Outline
//                * Stern-Symbole (★)
//                * fette Beschriftung
//
//  Layout (alle Koordinaten als Konstanten weiter unten):
//    +--------------------------------------------------+ y=0
//    | HEADER  Ort | Uhrzeit | Update                   |
//    +--------------------+-----------------------------+ y=28
//    | AKTUELL            | NACHT-SCORE                 |
//    +--------------------+-----------------------------+ y=160
//    | NACHT-WOLKEN-CHART (Gesamt/Low/Mid/High)         |
//    +--------------------------------------------------+ y=286
//    | HOUR TABLE  Stundenwerte Nacht                   |
//    +--------------------------------------------------+ y=335
//    | MINI CHARTS  Temp | Wind | Feuchte | Niederschlag|
//    +--------------------------------------------------+ y=395
//    | FORECAST  4–5 Tage                               |
//    +--------------------------------------------------+ y=445
//    | FAZIT                                            |
//    +--------------------------------------------------+ y=480
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================

#include "display_base.h"
#include "config.h"
#include "time_utils.h"
#include "astro_logic.h"
#include "local_sensor.h"
#include "seeing_api.h"
#include "nina_api.h"

#include <SPI.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// DISPLAY_RED:    Warnfarbe auf weissem Hintergrund.
//                 BWR → GxEPD_RED; BW → GxEPD_BLACK (immer noch sichtbar).
// DISPLAY_RED_BG: Warnfarbe auf schwarzem Hintergrund (z.B. Header).
//                 BWR → GxEPD_RED; BW → GxEPD_WHITE (auf Schwarz sichtbar).
#if DISPLAY_HAS_RED
  #define DISPLAY_RED     GxEPD_RED
  #define DISPLAY_RED_BG  GxEPD_RED
#else
  #define DISPLAY_RED     GxEPD_BLACK
  #define DISPLAY_RED_BG  GxEPD_WHITE
#endif

// ---------------------------------------------------------------------
//  Display-Treiber
//
//  WICHTIG: Waveshare hat mehrere 7.5" 3-Farben-Varianten.
//  Die richtige Treiber-Klasse hängt NICHT von der V2/V3-Revision ab,
//  sondern von Auflösung und Controller:
//
//    GxEPD2_750c_Z08     →  800 x 480   UC8179  (V2 UND V3 "(B)")
//    GxEPD2_750c_Z90     →  880 x 528   GD7965
//    GxEPD2_750c_Z90T7   →  880 x 528   (alternative Revision)
//
//  Für das 800x480 Panel (wie unser Layout erwartet) ist Z08 richtig,
//  auch bei der V3-Revision. Falls DOCH ein anderes Panel vorliegt und
//  das Display nichts anzeigt: einfach die Klasse in allen drei Zeilen
//  unten tauschen.
//
//  RAM-Sparmodus:
//  Der zweite Template-Parameter ist die Höhe des RAM-Framebuffers.
//  Bei voller Höhe (480) belegt das 3-Farben-Panel (800/8)*480*2 = 96 KB
//  DRAM – das passt bei ESP32 zusammen mit WiFi/HTTPS NICHT mehr.
//  Wir wählen HEIGHT / 4 = 120 Zeilen Puffer → ~24 KB statt 96 KB.
//  GxEPD2 rendert dann in 4 Seiten (firstPage/nextPage-Schleife).
// ---------------------------------------------------------------------
static GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT / 4> display(
    GxEPD2_750c_Z08(/*CS=*/EPD_PIN_CS,
                    /*DC=*/EPD_PIN_DC,
                    /*RST=*/EPD_PIN_RST,
                    /*BUSY=*/EPD_PIN_BUSY));

// ---------------------------------------------------------------------
//  Layout-Konstanten
// ---------------------------------------------------------------------
static const int16_t W = 800;
static const int16_t H = 480;

// Header
static const int16_t HDR_Y = 0;
static const int16_t HDR_H = 26;

// Aktuell-Block (links)
static const int16_t CUR_X = 4,   CUR_Y = 30;
static const int16_t CUR_W = 250, CUR_H = 130;

// Astro-Score-Box (Mitte)
static const int16_t AST_X = 258, AST_Y = 30;
static const int16_t AST_W = 284, AST_H = 130;

// Mond-Box (rechts)
static const int16_t MOON_X = 546, MOON_Y = 30;
static const int16_t MOON_W = 250, MOON_H = 130;

// Nacht-Cloud-Chart
static const int16_t CH_X = 4,   CH_Y = 166;
static const int16_t CH_W = 792, CH_H = 118;

// Stundentabelle
static const int16_t TBL_X = 4,   TBL_Y = 288;
static const int16_t TBL_W = 792, TBL_H = 45;

// Mini-Charts (4 Stk., je ~196 breit)
static const int16_t MC_X = 4,   MC_Y = 337;
static const int16_t MC_W = 792, MC_H = 56;

// Forecast (4–5 Tage) — reicht jetzt bis ans untere Display-Ende
static const int16_t FC_X = 4,   FC_Y = 397;
static const int16_t FC_W = 792, FC_H = 80;

// ---------------------------------------------------------------------
//  Hilfsfunktionen Zeichnen
// ---------------------------------------------------------------------
static void drawTextAt(int16_t x, int16_t y, const char* text,
                       const GFXfont* font, uint16_t color = GxEPD_BLACK) {
  display.setFont(font);
  display.setTextColor(color);
  display.setCursor(x, y);
  display.print(text);
}

static void drawTextAt(int16_t x, int16_t y, const String& s,
                       const GFXfont* font, uint16_t color = GxEPD_BLACK) {
  drawTextAt(x, y, s.c_str(), font, color);
}

// rechtsbündig
static void drawTextRight(int16_t xRight, int16_t y, const char* text,
                          const GFXfont* font, uint16_t color = GxEPD_BLACK) {
  display.setFont(font);
  display.setTextColor(color);
  int16_t bx, by; uint16_t bw, bh;
  display.getTextBounds(text, 0, 0, &bx, &by, &bw, &bh);
  display.setCursor(xRight - bw, y);
  display.print(text);
}

static void drawTextCentered(int16_t xCenter, int16_t y, const char* text,
                             const GFXfont* font, uint16_t color = GxEPD_BLACK) {
  display.setFont(font);
  display.setTextColor(color);
  int16_t bx, by; uint16_t bw, bh;
  display.getTextBounds(text, 0, 0, &bx, &by, &bw, &bh);
  display.setCursor(xCenter - bw / 2, y);
  display.print(text);
}

static void drawDashedLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                           uint16_t color, uint8_t dashOn = 6, uint8_t dashOff = 4) {
  // Vereinfachte Bresenham-Variante mit Dash-Counter
  int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int16_t err = dx + dy;
  uint16_t step = 0;
  while (true) {
    if ((step / dashOn) % 2 == 0 || dashOff == 0) {
      // an
      if ((step % (dashOn + dashOff)) < dashOn) display.drawPixel(x0, y0, color);
    }
    if (x0 == x1 && y0 == y1) break;
    int16_t e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
    step++;
  }
}

static void drawDottedLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                           uint16_t color) {
  int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int16_t err = dx + dy;
  uint16_t step = 0;
  while (true) {
    if ((step % 3) == 0) display.drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    int16_t e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
    step++;
  }
}

static void drawHLineDashed(int16_t x, int16_t y, int16_t w, uint16_t color) {
  drawDashedLine(x, y, x + w - 1, y, color);
}

// Einfaches Wolkensymbol
static void drawCloudIcon(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  display.fillCircle(cx - r, cy, r, color);
  display.fillCircle(cx + r, cy, r, color);
  display.fillCircle(cx,     cy - r/2, (int16_t)(r * 1.2f), color);
  display.fillRect(cx - r, cy, 2 * r, r, color);
}

// Sonne
static void drawSunIcon(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  display.fillCircle(cx, cy, r, color);
  // Strahlen
  for (int i = 0; i < 8; i++) {
    float a = (float)i * (PI / 4.0f);
    int16_t x1 = cx + (int16_t)((r + 3) * cos(a));
    int16_t y1 = cy + (int16_t)((r + 3) * sin(a));
    int16_t x2 = cx + (int16_t)((r + 9) * cos(a));
    int16_t y2 = cy + (int16_t)((r + 9) * sin(a));
    display.drawLine(x1, y1, x2, y2, color);
  }
}

// Mond (Halbmond)
static void drawMoonIcon(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  display.fillCircle(cx, cy, r, color);
  display.fillCircle(cx + r / 2, cy - r / 6, r, GxEPD_WHITE);
}

// Stern
static void drawStar(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  // Vereinfachter "Stern" als Plus + X
  display.drawLine(cx - r, cy, cx + r, cy, color);
  display.drawLine(cx, cy - r, cx, cy + r, color);
  display.drawLine(cx - r, cy - r, cx + r, cy + r, color);
  display.drawLine(cx - r, cy + r, cx + r, cy - r, color);
}

// Wind-Pfeil als kleines Symbol
static void drawWindIcon(int16_t x, int16_t y, uint16_t color) {
  display.drawLine(x,     y,     x + 12, y,     color);
  display.drawLine(x + 8, y - 3, x + 12, y,     color);
  display.drawLine(x + 8, y + 3, x + 12, y,     color);
  display.drawLine(x,     y + 4, x + 9,  y + 4, color);
}

// Feuchte (Tropfen)
static void drawDropIcon(int16_t cx, int16_t cy, uint16_t color) {
  display.fillTriangle(cx, cy - 6, cx - 4, cy + 1, cx + 4, cy + 1, color);
  display.fillCircle(cx, cy + 2, 4, color);
}

// ---------------------------------------------------------------------
//  Display-Init
// ---------------------------------------------------------------------
void displayInit() {
  // SPI auf konfigurierten Pins starten
  SPI.end();
  SPI.begin(EPD_PIN_SCK, /*MISO=*/-1, EPD_PIN_MOSI, EPD_PIN_CS);
  display.init(115200, true, 2, false);
  display.setRotation(0);  // 0 = Landscape 800x480
  display.setTextWrap(false);
}

void displaySleep() {
  display.hibernate();
}

// ---------------------------------------------------------------------
//  Batterie-Widget fuer den Header (schwarzer Hintergrund)
//  x, y = linke Oberkante des Icons; Text-Baseline = y + iconH - 1
// ---------------------------------------------------------------------
static void drawBatteryWidget(int16_t x, int16_t y, const BatteryInfo& batt) {
  const int16_t iconW = 22, iconH = 12;
  const int16_t textY = y + iconH - 1;  // Baseline bündig mit Icon-Unterkante

  if (batt.status == BATT_USB) {
    drawTextAt(x, textY, "USB", &FreeSans9pt7b, GxEPD_WHITE);
    return;
  }

  // Batterie-Outline
  display.drawRect(x, y, iconW, iconH, GxEPD_WHITE);
  // Pol-Nocke rechts
  display.fillRect(x + iconW, y + 3, 3, 6, GxEPD_WHITE);

  // Fuellstand (weiss = voll, rot = kritisch/niedrig)
  uint8_t pct = batt.percent;
  int16_t fillW = (int16_t)((int32_t)(iconW - 2) * pct / 100);
  uint16_t fillCol = (pct <= 20) ? DISPLAY_RED_BG : GxEPD_WHITE;
  if (fillW > 0) {
    display.fillRect(x + 1, y + 1, fillW, iconH - 2, fillCol);
  }

  // Prozenttext rechts neben dem Icon
  char buf[8];
  snprintf(buf, sizeof(buf), "%d%%", pct);
  uint16_t textCol = (pct <= 20) ? DISPLAY_RED_BG : GxEPD_WHITE;
  drawTextAt(x + iconW + 5, textY, buf, &FreeSans9pt7b, textCol);
}

// ---------------------------------------------------------------------
//  Einzelblock: HEADER (gemeinsame Basis fuer WEATHER und SESSION)
// ---------------------------------------------------------------------
static void drawHeaderImpl(const char* locationName, time_t fetchedAt,
                           const BatteryInfo& batt) {
  display.fillRect(0, 0, W, HDR_H, GxEPD_BLACK);

  drawTextAt(14, 19, locationName, &FreeSansBold12pt7b, GxEPD_WHITE);

  char hhmm[16];
  formatHHMM(time(nullptr), hhmm, sizeof(hhmm));
  drawTextCentered(W / 2, 19, hhmm, &FreeSansBold12pt7b, GxEPD_WHITE);

  if (batt.status != BATT_UNKNOWN) {
    drawBatteryWidget(575, 7, batt);
  }

  char updBuf[40];
  if (fetchedAt > 0) {
    char tmp[20];
    formatHHMM(fetchedAt, tmp, sizeof(tmp));
    snprintf(updBuf, sizeof(updBuf), "Update  %s", tmp);
  } else {
    snprintf(updBuf, sizeof(updBuf), "Update  --:--");
  }
  drawTextRight(W - 14, 19, updBuf, &FreeSans9pt7b, GxEPD_WHITE);
}

static void drawHeader(const WeatherData& wd, const BatteryInfo& batt) {
  const char* loc = wd.locationName.length() ? wd.locationName.c_str()
                                              : LOCATION_NAME;
  drawHeaderImpl(loc, wd.fetchedAt, batt);
}

// ---------------------------------------------------------------------
//  Findet die "aktuelle" Stunde im hourly-Array (oder die nächstgelegene)
// ---------------------------------------------------------------------
static int16_t findCurrentHourIdx(const WeatherData& wd) {
  if (wd.hourlyCount == 0) return -1;
  time_t now = time(nullptr);
  int16_t best = 0;
  long bestDiff = LONG_MAX;
  for (uint16_t i = 0; i < wd.hourlyCount; i++) {
    long d = labs((long)(wd.hourly[i].ts - now));
    if (d < bestDiff) { bestDiff = d; best = i; }
  }
  return best;
}

// ---------------------------------------------------------------------
//  Einzelblock: AKTUELL
// ---------------------------------------------------------------------
static void drawCurrent(const WeatherData& wd, const LocalSensorData& sensor) {
  // Rahmen
  display.drawRect(CUR_X, CUR_Y, CUR_W, CUR_H, GxEPD_BLACK);
  drawTextAt(CUR_X + 6, CUR_Y + 16, "AKTUELL", &FreeSansBold9pt7b);

  int16_t idx = findCurrentHourIdx(wd);
  if (idx < 0) {
    drawTextAt(CUR_X + 10, CUR_Y + 60, "Keine Daten",
               &FreeSansBold12pt7b, DISPLAY_RED);
    return;
  }
  const HourlyData& h = wd.hourly[idx];

  // Große Temperatur – mit dynamischer Platzierung von ° und C
  char tBuf[12];
  if (isnan(h.tempC)) snprintf(tBuf, sizeof(tBuf), "--");
  else                snprintf(tBuf, sizeof(tBuf), "%.1f", h.tempC);

  const int16_t tempX = CUR_X + 10;
  const int16_t tempY = CUR_Y + 85;
  drawTextAt(tempX, tempY, tBuf, &FreeSansBold24pt7b);

  // Tatsächliche Breite messen
  display.setFont(&FreeSansBold24pt7b);
  int16_t bx, by; uint16_t bw, bh;
  display.getTextBounds(tBuf, tempX, tempY, &bx, &by, &bw, &bh);

  const int16_t unitX = bx + bw + 4;
  display.drawCircle(unitX + 4, tempY - 28, 3, GxEPD_BLACK);
  display.drawCircle(unitX + 4, tempY - 28, 2, GxEPD_BLACK);
  drawTextAt(unitX + 12, tempY - 16, "C", &FreeSansBold12pt7b);

  // Werte rechte Spalte (kompakter, weil Box schmaler)
  int16_t rx = CUR_X + 140;
  int16_t ry = CUR_Y + 40;
  char buf[40];

  drawWindIcon(rx, ry - 4, GxEPD_BLACK);
  snprintf(buf, sizeof(buf), "%.0f km/h", isnan(h.windKmh) ? 0.0f : h.windKmh);
  uint16_t windCol = (h.windKmh > ASTRO_WIND_WARN) ? DISPLAY_RED : GxEPD_BLACK;
  drawTextAt(rx + 22, ry, buf, &FreeSans9pt7b, windCol);

  ry += 22;
  drawDropIcon(rx + 4, ry - 4, GxEPD_BLACK);
  snprintf(buf, sizeof(buf), "%.0f %%", isnan(h.humidity) ? 0.0f : h.humidity);
  uint16_t humCol = (h.humidity > ASTRO_HUMIDITY_WARN) ? DISPLAY_RED : GxEPD_BLACK;
  drawTextAt(rx + 22, ry, buf, &FreeSans9pt7b, humCol);

  ry += 22;
  snprintf(buf, sizeof(buf), "Tau %.1f C", isnan(h.dewpointC) ? 0.0f : h.dewpointC);
  drawTextAt(rx, ry, buf, &FreeSans9pt7b);

  ry += 22;
  if (!isnan(h.pressureHpa)) {
    snprintf(buf, sizeof(buf), "%.0f hPa", h.pressureHpa);
  } else {
    snprintf(buf, sizeof(buf), "-- hPa");
  }
  drawTextAt(rx, ry, buf, &FreeSans9pt7b);

  // INNEN-Zeile am unteren Rand der Box (nur wenn Sensor aktiv und gueltig)
  if (sensor.valid) {
    int16_t sepY = CUR_Y + CUR_H - 22;
    display.drawLine(CUR_X + 2, sepY, CUR_X + CUR_W - 2, sepY, GxEPD_BLACK);
    int16_t txtY = CUR_Y + CUR_H - 8;
    drawTextAt(CUR_X + 6, txtY, "IN:", &FreeSansBold9pt7b);
    char ibuf[40];
    snprintf(ibuf, sizeof(ibuf), " %.1fC  %.0f%%  %.0fhPa",
             sensor.tempC, sensor.humidity, sensor.pressureHPa);
    drawTextAt(CUR_X + 6 + 28, txtY, ibuf, &FreeSans9pt7b);
  }
}

// ---------------------------------------------------------------------
//  Einzelblock: ASTRO-SCORE
// ---------------------------------------------------------------------
static void drawAstroBox(const WeatherData& wd) {
  display.drawRect(AST_X, AST_Y, AST_W, AST_H, GxEPD_BLACK);
  drawTextAt(AST_X + 6, AST_Y + 16, "NACHT-BEWERTUNG", &FreeSansBold9pt7b);

  const AstroSummary& s = wd.summary;

  // Großer Score-Kreis links – kleiner, damit Platz fuer Text bleibt
  int16_t cx = AST_X + 45, cy = AST_Y + 72;
  display.drawCircle(cx, cy, 40, GxEPD_BLACK);
  display.drawCircle(cx, cy, 39, GxEPD_BLACK);

  if (s.verdict == ASTRO_NO_NIGHT) {
    drawTextCentered(cx, cy + 6, "--", &FreeSansBold18pt7b);
  } else {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)round(s.avgCloudScore));
    drawTextCentered(cx, cy + 6, buf, &FreeSansBold18pt7b);
  }

  // Verdict-Text + Symbole
  int16_t tx = AST_X + 100;
  int16_t ty = AST_Y + 50;
  const char* label;
  uint16_t labelCol = GxEPD_BLACK;
  switch (s.verdict) {
    case ASTRO_GOOD:     label = "GUT";      break;
    case ASTRO_MODERATE: label = "MITTEL";   break;
    case ASTRO_BAD:      label = "SCHLECHT"; labelCol = DISPLAY_RED; break;
    default:             label = "KEINE NACHT";
  }
  drawTextAt(tx, ty, label, &FreeSansBold18pt7b, labelCol);

  // Sterne eine Zeile unter dem Label – mit Luft dazwischen
  int16_t sy = ty + 22;
  if (s.verdict == ASTRO_GOOD) {
    for (int i = 0; i < 5; i++) drawStar(tx + 8 + i * 20, sy, 6, GxEPD_BLACK);
  } else if (s.verdict == ASTRO_MODERATE) {
    for (int i = 0; i < 3; i++) drawStar(tx + 8 + i * 20, sy, 6, GxEPD_BLACK);
  } else if (s.verdict == ASTRO_BAD) {
    drawStar(tx + 8, sy, 6, DISPLAY_RED);
  }

  // Nachtfenster + Stundenanzahl (knapp halten, damit das ')' nicht
  // am rechten Rand abgeschnitten wird)
  ty += 44;
  if (s.nightStart > 0 && s.nightEnd > 0) {
    char a[8], b[8];
    formatHHMM(s.nightStart, a, sizeof(a));
    formatHHMM(s.nightEnd,   b, sizeof(b));
    char buf[60];
    snprintf(buf, sizeof(buf), "Nacht %s-%s  %uh",
             a, b, s.nightHourCount);
    drawTextAt(tx, ty, buf, &FreeSans9pt7b);
  }

  // Warnungen
  ty += 20;
  if (s.windWarn) {
    drawTextAt(tx, ty, "Wind > 25 km/h", &FreeSansBold9pt7b, DISPLAY_RED);
    ty += 16;
  }
  if (s.humidityWarn) {
    drawTextAt(tx, ty, "Feuchte > 90%  (Tau!)", &FreeSansBold9pt7b, DISPLAY_RED);
  }
}

// ---------------------------------------------------------------------
//  Hilfsfkt: ermittelt die hourly-Indizes, die im Nachtfenster liegen
// ---------------------------------------------------------------------
static uint16_t collectNightIndices(const WeatherData& wd, uint16_t* outIdx, uint16_t maxOut) {
  uint16_t cnt = 0;
  for (uint16_t i = 0; i < wd.hourlyCount && cnt < maxOut; i++) {
    if (wd.astroHourly[i].inNightWindow) outIdx[cnt++] = i;
  }
  return cnt;
}

// ---------------------------------------------------------------------
//  Einzelblock: NACHT-CHART (Cloud-Verläufe)
//  X-Achse: Stunden im Nachtfenster
//  Y-Achse: 0..100 %
//  Linien: Total (schwarz solid), Low (rot solid),
//          Mid (schwarz gepunktet), High (schwarz gestrichelt)
// ---------------------------------------------------------------------
static void drawNightChart(const WeatherData& wd, const SeeingData& sd) {
  display.drawRect(CH_X, CH_Y, CH_W, CH_H, GxEPD_BLACK);
  drawTextAt(CH_X + 6, CH_Y + 16, "NACHT - WOLKEN VERLAUF (%)", &FreeSans9pt7b);

  // Plot-Bereich
  const int16_t pad = 28;
  int16_t px = CH_X + pad;
  int16_t py = CH_Y + 22;
  int16_t pw = CH_W - pad - 20;
  int16_t ph = CH_H - 22 - 18;

  // Achsen
  display.drawLine(px,        py,        px,        py + ph, GxEPD_BLACK);
  display.drawLine(px,        py + ph,   px + pw,   py + ph, GxEPD_BLACK);

  // Y-Gitter 25/50/75/100
  for (int v = 0; v <= 100; v += 25) {
    int16_t y = py + ph - (int16_t)(ph * v / 100);
    drawHLineDashed(px + 1, y, pw - 1, GxEPD_BLACK);
    char buf[8]; snprintf(buf, sizeof(buf), "%d", v);
    drawTextRight(px - 2, y + 4, buf, &FreeSans9pt7b);
  }

  // Indizes der Nachtstunden
  uint16_t idx[64];
  uint16_t n = collectNightIndices(wd, idx, 64);

  if (n < 2) {
    drawTextAt(px + 10, py + ph / 2, "Keine Nachtstunden im Datensatz.",
               &FreeSans9pt7b, DISPLAY_RED);
  } else {
    // Hilfsfkt: x-Pos einer Nachtstunde im Plot
    auto xOf = [&](uint16_t k) -> int16_t {
      if (n == 1) return px + pw / 2;
      return px + (int16_t)((long)pw * k / (n - 1));
    };
    auto yOf = [&](float v) -> int16_t {
      if (v < 0) v = 0; if (v > 100) v = 100;
      return py + ph - (int16_t)(ph * v / 100.0f);
    };

    // X-Achsen-Beschriftung (Stundenlabels)
    for (uint16_t k = 0; k < n; k++) {
      if (k % 2 == 0 || k == n - 1) {
        char hhmm[8];
        formatHHMM(wd.hourly[idx[k]].ts, hhmm, sizeof(hhmm));
        drawTextCentered(xOf(k), py + ph + 14, hhmm, &FreeSans9pt7b);
      }
    }

    // 4 Serien zeichnen
    for (uint16_t k = 1; k < n; k++) {
      int16_t x0 = xOf(k - 1), x1 = xOf(k);

      const HourlyData& h0 = wd.hourly[idx[k - 1]];
      const HourlyData& h1 = wd.hourly[idx[k]];

      // Total (gewichtet) – schwarz solid 2px
      float t0 = wd.astroHourly[idx[k - 1]].cloudScore;
      float t1 = wd.astroHourly[idx[k]].cloudScore;
      display.drawLine(x0, yOf(t0),     x1, yOf(t1),     GxEPD_BLACK);
      display.drawLine(x0, yOf(t0) - 1, x1, yOf(t1) - 1, GxEPD_BLACK);

      // Low – rot solid
      display.drawLine(x0, yOf(h0.cloudLow),  x1, yOf(h1.cloudLow),  DISPLAY_RED);

      // Mid – schwarz gepunktet
      drawDottedLine(x0, yOf(h0.cloudMid),  x1, yOf(h1.cloudMid),  GxEPD_BLACK);

      // High – schwarz gestrichelt
      drawDashedLine(x0, yOf(h0.cloudHigh), x1, yOf(h1.cloudHigh), GxEPD_BLACK);
    }

    // Seeing + Transparenz aus 7Timer! (3-h-Intervall, normiert auf 0..100 %)
    // Skala: 1 = exzellent (unten = 0 %), 8 = schlecht (oben = 100 %)
    // konsistent mit Wolken: "schlecht = oben"
    if (sd.valid && sd.count >= 2) {
      time_t t0 = wd.hourly[idx[0]].ts;
      time_t t1 = wd.hourly[idx[n - 1]].ts;
      if (t1 > t0) {
        auto tsToX = [&](time_t t) -> int16_t {
          float r = (float)(t - t0) / (float)(t1 - t0);
          if (r < 0.0f) r = 0.0f;
          if (r > 1.0f) r = 1.0f;
          return px + (int16_t)(r * (float)pw);
        };
        auto sToY = [&](uint8_t v) -> int16_t {
          float pct = (float)(v - 1) / 7.0f * 100.0f;
          return py + ph - (int16_t)((float)ph * pct / 100.0f);
        };
        for (uint8_t i = 1; i < sd.count; i++) {
          time_t ta = sd.points[i - 1].ts;
          time_t tb = sd.points[i].ts;
          if (tb < t0 - 10800L || ta > t1 + 10800L) continue;
          int16_t xa = tsToX(ta), xb = tsToX(tb);
          // Seeing – gestrichelt rot, 2 px
          int16_t ya = sToY(sd.points[i - 1].seeing);
          int16_t yb = sToY(sd.points[i].seeing);
          drawDashedLine(xa, ya,     xb, yb,     DISPLAY_RED, 4, 3);
          drawDashedLine(xa, ya - 1, xb, yb - 1, DISPLAY_RED, 4, 3);
          // Transparenz – gepunktet rot
          drawDottedLine(xa, sToY(sd.points[i - 1].transparency),
                         xb, sToY(sd.points[i].transparency), DISPLAY_RED);
        }
      }
    }
  }

  // Legende rechts oben – drei Zeilen (Zeile 3 ragt leicht in den Plot)
  int16_t lx = CH_X + CH_W - 260;
  int16_t ly = CH_Y + 12;
  // Zeile 1: Gesamt + Low
  display.drawLine(lx, ly, lx + 16, ly, GxEPD_BLACK);
  display.drawLine(lx, ly + 1, lx + 16, ly + 1, GxEPD_BLACK);
  drawTextAt(lx + 20, ly + 4, "Gesamt", &FreeSans9pt7b);
  display.drawLine(lx + 100, ly, lx + 116, ly, DISPLAY_RED);
  drawTextAt(lx + 120, ly + 4, "Low", &FreeSans9pt7b, DISPLAY_RED);
  // Zeile 2: Mid + High
  drawDottedLine(lx, ly + 16, lx + 16, ly + 16, GxEPD_BLACK);
  drawTextAt(lx + 20, ly + 20, "Mid", &FreeSans9pt7b);
  drawDashedLine(lx + 100, ly + 16, lx + 116, ly + 16, GxEPD_BLACK);
  drawTextAt(lx + 120, ly + 20, "High", &FreeSans9pt7b);
  // Zeile 3: Seeing + Transp (7Timer!) – nur wenn Daten vorhanden
  if (sd.valid) {
    drawDashedLine(lx, ly + 32, lx + 16, ly + 32, DISPLAY_RED, 4, 3);
    drawDashedLine(lx, ly + 31, lx + 16, ly + 31, DISPLAY_RED, 4, 3);
    drawTextAt(lx + 20, ly + 36, "Seeing", &FreeSans9pt7b, DISPLAY_RED);
    drawDottedLine(lx + 100, ly + 32, lx + 116, ly + 32, DISPLAY_RED);
    drawTextAt(lx + 120, ly + 36, "Transp", &FreeSans9pt7b, DISPLAY_RED);
  }
}

// ---------------------------------------------------------------------
//  Einzelblock: STUNDENTABELLE
//  Eine Zeile pro Stunde im Nachtfenster, kompakt.
//  Spalten: HH | Cloud% | Wind | Hum | T
// ---------------------------------------------------------------------
static void drawHourTable(const WeatherData& wd) {
  display.drawRect(TBL_X, TBL_Y, TBL_W, TBL_H, GxEPD_BLACK);
  drawTextAt(TBL_X + 6, TBL_Y + 16, "STUNDENWERTE NACHT", &FreeSans9pt7b);

  uint16_t idx[16];
  uint16_t n = collectNightIndices(wd, idx, 16);

  if (n == 0) {
    drawTextAt(TBL_X + 10, TBL_Y + 32, "—", &FreeSans9pt7b);
    return;
  }

  // Bis zu 12 Stunden anzeigen
  if (n > 12) n = 12;
  int16_t colW = (TBL_W - 60) / n;

  // Header-Spalte ganz links
  int16_t lx = TBL_X + 6;
  int16_t headY = TBL_Y + 28;
  drawTextAt(lx, headY,        "Std",   &FreeSans9pt7b);
  drawTextAt(lx, headY + 12,   "Wolk%", &FreeSans9pt7b);

  // Jede Stunde als Spalte
  for (uint16_t k = 0; k < n; k++) {
    int16_t x = TBL_X + 50 + k * colW;
    char hh[8];
    struct tm lt; time_t t = wd.hourly[idx[k]].ts; localtime_r(&t, &lt);
    snprintf(hh, sizeof(hh), "%02d", lt.tm_hour);
    drawTextCentered(x + colW / 2, headY, hh, &FreeSansBold9pt7b);

    int cs = (int)round(wd.astroHourly[idx[k]].cloudScore);
    char cb[8]; snprintf(cb, sizeof(cb), "%d", cs);
    uint16_t col = (cs > ASTRO_CLOUD_MODERATE) ? DISPLAY_RED : GxEPD_BLACK;
    drawTextCentered(x + colW / 2, headY + 12, cb, &FreeSans9pt7b, col);
  }
}

// ---------------------------------------------------------------------
//  Einzelblock: MINI-CHARTS
//  Vier kleine Diagramme (24h forward) für T / Wind / Hum / Niederschlag
// ---------------------------------------------------------------------
static void drawMiniChart(int16_t x, int16_t y, int16_t w, int16_t h,
                          const char* title,
                          const float* values, uint16_t n,
                          float vmin, float vmax,
                          bool barChart, uint16_t color) {
  display.drawRect(x, y, w, h, GxEPD_BLACK);
  drawTextAt(x + 4, y + 13, title, &FreeSans9pt7b);

  int16_t px = x + 4;
  int16_t py = y + 16;
  int16_t pw = w - 28;   // 24 px am rechten Rand fuer die Y-Achsen-Labels
  int16_t ph = h - 20;

  // Min/Max Auto wenn 0/0 übergeben
  if (vmin == 0 && vmax == 0) {
    vmin = 1e9f; vmax = -1e9f;
    for (uint16_t i = 0; i < n; i++) {
      if (isnan(values[i])) continue;
      if (values[i] < vmin) vmin = values[i];
      if (values[i] > vmax) vmax = values[i];
    }
    if (vmin == 1e9f) { vmin = 0; vmax = 1; }
    if (vmax - vmin < 1) vmax = vmin + 1;
  }

  auto yOf = [&](float v) -> int16_t {
    if (isnan(v)) v = vmin;
    float r = (v - vmin) / (vmax - vmin);
    if (r < 0) r = 0; if (r > 1) r = 1;
    return py + ph - (int16_t)(ph * r);
  };
  auto xOf = [&](uint16_t k) -> int16_t {
    if (n <= 1) return px + pw / 2;
    return px + (int16_t)((long)pw * k / (n - 1));
  };

  // Min/Max Beschriftung rechts
  char buf[12];
  snprintf(buf, sizeof(buf), "%.0f", vmax);
  drawTextRight(x + w - 2, y + 19, buf, &FreeSans9pt7b);
  snprintf(buf, sizeof(buf), "%.0f", vmin);
  drawTextRight(x + w - 2, y + h - 2, buf, &FreeSans9pt7b);

  if (n == 0) return;

  if (barChart) {
    int16_t bw = pw / (int16_t)n;
    if (bw < 1) bw = 1;
    for (uint16_t i = 0; i < n; i++) {
      float v = values[i];
      if (isnan(v) || v <= 0) continue;
      int16_t bx = px + i * bw;
      int16_t by = yOf(v);
      display.fillRect(bx, by, bw - 1, py + ph - by, color);
    }
  } else {
    for (uint16_t i = 1; i < n; i++) {
      display.drawLine(xOf(i - 1), yOf(values[i - 1]),
                       xOf(i),     yOf(values[i]), color);
    }
  }
}

static void drawMiniCharts(const WeatherData& wd) {
  // Wir zeigen die nächsten 24h ab "jetzt"
  int16_t startIdx = findCurrentHourIdx(wd);
  if (startIdx < 0) return;

  uint16_t maxN = 24;
  if (startIdx + maxN > wd.hourlyCount) maxN = wd.hourlyCount - startIdx;
  if (maxN < 2) return;

  static float tArr[24], wArr[24], hArr[24], pArr[24];
  for (uint16_t i = 0; i < maxN && i < 24; i++) {
    const HourlyData& h = wd.hourly[startIdx + i];
    tArr[i] = h.tempC;
    wArr[i] = h.windKmh;
    hArr[i] = h.humidity;
    pArr[i] = h.precipMm;
  }

  int16_t cw = MC_W / 4;
  drawMiniChart(MC_X + 0 * cw, MC_Y, cw - 4, MC_H, "Temp  (C)",
                tArr, maxN, 0, 0, false, GxEPD_BLACK);
  drawMiniChart(MC_X + 1 * cw, MC_Y, cw - 4, MC_H, "Wind  (km/h)",
                wArr, maxN, 0, 0, false, GxEPD_BLACK);
  drawMiniChart(MC_X + 2 * cw, MC_Y, cw - 4, MC_H, "Feuchte  (%)",
                hArr, maxN, 0, 100, false, GxEPD_BLACK);
  drawMiniChart(MC_X + 3 * cw, MC_Y, cw - 4, MC_H, "Regen  (mm)",
                pArr, maxN, 0, 0, true, DISPLAY_RED);
}

// ---------------------------------------------------------------------
//  Einzelblock: FORECAST 4-5 Tage
// ---------------------------------------------------------------------
//  Pro Tag: Wochentag, Tmin/Tmax, Niederschlag, kleines Symbol.
//  Bewertung der Nacht: berechnen wir hier "leichtgewichtig" anhand
//  der Stunden 22:00 bis 04:00 des Folgetags (vereinfacht).
// ---------------------------------------------------------------------
static AstroVerdict quickNightVerdict(const WeatherData& wd, time_t dayStart) {
  // Stunden zwischen dayStart+22h und dayStart+24h+4h
  time_t a = dayStart + 22L * 3600L;
  time_t b = dayStart + 28L * 3600L;
  double sum = 0; int cnt = 0;
  float maxWind = 0, maxHum = 0;
  for (uint16_t i = 0; i < wd.hourlyCount; i++) {
    time_t t = wd.hourly[i].ts;
    if (t >= a && t <= b) {
      sum += weightedCloudScore(wd.hourly[i].cloudLow,
                                wd.hourly[i].cloudMid,
                                wd.hourly[i].cloudHigh);
      cnt++;
      if (wd.hourly[i].windKmh  > maxWind) maxWind = wd.hourly[i].windKmh;
      if (wd.hourly[i].humidity > maxHum)  maxHum  = wd.hourly[i].humidity;
    }
  }
  if (cnt == 0) return ASTRO_NO_NIGHT;
  float avg = (float)(sum / cnt);
  AstroVerdict v;
  if      (avg <  ASTRO_CLOUD_GOOD)     v = ASTRO_GOOD;
  else if (avg <  ASTRO_CLOUD_MODERATE) v = ASTRO_MODERATE;
  else                                  v = ASTRO_BAD;
  if ((maxWind > ASTRO_WIND_WARN) || (maxHum > ASTRO_HUMIDITY_WARN)) {
    if      (v == ASTRO_GOOD)     v = ASTRO_MODERATE;
    else if (v == ASTRO_MODERATE) v = ASTRO_BAD;
  }
  return v;
}

static void drawForecast(const WeatherData& wd) {
  display.drawRect(FC_X, FC_Y, FC_W, FC_H, GxEPD_BLACK);
  drawTextAt(FC_X + 6, FC_Y + 14, "VORHERSAGE", &FreeSans9pt7b);

  uint8_t n = wd.dailyCount;
  if (n > 5) n = 5;
  if (n < 1) return;

  // Die Ueberschrift "VORHERSAGE" ist ~82 px breit - erste Spalte muss
  // deutlich nach rechts rutschen, sonst ueberlappt "Mi" direkt darauf.
  const int16_t labelW = 110;
  int16_t cellW = (FC_W - labelW) / n;
  for (uint8_t i = 0; i < n; i++) {
    int16_t x = FC_X + labelW + i * cellW;
    int16_t cx = x + cellW / 2;

    const DailyData& d = wd.daily[i];

    // Wochentag
    char wd3[6]; snprintf(wd3, sizeof(wd3), "%s", shortWeekdayDE(d.date));
    drawTextCentered(cx, FC_Y + 16, wd3, &FreeSansBold9pt7b);

    // Tmin / Tmax  – mit doppeltem Leerzeichen für optische Luft
    char tb[24];
    snprintf(tb, sizeof(tb), "%.0f / %.0f C",
             isnan(d.tMinC) ? 0.0f : d.tMinC,
             isnan(d.tMaxC) ? 0.0f : d.tMaxC);
    drawTextCentered(cx, FC_Y + 31, tb, &FreeSans9pt7b);

    // Nachtbewertung
    AstroVerdict v = quickNightVerdict(wd, d.date);
    const char* label;
    uint16_t col = GxEPD_BLACK;
    switch (v) {
      case ASTRO_GOOD:     label = "GUT";    break;
      case ASTRO_MODERATE: label = "MITTEL"; break;
      case ASTRO_BAD:      label = "BAD";    col = DISPLAY_RED; break;
      default:             label = "--";
    }
    drawTextCentered(cx, FC_Y + 44, label, &FreeSansBold9pt7b, col);
  }
}

// ---------------------------------------------------------------------
//  Mondphase als Icon: gefüllter Kreis mit Terminator-Ellipse.
//  phase: 0..1 (0=Neumond, 0.25=1.Viertel, 0.5=Voll, 0.75=L.Viertel)
//  Die Darstellung ist für die Nordhalbkugel korrekt (zunehmend:
//  Licht auf der rechten Seite).
// ---------------------------------------------------------------------
static void drawMoonPhaseIcon(int16_t cx, int16_t cy, int16_t r, float phase) {
  // Outline (immer)
  display.drawCircle(cx, cy, r,     GxEPD_BLACK);
  display.drawCircle(cx, cy, r - 1, GxEPD_BLACK);

  float illum = (1.0f - cosf(2.0f * PI * phase)) / 2.0f;

  // Extremfälle für schnelleren Code
  if (illum < 0.02f) {
    display.fillCircle(cx, cy, r - 1, GxEPD_BLACK);
    return;
  }
  if (illum > 0.98f) {
    return;  // Vollmond = nur Outline
  }

  const float cosPh = cosf(2.0f * PI * phase);
  const bool waxing = (phase < 0.5f);

  // Zeilenweise den dunklen Teil füllen
  for (int16_t y = -r + 1; y < r; y++) {
    int16_t xLimit  = (int16_t)sqrtf((float)(r * r - y * y));
    int16_t signedX = (int16_t)(cosPh * (float)xLimit);

    if (waxing) {
      // Dunkel: [-xLimit .. signedX]
      for (int16_t x = -xLimit; x <= signedX; x++) {
        display.drawPixel(cx + x, cy + y, GxEPD_BLACK);
      }
    } else {
      // Dunkel: [-signedX .. xLimit]
      for (int16_t x = -signedX; x <= xLimit; x++) {
        display.drawPixel(cx + x, cy + y, GxEPD_BLACK);
      }
    }
  }
}

// ---------------------------------------------------------------------
//  Block: MOND
//  Links das Mondphasen-Icon, rechts die wichtigsten Kennzahlen.
// ---------------------------------------------------------------------
static void drawMoonBox(const WeatherData& wd) {
  display.drawRect(MOON_X, MOON_Y, MOON_W, MOON_H, GxEPD_BLACK);
  drawTextAt(MOON_X + 6, MOON_Y + 16, "MOND", &FreeSansBold9pt7b);

  const MoonInfo& m = wd.moonNight;  // Mitte der Nacht ist relevanter

  // Icon links
  const int16_t iconR = 32;
  const int16_t iconCx = MOON_X + 45;
  const int16_t iconCy = MOON_Y + 74;
  drawMoonPhaseIcon(iconCx, iconCy, iconR, m.phase);

  // Text rechts
  const int16_t tx = MOON_X + 95;
  int16_t       ty = MOON_Y + 44;
  char buf[40];

  // Phasen-Name (etwas groesser)
  drawTextAt(tx, ty, m.phaseName, &FreeSansBold12pt7b);

  // Illumination
  ty += 20;
  snprintf(buf, sizeof(buf), "%.0f %% beleuchtet", m.illumination * 100.0f);
  drawTextAt(tx, ty, buf, &FreeSans9pt7b);

  // Höhe (rot wenn Mond sehr hoch + hell steht = stört)
  ty += 18;
  if (m.altitudeDeg > 0.0f) {
    snprintf(buf, sizeof(buf), "%.0f  ueber Hor.", m.altitudeDeg);
  } else {
    snprintf(buf, sizeof(buf), "%.0f  unter Hor.", -m.altitudeDeg);
  }
  // Grad-Symbol einfuegen: wir nutzen eine extra Darstellung
  drawTextAt(tx, ty, buf, &FreeSans9pt7b);
  // Klein den ° als Kreis an die Zahl setzen
  {
    int16_t bx, by; uint16_t bw, bh;
    display.setFont(&FreeSans9pt7b);
    // Breite der ersten Zahl messen
    char tmp[8];
    snprintf(tmp, sizeof(tmp), "%.0f",
             m.altitudeDeg > 0 ? m.altitudeDeg : -m.altitudeDeg);
    display.getTextBounds(tmp, tx, ty, &bx, &by, &bw, &bh);
    display.drawCircle(tx + bw + 2, ty - 8, 2, GxEPD_BLACK);
  }

  // Sky-Impact – als Label, rot nur bei hoher Aufhellung
  ty += 18;
  const char* impactLabel;
  uint16_t    impactCol = GxEPD_BLACK;
  if      (m.skyImpact < 0.10f) impactLabel = "Himmel:  dunkel";
  else if (m.skyImpact < 0.35f) impactLabel = "Himmel:  mittel";
  else                        { impactLabel = "Himmel:  HELL";
                                impactCol   = DISPLAY_RED; }
  drawTextAt(tx, ty, impactLabel, &FreeSansBold9pt7b, impactCol);
}

// ---------------------------------------------------------------------
//  Public: Komplettes Dashboard
// ---------------------------------------------------------------------
void renderDashboard(const WeatherData& wd, BatteryInfo batt,
                     const LocalSensorData& sensor,
                     const SeeingData& seeing) {
  if (!wd.valid) {
    renderFallback("Keine Wetterdaten verfuegbar");
    return;
  }

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    drawHeader(wd, batt);
    drawCurrent(wd, sensor);
    drawAstroBox(wd);
    drawMoonBox(wd);
    drawNightChart(wd, seeing);
    drawHourTable(wd);
    drawMiniCharts(wd);
    drawForecast(wd);
  } while (display.nextPage());
}

void renderFallback(const char* msg) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    // Header bleibt minimal (keine Daten)
    display.fillRect(0, 0, W, HDR_H, GxEPD_BLACK);
    drawTextAt(8, 19, "Astro-Wetter-Display", &FreeSansBold12pt7b, GxEPD_WHITE);

    // Zentrale Meldung
    drawTextCentered(W / 2, H / 2 - 20, "Erststart / Fehler", &FreeSansBold18pt7b);
    drawTextCentered(W / 2, H / 2 + 10, msg ? msg : "Bitte warten...",
                     &FreeSansBold12pt7b, DISPLAY_RED);
    drawTextCentered(W / 2, H / 2 + 40,
                     "Pruefe WLAN, Konfiguration und Internetzugang.",
                     &FreeSans9pt7b);
  } while (display.nextPage());
}

void renderPortalInfo(const char* apName) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Header
    display.fillRect(0, 0, W, HDR_H, GxEPD_BLACK);
    drawTextAt(8, 19, "AstroOyInk  —  WiFi-Konfiguration", &FreeSansBold12pt7b, GxEPD_WHITE);

    // Titelzeile
    int16_t y = 70;
    drawTextCentered(W / 2, y, "WLAN-Konfiguration erforderlich", &FreeSansBold18pt7b);

    // Schritt 1
    y += 55;
    drawTextAt(60, y, "1.  Smartphone oder PC mit folgendem WLAN verbinden:",
               &FreeSansBold12pt7b);
    y += 34;
    drawTextCentered(W / 2, y, apName ? apName : "AstroOyInk-??????",
                     &FreeSansBold18pt7b, DISPLAY_RED);

    // Schritt 2
    y += 50;
    drawTextAt(60, y, "2.  Browser oeffnen und aufrufen:", &FreeSansBold12pt7b);
    y += 34;
    drawTextCentered(W / 2, y, "192.168.4.1", &FreeSansBold18pt7b);

    // Schritt 3
    y += 46;
    drawTextAt(60, y, "3.  Formular ausfullen und speichern — Geraet startet neu.",
               &FreeSans9pt7b);

    // Hinweis Timeout
    y += 30;
    drawTextCentered(W / 2, y, "Portal schliesst nach 3 Minuten — danach erneuter Versuch.",
                     &FreeSans9pt7b);

    // Trennlinie
    display.drawLine(40, y + 14, W - 40, y + 14, GxEPD_BLACK);
  } while (display.nextPage());
}

// =====================================================================
//  SESSION-Dashboard
//  Layout (800 x 480):
//
//    y=  0- 26  HEADER (schwarz)
//    y= 27- 93  SEQUENZ-Block (Name, Target, Subs, Restzeit, Status, Alt/Az)
//    y= 94-479  3 Spalten: KAMERA | GUIDING | MOUNT
//
//  Farb-Logik:
//    Normaltext   → schwarz
//    Warnungen    → DISPLAY_RED (bei BW = schwarz, bei BWR = rot)
//    Invertierter Warnblock (nur BW, wenn DISPLAY_HAS_RED=0) für
//    kritische Meldungen wie [Guiding WARN/LOST].
// =====================================================================

// --- SESSION-Layout-Konstanten ---
static const int16_t SS_SEQ_Y1  = 48;    // Sequenz-Zeile 1 Baseline
static const int16_t SS_SEQ_Y2  = 72;    // Sequenz-Zeile 2 Baseline (Status/Alt)
static const int16_t SS_SEQ_BOT = 86;    // Unterkante Sequenz-Block

// Spaltenstart und -breiten
static const int16_t SS_C1_X  = 4,   SS_C1_W = 258;   // KAMERA
static const int16_t SS_C2_X  = 264, SS_C2_W = 268;   // GUIDING
static const int16_t SS_C3_X  = 534, SS_C3_W = 262;   // MOUNT

// Zeilenabstaende in den Spalten
static const int16_t SS_R_TITLE = SS_SEQ_BOT + 22;    // = 108
static const int16_t SS_R1      = SS_SEQ_BOT + 56;    // = 142
static const int16_t SS_R2      = SS_SEQ_BOT + 88;    // = 174
static const int16_t SS_R3      = SS_SEQ_BOT + 120;   // = 206
static const int16_t SS_R4      = SS_SEQ_BOT + 152;   // = 238
static const int16_t SS_R5      = SS_SEQ_BOT + 184;   // = 270

// Zeichnet einen Warn-Text: bei BWR rot, bei BW invertierter Block
static void drawWarnLabel(int16_t x, int16_t y, const char* text) {
#if DISPLAY_HAS_RED
  drawTextAt(x, y, text, &FreeSansBold9pt7b, DISPLAY_RED);
#else
  int16_t bx, by; uint16_t bw, bh;
  display.setFont(&FreeSansBold9pt7b);
  display.getTextBounds(text, x, y, &bx, &by, &bw, &bh);
  display.fillRect(bx - 2, by - 2, bw + 4, bh + 4, GxEPD_BLACK);
  drawTextAt(x, y, text, &FreeSansBold9pt7b, GxEPD_WHITE);
#endif
}

// Hilfsfunktion: RA (Stunden) → "05h 34m"
static void formatRAStr(float raH, char* buf, size_t sz) {
  int h = (int)raH;
  int m = (int)((raH - h) * 60.0f + 0.5f);
  if (m >= 60) { h++; m = 0; }
  snprintf(buf, sz, "%02dh %02dm", h, m);
}

// Hilfsfunktion: Dec (Grad) → "+35° 48'"
static void formatDecStr(float decDeg, char* buf, size_t sz) {
  char sign = (decDeg >= 0) ? '+' : '-';
  float absDec = fabsf(decDeg);
  int d = (int)absDec;
  int m = (int)((absDec - d) * 60.0f + 0.5f);
  if (m >= 60) { d++; m = 0; }
  snprintf(buf, sz, "%c%02d%c %02d'", sign, d, '\xB0', m);
}

// Hilfsfunktion: Restzeit in Sekunden → "1h 23m" oder "45 min"
static void formatEtaStr(uint32_t s, char* buf, size_t sz) {
  if (s == 0) { snprintf(buf, sz, "--"); return; }
  uint32_t h = s / 3600, m = (s % 3600) / 60;
  if (h > 0) snprintf(buf, sz, "%uh %02um", (unsigned)h, (unsigned)m);
  else        snprintf(buf, sz, "%u min",   (unsigned)m);
}

// Hilfsfunktion: Azimut (Grad) → Himmelsrichtung "N"/"NO"/"O"/...
static const char* azToDir(float az) {
  if (az < 22.5f || az >= 337.5f) return "N";
  if (az < 67.5f)  return "NO";
  if (az < 112.5f) return "O";
  if (az < 157.5f) return "SO";
  if (az < 202.5f) return "S";
  if (az < 247.5f) return "SW";
  if (az < 292.5f) return "W";
  return "NW";
}

// ---------------------------------------------------------------------
//  SESSION-Spalte: KAMERA
// ---------------------------------------------------------------------
static void drawSessionCamera(int16_t cx, int16_t cw, const NinaCameraInfo& c) {
  drawTextAt(cx + 6, SS_R_TITLE, "KAMERA", &FreeSansBold12pt7b);
  display.drawLine(cx + 2, SS_R_TITLE + 6, cx + cw - 2, SS_R_TITLE + 6, GxEPD_BLACK);

  char buf[48];

  // Kameraname (ggf. kuerzen)
  strlcpy(buf, c.connected ? c.name : "---", sizeof(buf));
  if (strlen(buf) > 18) buf[18] = '\0';
  drawTextAt(cx + 6, SS_R1, buf, &FreeSans9pt7b);

  snprintf(buf, sizeof(buf), "Filter: %s",
           (c.filterName[0] != '\0') ? c.filterName : "--");
  drawTextAt(cx + 6, SS_R2, buf, &FreeSans9pt7b);

  if (c.exposureS > 0) snprintf(buf, sizeof(buf), "Exp:    %.0f s", c.exposureS);
  else                 snprintf(buf, sizeof(buf), "Exp:    --");
  drawTextAt(cx + 6, SS_R3, buf, &FreeSans9pt7b);

  if (c.hfr > 0) snprintf(buf, sizeof(buf), "HFR:    %.1f px", c.hfr);
  else           snprintf(buf, sizeof(buf), "HFR:    --");
  drawTextAt(cx + 6, SS_R4, buf, &FreeSans9pt7b);
}

// ---------------------------------------------------------------------
//  SESSION-Spalte: GUIDING
// ---------------------------------------------------------------------
static void drawSessionGuiding(int16_t cx, int16_t cw,
                               const NinaGuidingInfo& g) {
  drawTextAt(cx + 6, SS_R_TITLE, "GUIDING", &FreeSansBold12pt7b);
  display.drawLine(cx + 2, SS_R_TITLE + 6, cx + cw - 2, SS_R_TITLE + 6, GxEPD_BLACK);

  char buf[40];

  if (!g.connected) {
    drawWarnLabel(cx + 6, SS_R1, "[Kein Guider]");
    return;
  }

  snprintf(buf, sizeof(buf), "RMS RA:  %.2f\"", g.rmsRA);
  drawTextAt(cx + 6, SS_R1, buf, &FreeSans9pt7b);

  snprintf(buf, sizeof(buf), "RMS Dec: %.2f\"", g.rmsDec);
  drawTextAt(cx + 6, SS_R2, buf, &FreeSans9pt7b);

  snprintf(buf, sizeof(buf), "Total:   %.2f\"", g.rmsTotal);
  drawTextAt(cx + 6, SS_R3, buf, &FreeSans9pt7b);

  // Status-Label: OK / WARN / LOST
  bool guidingLost = (strstr(g.state, "Lost") || strstr(g.state, "Stop"));
  bool guidingWarn = (!guidingLost && g.rmsTotal >= GUIDING_WARN_RMS_ARCSEC
                      && g.rmsTotal > 0);

  if (guidingLost) {
    drawWarnLabel(cx + 6, SS_R4, "[Guiding LOST]");
  } else if (guidingWarn) {
    drawWarnLabel(cx + 6, SS_R4, "[Guiding WARN]");
  } else {
    drawTextAt(cx + 6, SS_R4, "[Guiding OK]", &FreeSansBold9pt7b);
  }

  drawTextAt(cx + 6, SS_R5, g.state[0] ? g.state : "---", &FreeSans9pt7b);
}

// ---------------------------------------------------------------------
//  SESSION-Spalte: MOUNT
// ---------------------------------------------------------------------
static void drawSessionMount(int16_t cx, int16_t cw,
                             const NinaMountInfo& m) {
  drawTextAt(cx + 6, SS_R_TITLE, "MOUNT", &FreeSansBold12pt7b);
  display.drawLine(cx + 2, SS_R_TITLE + 6, cx + cw - 2, SS_R_TITLE + 6, GxEPD_BLACK);

  char buf[48];

  // Mountname (kuerzen)
  strlcpy(buf, m.connected ? m.name : "---", sizeof(buf));
  if (strlen(buf) > 18) buf[18] = '\0';
  drawTextAt(cx + 6, SS_R1, buf, &FreeSans9pt7b);

  char ra[16];
  formatRAStr(m.raH, ra, sizeof(ra));
  snprintf(buf, sizeof(buf), "RA:  %s", ra);
  drawTextAt(cx + 6, SS_R2, buf, &FreeSans9pt7b);

  char dec[16];
  formatDecStr(m.decDeg, dec, sizeof(dec));
  snprintf(buf, sizeof(buf), "Dec: %s", dec);
  drawTextAt(cx + 6, SS_R3, buf, &FreeSans9pt7b);

  const char* pier = (m.pierSide == 0) ? "West"
                   : (m.pierSide == 1) ? "East"
                   :                     "--";
  snprintf(buf, sizeof(buf), "Pier: %s", pier);
  drawTextAt(cx + 6, SS_R4, buf, &FreeSans9pt7b);

  // Meridian-Flip-Countdown
  if (m.flipEtaS < 0) {
    drawTextAt(cx + 6, SS_R5, "Flip: --", &FreeSans9pt7b);
  } else {
    char eta[20];
    formatEtaStr((uint32_t)m.flipEtaS, eta, sizeof(eta));
    snprintf(buf, sizeof(buf), "Flip: %s", eta);
    // Warnung wenn Flip in < 10 Minuten
    if (m.flipEtaS < 600) {
      drawWarnLabel(cx + 6, SS_R5, buf);
    } else {
      drawTextAt(cx + 6, SS_R5, buf, &FreeSans9pt7b);
    }
  }
}

// ---------------------------------------------------------------------
//  renderSessionDashboard — public
// ---------------------------------------------------------------------
void renderSessionDashboard(const NinaData& data, const BatteryInfo& batt,
                            const char* locationName) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // --- Header ---
    drawHeaderImpl(locationName ? locationName : LOCATION_NAME,
                   time(nullptr), batt);

    if (!data.valid) {
      drawTextCentered(W / 2, H / 2, "N.I.N.A. nicht erreichbar",
                       &FreeSansBold18pt7b, DISPLAY_RED);
      continue;
    }

    const NinaSequenceInfo& seq = data.sequence;

    // --- Sequenz-Block ---
    // Zeile 1: SEQUENZ + Target-Name | Target i/n | Sub i/n
    char line1[128];
    {
      char seqLabel[72];
      if (seq.targetName[0] != '\0') {
        snprintf(seqLabel, sizeof(seqLabel), "SEQUENZ: %s", seq.targetName);
      } else if (seq.sequenceName[0] != '\0') {
        snprintf(seqLabel, sizeof(seqLabel), "SEQUENZ: %s", seq.sequenceName);
      } else {
        snprintf(seqLabel, sizeof(seqLabel), "SEQUENZ: ---");
      }

      char targets[24] = "";
      if (seq.targetTotal > 0 && seq.targetIndex >= 0) {
        snprintf(targets, sizeof(targets), "  Target %d/%d",
                 seq.targetIndex + 1, (int)seq.targetTotal);
      }

      char subs[24] = "";
      if (seq.subsTotal > 0) {
        snprintf(subs, sizeof(subs), "  Sub %d/%d",
                 (int)seq.subsDone, (int)seq.subsTotal);
      }

      char eta[20] = "";
      if (seq.etaS > 0) {
        char etaBuf[16];
        formatEtaStr(seq.etaS, etaBuf, sizeof(etaBuf));
        snprintf(eta, sizeof(eta), "  Noch %s", etaBuf);
      }

      snprintf(line1, sizeof(line1), "%s%s%s%s", seqLabel, targets, subs, eta);
    }
    drawTextAt(6, SS_SEQ_Y1, line1, &FreeSansBold9pt7b);

    // Zeile 2: Status links | Alt/Az rechts
    char statusBuf[32];
    snprintf(statusBuf, sizeof(statusBuf), "Status: %s",
             seq.status[0] ? seq.status : "---");

    bool statusWarn = (strcmp(seq.status, "Slewing") == 0 ||
                       strcmp(seq.status, "Pausiert") == 0);
    if (statusWarn) {
      drawWarnLabel(6, SS_SEQ_Y2, statusBuf);
    } else {
      drawTextAt(6, SS_SEQ_Y2, statusBuf, &FreeSans9pt7b);
    }

    // Alt/Az rechts in Zeile 2
    if (data.mount.connected && data.mount.altDeg > -90.0f) {
      char altaz[40];
      snprintf(altaz, sizeof(altaz), "Alt: %.0f%c  Az: %.0f%c %s",
               data.mount.altDeg, '\xB0',
               data.mount.azDeg,  '\xB0',
               azToDir(data.mount.azDeg));
      drawTextRight(W - 6, SS_SEQ_Y2, altaz, &FreeSans9pt7b);
    }

    // Trennlinie unter Sequenz-Block
    display.drawLine(4, SS_SEQ_BOT, W - 4, SS_SEQ_BOT, GxEPD_BLACK);

    // --- Spalten-Trennlinien ---
    display.drawLine(SS_C2_X - 2, SS_SEQ_BOT + 2, SS_C2_X - 2, H - 2, GxEPD_BLACK);
    display.drawLine(SS_C3_X - 2, SS_SEQ_BOT + 2, SS_C3_X - 2, H - 2, GxEPD_BLACK);

    // --- Die drei Spalten ---
    drawSessionCamera(SS_C1_X, SS_C1_W, data.camera);
    drawSessionGuiding(SS_C2_X, SS_C2_W, data.guiding);
    drawSessionMount(SS_C3_X, SS_C3_W, data.mount);

  } while (display.nextPage());
}
