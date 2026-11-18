// =====================================================================
//  tools/preview/stubs/Print.h
//  Minimaler Print-Ersatz (Basisklasse von Adafruit_GFX).
// =====================================================================
#ifndef PREVIEW_PRINT_H
#define PREVIEW_PRINT_H

#include "Arduino.h"

class Print {
 public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t* b, size_t n) {
    size_t r = 0;
    while (n--) r += write(*b++);
    return r;
  }
  size_t write(const char* s) { return write((const uint8_t*)s, strlen(s)); }
  size_t print(const char* s) { return write(s); }
  size_t print(const String& s) { return write(s.c_str()); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(int v) {
    char b[16];
    snprintf(b, sizeof(b), "%d", v);
    return write(b);
  }
};

#endif // PREVIEW_PRINT_H
