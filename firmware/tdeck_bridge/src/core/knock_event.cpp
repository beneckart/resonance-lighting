#include "knock_event.h"

#include <string.h>

bool knockBuildBroadcastEvent(NbEvent &event, uint32_t eventId,
                              uint16_t pulseMs, uint32_t fireInMs) {
  if (eventId == 0 || fireInMs > NB_EVENT_STRIKE_MAX_DELAY_MS ||
      (fireInMs && fireInMs < NB_EVENT_STRIKE_MIN_FUTURE_MS))
    return false;
  if (pulseMs < NB_EVENT_STRIKE_MIN_MS) pulseMs = NB_EVENT_STRIKE_MIN_MS;
  if (pulseMs > NB_EVENT_STRIKE_MAX_MS) pulseMs = NB_EVENT_STRIKE_MAX_MS;
  memset(&event, 0, sizeof(event));
  event.event_id = eventId;
  event.fire_in_ms = fireInMs;
  event.kind = NB_EVENT_SOLENOID_STRIKE;
  event.params[NB_EVENT_STRIKE_PULSE_LO_OFFSET] = (uint8_t)(pulseMs & 0xFF);
  event.params[NB_EVENT_STRIKE_PULSE_HI_OFFSET] = (uint8_t)(pulseMs >> 8);
  return true;
}
