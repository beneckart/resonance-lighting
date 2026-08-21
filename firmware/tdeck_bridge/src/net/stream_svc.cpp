#include "stream_svc.h"

#include <Arduino.h>
#include <string.h>

#include "../core/census.h"
#include "census_svc.h"
#include "mesh_tx.h"

static StreamMode gMode = StreamMode::OFF;
static uint8_t gClassFilter = 0;
static uint8_t gR = 0, gG = 0, gB = 0, gW = 0, gDim = 255;
static int gTargets = 0;
static uint32_t gLastWaveMs = 0;

StreamMode streamMode() { return gMode; }
int streamTargetCount() { return gTargets; }

bool streamSolid(uint8_t classFilter, uint8_t r, uint8_t g, uint8_t b,
                 uint8_t w, uint8_t dim) {
  gClassFilter = classFilter;
  gR = r;
  gG = g;
  gB = b;
  gW = w;
  gDim = dim;
  gMode = StreamMode::SOLID;
  return true;
}

void streamStop() { gMode = StreamMode::OFF; }

static uint8_t scale(uint8_t v) { return (uint8_t)(((uint16_t)v * gDim) / 255); }

void streamSvcTick(uint32_t nowMs) {
  if (gMode == StreamMode::OFF) return;
  if (nowMs - gLastWaveMs < 125) return;  // 8 Hz waves
  gLastWaveMs = nowMs;

  static CensusView rows[64];
  size_t n = censusSnapshotSafe(rows, 64, nowMs);
  MeshDirectEntry entries[18];
  int count = 0;
  gTargets = 0;
  for (size_t i = 0; i < n; ++i) {
    if (rows[i].ageMs >= census().freshMs()) continue;
    if (gClassFilter && rows[i].fixtureClass != gClassFilter) continue;
    memcpy(entries[count].id, rows[i].id, 3);
    entries[count].r = scale(gR);
    entries[count].g = scale(gG);
    entries[count].b = scale(gB);
    entries[count].w = scale(gW);
    ++gTargets;
    if (++count == 18) {  // frame full: send and continue the wave
      meshDirectFrame(entries, (uint8_t)count, 0x01 /*micro-lease*/);
      count = 0;
    }
  }
  if (count > 0) meshDirectFrame(entries, (uint8_t)count, 0x01);
}
