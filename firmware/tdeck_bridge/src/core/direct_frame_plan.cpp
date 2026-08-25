#include "direct_frame_plan.h"

#include <string.h>

static bool realId(const uint8_t id[3]) {
  return id[0] != 0 || id[1] != 0 || id[2] != 0;
}

static uint8_t scale(uint8_t value, uint8_t dim) {
  return (uint8_t)(((uint16_t)value * dim) / 255);
}

size_t directFramePlan(const CensusView *rows, size_t rowCount,
                       uint32_t freshMs, uint8_t classFilter, uint8_t r,
                       uint8_t g, uint8_t b, uint8_t w, uint8_t dim,
                       bool visible, DirectPlanEntry *out, size_t outCap) {
  if (!rows || !out || outCap == 0) return 0;
  size_t n = 0;
  for (size_t i = 0; i < rowCount && n < outCap; ++i) {
    if (rows[i].ageMs >= freshMs || !realId(rows[i].id)) continue;
    if (classFilter && rows[i].fixtureClass != classFilter) continue;
    memcpy(out[n].id, rows[i].id, 3);
    out[n].r = visible ? scale(r, dim) : 0;
    out[n].g = visible ? scale(g, dim) : 0;
    out[n].b = visible ? scale(b, dim) : 0;
    out[n].w = visible ? scale(w, dim) : 0;
    ++n;
  }

  for (size_t i = 1; i < n; ++i) {
    DirectPlanEntry entry = out[i];
    size_t j = i;
    while (j > 0 && memcmp(out[j - 1].id, entry.id, 3) > 0) {
      out[j] = out[j - 1];
      --j;
    }
    out[j] = entry;
  }
  return n;
}

size_t directFrameChunkCount(size_t entryCount, size_t maxPerFrame) {
  if (entryCount == 0 || maxPerFrame == 0) return 0;
  return (entryCount + maxPerFrame - 1) / maxPerFrame;
}

size_t directFrameChunkSize(size_t entryCount, size_t chunkIndex,
                            size_t maxPerFrame) {
  if (maxPerFrame == 0) return 0;
  size_t start = chunkIndex * maxPerFrame;
  if (start >= entryCount) return 0;
  size_t remaining = entryCount - start;
  return remaining < maxPerFrame ? remaining : maxPerFrame;
}

bool directFrameBlinkVisible(uint32_t elapsedMs) {
  return ((elapsedMs / 500U) & 1U) == 0;
}

