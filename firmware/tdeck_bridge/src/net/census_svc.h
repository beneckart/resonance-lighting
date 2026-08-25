#pragma once

#include "../core/census.h"

// Shared bound for full-fleet consumers. Keep this at the PSRAM allocation
// capacity so a class-targeted stream never silently stops at the first 64
// fixtures in a roughly 130-light deployment.
static constexpr size_t CENSUS_MAX_TRACKED = 192;

// Owns the PSRAM-backed census + rx ring drain. The ESP-NOW callback pushes
// raw frames into the ring (espnow_link); this service drains and ingests
// them at loop cadence. M1 grows this into the census task + nb-* emitters.

void censusSvcBegin(uint32_t nowMs);  // allocates PeerStat[CENSUS_MAX_TRACKED]
void censusSvcTick(uint32_t nowMs);   // drain ring -> ingest, close PDR windows
Census &census();                     // loop-context only (writer side)
uint32_t censusRingDrops();

// Cross-task read access (UI task): short critical sections around the same
// spinlock the writer holds during ingest/window-close.
size_t censusSnapshotSafe(CensusView *out, size_t maxOut, uint32_t nowMs);
void censusCountsSafe(int *live, int *seen, uint32_t nowMs);
bool censusPeerSafe(const uint8_t id[3], PeerStat *out);  // full copy for detail views
size_t censusQuietListSafe(uint32_t quietS, CensusView *out, size_t maxOut,
                           uint32_t nowMs);
uint16_t censusObservedPermilleSafe();
uint32_t censusFreshMsSafe();

// Sniffer tail: last-N observed frame summaries (all types, not just HBs).
struct TailFrame {
  uint32_t ms;
  uint8_t id[3];
  uint8_t type;
  int8_t rssi;
  uint8_t len;
};
size_t censusTailSafe(TailFrame *out, size_t maxOut);  // newest first
