// =====================================================================
//  time_utils.cpp
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "time_utils.h"
#include "config.h"

bool timeSyncNTP(const char* posixTz) {
  // configTzTime setzt den POSIX-TZ-String UND die NTP-Server in einem Rutsch
  // und speichert TZ korrekt – anders als die Kombination
  // setenv("TZ",...) + configTime(0,0,...), bei der configTime das TZ
  // wieder auf UTC zurücksetzt.
  configTzTime(posixTz, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

  // Auf Zeit warten – max ~10 s
  const uint32_t startMs = millis();
  time_t now = 0;
  while ((millis() - startMs) < 10000UL) {
    now = time(nullptr);
    if (now > 1700000000) {  // ungefähr 2023+
      struct tm lt; localtime_r(&now, &lt);
      Serial.printf("[TIME] NTP sync ok: %ld  (lokal %02d:%02d:%02d, tz=%s)\n",
                    (long)now, lt.tm_hour, lt.tm_min, lt.tm_sec, posixTz);
      return true;
    }
    delay(200);
  }
  Serial.println("[TIME] NTP sync FAILED");
  return false;
}

time_t parseIsoLocal(const char* iso) {
  // Erwartet: "YYYY-MM-DDTHH:MM" oder "YYYY-MM-DDTHH:MM:SS"
  if (!iso || strlen(iso) < 16) return 0;

  struct tm t = {};
  int Y, M, D, h, m, s = 0;
  int n = sscanf(iso, "%4d-%2d-%2dT%2d:%2d:%2d", &Y, &M, &D, &h, &m, &s);
  if (n < 5) return 0;

  t.tm_year = Y - 1900;
  t.tm_mon  = M - 1;
  t.tm_mday = D;
  t.tm_hour = h;
  t.tm_min  = m;
  t.tm_sec  = s;
  t.tm_isdst = -1;   // mktime soll DST aus TZ ermitteln

  // Da Open-Meteo bei timezone=Europe/Berlin lokale Zeitwerte liefert,
  // ist mktime() korrekt – es interpretiert tm als Lokalzeit.
  return mktime(&t);
}

void formatHHMM(time_t t, char* out, size_t outLen) {
  struct tm lt;
  localtime_r(&t, &lt);
  snprintf(out, outLen, "%02d:%02d", lt.tm_hour, lt.tm_min);
}

void formatLong(time_t t, char* out, size_t outLen) {
  struct tm lt;
  localtime_r(&t, &lt);
  snprintf(out, outLen, "%s %02d.%02d. %02d:%02d",
           shortWeekdayDE(t),
           lt.tm_mday, lt.tm_mon + 1,
           lt.tm_hour, lt.tm_min);
}

const char* shortWeekdayDE(time_t t) {
  static const char* DAYS[7] = {"So","Mo","Di","Mi","Do","Fr","Sa"};
  struct tm lt;
  localtime_r(&t, &lt);
  if (lt.tm_wday < 0 || lt.tm_wday > 6) return "??";
  return DAYS[lt.tm_wday];
}
