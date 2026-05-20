// =====================================================================
//  time_utils.h
// =====================================================================
#ifndef ASTRO_TIME_UTILS_H
#define ASTRO_TIME_UTILS_H

#include <Arduino.h>
#include <time.h>

// Zeit per NTP synchronisieren. Blockiert bis ~10 s.
// Liefert true, wenn die Zeit plausibel gesetzt wurde.
bool timeSyncNTP();

// Wandelt einen ISO-8601-String "YYYY-MM-DDTHH:MM" (lokal) in time_t (lokal).
// Für Open-Meteo, das bei timezone= Lokalzeit liefert.
time_t parseIsoLocal(const char* iso);

// "HH:MM" formatieren
void   formatHHMM(time_t t, char* out, size_t outLen);

// "Tag DD.MM. HH:MM" formatieren
void   formatLong(time_t t, char* out, size_t outLen);

// "Mo", "Di", ... (deutsch)
const char* shortWeekdayDE(time_t t);

#endif // ASTRO_TIME_UTILS_H
