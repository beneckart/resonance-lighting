#pragma once

#include <stdint.h>

#include "packet.h"

#define STRIKE_EVENT_SEEN_MAX 8
#define STRIKE_EVENT_MAX_LATE_MS 250UL

enum StrikeEventAccept : uint8_t {
  STRIKE_EVENT_INVALID = 0,
  STRIKE_EVENT_DUPLICATE = 1,
  STRIKE_EVENT_BUSY = 2,
  STRIKE_EVENT_ARMED_IMMEDIATE = 3,
  STRIKE_EVENT_ARMED_FUTURE = 4,
};

enum StrikeEventTick : uint8_t {
  STRIKE_EVENT_IDLE = 0,
  STRIKE_EVENT_WAITING = 1,
  STRIKE_EVENT_FIRE = 2,
  STRIKE_EVENT_EXPIRED = 3,
};

struct StrikeEventState {
  uint32_t seen[STRIKE_EVENT_SEEN_MAX];
  uint8_t seenCount;
  uint8_t seenNext;
  bool pending;
  uint32_t fireAtMs;
  uint16_t pulseMs;
};

void strikeEventInit(StrikeEventState &state);
StrikeEventAccept strikeEventAccept(StrikeEventState &state,
                                    const NbEvent &event, uint32_t nowMs);
StrikeEventTick strikeEventTick(StrikeEventState &state, uint32_t nowMs,
                                uint16_t &pulseMs);
