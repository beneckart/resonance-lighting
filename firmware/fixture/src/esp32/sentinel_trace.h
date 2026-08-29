#pragma once

#include <stdint.h>

#include "../core/sentinel_trace.h"

class WebServer;

bool sentinelTraceBuild();
bool sentinelTraceSmokeBuild();
uint32_t sentinelTraceTargetId();
bool sentinelTraceTargetMatches();
bool sentinelTraceOwnsLoop();
bool sentinelTraceSkipInitialSensors();
uint8_t sentinelTracePhase();
uint32_t sentinelTraceCapacity();
uint32_t sentinelTraceCount();
uint32_t sentinelTraceOverwrites();
bool sentinelTracePersisted();
bool sentinelTraceRecoveryOnly();
const char *sentinelTracePersistenceState();

void sentinelTraceInit();
void sentinelTraceTick();
void sentinelTraceHandleHttp(WebServer &server);
