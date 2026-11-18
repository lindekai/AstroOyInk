// =====================================================================
//  src/data_sources/nina_sequence.h
//  Auswertung von GET /v2/api/sequence/state (Advanced API v2).
//
//  Die API liefert die Sequenz als Baum:
//    [ {GlobalTriggers}, Start-Bereich, Ziel-Bereich, Ende-Bereich ]
//  Jeder Knoten hat Status (CREATED/RUNNING/FINISHED/SKIPPED/...),
//  Name, ggf. Items (Kinder), Conditions, Iterations und — bei
//  Target-Containern — Target{TargetName}.
//  Vorgehen wie im NINA Pocket Companion: Knoten mit Status RUNNING
//  suchen und daraus Target, Schritt und Sub-Fortschritt ableiten.
//
//  Reine Logik (nur ArduinoJson) → Host-Test: test_nina_sequence.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================
#ifndef ASTROOYINK_NINA_SEQUENCE_H
#define ASTROOYINK_NINA_SEQUENCE_H

#include <ArduinoJson.h>
#include "nina_api.h"

// sequence/state ist tief verschachtelt; ArduinoJson erlaubt default 10
#define NINA_SEQ_NESTING_LIMIT 40

// true, wenn irgendein Knoten im Baum Status RUNNING hat.
bool ninaSequenceTreeRunning(JsonVariantConst resp);

// Fuellt running, targetName, targetIndex (0-basiert), targetTotal,
// subsDone, subsTotal und status. sequenceName, etaS und paused liefert
// sequence/state nicht — sie bleiben leer/0.
void ninaParseSequenceState(JsonVariantConst resp, NinaSequenceInfo& out);

#endif // ASTROOYINK_NINA_SEQUENCE_H
