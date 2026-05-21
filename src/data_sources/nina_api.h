// =====================================================================
//  src/data_sources/nina_api.h
//  N.I.N.A. Advanced API v2 — REST-Client fuer den SESSION-Modus.
//  Getestet gegen N.I.N.A. 3.x + Advanced API Plugin (Port 1888).
//  Kein API-Key erforderlich.
//
//  Abgerufene Endpunkte:
//    GET /v2/api/application/version     Probe / Erreichbarkeitstest
//    GET /v2/api/sequence                Sequenzstatus + Fortschritt
//    GET /v2/api/equipment/camera/info   Kameraname
//    GET /v2/api/image-history/0         Letztes Bild (Filter, Exp, HFR)
//    GET /v2/api/equipment/mount/info    Koordinaten, Alt/Az, Pier, Flip
//    GET /v2/api/equipment/guider/info   Guiding RMS + Status
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_NINA_API_H
#define ASTROOYINK_NINA_API_H

#include <Arduino.h>
#include <stdint.h>

// Feldgroessen fuer Strings in den Structs
#define NINA_STR_LONG   64
#define NINA_STR_SHORT  32

// ---------------------------------------------------------------------
//  Daten-Strukturen
// ---------------------------------------------------------------------

struct NinaSequenceInfo {
  bool     running;                      // Sequenz laeuft gerade
  bool     paused;                       // Sequenz angehalten
  char     sequenceName[NINA_STR_LONG];  // Name der geladenen Sequenz
  char     targetName[NINA_STR_LONG];    // Name des aktuellen Targets
  int16_t  targetIndex;   // aktuelles Target (1-basiert), -1 = unbekannt
  int16_t  targetTotal;   // Gesamtanzahl Targets in der Sequenz
  int16_t  subsDone;      // erledigte Subs im aktuellen Target
  int16_t  subsTotal;     // Gesamt-Subs im aktuellen Target
  uint32_t etaS;          // Geschaetzte Restzeit in Sekunden
  char     status[NINA_STR_SHORT]; // "Imaging" / "Focusing" / "Slewing" /
                                   // "Pausiert" / "Idle"
};

struct NinaCameraInfo {
  bool  connected;
  char  name[NINA_STR_LONG];
  char  filterName[NINA_STR_SHORT];  // aus letztem Bild (image-history)
  float exposureS;                   // Belichtungszeit letztes Bild in s
  float hfr;                         // Half-Flux-Radius letztes Bild in px
};

struct NinaGuidingInfo {
  bool  connected;
  float rmsRA;    // RMS in Bogensekunden
  float rmsDec;
  float rmsTotal;
  char  state[NINA_STR_SHORT];  // "Guiding" / "Stopped" / "Paused" / "LostLock"
};

struct NinaMountInfo {
  bool    connected;
  char    name[NINA_STR_LONG];
  float   raH;        // Rektaszension in Stunden (0..24)
  float   decDeg;     // Deklination in Grad (-90..+90)
  float   altDeg;     // Hoehe ueber Horizont in Grad
  float   azDeg;      // Azimut in Grad (0=N, 90=O, 180=S, 270=W)
  uint8_t pierSide;   // 0 = West, 1 = East, 2 = Unknown
  int32_t flipEtaS;   // Sekunden bis Meridian-Flip; -1 = unbekannt
};

struct NinaData {
  bool             valid;
  NinaSequenceInfo sequence;
  NinaCameraInfo   camera;
  NinaGuidingInfo  guiding;
  NinaMountInfo    mount;
};

// ---------------------------------------------------------------------
//  API-Funktionen
// ---------------------------------------------------------------------

// Schneller Ping: gibt true zurueck wenn N.I.N.A. antwortet.
bool ninaProbe(const char* host, uint16_t port);

// Prueft ob gerade eine Sequenz laeuft (leichter als ninaFetchAll).
bool ninaIsSequenceRunning(const char* host, uint16_t port);

// Laedt alle Session-Daten (5 API-Aufrufe, ~3-8 s).
// Gibt false zurueck wenn N.I.N.A. nicht erreichbar.
bool ninaFetchAll(const char* host, uint16_t port, NinaData& out);

#endif // ASTROOYINK_NINA_API_H
