// =====================================================================
//  tests/test_nina_sequence.cpp
//  Validiert die Auswertung von sequence/state gegen echte Antworten
//  der Advanced API (tests/fixtures/) und synthetische Faelle.
// =====================================================================
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/data_sources/nina_sequence.h"

static int failures = 0;
static void expectStr(const char* label, const char* got, const char* want) {
  bool ok = std::strcmp(got, want) == 0;
  printf("  %-40s got=%-14s want=%-14s %s\n", label, got, want, ok ? "OK" : "FAIL");
  if (!ok) failures++;
}
static void expectInt(const char* label, int got, int want) {
  bool ok = got == want;
  printf("  %-40s got=%-14d want=%-14d %s\n", label, got, want, ok ? "OK" : "FAIL");
  if (!ok) failures++;
}

static NinaSequenceInfo parse(const std::string& json, bool& treeRunning) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(
      doc, json, DeserializationOption::NestingLimit(NINA_SEQ_NESTING_LIMIT));
  if (err) {
    printf("  JSON-Fehler: %s\n", err.c_str());
    failures++;
  }
  NinaSequenceInfo s = {};
  ninaParseSequenceState(doc["Response"], s);
  treeRunning = ninaSequenceTreeRunning(doc["Response"]);
  return s;
}

static std::string readFile(const char* path) {
  std::ifstream f(path);
  std::stringstream ss;
  ss << f.rdbuf();
  if (ss.str().empty()) {
    printf("  Datei fehlt: %s\n", path);
    failures++;
  }
  return ss.str();
}

int main() {
  printf("=== N.I.N.A. sequence/state Tests ===\n\n");
  bool tr;

  printf("running-1 (Coma Cluster, Zeit-Schleife):\n");
  NinaSequenceInfo s = parse(readFile("fixtures/sequence-state-running-1.json"), tr);
  expectInt("running",     s.running, 1);
  expectInt("treeRunning", tr, 1);
  expectStr("targetName",  s.targetName, "Coma Cluster");
  expectInt("targetIndex", s.targetIndex, 0);
  expectInt("targetTotal", s.targetTotal, 1);
  expectInt("subsDone",    s.subsDone, 4);
  expectInt("subsTotal",   s.subsTotal, 0);
  expectStr("status",      s.status, "Imaging");

  printf("\nrunning-2 (ein Sub weiter):\n");
  s = parse(readFile("fixtures/sequence-state-running-2.json"), tr);
  expectInt("subsDone",    s.subsDone, 5);

  printf("\nidle (Advanced API Fixture):\n");
  s = parse(readFile("fixtures/sequence-state-idle.json"), tr);
  expectInt("running",     s.running, 0);
  expectInt("treeRunning", tr, 0);
  expectStr("status",      s.status, "Idle");

  printf("\ncreated (live, leere Sequenz):\n");
  s = parse(readFile("fixtures/sequence-state-created.json"), tr);
  expectInt("running",     s.running, 0);
  expectInt("targetTotal", s.targetTotal, 0);
  expectStr("targetName",  s.targetName, "");

  printf("\nsynthetisch: 2 Targets, Zaehlschleife 12/30:\n");
  s = parse(R"({"Response":[{"GlobalTriggers":[]},
    {"Name":"Start","Status":"FINISHED","Items":[]},
    {"Name":"Ziele","Status":"RUNNING","Items":[
      {"Name":"M31_Container","Status":"FINISHED","Target":{"TargetName":"M31"},"Items":[]},
      {"Name":"M42_Container","Status":"RUNNING","Target":{"TargetName":"M42"},"Items":[
        {"Name":"Loop","Status":"RUNNING","Iterations":12,
         "Conditions":[{"Iterations":30,"CompletedIterations":12,"Status":"RUNNING"}],
         "Items":[{"Name":"Belichtung","Status":"RUNNING","ExposureTime":180.0}]}]},
      {"Name":"M45_Container","Status":"CREATED","Target":{"TargetName":"M45"},"Items":[]}]},
    {"Name":"Ende","Status":"CREATED","Items":[]}]})", tr);
  expectStr("targetName",  s.targetName, "M42");
  expectInt("targetIndex", s.targetIndex, 1);
  expectInt("targetTotal", s.targetTotal, 3);
  expectInt("subsDone",    s.subsDone, 12);
  expectInt("subsTotal",   s.subsTotal, 30);
  expectStr("status",      s.status, "Imaging");

  printf("\nsynthetisch: Autofokus laeuft:\n");
  s = parse(R"({"Response":[{"Name":"Ziele","Status":"RUNNING","Items":[
      {"Name":"M42_Container","Status":"RUNNING","Target":{"TargetName":"M42"},"Items":[
        {"Name":"Autofokus ausfuehren","Status":"RUNNING"},
        {"Name":"Loop","Status":"CREATED","Iterations":0,"Conditions":[],
         "Items":[{"Name":"Belichtung","Status":"CREATED","ExposureTime":180.0}]}]}]}]})", tr);
  expectStr("status",      s.status, "Focusing");

  printf("\nsynthetisch: Startbereich wartet, naechstes Target:\n");
  s = parse(R"({"Response":[
      {"Name":"Start","Status":"RUNNING","Items":[{"Name":"Zeit abwarten","Status":"RUNNING"}]},
      {"Name":"Ziele","Status":"CREATED","Items":[
        {"Name":"M42_Container","Status":"CREATED","Target":{"TargetName":"M42"},"Items":[]}]}]})", tr);
  expectInt("running",     s.running, 1);
  expectStr("targetName",  s.targetName, "M42");
  expectInt("targetIndex", s.targetIndex, -1);
  expectStr("status",      s.status, "Aktiv");

  printf("\n=== Result: %s ===\n", failures == 0 ? "ALL PASSED" : "SOME FAILED");
  return failures == 0 ? 0 : 1;
}
