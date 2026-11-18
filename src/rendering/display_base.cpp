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
//    x=0                    x=258      x=546              x=796
//    +----------------------+----------+------------------+ y=0
//    | HEADER  Ort | Uhrzeit | Update  (x=0..796)        |
//    +----------------------+----------+------------------+ y=28
//    | AKTUELL x=4..254     | NACHT-SCORE x=258..542     | MOND x=546..796 |
//    +----------------------+----------+------------------+ y=160
//    | NACHT-WOLKEN-CHART  x=4..796  (Gesamt/Low/Mid/High)              |
//    +-------------------------------------------------------------------+ y=286
//    | HOUR TABLE  x=4..796  (Std / Wolk% / ISS-Symbol)                 |
//    +-------------------------------------------------------------------+ y=347
//    | MINI CHARTS  x=4..796  Temp | Wind | Feuchte | Regen (~196 px je)|
//    +-------------------------------------------------------------------+ y=409
//    | FORECAST  x=4..796  4–5 Tage                                      |
//    +-------------------------------------------------------------------+ y=480
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================

#include "display_base.h"
#include "../../config.h"
#include "../common/time_utils.h"
#include "../common/astro_logic.h"
#include "../common/local_sensor.h"
#include "../data_sources/seeing_api.h"
#include "../data_sources/iss_api.h"
#include "../data_sources/nina_api.h"

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

// Nacht-Cloud-Chart (Titelzeile mit Legende, darunter der Plot)
static const int16_t CH_X = 4,   CH_Y = 164;
static const int16_t CH_W = 792, CH_H = 106;

// Stundentabelle (Uhr / Wolken % / ISS)
static const int16_t TBL_X = 4,   TBL_Y = 274;
static const int16_t TBL_W = 792, TBL_H = 70;

// Mini-Charts (4 Stk., je ~194 breit)
static const int16_t MC_X = 4,   MC_Y = 348;
static const int16_t MC_W = 792, MC_H = 56;

// Forecast (4–5 Tage) — reicht bis ans untere Display-Ende
static const int16_t FC_X = 4,   FC_Y = 408;
static const int16_t FC_W = 792, FC_H = 70;

// ---------------------------------------------------------------------
//  Hilfsfunktionen Text
//  Die GFX-Fonts kennen kein Grad-Zeichen. Im Text steht dafuer ein
//  Backtick (`), der als kleiner Kreis gezeichnet wird: "12`C" → 12°C.
//  Breiten werden ueber die Laufweite (xAdvance) gemessen — so wandert
//  auch der Cursor, damit sitzen zentrierte/rechtsbuendige Texte exakt.
// ---------------------------------------------------------------------
static const char DEG = '`';

// Ab dieser Mondhoehe wird der Wert in der Stundentabelle rot
#define MOON_ALT_RED_DEG 30.0f

static int16_t degRadius(const GFXfont* f) {
  return f->yAdvance >= 40 ? 4 : (f->yAdvance >= 28 ? 3 : 2);
}
static int16_t degAdvance(const GFXfont* f) { return 2 * degRadius(f) + 3; }

// Hoehe der Ziffern ueber der Grundlinie
static int16_t capHeight(const GFXfont* f) {
  return f->glyph['0' - f->first].height;
}

static int16_t textWidth(const char* text, const GFXfont* font) {
  int16_t w = 0;
  for (const char* p = text; *p; p++) {
    uint8_t c = (uint8_t)*p;
    if (c == (uint8_t)DEG) { w += degAdvance(font); continue; }
    if (c < font->first || c > font->last) continue;
    w += font->glyph[c - font->first].xAdvance;
  }
  return w;
}

static void drawTextAt(int16_t x, int16_t y, const char* text,
                       const GFXfont* font, uint16_t color = GxEPD_BLACK) {
  display.setFont(font);
  display.setTextColor(color);
  display.setCursor(x, y);
  for (const char* p = text; *p; p++) {
    if (*p != DEG) { display.write((uint8_t)*p); continue; }
    int16_t r  = degRadius(font);
    int16_t cx = display.getCursorX() + r + 1;
    int16_t cy = y - capHeight(font) + r;
    display.drawCircle(cx, cy, r, color);
    if (r >= 3) display.drawCircle(cx, cy, r - 1, color);
    display.setCursor(display.getCursorX() + degAdvance(font), y);
  }
}

static void drawTextAt(int16_t x, int16_t y, const String& s,
                       const GFXfont* font, uint16_t color = GxEPD_BLACK) {
  drawTextAt(x, y, s.c_str(), font, color);
}

// rechtsbündig
static void drawTextRight(int16_t xRight, int16_t y, const char* text,
                          const GFXfont* font, uint16_t color = GxEPD_BLACK) {
  drawTextAt(xRight - textWidth(text, font), y, text, font, color);
}

static void drawTextCentered(int16_t xCenter, int16_t y, const char* text,
                             const GFXfont* font, uint16_t color = GxEPD_BLACK) {
  drawTextAt(xCenter - textWidth(text, font) / 2, y, text, font, color);
}

// Groesster Font aus fonts[], mit dem text in maxW passt (sonst der letzte)
static const GFXfont* fitFont(const char* text, int16_t maxW,
                              const GFXfont* const* fonts, uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    if (textWidth(text, fonts[i]) <= maxW) return fonts[i];
  }
  return fonts[n - 1];
}

static const GFXfont* fitFont(const char* text, int16_t maxW,
                              const GFXfont* large, const GFXfont* small) {
  return textWidth(text, large) <= maxW ? large : small;
}

// ---------------------------------------------------------------------
//  Hilfsfunktionen Zeichnen
// ---------------------------------------------------------------------

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

