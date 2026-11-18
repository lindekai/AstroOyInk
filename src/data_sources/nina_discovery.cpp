// =====================================================================
//  src/data_sources/nina_discovery.cpp
//  Auto-Discovery des N.I.N.A.-PCs (siehe nina_discovery.h).
//  Scan-Verfahren uebernommen aus NINA Pocket Companion.
//
//  Author:  Kai Linde
//  Credits: Dark Matters Discord
// =====================================================================

#include "nina_discovery.h"
#include "nina_api.h"
#include "../common/app_config.h"
#include <WiFi.h>
#include <lwip/sockets.h>

static const int      kScanBatch   = 8;    // parallele Sockets (lwIP erlaubt ~16 insgesamt)
static const uint32_t kScanWaitMs  = 250;  // pro Batch; LAN-Hosts antworten in wenigen ms
static const uint32_t kCachedWaitMs = 500; // TCP-Test der gemerkten Adresse

// ---------------------------------------------------------------------
//  Startet nicht-blockierende connect()s auf ips[0..n-1]:port und wartet
//  hoechstens waitMs. open[i] = true, wenn der Port angenommen hat.
//  Gibt die Anzahl tatsaechlich gepruefter Adressen zurueck (weniger als
//  n, wenn lwIP keine Sockets mehr frei hat).
// ---------------------------------------------------------------------
static int probeBatch(const uint32_t* ips, int n, uint16_t port,
                      uint32_t waitMs, bool* open) {
  int fds[kScanBatch];
  bool done[kScanBatch] = {};
  int used = 0;

  for (; used < n && used < kScanBatch; used++) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) break;
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
    sockaddr_in a = {};
    a.sin_family      = AF_INET;
    a.sin_port        = htons(port);
    a.sin_addr.s_addr = htonl(ips[used]);
    connect(s, (sockaddr*)&a, sizeof(a));
    fds[used]  = s;
    open[used] = false;
  }

  int pending = used;
  uint32_t t0 = millis();
  while (pending > 0 && millis() - t0 < waitMs) {
    fd_set wr;
    FD_ZERO(&wr);
    int maxfd = -1;
    for (int i = 0; i < used; i++) {
      if (!done[i]) {
        FD_SET(fds[i], &wr);
        if (fds[i] > maxfd) maxfd = fds[i];
      }
    }
    uint32_t left = waitMs - (millis() - t0);
    timeval tv = {(time_t)(left / 1000), (suseconds_t)((left % 1000) * 1000)};
    if (select(maxfd + 1, nullptr, &wr, nullptr, &tv) <= 0) break;

    for (int i = 0; i < used; i++) {
      if (!done[i] && FD_ISSET(fds[i], &wr)) {
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(fds[i], SOL_SOCKET, SO_ERROR, &err, &len);
        open[i] = (err == 0);
        done[i] = true;
        pending--;
      }
    }
  }

  for (int i = 0; i < used; i++) close(fds[i]);
  return used;
}

static void ipToStr(uint32_t ip, char* out, size_t outLen) {
  snprintf(out, outLen, "%u.%u.%u.%u",
           (unsigned)(ip >> 24), (unsigned)((ip >> 16) & 0xFF),
           (unsigned)((ip >> 8) & 0xFF), (unsigned)(ip & 0xFF));
}

// ---------------------------------------------------------------------
//  Gemerkte Adresse pruefen: erst kurzer TCP-Test (spart den 5-s-Timeout
//  des HTTPClient, wenn der PC aus ist), dann ninaProbe().
// ---------------------------------------------------------------------
static bool checkCached(const char* host, uint16_t port) {
  IPAddress ip;
  if (!ip.fromString(host)) return false;
  uint32_t a = ((uint32_t)ip[0] << 24) | ((uint32_t)ip[1] << 16) |
               ((uint32_t)ip[2] << 8)  |  (uint32_t)ip[3];
  bool open = false;
  if (probeBatch(&a, 1, port, kCachedWaitMs, &open) != 1 || !open) return false;
  return ninaProbe(host, port);
}

// ---------------------------------------------------------------------
//  /24-Netz absuchen. Treffer in out.
// ---------------------------------------------------------------------
static bool scanSubnet(uint16_t port, char* out, size_t outLen) {
  IPAddress me = WiFi.localIP();
  uint32_t base = ((uint32_t)me[0] << 24) | ((uint32_t)me[1] << 16) |
                  ((uint32_t)me[2] << 8);
  uint8_t own = me[3];

  Serial.printf("[NINA] Suche N.I.N.A. in %u.%u.%u.0/24 auf Port %u ...\n",
                me[0], me[1], me[2], port);
  uint32_t t0 = millis();

  uint16_t next = 1;
  while (next <= 254) {
    uint32_t ips[kScanBatch];
    int n = 0;
    uint16_t h = next;
    while (n < kScanBatch && h <= 254) {
      if (h != own) ips[n++] = base | h;
      h++;
    }
    if (n == 0) break;

    bool open[kScanBatch];
    int used = probeBatch(ips, n, port, kScanWaitMs, open);
    if (used == 0) {
      Serial.println("[NINA] Keine Sockets frei, Suche abgebrochen");
      return false;
    }
    // Nicht gepruefte Adressen im naechsten Durchlauf erneut versuchen
    next = (used == n) ? h : (uint16_t)(ips[used] & 0xFF);

    for (int i = 0; i < used; i++) {
      if (!open[i]) continue;
      char host[16];
      ipToStr(ips[i], host, sizeof(host));
      Serial.printf("[NINA] Port offen bei %s, pruefe API ...\n", host);
      if (ninaProbe(host, port)) {
        strlcpy(out, host, outLen);
        Serial.printf("[NINA] Gefunden: %s (%lu ms)\n", host,
                      (unsigned long)(millis() - t0));
        return true;
      }
    }
  }

  Serial.printf("[NINA] Nicht gefunden (%lu ms)\n",
                (unsigned long)(millis() - t0));
  return false;
}

// ---------------------------------------------------------------------
bool ninaAutoHost(uint16_t port, bool scanAllowed, char* out, size_t outLen) {
  out[0] = '\0';

  char cached[64];
  loadNinaAutoHost(cached, sizeof(cached));
  if (cached[0] != '\0') {
    Serial.printf("[NINA] Pruefe gemerkte Adresse %s ...\n", cached);
    if (checkCached(cached, port)) {
      strlcpy(out, cached, outLen);
      return true;
    }
  }

  if (!scanAllowed) {
    Serial.println("[NINA] Ausserhalb des Scan-Fensters, keine Suche");
    return false;
  }

  if (!scanSubnet(port, out, outLen)) return false;
  if (strcmp(out, cached) != 0) saveNinaAutoHost(out);  // Flash nur bei Aenderung
  return true;
}
