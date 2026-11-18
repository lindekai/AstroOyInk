// =====================================================================
//  src/data_sources/nina_sequence.cpp
//  Auswertung von sequence/state (siehe nina_sequence.h).
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================

#include "nina_sequence.h"
#include <string.h>
#include <ctype.h>

// Zwischenstand beim Durchlaufen des Baums
struct SeqWalk {
  bool        running       = false;
  int16_t     targetCount   = 0;
  int16_t     runningTarget = -1;   // 0-basiert, -1 = keins laeuft
  const char* target        = "";
  const char* nextTarget    = "";
  const char* step          = "";   // erster laufender Einzelschritt
  JsonVariantConst exposure;        // Belichtung im laufenden Target
  JsonVariantConst exposureParent;  // Container mit der Belichtungsschleife
  bool        expRunning    = false;
};

static bool isStatus(JsonVariantConst node, const char* st) {
  const char* s = node["Status"] | "";
  return strcmp(s, st) == 0;
}

// strstr ohne Gross-/Kleinschreibung (needle in Kleinbuchstaben)
static bool containsNoCase(const char* hay, const char* needle) {
  size_t n = strlen(needle);
  for (; *hay; hay++) {
    size_t i = 0;
    while (i < n && hay[i] && tolower((unsigned char)hay[i]) == needle[i]) i++;
    if (i == n) return true;
  }
  return false;
}

static void walk(JsonVariantConst node, JsonVariantConst parent,
                 bool inRunTarget, SeqWalk& w) {
  if (node.is<JsonArrayConst>()) {
    for (JsonVariantConst c : node.as<JsonArrayConst>()) {
      walk(c, parent, inRunTarget, w);
    }
    return;
  }
  if (!node.is<JsonObjectConst>()) return;

  bool run = isStatus(node, "RUNNING");
  if (run) w.running = true;

  JsonVariantConst tgt = node["Target"];
  if (!tgt.isNull()) {
    const char* name = tgt["TargetName"] | "";
    if (run && w.runningTarget < 0) {
      w.target        = name;
      w.runningTarget = w.targetCount;
      inRunTarget     = true;
    } else if (isStatus(node, "CREATED") && w.nextTarget[0] == '\0') {
      w.nextTarget = name;
    }
    w.targetCount++;
  }

  JsonVariantConst items = node["Items"];
  if (items.isNull()) {
    // Einzelschritt: der erste laufende ist der aktuelle Schritt
    if (run && w.step[0] == '\0') w.step = node["Name"] | "";
    bool isExposure = !node["ExposureTime"].isNull();
    if (isExposure && inRunTarget && !isStatus(node, "FINISHED") &&
        !w.expRunning && (run || w.exposure.isNull())) {
      w.exposure       = node;
      w.exposureParent = parent;
      w.expRunning     = run;
    }
    return;
  }
  walk(items, node, inRunTarget, w);
}

// ---------------------------------------------------------------------
bool ninaSequenceTreeRunning(JsonVariantConst resp) {
  SeqWalk w;
  walk(resp, JsonVariantConst(), false, w);
  return w.running;
}

void ninaParseSequenceState(JsonVariantConst resp, NinaSequenceInfo& out) {
  SeqWalk w;
  walk(resp, JsonVariantConst(), false, w);

  out.running     = w.running;
  out.paused      = false;
  out.targetTotal = w.targetCount;
  out.targetIndex = w.runningTarget;

  // Laeuft noch kein Target (z. B. Startbereich wartet auf Daemmerung),
  // das naechste anzeigen
  const char* name = w.target[0] ? w.target : w.nextTarget;
  strlcpy(out.targetName, name, sizeof(out.targetName));

  // Sub-Fortschritt: Iterations des Containers um die Belichtung = erledigte
  // Subs. Gesamtzahl nur bei Zaehlschleife (Condition mit Iterations);
  // zeitbasierte Schleifen ("Bis Zeitpunkt wiederholen") haben keine.
  out.subsDone  = 0;
  out.subsTotal = 0;
  if (!w.exposure.isNull()) {
    JsonVariantConst p = w.exposureParent;
    out.subsDone = (int16_t)(p["Iterations"] | 0);
    JsonArrayConst conds = p["Conditions"].as<JsonArrayConst>();
    for (JsonVariantConst c : conds) {
      if (!c["Iterations"].is<int>()) continue;
      out.subsTotal = (int16_t)(c["Iterations"] | 0);
      if (c["CompletedIterations"].is<int>()) {
        out.subsDone = (int16_t)(c["CompletedIterations"] | 0);
      }
      break;
    }
  }

  // Status
  if (!w.running) {
    strlcpy(out.status, "Idle", sizeof(out.status));
  } else if (containsNoCase(w.step, "fokus") || containsNoCase(w.step, "focus")) {
    strlcpy(out.status, "Focusing", sizeof(out.status));
  } else if (w.expRunning) {
    strlcpy(out.status, "Imaging", sizeof(out.status));
  } else {
    strlcpy(out.status, "Aktiv", sizeof(out.status));
  }
}