// Gefuellter 5-zackiger Stern (flaechig statt Linien: rendert sauberer)
static void drawStar(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  int16_t ox[5], oy[5], ix[5], iy[5];
  for (int i = 0; i < 5; i++) {
    float a = -PI / 2 + i * 2 * PI / 5;
    float b = a + PI / 5;
    ox[i] = cx + (int16_t)roundf(r * cosf(a));
    oy[i] = cy + (int16_t)roundf(r * sinf(a));
    ix[i] = cx + (int16_t)roundf(r * 0.42f * cosf(b));
    iy[i] = cy + (int16_t)roundf(r * 0.42f * sinf(b));
  }
  for (int i = 0; i < 5; i++) {
    int j = (i + 1) % 5;
    display.fillTriangle(cx, cy, ox[i], oy[i], ix[i], iy[i], color);
    display.fillTriangle(cx, cy, ix[i], iy[i], ox[j], oy[j], color);
  }
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

  // Fuellstand — immer weiss: Rot auf schwarzem Header ist auf dem
  // Panel kaum lesbar
  uint8_t pct = batt.percent;
  int16_t fillW = (int16_t)((int32_t)(iconW - 2) * pct / 100);
  if (fillW > 0) {
    display.fillRect(x + 1, y + 1, fillW, iconH - 2, GxEPD_WHITE);
  }

  // Prozenttext rechts neben dem Icon
  char buf[8];
  snprintf(buf, sizeof(buf), "%d%%", pct);
  drawTextAt(x + iconW + 5, textY, buf, &FreeSans9pt7b, GxEPD_WHITE);
}

