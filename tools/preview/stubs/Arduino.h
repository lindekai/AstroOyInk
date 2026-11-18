// =====================================================================
//  tools/preview/stubs/Arduino.h
//  Minimaler Arduino-Ersatz fuer die Host-Vorschau. Nur was der
//  Rendering-Code und Adafruit GFX brauchen.
// =====================================================================
#ifndef PREVIEW_ARDUINO_H
#define PREVIEW_ARDUINO_H

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <math.h>
#include <string>

#define PROGMEM
#define HIGH 1
#define LOW  0
#define INPUT  0
#define OUTPUT 1

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#define radians(deg) ((deg) * PI / 180.0)
#define degrees(rad) ((rad) * 180.0 / PI)

typedef bool    boolean;
typedef uint8_t byte;

class __FlashStringHelper;

class String {
 public:
  String(const char* c = "") : s_(c ? c : "") {}
  String(const std::string& x) : s_(x) {}
  String(int v) : s_(std::to_string(v)) {}
  String(unsigned v) : s_(std::to_string(v)) {}
  String(long v) : s_(std::to_string(v)) {}
  String(float v, int dec = 2) {
    char b[32];
    snprintf(b, sizeof(b), "%.*f", dec, v);
    s_ = b;
  }
  const char* c_str() const { return s_.c_str(); }
  unsigned length() const { return (unsigned)s_.size(); }
  String& operator+=(const String& o) { s_ += o.s_; return *this; }
  String& operator+=(const char* o) { s_ += o; return *this; }
  friend String operator+(const String& a, const String& b) { return String(a.s_ + b.s_); }
  friend String operator+(const String& a, const char* b) { return String(a.s_ + b); }
  friend String operator+(const char* a, const String& b) { return String(a + b.s_); }
  bool operator==(const char* o) const { return s_ == o; }
  bool operator==(const String& o) const { return s_ == o.s_; }

 private:
  std::string s_;
};

struct PreviewSerial {
  void begin(long) {}
  void flush() {}
  int printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return r;
  }
  void print(const char* s) { fputs(s, stderr); }
  void print(const String& s) { fputs(s.c_str(), stderr); }
  void println(const char* s = "") { fprintf(stderr, "%s\n", s); }
  void println(const String& s) { fprintf(stderr, "%s\n", s.c_str()); }
};
static PreviewSerial Serial;

inline uint32_t millis() { return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC); }
inline void delay(uint32_t) {}
inline int  digitalRead(int) { return LOW; }
inline void pinMode(int, int) {}

// time_utils.cpp ruft das beim NTP-Sync — hier nur die Zeitzone setzen
inline void configTzTime(const char* tz, const char*, const char* = nullptr,
                         const char* = nullptr) {
  setenv("TZ", tz, 1);
  tzset();
}

#include "Print.h"

#endif // PREVIEW_ARDUINO_H
