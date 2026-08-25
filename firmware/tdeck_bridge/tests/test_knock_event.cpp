#include <stdio.h>

#include "core/knock_event.h"

static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FAIL %d: %s\n", __LINE__, #x); ++fails; } } while (0)

static uint16_t pulse(const NbEvent &event) {
  return (uint16_t)event.params[NB_EVENT_STRIKE_PULSE_LO_OFFSET] |
         ((uint16_t)event.params[NB_EVENT_STRIKE_PULSE_HI_OFFSET] << 8);
}

int main() {
  NbEvent event = {};
  CHECK(knockBuildBroadcastEvent(event, 0x12345678, 40, 0));
  CHECK(event.event_id == 0x12345678);
  CHECK(event.kind == NB_EVENT_SOLENOID_STRIKE);
  CHECK(event.fire_in_ms == 0);
  CHECK(pulse(event) == 40);

  CHECK(knockBuildBroadcastEvent(event, 1, 1, 1000));
  CHECK(event.fire_in_ms == 1000);
  CHECK(pulse(event) == NB_EVENT_STRIKE_MIN_MS);
  CHECK(knockBuildBroadcastEvent(event, 2, 999, 5000));
  CHECK(pulse(event) == NB_EVENT_STRIKE_MAX_MS);
  CHECK(!knockBuildBroadcastEvent(event, 0, 40, 0));
  CHECK(!knockBuildBroadcastEvent(
      event, 3, 40, NB_EVENT_STRIKE_MIN_FUTURE_MS - 1));
  CHECK(!knockBuildBroadcastEvent(
      event, 4, 40, NB_EVENT_STRIKE_MAX_DELAY_MS + 1));

  printf("knock_event %s\n", fails ? "FAIL" : "ok");
  return fails ? 1 : 0;
}
