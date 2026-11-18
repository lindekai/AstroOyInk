// =====================================================================
//  tools/preview/stubs/GxEPD2_3C.h
//  Ersatz fuer GxEPD2 in der Host-Vorschau: zeichnet in einen 800x480-
//  Puffer und schreibt am Ende des Page-Loops ein PNG.
//
//  Farblogik wie GxEPD2_3C::drawPixel (GxEPD2 1.6.x): WHITE → weiss,
//  BLACK → schwarz, RED/YELLOW → rot, jede andere Farbe → weiss.
// =====================================================================
#ifndef PREVIEW_GXEPD2_3C_H
#define PREVIEW_GXEPD2_3C_H

#include <Adafruit_GFX.h>
#include <zlib.h>
#include <vector>

#define GxEPD_BLACK     0x0000
#define GxEPD_WHITE     0xFFFF
#define GxEPD_DARKGREY  0x7BEF
#define GxEPD_LIGHTGREY 0xC618
#define GxEPD_RED       0xF800
#define GxEPD_YELLOW    0xFFE0

// Ziel-Datei fuer den naechsten Page-Loop (setzt preview_main.cpp)
inline const char* g_previewOut = "preview.png";

// Panel-Toene, angelehnt an das echte 7,5"-B/W/R-Panel
static const uint8_t kPanelRgb[3][3] = {
  {0xEC, 0xEB, 0xE4},   // 0 = weiss (Papier)
  {0x1B, 0x1B, 0x1B},   // 1 = schwarz
  {0xB0, 0x1E, 0x28},   // 2 = rot
};

inline void previewWritePng(const char* path, const uint8_t* px, int w, int h,
                            int scale) {
  int ow = w * scale, oh = h * scale;
  std::vector<uint8_t> raw;
  raw.reserve((size_t)(ow * 3 + 1) * oh);
  for (int y = 0; y < oh; y++) {
    raw.push_back(0);  // Filter: none
    for (int x = 0; x < ow; x++) {
      const uint8_t* c = kPanelRgb[px[(y / scale) * w + x / scale]];
      raw.insert(raw.end(), c, c + 3);
    }
  }
  uLongf zlen = compressBound(raw.size());
  std::vector<uint8_t> z(zlen);
  compress2(z.data(), &zlen, raw.data(), raw.size(), 9);
  z.resize(zlen);

  FILE* f = fopen(path, "wb");
  if (!f) { perror(path); return; }
  auto be32 = [&](uint32_t v) {
    uint8_t b[4] = {uint8_t(v >> 24), uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)};
    fwrite(b, 1, 4, f);
  };
  auto chunk = [&](const char* type, const uint8_t* data, uint32_t len) {
    be32(len);
    fwrite(type, 1, 4, f);
    if (len) fwrite(data, 1, len, f);
    uLong crc = crc32(0, (const Bytef*)type, 4);
    if (len) crc = crc32(crc, data, len);
    be32((uint32_t)crc);
  };
  static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  fwrite(sig, 1, 8, f);
  uint8_t ihdr[13] = {uint8_t(ow >> 24), uint8_t(ow >> 16), uint8_t(ow >> 8), uint8_t(ow),
                      uint8_t(oh >> 24), uint8_t(oh >> 16), uint8_t(oh >> 8), uint8_t(oh),
                      8, 2, 0, 0, 0};  // 8 bit, RGB
  chunk("IHDR", ihdr, 13);
  chunk("IDAT", z.data(), (uint32_t)z.size());
  chunk("IEND", nullptr, 0);
  fclose(f);
  fprintf(stderr, "[PREVIEW] %s geschrieben (%dx%d)\n", path, ow, oh);
}

class GxEPD2_750c_Z08 {
 public:
  static const uint16_t WIDTH  = 800;
  static const uint16_t HEIGHT = 480;
  GxEPD2_750c_Z08(int, int, int, int) {}
};

inline int g_previewScale = 1;

template <typename Driver, uint16_t PageHeight>
class GxEPD2_3C : public Adafruit_GFX {
 public:
  explicit GxEPD2_3C(Driver) : Adafruit_GFX(Driver::WIDTH, Driver::HEIGHT),
                               px_(Driver::WIDTH * Driver::HEIGHT, 0) {}

  void init(uint32_t = 0, bool = true, uint16_t = 10, bool = false) {}
  void setFullWindow() {}
  void hibernate() {}
  void powerOff() {}

  void firstPage() { std::fill(px_.begin(), px_.end(), 0); }
  bool nextPage() {
    previewWritePng(g_previewOut, px_.data(), Driver::WIDTH, Driver::HEIGHT,
                    g_previewScale);
    return false;
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < 0 || x >= width() || y < 0 || y >= height()) return;
    switch (getRotation()) {
      case 1: { int16_t t = x; x = Driver::WIDTH - y - 1; y = t; break; }
      case 2: x = Driver::WIDTH - x - 1; y = Driver::HEIGHT - y - 1; break;
      case 3: { int16_t t = x; x = y; y = Driver::HEIGHT - t - 1; break; }
    }
    uint8_t v = 0;
    if (color == GxEPD_BLACK) v = 1;
    else if (color == GxEPD_RED || color == GxEPD_YELLOW) v = 2;
    px_[(size_t)y * Driver::WIDTH + x] = v;
  }

 private:
  std::vector<uint8_t> px_;
};

#endif // PREVIEW_GXEPD2_3C_H
