#pragma once

#include <stdint.h>

#include "../core/pattern_model.h"

// Owner-aware integration seam. The integration lane binds these hooks to the
// existing single stream service. Start must replace its prior owner, stop must
// stop only a Patterns-owned stream, and active must become false if another
// app replaces Patterns. App code never writes ESP-NOW directly.
struct PatternStreamHooks {
  bool (*start)(const PatternSettings &settings, uint32_t startedMs);
  void (*stop)();
  bool (*active)();
  int (*targetCount)();
};

void appPatternsSetStreamHooks(const PatternStreamHooks *hooks);
void appPatternsOpen();
