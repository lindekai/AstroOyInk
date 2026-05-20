// =====================================================================
//  weather_api.h
//  Laedt und parst die Open-Meteo-Vorhersage.
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTRO_WEATHER_API_H
#define ASTRO_WEATHER_API_H

#include "data_types.h"

// Baut die Open-Meteo-URL zusammen (für Logging/Debug).
String buildOpenMeteoUrl(float lat, float lon, uint8_t days);

// Holt das JSON, parst es und befüllt 'out'.
// Rückgabe: true bei Erfolg.  Bei false bleibt 'out' unverändert,
// damit der Aufrufer weiterhin die alten Daten anzeigen kann.
bool fetchWeather(float lat, float lon, uint8_t days, WeatherData& out);

#endif // ASTRO_WEATHER_API_H
