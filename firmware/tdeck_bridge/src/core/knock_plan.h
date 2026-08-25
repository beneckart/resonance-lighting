#pragma once

#include <stddef.h>
#include <stdint.h>

#include "census.h"

// Select fresh, real fixture IDs for a targeted knock roll. Output is sorted
// by short ID so the rollout is deterministic rather than heartbeat-order
// dependent. Returns the number written, capped by outCap.
size_t knockPlanFresh(const CensusView *rows, size_t rowCount,
                      uint32_t freshMs, uint8_t out[][3], size_t outCap);

