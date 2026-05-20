// =====================================================================
//  moon_calc.cpp
//  Implementierung nach Meeus (vereinfacht).
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#include "moon_calc.h"
#include <math.h>

static const double DEG = M_PI / 180.0;

// --- Julianisches Datum aus Unix-UTC -----------------------------------
static inline double julianDate(time_t utcTime) {
  return (double)utcTime / 86400.0 + 2440587.5;
}

// --- Synodische Phase (0..1) -------------------------------------------
static double computePhase(double JD) {
  const double synodicMonth = 29.5305888531;
  const double refNewMoon   = 2451550.09765;   // 2000-01-06 18:14 UTC
  double days  = JD - refNewMoon;
  double phase = fmod(days / synodicMonth, 1.0);
  if (phase < 0) phase += 1.0;
  return phase;
}

// --- Grobe Mondposition (ekliptische Länge/Breite) ---------------------
// Nur die Haupt-Terme; Fehler ca. 1 Grad, für Höhe völlig ausreichend.
static void moonEclipticPos(double JD, double& lambdaDeg, double& betaDeg) {
  double T = (JD - 2451545.0) / 36525.0;

  double L  = 218.3164591 + 481267.88134236 * T;   // mittlere Länge
  double D  = 297.8502042 + 445267.1115168  * T;   // mittlere Elongation
  double Mm = 134.9634114 + 477198.8676313  * T;   // mittlere Anomalie Mond
  double F  =  93.2720993 + 483202.0175273  * T;   // Breite-Argument

  // auf 0..360 normalisieren
  auto norm = [](double x){ double r = fmod(x, 360.0); if (r < 0) r += 360.0; return r; };
  L  = norm(L);
  D  = norm(D);
  Mm = norm(Mm);
  F  = norm(F);

  // Haupt-Korrekturterme für die ekliptische Länge (Grad)
  lambdaDeg = L
            + 6.289  * sin(Mm * DEG)
            - 1.274  * sin((2*D - Mm) * DEG)
            + 0.658  * sin(2*D * DEG)
            - 0.186  * sin((Mm - 2*D) * DEG);
  lambdaDeg = norm(lambdaDeg);

  // Ekliptische Breite
  betaDeg   = 5.128  * sin(F * DEG)
            + 0.281  * sin((Mm + F) * DEG)
            - 0.278  * sin((Mm - F) * DEG);
}

// --- Greenwich Mean Sidereal Time in Grad ------------------------------
static double gmstDeg(double JD) {
  double T = (JD - 2451545.0) / 36525.0;
  double gmst = 280.46061837
              + 360.98564736629 * (JD - 2451545.0)
              +   0.000387933   * T * T
              -   T * T * T / 38710000.0;
  double r = fmod(gmst, 360.0);
  if (r < 0) r += 360.0;
  return r;
}

// --- Phasen-Name -------------------------------------------------------
static const char* nameForPhase(double p) {
  if      (p < 0.03) return "Neumond";
  else if (p < 0.22) return "Zun. Sichel";
  else if (p < 0.28) return "1. Viertel";
  else if (p < 0.47) return "Zun. Mond";
  else if (p < 0.53) return "Vollmond";
  else if (p < 0.72) return "Abn. Mond";
  else if (p < 0.78) return "L. Viertel";
  else if (p < 0.97) return "Abn. Sichel";
  else               return "Neumond";
}

// --- Hauptfunktion -----------------------------------------------------
MoonInfo computeMoon(time_t utcTime, float latitudeDeg, float longitudeDeg) {
  MoonInfo m;

  const double JD     = julianDate(utcTime);
  const double phase  = computePhase(JD);
  const double illum  = (1.0 - cos(2.0 * M_PI * phase)) / 2.0;

  m.phase         = (float)phase;
  m.illumination  = (float)illum;
  m.phaseName     = nameForPhase(phase);

  // Mondposition in ekliptischen Koordinaten
  double lambdaD, betaD;
  moonEclipticPos(JD, lambdaD, betaD);

  // In äquatoriale Koordinaten (Rektaszension, Deklination)
  const double epsilon = 23.4397 * DEG;
  const double lambdaR = lambdaD * DEG;
  const double betaR   = betaD   * DEG;

  double sinDelta = sin(betaR) * cos(epsilon)
                  + cos(betaR) * sin(epsilon) * sin(lambdaR);
  double delta    = asin(sinDelta);
  double alpha    = atan2(sin(lambdaR) * cos(epsilon) - tan(betaR) * sin(epsilon),
                          cos(lambdaR));

  // Local sidereal time (ortsabhängig)
  double lstDeg = gmstDeg(JD) + longitudeDeg;
  double H      = lstDeg * DEG - alpha;        // Stundenwinkel (rad)

  // Höhe über Horizont
  const double latR = latitudeDeg * DEG;
  double sinAlt = sin(latR) * sin(delta)
                + cos(latR) * cos(delta) * cos(H);
  double altR   = asin(sinAlt);
  m.altitudeDeg = (float)(altR / DEG);

  // Grobes "Sky Impact": unter Horizont → 0, sonst illum · sin(höhe)
  if (m.altitudeDeg <= 0.0f) {
    m.skyImpact = 0.0f;
  } else {
    m.skyImpact = (float)(illum * sinAlt);
  }

  return m;
}
