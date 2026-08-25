#include "strike_event.h"

#include <string.h>

void strikeEventInit(StrikeEventState &state) { memset(&state, 0, sizeof(state)); }

static bool seen(const StrikeEventState &state, uint32_t eventId) {
  for (uint8_t i = 0; i < state.seenCount; ++i)
    if (state.seen[i] == eventId) return true;
  return false;
}

static void remember(StrikeEventState &state, uint32_t eventId) {
  if (state.seenCount < STRIKE_EVENT_SEEN_MAX) {
    state.seen[state.seenCount++] = eventId;
    return;
  }
  state.seen[state.seenNext] = eventId;
  state.seenNext = (uint8_t)((state.seenNext + 1U) % STRIKE_EVENT_SEEN_MAX);
}

StrikeEventAccept strikeEventAccept(StrikeEventState &state,
                                    const NbEvent &event, uint32_t nowMs) {
  if (event.kind != NB_EVENT_SOLENOID_STRIKE || event.event_id == 0 ||
      event.fire_in_ms > NB_EVENT_STRIKE_MAX_DELAY_MS)
    return STRIKE_EVENT_INVALID;
  if (seen(state, event.event_id)) return STRIKE_EVENT_DUPLICATE;
  remember(state, event.event_id);
  // One pending physical strike is the safe bound. A second logical event is
  // remembered and ignored instead of firing later after the first completes.
  if (state.pending) return STRIKE_EVENT_BUSY;

  uint16_t pulse =
      (uint16_t)event.params[NB_EVENT_STRIKE_PULSE_LO_OFFSET] |
      ((uint16_t)event.params[NB_EVENT_STRIKE_PULSE_HI_OFFSET] << 8);
  if (pulse < NB_EVENT_STRIKE_MIN_MS) pulse = NB_EVENT_STRIKE_MIN_MS;
  if (pulse > NB_EVENT_STRIKE_MAX_MS) pulse = NB_EVENT_STRIKE_MAX_MS;
  state.pending = true;
  state.fireAtMs = nowMs + event.fire_in_ms;
  state.pulseMs = pulse;
  return event.fire_in_ms ? STRIKE_EVENT_ARMED_FUTURE
                          : STRIKE_EVENT_ARMED_IMMEDIATE;
}

StrikeEventTick strikeEventTick(StrikeEventState &state, uint32_t nowMs,
                                uint16_t &pulseMs) {
  if (!state.pending) return STRIKE_EVENT_IDLE;
  int32_t late = (int32_t)(nowMs - state.fireAtMs);
  if (late < 0) return STRIKE_EVENT_WAITING;
  state.pending = false;
  if ((uint32_t)late > STRIKE_EVENT_MAX_LATE_MS) return STRIKE_EVENT_EXPIRED;
  pulseMs = state.pulseMs;
  return STRIKE_EVENT_FIRE;
}
