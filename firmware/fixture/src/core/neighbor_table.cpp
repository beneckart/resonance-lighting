#include "neighbor_table.h"

#include <string.h>

void neighborTableInit(NeighborTable &t) {
  memset(&t, 0, sizeof(t));
}

static NeighborEntry *find(NeighborTable &t, const uint8_t id[3]) {
  for (int i = 0; i < NEIGHBOR_TABLE_SIZE; i++)
    if (t.entries[i].used && memcmp(t.entries[i].id, id, 3) == 0)
      return &t.entries[i];
  return nullptr;
}

NeighborEntry *neighborUpsert(NeighborTable &t, const uint8_t id[3],
                              uint32_t nowMs, int8_t rssi) {
  NeighborEntry *e = find(t, id);
  if (e) {
    e->lastHeardMs = nowMs;
    // EWMA alpha 1/8.
    e->rssiEwma = (int8_t)(((int)e->rssiEwma * 7 + (int)rssi) / 8);
    return e;
  }
  // Free slot?
  for (int i = 0; i < NEIGHBOR_TABLE_SIZE; i++) {
    if (!t.entries[i].used) {
      e = &t.entries[i];
      memset(e, 0, sizeof(*e));
      e->used = true;
      memcpy(e->id, id, 3);
      e->lastHeardMs = nowMs;
      e->rssiEwma = rssi;
      return e;
    }
  }
  // Eviction: stalest entry older than 10 s first.
  NeighborEntry *stalest = nullptr;
  for (int i = 0; i < NEIGHBOR_TABLE_SIZE; i++) {
    NeighborEntry *cand = &t.entries[i];
    if (nowMs - cand->lastHeardMs < 10000) continue;
    if (!stalest || cand->lastHeardMs < stalest->lastHeardMs) stalest = cand;
  }
  if (!stalest) {
    // All fresh: evict the weakest only if the newcomer is >6 dB stronger
    // (hysteresis prevents churn at the RF margin). Pinned ids are immune.
    NeighborEntry *weakest = nullptr;
    for (int i = 0; i < NEIGHBOR_TABLE_SIZE; i++) {
      NeighborEntry *cand = &t.entries[i];
      bool isPinned = false;
      for (int p = 0; p < t.pinnedCount; p++)
        if (memcmp(t.pinned[p], cand->id, 3) == 0) isPinned = true;
      if (isPinned) continue;
      if (!weakest || cand->rssiEwma < weakest->rssiEwma) weakest = cand;
    }
    if (!weakest || rssi <= weakest->rssiEwma + 6)
      return nullptr; // not tracked; caller may still use the payload
    stalest = weakest;
  }
  memset(stalest, 0, sizeof(*stalest));
  stalest->used = true;
  memcpy(stalest->id, id, 3);
  stalest->lastHeardMs = nowMs;
  stalest->rssiEwma = rssi;
  return stalest;
}

void neighborSetPinned(NeighborTable &t, const uint8_t ids[][3], uint8_t count) {
  if (count > NEIGHBOR_PINNED_MAX) count = NEIGHBOR_PINNED_MAX;
  t.pinnedCount = count;
  for (int i = 0; i < count; i++) memcpy(t.pinned[i], ids[i], 3);
}

void neighborClearPinned(NeighborTable &t) { t.pinnedCount = 0; }

static void fillView(NeighborView &v, const NeighborEntry &e, uint32_t nowMs) {
  memcpy(v.id, e.id, 3);
  v.state = e.choreoState;
  v.programId = e.programId;
  v.generation = e.generation;
  v.ageMs = nowMs - e.lastHeardMs;
  v.rssi = e.rssiEwma;
}

uint8_t neighborSnapshot(const NeighborTable &t, uint32_t nowMs, uint32_t freshMs,
                         NeighborView *out, uint8_t maxOut) {
  uint8_t n = 0;
  if (t.pinnedCount > 0) {
    for (int p = 0; p < t.pinnedCount && n < maxOut; p++) {
      for (int i = 0; i < NEIGHBOR_TABLE_SIZE; i++) {
        const NeighborEntry &e = t.entries[i];
        if (!e.used || memcmp(e.id, t.pinned[p], 3) != 0) continue;
        if (nowMs - e.lastHeardMs >= freshMs) break; // stale = absent
        fillView(out[n++], e, nowMs);
        break;
      }
    }
    return n;
  }
  // RSSI mode: fresh entries, strongest first (selection by repeated max --
  // table is small).
  bool taken[NEIGHBOR_TABLE_SIZE] = {};
  while (n < maxOut) {
    int best = -1;
    for (int i = 0; i < NEIGHBOR_TABLE_SIZE; i++) {
      const NeighborEntry &e = t.entries[i];
      if (!e.used || taken[i] || nowMs - e.lastHeardMs >= freshMs) continue;
      if (best < 0 || e.rssiEwma > t.entries[best].rssiEwma) best = i;
    }
    if (best < 0) break;
    taken[best] = true;
    fillView(out[n++], t.entries[best], nowMs);
  }
  return n;
}
