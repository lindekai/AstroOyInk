// =====================================================================
//  src/data_sources/nina_discovery.h
//  Auto-Discovery: findet den N.I.N.A.-PC im lokalen Netz.
//
//  Ablauf:
//    1. Zuletzt gefundene Adresse (NVS) pruefen — kurzer TCP-Test,
//       danach Bestaetigung per ninaProbe().
//    2. Antwortet sie nicht und ist scanAllowed: alle Adressen des
//       eigenen /24-Netzes auf offenen Port pruefen (8 parallele
//       nicht-blockierende connect()s, ca. 8 s fuer das ganze Netz).
//       Jeder Treffer wird per ninaProbe() bestaetigt.
//  Nur HTTP GET, keine veraendernden API-Aufrufe.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_NINA_DISCOVERY_H
#define ASTROOYINK_NINA_DISCOVERY_H

#include <Arduino.h>

// Ermittelt den N.I.N.A.-Host. Treffer landet in out und wird im NVS
// gemerkt. false = nicht gefunden (out bleibt leer).
bool ninaAutoHost(uint16_t port, bool scanAllowed, char* out, size_t outLen);

#endif // ASTROOYINK_NINA_DISCOVERY_H
