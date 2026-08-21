#pragma once

#include <stdint.h>

// The single-writer frame streamer (one packet does NOT hold a color: fixtures
// revert after 3 s of silence, so "solid color" = sustained NB_DIRECT_FRAME
// waves at 8 Hz — the tools/lights doctrine). Exactly one owner at a time;
// the Zones and Patterns apps share this service, never the radio directly.

enum class StreamMode : uint8_t { OFF = 0, SOLID = 1 };

void streamSvcTick(uint32_t nowMs);  // loop cadence; sends at 8 Hz when active

// classFilter: 0 = all classes, 1-4 = that fixture class only (latched class
// from the census; hb-full cadence means the map needs ~60 s of listening).
bool streamSolid(uint8_t classFilter, uint8_t r, uint8_t g, uint8_t b,
                 uint8_t w, uint8_t dim /*0-255 client-side scale*/);
void streamStop();  // stop sending; fixtures revert via staleness/micro-lease
StreamMode streamMode();
int streamTargetCount();  // fixtures covered by the last wave
