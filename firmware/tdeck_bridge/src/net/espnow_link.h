#pragma once

#include <stdint.h>

// ESP-NOW receive side (M0: counters + last-frame info; the PSRAM rx_ring and
// census arrive in M1). Raw send is deliberately NOT exposed here — mesh_tx
// (M1) will be the only Nb-packet sender (single-writer doctrine).

bool espnowUp();
bool espnowEnsureUp();   // idempotent; init on the CURRENT radio channel
void espnowDown();       // deinit (used around Wi-Fi re-association)

struct MeshStats {
  uint32_t frames;       // valid Nb frames (ver match)
  uint32_t dropped;      // wrong ver / short frames
  uint32_t lastFrameMs;  // millis() of last valid frame
  uint8_t lastSrcId[3];
  int8_t lastRssi;
  uint8_t lastType;
};
MeshStats espnowStats();

// Census plumbing: valid frames are also copied into this ring from the radio
// callback; census_svc drains it at loop cadence.
struct RxItem;
void espnowAttachRing(RxItem *storage, uint32_t capPow2);
bool espnowRingPop(RxItem *out);
uint32_t espnowRingDrops();

// Send accounting lives here because the send callback must be re-registered
// on every esp_now_init (the guard/reconnect path deinits and re-inits).
uint32_t espnowSendOk();
uint32_t espnowSendFail();
