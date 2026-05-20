// =====================================================================
//  data_types.h
//  Strukturen für Wetter- und Astrodaten.
//  Bewusst flach gehalten (POD), damit ArduinoJson direkt befüllen kann
//  und der Speicherbedarf vorhersagbar bleibt.
// =====================================================================
#ifndef ASTRO_DATA_TYPES_H
#define ASTRO_DATA_TYPES_H

#include <Arduino.h>
#include "config.h"
#include "moon_calc.h"

// Wir lesen 5 Tage * 24 Stunden hourly = 120 Werte. Reserve.
#define MAX_HOURLY   (FORECAST_DAYS * 24)
#define MAX_DAILY     7

// ---------------------------------------------------------------------
//  Eine Stunde aus dem Open-Meteo "hourly"-Block
// ---------------------------------------------------------------------
struct HourlyData {
  time_t  ts;             // Unix-Zeitstempel der Stunde (lokal)
  float   tempC;          // °C
  float   humidity;       // %
  float   dewpointC;      // °C
  float   windKmh;        // km/h
  float   cloudTotal;     // %
  float   cloudLow;       // %
  float   cloudMid;       // %
  float   cloudHigh;      // %
  float   precipMm;       // mm
  float   pressureHpa;    // hPa
};

// ---------------------------------------------------------------------
//  Ein Tag aus dem Open-Meteo "daily"-Block
// ---------------------------------------------------------------------
struct DailyData {
  time_t  date;          // Mitternacht des Tages, lokal
  float   tMaxC;
  float   tMinC;
  float   precipSumMm;
  time_t  sunrise;       // Unix lokal
  time_t  sunset;        // Unix lokal
};

// ---------------------------------------------------------------------
//  Astro-Bewertung einer einzelnen Stunde
// ---------------------------------------------------------------------
struct AstroHour {
  time_t  ts;
  float   cloudScore;    // gewichteter Wolken-Score 0..100
  float   windKmh;
  float   humidity;
  bool    inNightWindow; // true = Stunde liegt im "echten" Nachtfenster
};

// ---------------------------------------------------------------------
//  Bewertung der ganzen Nacht
// ---------------------------------------------------------------------
enum AstroVerdict : uint8_t {
  ASTRO_GOOD = 0,
  ASTRO_MODERATE = 1,
  ASTRO_BAD = 2,
  ASTRO_NO_NIGHT = 3   // z. B. Polartag oder noch keine Daten
};

struct AstroSummary {
  AstroVerdict verdict;
  float        avgCloudScore;   // 0..100
  float        maxWindKmh;
  float        maxHumidity;
  uint8_t      nightHourCount;  // Stunden im Nachtfenster
  time_t       nightStart;
  time_t       nightEnd;
  bool         windWarn;
  bool         humidityWarn;
  String       conclusion;      // Kurztext für Fazit
};

// ---------------------------------------------------------------------
//  Sammelstruktur, die zwischen den Modulen herumgereicht wird
// ---------------------------------------------------------------------
struct WeatherData {
  // Metadaten
  bool      valid;          // Daten brauchbar?
  time_t    fetchedAt;      // Zeitpunkt des letzten erfolgreichen Fetch
  String    locationName;

  // Hourly
  uint16_t  hourlyCount;
  HourlyData hourly[MAX_HOURLY];

  // Daily
  uint8_t   dailyCount;
  DailyData daily[MAX_DAILY];

  // Pro Stunde berechnete Astro-Werte (parallel zu hourly[])
  AstroHour astroHourly[MAX_HOURLY];

  // Aggregierte Bewertung der kommenden Nacht
  AstroSummary summary;

  // Mond: aktuell + Mitte der Nacht (für Astro-Planung wichtiger)
  MoonInfo moonNow;
  MoonInfo moonNight;
};

#endif // ASTRO_DATA_TYPES_H
