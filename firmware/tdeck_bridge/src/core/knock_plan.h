#pragma once

#include <stddef.h>
#include <stdint.h>

#include "census.h"

// Select fresh, real fixture IDs for any exact-target fleet campaign. Output
// is sorted by short ID so the rollout is deterministic rather than heartbeat-
// order dependent. Returns the number written, capped by outCap.
size_t targetPlanFresh(const CensusView *rows, size_t rowCount,
                       uint32_t freshMs, uint8_t out[][3], size_t outCap);

// Compatibility name retained for the original knocker consumer.
size_t knockPlanFresh(const CensusView *rows, size_t rowCount,
                      uint32_t freshMs, uint8_t out[][3], size_t outCap);
