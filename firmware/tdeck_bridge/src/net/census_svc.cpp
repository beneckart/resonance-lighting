#include "census_svc.h"

#include <Arduino.h>
#include <string.h>

#include "espnow_link.h"
#include "fixture/src/core/packet.h"
#include "mesh_tx.h"
#include "nb_emit.h"

#define CENSUS_FRESH_MS 5000
#define CENSUS_WINDOW_MS 60000
#define CENSUS_RING_CAP 256      // power of two; ~70 KB PSRAM

static Census gCensus;
static PeerStat *gStorage = nullptr;
static portMUX_TYPE gCensusLock = portMUX_INITIALIZER_UNLOCKED;

#define TAIL_CAP 32
static TailFrame gTail[TAIL_CAP];
static uint32_t gTailNext = 0;  // monotonically increasing write index
static portMUX_TYPE gTailLock = portMUX_INITIALIZER_UNLOCKED;

static void noteTailFrame(const RxItem &item, const NbHeader *h) {
  TailFrame f;
  f.ms = item.ms;
  memcpy(f.id, h->src_id, 3);
  f.type = h->type;
  f.rssi = item.rssi;
  f.len = item.len;
  taskENTER_CRITICAL(&gTailLock);
  gTail[gTailNext % TAIL_CAP] = f;
  ++gTailNext;
  taskEXIT_CRITICAL(&gTailLock);
}

size_t censusTailSafe(TailFrame *out, size_t maxOut) {
  taskENTER_CRITICAL(&gTailLock);
  uint32_t have = gTailNext < TAIL_CAP ? gTailNext : TAIL_CAP;
  size_t n = 0;
  for (uint32_t i = 0; i < have && n < maxOut; ++i)
    out[n++] = gTail[(gTailNext - 1 - i) % TAIL_CAP];
  taskEXIT_CRITICAL(&gTailLock);
  return n;
}

void censusSvcBegin(uint32_t nowMs) {
  gStorage = (PeerStat *)ps_malloc(sizeof(PeerStat) * CENSUS_MAX_TRACKED);
  RxItem *ringBuf = (RxItem *)ps_malloc(sizeof(RxItem) * CENSUS_RING_CAP);
  if (!gStorage || !ringBuf) {
    Serial.println("census: PSRAM alloc FAILED");
    return;
  }
  gCensus.init(gStorage, CENSUS_MAX_TRACKED, CENSUS_FRESH_MS, CENSUS_WINDOW_MS,
               nowMs);
  espnowAttachRing(ringBuf, CENSUS_RING_CAP);
}

void censusSvcTick(uint32_t nowMs) {
  if (!gStorage) return;
  RxItem item;
  int budget = 32;  // bound per-loop work; ring absorbs bursts
  while (budget-- > 0 && espnowRingPop(&item)) {
    const NbHeader *h = (const NbHeader *)item.data;
    if (memcmp(h->src_id, meshMyId(), 3) == 0) continue;  // our own bursts
    noteTailFrame(item, h);
    taskENTER_CRITICAL(&gCensusLock);
    bool consumed = gCensus.ingest(item, nowMs);
    taskEXIT_CRITICAL(&gCensusLock);
    if (consumed) continue;
    if (h->type == NB_SCANAP) nbEmitScanAp(item);
    else if (h->type == NB_NEIGHBOR_REPORT) nbEmitNeighborReport(item);
  }
  taskENTER_CRITICAL(&gCensusLock);
  gCensus.tickWindow(nowMs);
  taskEXIT_CRITICAL(&gCensusLock);
  nbEmitTick(nowMs);
}

Census &census() { return gCensus; }

uint32_t censusRingDrops() { return espnowRingDrops(); }

size_t censusSnapshotSafe(CensusView *out, size_t maxOut, uint32_t nowMs) {
  taskENTER_CRITICAL(&gCensusLock);
  size_t n = gCensus.snapshot(out, maxOut, nowMs);
  taskEXIT_CRITICAL(&gCensusLock);
  return n;
}

void censusCountsSafe(int *live, int *seen, uint32_t nowMs) {
  taskENTER_CRITICAL(&gCensusLock);
  if (live) *live = gCensus.liveCount(nowMs);
  if (seen) *seen = gCensus.seenCount();
  taskEXIT_CRITICAL(&gCensusLock);
}

bool censusPeerSafe(const uint8_t id[3], PeerStat *out) {
  taskENTER_CRITICAL(&gCensusLock);
  const PeerStat *p = gCensus.byId(id);
  if (p) *out = *p;
  taskEXIT_CRITICAL(&gCensusLock);
  return p != nullptr;
}

size_t censusQuietListSafe(uint32_t quietS, CensusView *out, size_t maxOut,
                           uint32_t nowMs) {
  taskENTER_CRITICAL(&gCensusLock);
  size_t n = gCensus.quietList(quietS, out, maxOut, nowMs);
  taskEXIT_CRITICAL(&gCensusLock);
  return n;
}

uint16_t censusObservedPermilleSafe() {
  taskENTER_CRITICAL(&gCensusLock);
  uint16_t observed = gCensus.observedPermille();
  taskEXIT_CRITICAL(&gCensusLock);
  return observed;
}

uint32_t censusFreshMsSafe() {
  taskENTER_CRITICAL(&gCensusLock);
  uint32_t freshMs = gCensus.freshMs();
  taskEXIT_CRITICAL(&gCensusLock);
  return freshMs;
}