// ---------------------------------------------------------------------
//  Einzelblock: HEADER (gemeinsame Basis fuer WEATHER und SESSION)
// ---------------------------------------------------------------------
static void drawHeaderImpl(const char* locationName, time_t fetchedAt,
                           const BatteryInfo& batt) {
  display.fillRect(0, 0, W, HDR_H, GxEPD_BLACK);

  drawTextAt(14, 19, locationName, &FreeSansBold12pt7b, GxEPD_WHITE);

  // Mitte: Datum (die Uhrzeit steht rechts als Stand — beides waere doppelt,
  // weil das Display nur beim Update neu gezeichnet wird)
  char dateBuf[16] = "--";
  time_t now = time(nullptr);
  if (now > 1700000000) {
    struct tm lt; localtime_r(&now, &lt);
    snprintf(dateBuf, sizeof(dateBuf), "%s %02d.%02d.",
             shortWeekdayDE(now), lt.tm_mday, lt.tm_mon + 1);
  }
  drawTextCentered(W / 2, 19, dateBuf, &FreeSansBold12pt7b, GxEPD_WHITE);

  if (batt.status != BATT_UNKNOWN) {
    drawBatteryWidget(575, 7, batt);
  }

  char updBuf[40];
  if (fetchedAt > 0) {
    char tmp[20];
    formatHHMM(fetchedAt, tmp, sizeof(tmp));
    snprintf(updBuf, sizeof(updBuf), "Stand %s", tmp);
  } else {
    snprintf(updBuf, sizeof(updBuf), "Stand --:--");
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

  // Werte rechte Spalte: 4 Zeilen à 20 px, Icons/Text in einer Flucht
  int16_t rx = CUR_X + 142;
  int16_t ry = CUR_Y + 38;
  const int16_t tx = rx + 20;
  char buf[40];

  drawWindIcon(rx, ry - 5, GxEPD_BLACK);
  snprintf(buf, sizeof(buf), "%.0f km/h", isnan(h.windKmh) ? 0.0f : h.windKmh);
  uint16_t windCol = (h.windKmh > ASTRO_WIND_WARN) ? DISPLAY_RED : GxEPD_BLACK;
  drawTextAt(tx, ry, buf, &FreeSans9pt7b, windCol);

  ry += 20;
  drawDropIcon(rx + 5, ry - 6, GxEPD_BLACK);
  snprintf(buf, sizeof(buf), "%.0f %%", isnan(h.humidity) ? 0.0f : h.humidity);
  uint16_t humCol = (h.humidity > ASTRO_HUMIDITY_WARN) ? DISPLAY_RED : GxEPD_BLACK;
  drawTextAt(tx, ry, buf, &FreeSans9pt7b, humCol);

  ry += 20;
  snprintf(buf, sizeof(buf), "Tau %.1f`C", isnan(h.dewpointC) ? 0.0f : h.dewpointC);
  drawTextAt(tx, ry, buf, &FreeSans9pt7b);

  ry += 20;
  if (!isnan(h.pressureHpa)) {
    snprintf(buf, sizeof(buf), "%.0f hPa", h.pressureHpa);
  } else {
    snprintf(buf, sizeof(buf), "-- hPa");
  }
  drawTextAt(tx, ry, buf, &FreeSans9pt7b);

  // INNEN-Zeile am unteren Rand der Box (nur wenn Sensor aktiv und gueltig)
  if (sensor.valid) {
    int16_t sepY = CUR_Y + CUR_H - 26;
    display.drawLine(CUR_X + 1, sepY, CUR_X + CUR_W - 2, sepY, GxEPD_BLACK);
    int16_t txtY = CUR_Y + CUR_H - 8;
    drawTextAt(CUR_X + 8, txtY, "Innen", &FreeSansBold9pt7b);
    char ibuf[40];
    // Luftdruck fehlt bewusst: innen = aussen, steht schon oben
    snprintf(ibuf, sizeof(ibuf), "%.1f`C     %.0f %%", sensor.tempC, sensor.humidity);
    drawTextAt(CUR_X + 66, txtY, ibuf, &FreeSans9pt7b);
  }
}

// ---------------------------------------------------------------------
//  Einzelblock: ASTRO-SCORE
// ---------------------------------------------------------------------
static void drawAstroBox(const WeatherData& wd) {
  display.drawRect(AST_X, AST_Y, AST_W, AST_H, GxEPD_BLACK);
  drawTextAt(AST_X + 6, AST_Y + 16, "NACHT-BEWERTUNG", &FreeSansBold9pt7b);

  const AstroSummary& s = wd.summary;

  // Score-Kreis links
  const int16_t cx = AST_X + 42, cy = AST_Y + 76, r = 34;
  display.drawCircle(cx, cy, r,     GxEPD_BLACK);
  display.drawCircle(cx, cy, r - 1, GxEPD_BLACK);

  char buf[60];
  if (s.verdict == ASTRO_NO_NIGHT) {
    snprintf(buf, sizeof(buf), "--");
  } else {
    snprintf(buf, sizeof(buf), "%d%%", (int)round(s.avgCloudScore));
  }
  static const GFXfont* const kScoreFonts[] = {&FreeSansBold18pt7b, &FreeSansBold12pt7b};
  drawTextCentered(cx, cy + 9, buf, fitFont(buf, 2 * r - 8, kScoreFonts, 2));

  // Textspalte rechts vom Kreis — breit genug fuer "SCHLECHT" in 18pt
  const int16_t tx   = AST_X + 84;
  const int16_t maxW = AST_X + AST_W - 6 - tx;

  const char* label;
  uint16_t labelCol = GxEPD_BLACK;
  switch (s.verdict) {
    case ASTRO_GOOD:     label = "GUT";      break;
    case ASTRO_MODERATE: label = "MITTEL";   break;
    case ASTRO_BAD:      label = "SCHLECHT"; labelCol = DISPLAY_RED; break;
    default:             label = "KEINE NACHT";
  }
  static const GFXfont* const kVerdictFonts[] = {&FreeSansBold18pt7b, &FreeSansBold12pt7b};
  drawTextAt(tx, AST_Y + 54, label, fitFont(label, maxW, kVerdictFonts, 2), labelCol);

  // Sterne unter dem Label (nur fuer gute/mittlere Naechte)
  int16_t stars = (s.verdict == ASTRO_GOOD) ? 5 : (s.verdict == ASTRO_MODERATE) ? 3 : 0;
  for (int16_t i = 0; i < stars; i++) {
    drawStar(tx + 8 + i * 20, AST_Y + 70, 8, GxEPD_BLACK);
  }
  // Ohne Sterne ruecken die Folgezeilen nach oben
  int16_t ty = AST_Y + (stars ? 98 : 84);

  // Nachtfenster + Stundenanzahl
  if (s.nightStart > 0 && s.nightEnd > 0) {
    char a[8], b[8];
    formatHHMM(s.nightStart, a, sizeof(a));
    formatHHMM(s.nightEnd,   b, sizeof(b));
    snprintf(buf, sizeof(buf), "Nacht %s - %s (%u h)", a, b, s.nightHourCount);
    if (textWidth(buf, &FreeSans9pt7b) > maxW) {
      snprintf(buf, sizeof(buf), "%s - %s (%u h)", a, b, s.nightHourCount);
    }
    drawTextAt(tx, ty, buf, &FreeSans9pt7b);
  }
  ty += 21;

  // Warnungen — eine Zeile, damit nichts aus der Box laeuft
  const char* warn = nullptr;
  if (s.windWarn && s.humidityWarn) warn = "Wind + Tau-Gefahr";
  else if (s.windWarn)              warn = "Wind > 25 km/h";
  else if (s.humidityWarn)          warn = "Tau-Gefahr (> 90 %)";
  if (warn) drawTextAt(tx, ty, warn, &FreeSansBold9pt7b, DISPLAY_RED);
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
//  Einzelblock: NACHT-CHART
//  X-Achse: Stunden im Nachtfenster, Y-Achse: 0..100 % ("oben = schlecht")
//  Wenige Linien, alle 2 px stark (duenne/gepunktete Linien flimmern
//  auf dem 3-Farb-Panel):
//    Wolken (gewichteter Score)  schwarz durchgezogen
//    Seeing      (7Timer!)       rot durchgezogen
//    Transparenz (7Timer!)       schwarz gestrichelt, zuletzt gezeichnet:
//                                7Timer liefert oft Seeing == Transparenz,
//                                rot-auf-rot waere dann unsichtbar
// ---------------------------------------------------------------------
static void drawThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                          uint16_t color, bool dashed) {
  if (dashed) {
    drawDashedLine(x0, y0,     x1, y1,     color, 7, 5);
    drawDashedLine(x0, y0 - 1, x1, y1 - 1, color, 7, 5);
  } else {
    display.drawLine(x0, y0,     x1, y1,     color);
    display.drawLine(x0, y0 - 1, x1, y1 - 1, color);
  }
}

static void drawNightChart(const WeatherData& wd, const SeeingData& sd) {
  display.drawRect(CH_X, CH_Y, CH_W, CH_H, GxEPD_BLACK);
  drawTextAt(CH_X + 6, CH_Y + 18, "WOLKEN IN DER NACHT (%)", &FreeSansBold9pt7b);

  // Legende rechts in der Titelzeile
  {
    struct Item { const char* label; uint16_t color; bool dashed; };
    Item items[3];
    uint8_t cnt = 0;
    items[cnt++] = {"Wolken", GxEPD_BLACK, false};
    if (sd.valid) {
      items[cnt++] = {"Seeing", DISPLAY_RED, false};
      items[cnt++] = {"Transparenz", GxEPD_BLACK, true};
    }
    const int16_t sampleW = 20, gap = 6, itemGap = 18;
    int16_t total = 0;
    for (uint8_t i = 0; i < cnt; i++) {
      total += sampleW + gap + textWidth(items[i].label, &FreeSans9pt7b);
      if (i + 1 < cnt) total += itemGap;
    }
    int16_t lx = CH_X + CH_W - 8 - total;
    for (uint8_t i = 0; i < cnt; i++) {
      drawThickLine(lx, CH_Y + 13, lx + sampleW, CH_Y + 13, items[i].color, items[i].dashed);
      lx += sampleW + gap;
      drawTextAt(lx, CH_Y + 18, items[i].label, &FreeSans9pt7b, items[i].color);
      lx += textWidth(items[i].label, &FreeSans9pt7b) + itemGap;
    }
  }

  // Plot-Bereich unter der Titelzeile
  const int16_t px = CH_X + 36;
  const int16_t py = CH_Y + 32;
  const int16_t pw = CH_W - 36 - 24;
  const int16_t ph = CH_H - 32 - 20;

  // Achsen + ruhiges Gitter (Punkte im 6-px-Raster bei 50 %)
  display.drawLine(px, py, px, py + ph, GxEPD_BLACK);
  display.drawLine(px, py + ph, px + pw, py + ph, GxEPD_BLACK);
  for (int16_t x = px + 6; x < px + pw; x += 6) {
    display.drawPixel(x, py, GxEPD_BLACK);
    display.drawPixel(x, py + ph / 2, GxEPD_BLACK);
  }
  drawTextRight(px - 5, py + 5,          "100", &FreeSans9pt7b);
  drawTextRight(px - 5, py + ph / 2 + 5, "50",  &FreeSans9pt7b);
  drawTextRight(px - 5, py + ph + 5,     "0",   &FreeSans9pt7b);

  uint16_t idx[64];
  uint16_t n = collectNightIndices(wd, idx, 64);
  if (n < 2) {
    drawTextAt(px + 10, py + ph / 2 - 6, "Keine Nachtstunden im Datensatz.",
               &FreeSans9pt7b, DISPLAY_RED);
    return;
  }

  auto xOf = [&](uint16_t k) -> int16_t {
    return px + (int16_t)((long)pw * k / (n - 1));
  };
  auto yOf = [&](float v) -> int16_t {
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    return py + ph - (int16_t)(ph * v / 100.0f);
  };

  // X-Beschriftung alle 2 Stunden, innerhalb der Box gehalten
  for (uint16_t k = 0; k < n; k += 2) {
    char hhmm[8];
    formatHHMM(wd.hourly[idx[k]].ts, hhmm, sizeof(hhmm));
    int16_t w = textWidth(hhmm, &FreeSans9pt7b);
    int16_t x = xOf(k) - w / 2;
    if (x < px) x = px;                                  // nicht unter die Y-Labels
    if (x + w > CH_X + CH_W - 4) x = CH_X + CH_W - 4 - w;
    drawTextAt(x, py + ph + 17, hhmm, &FreeSans9pt7b);
  }

  // Seeing zuerst, damit die Wolkenlinie obenauf liegt.
  // Skala 1 (exzellent) .. 8 (schlecht) → 0..100 %
  if (sd.valid && sd.count >= 2) {
    time_t t0 = wd.hourly[idx[0]].ts;
    time_t t1 = wd.hourly[idx[n - 1]].ts;
    auto tsToX = [&](time_t t) -> int16_t {
      float r = (float)(t - t0) / (float)(t1 - t0);
      if (r < 0.0f) r = 0.0f;
      if (r > 1.0f) r = 1.0f;
      return px + (int16_t)(r * (float)pw);
    };
    auto sToY = [&](uint8_t v) -> int16_t {
      return yOf((float)(v - 1) / 7.0f * 100.0f);
    };
    for (uint8_t i = 1; i < sd.count; i++) {
      time_t ta = sd.points[i - 1].ts;
      time_t tb = sd.points[i].ts;
      if (tb < t0 - 10800L || ta > t1 + 10800L) continue;
      int16_t xa = tsToX(ta), xb = tsToX(tb);
      if (xa == xb) continue;
      drawThickLine(xa, sToY(sd.points[i - 1].seeing),
                    xb, sToY(sd.points[i].seeing), DISPLAY_RED, false);
    }
  }

  for (uint16_t k = 1; k < n; k++) {
    drawThickLine(xOf(k - 1), yOf(wd.astroHourly[idx[k - 1]].cloudScore),
                  xOf(k),     yOf(wd.astroHourly[idx[k]].cloudScore),
                  GxEPD_BLACK, false);
  }

  if (sd.valid && sd.count >= 2) {
    time_t t0 = wd.hourly[idx[0]].ts;
    time_t t1 = wd.hourly[idx[n - 1]].ts;
    auto tsToX = [&](time_t t) -> int16_t {
      float r = (float)(t - t0) / (float)(t1 - t0);
      if (r < 0.0f) r = 0.0f;
      if (r > 1.0f) r = 1.0f;
      return px + (int16_t)(r * (float)pw);
    };
    auto sToY = [&](uint8_t v) -> int16_t {
      return yOf((float)(v - 1) / 7.0f * 100.0f);
    };
    for (uint8_t i = 1; i < sd.count; i++) {
      time_t ta = sd.points[i - 1].ts;
      time_t tb = sd.points[i].ts;
      if (tb < t0 - 10800L || ta > t1 + 10800L) continue;
      int16_t xa = tsToX(ta), xb = tsToX(tb);
      if (xa == xb) continue;
      drawThickLine(xa, sToY(sd.points[i - 1].transparency),
                    xb, sToY(sd.points[i].transparency), GxEPD_BLACK, true);
    }
  }
}

// ---------------------------------------------------------------------
//  Einzelblock: STUNDENTABELLE
//  Spalte je Nachtstunde: Uhrzeit, Wolken-Score, Mondhoehe.
//  Die ISS-Zeile (open-notify) erscheint nur, wenn keine Mondhoehen
//  berechnet sind - fuer vier Zeilen ist die Tabelle zu niedrig.
// ---------------------------------------------------------------------
static void drawHourTable(const WeatherData& wd, const ISSData& iss) {
  display.drawRect(TBL_X, TBL_Y, TBL_W, TBL_H, GxEPD_BLACK);

  uint16_t idx[16];
  uint16_t n = collectNightIndices(wd, idx, 16);
  if (n > 12) n = 12;

  // ISS-Aufgang je Spalte (0 = keiner)
  time_t issAt[12] = {0};
  bool anyIss = false;
  bool moonRow = false;
  for (uint16_t k = 0; k < n; k++) {
    if (!isnan(wd.astroHourly[idx[k]].moonAltDeg)) { moonRow = true; break; }
  }
  for (uint16_t k = 0; k < n && iss.valid && !moonRow; k++) {
    time_t hStart = wd.hourly[idx[k]].ts;
    for (uint8_t p = 0; p < iss.count; p++) {
      time_t rise = iss.passes[p].riseTime;
      if (rise >= hStart && rise < hStart + 3600L) { issAt[k] = rise; anyIss = true; break; }
    }
  }

  const bool thirdRow = anyIss || moonRow;
  const int16_t y1 = TBL_Y + (thirdRow ? 22 : 28);
  const int16_t y2 = y1 + 22;
  const int16_t y3 = y2 + 21;
  const int16_t labelW = 92;

  drawTextAt(TBL_X + 8, y1, "Uhr",      &FreeSansBold9pt7b);
  drawTextAt(TBL_X + 8, y2, "Wolken %", &FreeSans9pt7b);
  if (anyIss)  drawTextAt(TBL_X + 8, y3, "ISS", &FreeSansBold9pt7b, DISPLAY_RED);
  if (moonRow) drawTextAt(TBL_X + 8, y3, "Mond", &FreeSans9pt7b);

  if (n == 0) {
    drawTextAt(TBL_X + labelW, y2, "Keine Nachtstunden", &FreeSans9pt7b);
    return;
  }

  int16_t colW = (TBL_W - labelW - 6) / n;
  for (uint16_t k = 0; k < n; k++) {
    int16_t cx = TBL_X + labelW + k * colW + colW / 2;

    struct tm lt;
    time_t hStart = wd.hourly[idx[k]].ts;
    localtime_r(&hStart, &lt);
    char hh[8]; snprintf(hh, sizeof(hh), "%02d", lt.tm_hour);
    drawTextCentered(cx, y1, hh, &FreeSansBold9pt7b);

    int cs = (int)round(wd.astroHourly[idx[k]].cloudScore);
    char cb[8]; snprintf(cb, sizeof(cb), "%d", cs);
    uint16_t col = (cs > ASTRO_CLOUD_MODERATE) ? DISPLAY_RED : GxEPD_BLACK;
    drawTextCentered(cx, y2, cb, &FreeSans9pt7b, col);

    if (moonRow) {
      // ueber dem Horizont Grad, sonst Strich. Hoch stehender Mond rot.
      float alt = wd.astroHourly[idx[k]].moonAltDeg;
      char mb[8];
      uint16_t mcol = GxEPD_BLACK;
      if (isnan(alt) || alt < 0) {
        strlcpy(mb, "-", sizeof(mb));
      } else {
        snprintf(mb, sizeof(mb), "%d`", (int)lroundf(alt));
        if (alt >= MOON_ALT_RED_DEG) mcol = DISPLAY_RED;
      }
      drawTextCentered(cx, y3, mb, &FreeSans9pt7b, mcol);
    }

    if (issAt[k]) {
      char hhmm[8];
      formatHHMM(issAt[k], hhmm, sizeof(hhmm));
      drawTextCentered(cx, y3, hhmm, &FreeSansBold9pt7b, DISPLAY_RED);
    }
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
  drawTextAt(x + 6, y + 17, title, &FreeSans9pt7b);

  // Plot unter der Titelzeile, rechts eine Spalte fuer die Min/Max-Labels
  const int16_t lblW = 32;
  int16_t px = x + 6;
  int16_t py = y + 25;
  int16_t pw = w - 12 - lblW;
  int16_t ph = h - 25 - 6;

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

  // Min/Max Beschriftung rechts, auf Hoehe von Plot-Ober-/Unterkante
  char buf[12];
  snprintf(buf, sizeof(buf), "%.0f", vmax);
  drawTextRight(x + w - 6, py + 9, buf, &FreeSans9pt7b);
  snprintf(buf, sizeof(buf), "%.0f", vmin);
  drawTextRight(x + w - 6, py + ph, buf, &FreeSans9pt7b);

  if (n == 0) return;

  if (barChart) {
    int16_t bw = pw / (int16_t)n;
    if (bw < 1) bw = 1;
    for (uint16_t i = 0; i < n; i++) {
      float v = values[i];
      if (isnan(v) || v <= 0) continue;
      int16_t bx = px + i * bw;
      int16_t by = yOf(v);
      if (py + ph - by < 3) by = py + ph - 3;   // 0,1 mm sonst nur ein Strich
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

  // 4 gleich breite Boxen mit 4 px Abstand, die letzte buendig am Rand
  int16_t cw = (MC_W + 4) / 4;
  drawMiniChart(MC_X + 0 * cw, MC_Y, cw - 4, MC_H, "Temperatur (`C)",
                tArr, maxN, 0, 0, false, GxEPD_BLACK);
  drawMiniChart(MC_X + 1 * cw, MC_Y, cw - 4, MC_H, "Wind (km/h)",
                wArr, maxN, 0, 0, false, GxEPD_BLACK);
  drawMiniChart(MC_X + 2 * cw, MC_Y, cw - 4, MC_H, "Feuchte (%)",
                hArr, maxN, 0, 100, false, GxEPD_BLACK);
  drawMiniChart(MC_X + 3 * cw, MC_Y, MC_X + MC_W - (MC_X + 3 * cw), MC_H, "Regen (mm)",
                pArr, maxN, 0, 0, true, DISPLAY_RED);
}

// ---------------------------------------------------------------------
//  Einzelblock: FORECAST (FORECAST_SHOW_DAYS Tage)
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

  uint8_t n = wd.dailyCount;
  if (n > FORECAST_SHOW_DAYS) n = FORECAST_SHOW_DAYS;
  if (n < 1) return;

  int16_t cellW = FC_W / n;
  for (uint8_t i = 0; i < n; i++) {
    int16_t x = FC_X + i * cellW;
    int16_t cx = x + cellW / 2;

    const DailyData& d = wd.daily[i];

    // Wochentag
    char wd3[6]; snprintf(wd3, sizeof(wd3), "%s", shortWeekdayDE(d.date));
    drawTextCentered(cx, FC_Y + 20, wd3, &FreeSansBold9pt7b);

    // Tmin / Tmax
    char tb[24];
    // ganzzahlig runden, sonst wird -0,4 zu "-0"
    snprintf(tb, sizeof(tb), "%ld` / %ld`",
             isnan(d.tMinC) ? 0L : lroundf(d.tMinC),
             isnan(d.tMaxC) ? 0L : lroundf(d.tMaxC));
    drawTextCentered(cx, FC_Y + 41, tb, &FreeSans9pt7b);

    // Nachtbewertung (gleiche Begriffe wie NACHT-BEWERTUNG)
    AstroVerdict v = quickNightVerdict(wd, d.date);
    const char* label;
    uint16_t col = GxEPD_BLACK;
    switch (v) {
      case ASTRO_GOOD:     label = "GUT";      break;
      case ASTRO_MODERATE: label = "MITTEL";   break;
      case ASTRO_BAD:      label = "SCHLECHT"; col = DISPLAY_RED; break;
      default:             label = "--";
    }
    drawTextCentered(cx, FC_Y + 62, label, &FreeSansBold9pt7b, col);
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
// Himmelsaufhellung durch den Mond als Label, rot nur bei hoher Aufhellung
static const char* moonSkyLabel(const MoonInfo& m, uint16_t& color) {
  color = GxEPD_BLACK;
  if (m.skyImpact < 0.10f) return "Himmel: dunkel";
  if (m.skyImpact < 0.35f) return "Himmel: mittel";
  color = DISPLAY_RED;
  return "Himmel: HELL";
}

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

  // Höhe ueber/unter dem Horizont
  ty += 20;
  if (m.altitudeDeg > 0.0f) {
    snprintf(buf, sizeof(buf), "Hoehe %.0f`", m.altitudeDeg);
  } else {
    snprintf(buf, sizeof(buf), "unter Horizont");
  }
  drawTextAt(tx, ty, buf, &FreeSans9pt7b);

  // Sky-Impact – als Label, rot nur bei hoher Aufhellung
  ty += 20;
  uint16_t impactCol;
  drawTextAt(tx, ty, moonSkyLabel(m, impactCol), &FreeSansBold9pt7b, impactCol);
}

// ---------------------------------------------------------------------
//  Public: Komplettes Dashboard
// ---------------------------------------------------------------------
void renderDashboard(const WeatherData& wd, BatteryInfo batt,
                     const LocalSensorData& sensor,
                     const SeeingData& seeing,
                     const ISSData& iss) {
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
    drawHourTable(wd, iss);
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
    drawTextAt(14, 19, "AstroOyInk", &FreeSansBold12pt7b, GxEPD_WHITE);

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
    drawTextAt(14, 19, "AstroOyInk - WLAN-Einrichtung", &FreeSansBold12pt7b, GxEPD_WHITE);

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
    drawTextAt(60, y, "3.  Formular ausfuellen und speichern - das Geraet startet neu.",
               &FreeSans9pt7b);

    // Hinweis Timeout
    y += 30;
    drawTextCentered(W / 2, y, "Das Portal schliesst nach 3 Minuten, danach neuer Versuch.",
                     &FreeSans9pt7b);

    // Trennlinie
    display.drawLine(40, y + 14, W - 40, y + 14, GxEPD_BLACK);
  } while (display.nextPage());
}

// =====================================================================
//  SESSION-Dashboard
//  Layout (800 x 480), gleiche Gestaltung wie WEATHER: Kaesten mit
//  9pt-Titel, je Kasten ein grosser Leitwert.
//
//    +--------------------------------------------------------+ y=0
//    | HEADER  Ort | Datum | Akku | Stand                     |
//    +--------------------------------------------------------+ y=30
//    | SEQUENZ                                    Target i/n  |
//    | <Targetname gross>                           <Status>  |
//    | Sub i/n  [#########...............]        Noch 1h 23m |
//    +------------------+------------------+------------------+ y=130
//    | KAMERA           | GUIDING          | MOUNT            |
//    |   2.31           |   0.57"          |   45 min         |
//    |   HFR (px)       |   RMS gesamt     |   bis Flip       |
//    |   Details ...    |   Details ...    |   Details ...    |
//    +------------------+------------------+------------------+ y=404
//    | NACHT  Mond | Hoehe | Himmel            Sonnenaufgang  |
//    +--------------------------------------------------------+ y=478
//
//  Farb-Logik: Normaltext schwarz, Warnungen DISPLAY_RED (bei BW-Panel
//  invertierter Block, siehe drawWarnLabel).
// =====================================================================

// --- SESSION-Layout-Konstanten ---
static const int16_t SS_SEQ_X = 4,   SS_SEQ_Y = 30;
static const int16_t SS_SEQ_W = 792, SS_SEQ_H = 96;

static const int16_t SS_COL_Y = 130, SS_COL_H = 270;

static const int16_t SS_NIGHT_X = 4,   SS_NIGHT_Y = 404;
static const int16_t SS_NIGHT_W = 792, SS_NIGHT_H = 74;

// Zeilen innerhalb einer Spalte (relativ zu SS_COL_Y)
static const int16_t SS_BIG_Y     = 72;   // Baseline Leitwert (24pt)
static const int16_t SS_CAPTION_Y = 96;   // Beschriftung unter dem Leitwert
static const int16_t SS_ROW0_Y    = 140;  // erste Detailzeile
static const int16_t SS_ROW_STEP  = 26;

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

// Hilfsfunktion: Dec (Grad) → "+35`48'" (Backtick = Gradzeichen)
static void formatDecStr(float decDeg, char* buf, size_t sz) {
  char sign = (decDeg >= 0) ? '+' : '-';
  float absDec = fabsf(decDeg);
  int d = (int)absDec;
  int m = (int)((absDec - d) * 60.0f + 0.5f);
  if (m >= 60) { d++; m = 0; }
  snprintf(buf, sz, "%c%02d%c %02d'", sign, d, DEG, m);
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

// Spaltenkasten mit Titel, grossem Leitwert und Beschriftung darunter
static void drawSessionColumnHead(int16_t x, int16_t w, const char* title,
                                  const char* big, const char* caption,
                                  uint16_t bigColor) {
  display.drawRect(x, SS_COL_Y, w, SS_COL_H, GxEPD_BLACK);
  drawTextAt(x + 6, SS_COL_Y + 18, title, &FreeSansBold9pt7b);
  const GFXfont* f = fitFont(big, w - 16, &FreeSansBold24pt7b, &FreeSansBold18pt7b);
  drawTextCentered(x + w / 2, SS_COL_Y + SS_BIG_Y, big, f, bigColor);
  drawTextCentered(x + w / 2, SS_COL_Y + SS_CAPTION_Y, caption, &FreeSans9pt7b);
  display.drawLine(x + 10, SS_COL_Y + 112, x + w - 10, SS_COL_Y + 112, GxEPD_BLACK);
}

// Detailzeile: Label links, Wert rechtsbuendig. Ohne Label (Geraetename)
// steht der Wert fett links.
static void drawSessionRow(int16_t x, int16_t w, uint8_t row,
                           const char* label, const char* value,
                           uint16_t valueColor = GxEPD_BLACK) {
  int16_t y = SS_COL_Y + SS_ROW0_Y + row * SS_ROW_STEP;
  if (!label || !label[0]) {
    drawTextAt(x + 10, y, value, fitFont(value, w - 20, &FreeSansBold9pt7b, &FreeSans9pt7b),
               valueColor);
    return;
  }
  drawTextAt(x + 10, y, label, &FreeSans9pt7b);
  const int16_t maxW = w - 20 - textWidth(label, &FreeSans9pt7b) - 8;
  const GFXfont* f = fitFont(value, maxW, &FreeSansBold9pt7b, &FreeSans9pt7b);
  drawTextRight(x + w - 10, y, value, f, valueColor);
}

// ---------------------------------------------------------------------
//  SESSION-Spalte: KAMERA — Leitwert HFR des letzten Bildes
// ---------------------------------------------------------------------
static void drawSessionCamera(int16_t x, int16_t w, const NinaCameraInfo& c) {
  char big[16];
  if (c.hfr > 0) snprintf(big, sizeof(big), "%.2f", c.hfr);
  else           strlcpy(big, "--", sizeof(big));
  drawSessionColumnHead(x, w, "KAMERA", big, "HFR (px)", GxEPD_BLACK);

  char buf[24];
  drawSessionRow(x, w, 0, "", c.connected ? c.name : "nicht verbunden",
                 c.connected ? GxEPD_BLACK : DISPLAY_RED);
  drawSessionRow(x, w, 1, "Filter",
                 (c.filterName[0] != '\0') ? c.filterName : "--");
  if (c.exposureS > 0) snprintf(buf, sizeof(buf), "%.0f s", c.exposureS);
  else                 strlcpy(buf, "--", sizeof(buf));
  drawSessionRow(x, w, 2, "Belichtung", buf);

  // Sensor-Temperatur ist/soll. Rot, wenn gekuehlt werden soll, der
  // Kuehler aber aus ist oder das Soll um mehr als 1,5 K verfehlt wird.
  if (!c.connected || isnan(c.tempC)) {
    drawSessionRow(x, w, 3, "Sensor", "--");
    return;
  }
  bool tempWarn = c.canCool && !isnan(c.setpointC) &&
                  (!c.coolerOn || fabsf(c.tempC - c.setpointC) > 1.5f);
  snprintf(buf, sizeof(buf), "%.1f`C", c.tempC);
  drawSessionRow(x, w, 3, "Sensor", buf, tempWarn ? DISPLAY_RED : GxEPD_BLACK);
  if (c.canCool && !isnan(c.setpointC)) {
    if (c.coolerOn) snprintf(buf, sizeof(buf), "%.0f`C / %.0f %%", c.setpointC, c.coolerPowerPct);
    else            snprintf(buf, sizeof(buf), "%.0f`C / aus", c.setpointC);
    drawSessionRow(x, w, 4, "Soll / Kuehler", buf);
  }
}

// ---------------------------------------------------------------------
//  SESSION-Spalte: GUIDING — Leitwert RMS gesamt
// ---------------------------------------------------------------------
static void drawSessionGuiding(int16_t x, int16_t w, const NinaGuidingInfo& g) {
  bool guidingLost = g.connected &&
                     (strstr(g.state, "Lost") || strstr(g.state, "Stop"));
  bool guidingWarn = g.connected && !guidingLost && g.rmsTotal > 0 &&
                     g.rmsTotal >= GUIDING_WARN_RMS_ARCSEC;

  char big[16];
  if (g.connected && g.rmsTotal > 0) snprintf(big, sizeof(big), "%.2f\"", g.rmsTotal);
  else                               strlcpy(big, "--", sizeof(big));
  drawSessionColumnHead(x, w, "GUIDING", big, "RMS gesamt",
                        (guidingLost || guidingWarn) ? DISPLAY_RED : GxEPD_BLACK);

  if (!g.connected) {
    drawSessionRow(x, w, 0, "", "kein Guider", DISPLAY_RED);
    return;
  }

  char buf[24];
  snprintf(buf, sizeof(buf), "%.2f\"", g.rmsRA);
  drawSessionRow(x, w, 0, "RA", buf);
  snprintf(buf, sizeof(buf), "%.2f\"", g.rmsDec);
  drawSessionRow(x, w, 1, "Dec", buf);
  drawSessionRow(x, w, 2, "Status", g.state[0] ? g.state : "--");

  const char* verdict = guidingLost ? "Guiding verloren"
                      : guidingWarn ? "Guiding unruhig"
                      :               "Guiding OK";
  int16_t y = SS_COL_Y + SS_ROW0_Y + 3 * SS_ROW_STEP;
  if (guidingLost || guidingWarn) {
    drawWarnLabel(x + 10, y, verdict);
  } else {
    drawTextAt(x + 10, y, verdict, &FreeSansBold9pt7b);
  }
}

// ---------------------------------------------------------------------
//  SESSION-Spalte: MOUNT — Leitwert Zeit bis zum Meridian-Flip
// ---------------------------------------------------------------------
static void drawSessionMount(int16_t x, int16_t w, const NinaMountInfo& m) {
  char big[20];
  char caption[32] = "bis Flip";
  bool flipSoon = false;
  if (m.connected && m.flipEtaS >= 0) {
    formatEtaStr((uint32_t)m.flipEtaS, big, sizeof(big));
    flipSoon = (m.flipEtaS < 600);   // Warnung wenn Flip in < 10 Minuten
    // ungefaehre Uhrzeit des Flips, auf 5 Minuten gerundet
    time_t now = time(nullptr);
    if (now > 1700000000) {
      time_t at = now + m.flipEtaS;
      at = (at + 150) / 300 * 300;
      char hhmm[8];
      formatHHMM(at, hhmm, sizeof(hhmm));
      snprintf(caption, sizeof(caption), "bis Flip ~%s", hhmm);
    }
  } else {
    strlcpy(big, "--", sizeof(big));
  }
  drawSessionColumnHead(x, w, "MOUNT", big, caption,
                        flipSoon ? DISPLAY_RED : GxEPD_BLACK);

  if (!m.connected) {
    drawSessionRow(x, w, 0, "", "nicht verbunden", DISPLAY_RED);
    return;
  }

  char buf[32];
  drawSessionRow(x, w, 0, "", m.name);
  formatRAStr(m.raH, buf, sizeof(buf));
  drawSessionRow(x, w, 1, "RA", buf);
  formatDecStr(m.decDeg, buf, sizeof(buf));
  drawSessionRow(x, w, 2, "Dec", buf);
  snprintf(buf, sizeof(buf), "%.0f` / %.0f` %s",
           m.altDeg, m.azDeg, azToDir(m.azDeg));
  drawSessionRow(x, w, 3, "Alt / Az", buf, (m.altDeg < 20.0f) ? DISPLAY_RED : GxEPD_BLACK);
  const char* pier = (m.pierSide == 0) ? "West"
                   : (m.pierSide == 1) ? "Ost"
                   :                     "--";
  drawSessionRow(x, w, 4, "Pier", pier);
}

// ---------------------------------------------------------------------
//  SESSION-Block: SEQUENZ — Target, Status, Fortschritt der Subs
// ---------------------------------------------------------------------
static void drawSessionSequence(const NinaSequenceInfo& seq) {
  const int16_t x = SS_SEQ_X, y = SS_SEQ_Y, w = SS_SEQ_W;
  display.drawRect(x, y, w, SS_SEQ_H, GxEPD_BLACK);
  drawTextAt(x + 6, y + 18, "SEQUENZ", &FreeSansBold9pt7b);

  char buf[48];
  if (seq.targetTotal > 0 && seq.targetIndex >= 0) {
    snprintf(buf, sizeof(buf), "Target %d / %d", seq.targetIndex + 1, (int)seq.targetTotal);
    drawTextRight(x + w - 8, y + 18, buf, &FreeSansBold9pt7b);
  }

  // Status rechts in der Namenszeile, rot bei Slewing/Pausiert
  const char* status = seq.status[0] ? seq.status : "--";
  bool statusWarn = (strcmp(seq.status, "Slewing") == 0 ||
                     strcmp(seq.status, "Pausiert") == 0);
  int16_t statusW = textWidth(status, &FreeSansBold12pt7b);
  drawTextRight(x + w - 8, y + 52, status, &FreeSansBold12pt7b,
                statusWarn ? DISPLAY_RED : GxEPD_BLACK);

  // Targetname gross, notfalls kleiner
  const char* name = seq.targetName[0]   ? seq.targetName
                   : seq.sequenceName[0] ? seq.sequenceName
                   :                       "--";
  const int16_t nameMax = w - 16 - statusW - 24;
  const GFXfont* nf = fitFont(name, nameMax, &FreeSansBold18pt7b, &FreeSansBold12pt7b);
  drawTextAt(x + 8, y + 54, name, nf);

  // Fortschritt: "Sub i/n" | Balken | Restzeit
  const int16_t rowY = y + 84;
  if (seq.subsTotal > 0) {
    snprintf(buf, sizeof(buf), "Sub %d / %d", (int)seq.subsDone, (int)seq.subsTotal);
  } else if (seq.subsDone > 0) {
    snprintf(buf, sizeof(buf), "Sub %d", (int)seq.subsDone);  // zeitbasierte Schleife
  } else {
    strlcpy(buf, "Sub --", sizeof(buf));
  }
  drawTextAt(x + 8, rowY, buf, &FreeSansBold9pt7b);

  char eta[24] = "";
  if (seq.etaS > 0) {
    char e[16];
    formatEtaStr(seq.etaS, e, sizeof(e));
    snprintf(eta, sizeof(eta), "noch %s", e);
    drawTextRight(x + w - 8, rowY, eta, &FreeSans9pt7b);
  }

  if (seq.subsTotal > 0) {
    const int16_t bx = x + 120;
    const int16_t bw = (x + w - 8) - (eta[0] ? textWidth(eta, &FreeSans9pt7b) + 16 : 0) - bx;
    const int16_t bh = 12, by = rowY - bh + 1;
    display.drawRect(bx, by, bw, bh, GxEPD_BLACK);
    int32_t done = seq.subsDone < 0 ? 0 : seq.subsDone;
    if (done > seq.subsTotal) done = seq.subsTotal;
    int16_t fw = (int16_t)((int32_t)(bw - 4) * done / seq.subsTotal);
    if (fw > 0) display.fillRect(bx + 2, by + 2, fw, bh - 4, GxEPD_BLACK);
  }
}

// ---------------------------------------------------------------------
//  SESSION-Block: NACHT — Mond (lokal berechnet) und Sonnenaufgang
// ---------------------------------------------------------------------
static void drawSessionNight(const MoonInfo& moon, int32_t sunriseTodS) {
  const int16_t x = SS_NIGHT_X, y = SS_NIGHT_Y, w = SS_NIGHT_W;
  display.drawRect(x, y, w, SS_NIGHT_H, GxEPD_BLACK);
  drawTextAt(x + 6, y + 18, "NACHT", &FreeSansBold9pt7b);

  drawMoonPhaseIcon(x + 120, y + SS_NIGHT_H / 2, 24, moon.phase);

  char buf[48];
  snprintf(buf, sizeof(buf), "%s, %.0f %% beleuchtet",
           moon.phaseName, moon.illumination * 100.0f);
  drawTextAt(x + 156, y + 30, buf, &FreeSansBold9pt7b);

  if (moon.altitudeDeg > 0.0f) snprintf(buf, sizeof(buf), "Hoehe %.0f`", moon.altitudeDeg);
  else                         strlcpy(buf, "unter Horizont", sizeof(buf));
  drawTextAt(x + 156, y + 54, buf, &FreeSans9pt7b);
  uint16_t skyCol;
  drawTextAt(x + 156 + textWidth(buf, &FreeSans9pt7b) + 24, y + 54,
             moonSkyLabel(moon, skyCol), &FreeSansBold9pt7b, skyCol);

  if (sunriseTodS >= 0) {
    snprintf(buf, sizeof(buf), "%02ld:%02ld",
             (long)(sunriseTodS / 3600), (long)((sunriseTodS % 3600) / 60));
    drawTextRight(x + w - 10, y + 30, "Sonnenaufgang", &FreeSans9pt7b);
    drawTextRight(x + w - 10, y + 58, buf, &FreeSansBold12pt7b);
  }
}

// ---------------------------------------------------------------------
//  renderSessionDashboard — public
// ---------------------------------------------------------------------
void renderSessionDashboard(const NinaData& data, const BatteryInfo& batt,
                            const char* locationName,
                            const MoonInfo& moon, int32_t sunriseTodS) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    drawHeaderImpl(locationName ? locationName : LOCATION_NAME,
                   time(nullptr), batt);

    if (!data.valid) {
      drawTextCentered(W / 2, H / 2, "N.I.N.A. nicht erreichbar",
                       &FreeSansBold18pt7b, DISPLAY_RED);
      continue;
    }

    drawSessionSequence(data.sequence);

    // 3 gleich breite Spalten mit 4 px Abstand, die letzte buendig am Rand
    const int16_t cw = (SS_SEQ_W + 4) / 3;
    drawSessionCamera (SS_SEQ_X,          cw - 4, data.camera);
    drawSessionGuiding(SS_SEQ_X + cw,     cw - 4, data.guiding);
    drawSessionMount  (SS_SEQ_X + 2 * cw, SS_SEQ_X + SS_SEQ_W - (SS_SEQ_X + 2 * cw),
                       data.mount);

    drawSessionNight(moon, sunriseTodS);
  } while (display.nextPage());
}
