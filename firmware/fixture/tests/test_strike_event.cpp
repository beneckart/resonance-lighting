#include <stdio.h>

#include "../src/core/strike_event.h"

static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FAIL %d: %s\n", __LINE__, #x); ++fails; } } while (0)

static NbEvent event(uint32_t id, uint32_t delayMs, uint16_t pulseMs) {
  NbEvent out = {};
  out.event_id = id;
  out.fire_in_ms = delayMs;
  out.kind = NB_EVENT_SOLENOID_STRIKE;
  out.params[NB_EVENT_STRIKE_PULSE_LO_OFFSET] = (uint8_t)(pulseMs & 0xFF);
  out.params[NB_EVENT_STRIKE_PULSE_HI_OFFSET] = (uint8_t)(pulseMs >> 8);
  return out;
}

int main() {
  StrikeEventState state;
  strikeEventInit(state);
  uint16_t pulse = 0;

  NbEvent scheduled = event(0x12345678, 1000, 40);
  CHECK(strikeEventAccept(state, scheduled, 5000) == STRIKE_EVENT_ARMED_FUTURE);
  CHECK(strikeEventAccept(state, scheduled, 5010) == STRIKE_EVENT_DUPLICATE);
  CHECK(strikeEventTick(state, 5999, pulse) == STRIKE_EVENT_WAITING);
  CHECK(strikeEventTick(state, 6000, pulse) == STRIKE_EVENT_FIRE);
  CHECK(pulse == 40);
  CHECK(strikeEventTick(state, 6001, pulse) == STRIKE_EVENT_IDLE);

  NbEvent immediate = event(2, 0, 1);
  CHECK(strikeEventAccept(state, immediate, 7000) == STRIKE_EVENT_ARMED_IMMEDIATE);
  CHECK(strikeEventTick(state, 7000, pulse) == STRIKE_EVENT_FIRE);
  CHECK(pulse == NB_EVENT_STRIKE_MIN_MS);

  NbEvent late = event(3, 1000, 500);
  CHECK(strikeEventAccept(state, late, 8000) == STRIKE_EVENT_ARMED_FUTURE);
  CHECK(strikeEventTick(state, 9251, pulse) == STRIKE_EVENT_EXPIRED);

  NbEvent first = event(4, 1000, 40);
  NbEvent busy = event(5, 1000, 40);
  CHECK(strikeEventAccept(state, first, 10000) == STRIKE_EVENT_ARMED_FUTURE);
  CHECK(strikeEventAccept(state, busy, 10001) == STRIKE_EVENT_BUSY);
  CHECK(strikeEventAccept(state, busy, 11001) == STRIKE_EVENT_DUPLICATE);
  CHECK(strikeEventTick(state, 11000, pulse) == STRIKE_EVENT_FIRE);

  NbEvent bad = event(0, 0, 40);
  CHECK(strikeEventAccept(state, bad, 12000) == STRIKE_EVENT_INVALID);
  bad = event(6, NB_EVENT_STRIKE_MAX_DELAY_MS + 1, 40);
  CHECK(strikeEventAccept(state, bad, 12000) == STRIKE_EVENT_INVALID);
  bad = event(7, 0, 40);
  bad.kind = NB_EVENT_PRESENCE_WAVE;
  CHECK(strikeEventAccept(state, bad, 12000) == STRIKE_EVENT_INVALID);

  printf("strike_event %s\n", fails ? "FAIL" : "ok");
  return fails ? 1 : 0;
}
