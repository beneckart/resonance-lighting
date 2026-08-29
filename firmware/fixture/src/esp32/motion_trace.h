// Exact-target, test-image-only MSA311/range/visible-output flight recorder.
// Samples accumulate in PSRAM during ordinary ESP-NOW operation and are read
// later through a bounded maintenance-WiFi NDJSON endpoint.
#pragma once

#include <stdint.h>

class WebServer;

bool motionTraceBuild();
bool motionTraceTargetMatches();
uint32_t motionTraceTargetId();
uint32_t motionTraceCapacity();
uint32_t motionTraceCount();
uint32_t motionTraceOverwrites();
void motionTraceInit();
void motionTraceTick();
void motionTraceHandleHttp(WebServer &server);
