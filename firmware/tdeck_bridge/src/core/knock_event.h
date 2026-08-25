#pragma once

#include <stdint.h>

#include "fixture/src/core/packet.h"

// Build the payload portion of one deduplicated fleet strike event. The mesh
// layer adds the canonical header and repeats the same logical event for RF
// reliability. Returns false for an invalid ID or excessive future delay.
bool knockBuildBroadcastEvent(NbEvent &event, uint32_t eventId,
                              uint16_t pulseMs, uint32_t fireInMs);
