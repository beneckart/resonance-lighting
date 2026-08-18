// Neighbor tracking for the whole 150-node broadcast domain. Only the
// RF-nearest ~16 matter to live behavior, but retaining the full heard roster
// permits a short, operator-requested pairwise RSSI survey before field pack.
// Two adjacency sources for the CA (Ben, 2026-07-30):
//   RSSI mode (default): strongest fresh entries -- expected to be marginal
//     indoors (8-17 dB placement variance) but free to try;
//   pinned mode: host-pushed explicit adjacency (NB_NEIGHBOR_SET) from the
//     2x10 rig color-ordering workflow -- overrides RSSI for CA purposes while
//     the table keeps tracking everything for telemetry.
// Pure/native; M2 reserves the censored-median RSSI window hooks (locate).
#pragma once

#include <stdint.h>

#define NEIGHBOR_TABLE_SIZE 160
#define NEIGHBOR_PINNED_MAX 8

struct NeighborEntry {
  bool used;
  uint8_t id[3];
  uint32_t lastHeardMs;
  int8_t rssiEwma;   // alpha 1/8
  uint8_t choreoState;
  uint8_t programId;
  uint16_t generation;
  uint8_t tier;
  uint8_t flags;      // latest NbChoreoState capability/status flags
  uint32_t lastSeq;
};

struct NeighborTable {
  NeighborEntry entries[NEIGHBOR_TABLE_SIZE];
  uint8_t pinned[NEIGHBOR_PINNED_MAX][3];
  uint8_t pinnedCount; // 0 = RSSI mode
};

void neighborTableInit(NeighborTable &t);

// Track any packet source; heartbeat/choreo callers update the state fields.
NeighborEntry *neighborUpsert(NeighborTable &t, const uint8_t id[3],
                              uint32_t nowMs, int8_t rssi);

void neighborSetPinned(NeighborTable &t, const uint8_t ids[][3], uint8_t count);
void neighborClearPinned(NeighborTable &t);

// The CA-facing view: fresh neighbors only (age < freshMs), from the pinned
// set when configured, else the strongest-RSSI fresh entries. Writes up to
// maxOut entries into out[]; returns the count. A pinned neighbor never heard
// from (or stale) is simply absent -- loss looks organic, never blocking.
struct NeighborView {
  uint8_t id[3];
  uint8_t state;
  uint8_t programId;
  uint16_t generation;
  uint8_t flags;
  uint32_t ageMs;
  int8_t rssi;
};
uint8_t neighborSnapshot(const NeighborTable &t, uint32_t nowMs, uint32_t freshMs,
                         NeighborView *out, uint8_t maxOut);

// Locate-facing view: ignores a pinned CA map and returns every fresh heard
// peer strongest-first. It is used only during an explicit bounded survey.
uint8_t neighborSurveySnapshot(const NeighborTable &t, uint32_t nowMs,
                               uint32_t freshMs, NeighborView *out,
                               uint8_t maxOut);
