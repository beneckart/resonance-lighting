#pragma once

#include <stddef.h>
#include <stdint.h>

#include "census.h"

struct DirectPlanEntry {
  uint8_t id[3];
  uint8_t r, g, b, w;
};

// Build the complete fresh/class-filtered direct-frame wave. Output is sorted
// by fixture ID for deterministic packet grouping. Color scaling is client-side
// because the fixture's bridge brightness field is not yet authoritative.
size_t directFramePlan(const CensusView *rows, size_t rowCount,
                       uint32_t freshMs, uint8_t classFilter, uint8_t r,
                       uint8_t g, uint8_t b, uint8_t w, uint8_t dim,
                       bool visible, DirectPlanEntry *out, size_t outCap);

// ESP-NOW direct frames carry at most 18 entries. These helpers keep the
// chunking contract native-testable rather than burying it in Arduino code.
size_t directFrameChunkCount(size_t entryCount, size_t maxPerFrame = 18);
size_t directFrameChunkSize(size_t entryCount, size_t chunkIndex,
                            size_t maxPerFrame = 18);

// One-second blink: 500 ms visible, 500 ms dark.
bool directFrameBlinkVisible(uint32_t elapsedMs);

