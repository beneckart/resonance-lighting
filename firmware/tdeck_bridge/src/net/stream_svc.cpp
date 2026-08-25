#include "stream_svc.h"

#include <Arduino.h>
#include <string.h>

#include "../core/census.h"
#include "../core/direct_frame_plan.h"
#include "census_svc.h"
#include "mesh_tx.h"

static StreamMode gMode = StreamMode::OFF;
static uint8_t gClassFilter = 0;
static uint8_t gR = 0, gG = 0, gB = 0, gW = 0, gDim = 255;
static int gTargets = 0;
static uint32_t gLastWaveMs = 0;
static uint32_t gModeStartedMs = 0;

StreamMode streamMode() { return gMode; }
int streamTargetCount() { return gTargets; }

static bool streamStart(StreamMode mode, uint8_t classFilter, uint8_t r,
                        uint8_t g, uint8_t b, uint8_t w, uint8_t dim) {
  gClassFilter = classFilter;
  gR = r;
  gG = g;
  gB = b;
  gW = w;
  gDim = dim;
  gMode = mode;
  gModeStartedMs = millis();
  gLastWaveMs = 0;
  return true;
}

bool streamSolid(uint8_t classFilter, uint8_t r, uint8_t g, uint8_t b,
                 uint8_t w, uint8_t dim) {
  return streamStart(StreamMode::SOLID, classFilter, r, g, b, w, dim);
}

bool streamBlink(uint8_t classFilter, uint8_t r, uint8_t g, uint8_t b,
                 uint8_t w, uint8_t dim) {
  return streamStart(StreamMode::BLINK, classFilter, r, g, b, w, dim);
}

void streamStop() {
  gMode = StreamMode::OFF;
  gTargets = 0;
}

void streamSvcTick(uint32_t nowMs) {
  if (gMode == StreamMode::OFF) return;
  if (nowMs - gLastWaveMs < 125) return;  // 8 Hz waves
  gLastWaveMs = nowMs;

  static CensusView rows[CENSUS_MAX_TRACKED];
  static DirectPlanEntry plan[CENSUS_MAX_TRACKED];
  size_t n = censusSnapshotSafe(rows, CENSUS_MAX_TRACKED, nowMs);
  bool visible = gMode != StreamMode::BLINK ||
                 directFrameBlinkVisible(nowMs - gModeStartedMs);
  size_t planned = directFramePlan(
      rows, n, censusFreshMsSafe(), gClassFilter, gR, gG, gB, gW, gDim,
      visible, plan, CENSUS_MAX_TRACKED);
  gTargets = (int)planned;
  uint8_t flags = gMode == StreamMode::BLINK
                      ? 0x03 /*micro-lease + hard-cut blink edge*/
                      : 0x01 /*micro-lease*/;
  MeshDirectEntry entries[18];
  size_t chunks = directFrameChunkCount(planned, 18);
  for (size_t chunk = 0; chunk < chunks; ++chunk) {
    size_t count = directFrameChunkSize(planned, chunk, 18);
    size_t start = chunk * 18;
    for (size_t i = 0; i < count; ++i) {
      memcpy(entries[i].id, plan[start + i].id, 3);
      entries[i].r = plan[start + i].r;
      entries[i].g = plan[start + i].g;
      entries[i].b = plan[start + i].b;
      entries[i].w = plan[start + i].w;
    }
    meshDirectFrame(entries, (uint8_t)count, flags);
  }
}
