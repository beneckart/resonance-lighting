#include "daytime_ritual.h"

#include <string.h>

static constexpr uint32_t kHourMs = 3600000UL;
static constexpr uint32_t kUnisonAtMs = 5000UL;
static constexpr uint32_t kRollStartMs = 12000UL;
static constexpr uint32_t kRollSlotMs = 500UL;
static constexpr uint8_t kRollSlots = 48;
static constexpr uint32_t kAfterStartMs = 42000UL;
static constexpr uint32_t kAfterSlotMs = 500UL;
static constexpr uint8_t kAfterSlots = 8;
static constexpr uint32_t kEventLateMs = 350UL;

void daytimeRitualInit(DaytimeRitualState &state) {
  memset(&state, 0, sizeof(state));
}

static uint32_t fixtureHash(const uint8_t id[3]) {
  uint32_t h = 2166136261UL;
  for (uint8_t i = 0; i < 3; ++i) {
    h ^= id[i];
    h *= 16777619UL;
  }
  h ^= h >> 16;
  h *= 0x7FEB352DUL;
  h ^= h >> 15;
  return h;
}

static bool due(uint32_t elapsedMs, uint32_t dueMs) {
  return elapsedMs >= dueMs && elapsedMs - dueMs <= kEventLateMs;
}

static uint32_t windowHourKey(uint32_t utcS) {
  uint32_t hourKey = utcS / 3600UL;
  if (utcS % 3600UL >= 3600UL - DAYTIME_RITUAL_PREWAKE_S) ++hourKey;
  return hourKey;
}

static bool alreadyAttempted(const DaytimeRitualState &state, uint8_t event,
                             uint32_t eventKey) {
  return event >= DAYTIME_RITUAL_UNISON && event <= DAYTIME_RITUAL_AFTER &&
         state.lastEventKey[event - 1] == eventKey;
}

static void markAttempted(DaytimeRitualState &state, uint8_t event,
                          uint32_t eventKey) {
  if (event >= DAYTIME_RITUAL_UNISON && event <= DAYTIME_RITUAL_AFTER)
    state.lastEventKey[event - 1] = eventKey;
}

DaytimeRitualOutputs daytimeRitualTick(DaytimeRitualState &state,
                                       const DaytimeRitualInputs &in) {
  DaytimeRitualOutputs out = {};
  if (!in.enabled || !in.scheduledDay || !in.energyReady ||
      !in.authorityFree || !in.utcValid || in.subMs >= 1000 ||
      in.uncertaintyMs > DAYTIME_RITUAL_ORGANIC_MAX_UNCERT_MS)
    return out;

  if (in.allowedHourKey && windowHourKey(in.utcS) != in.allowedHourKey)
    return out;

  uint32_t elapsedMs = (in.utcS % 3600UL) * 1000UL + in.subMs;
  // The prewake portion belongs to the upcoming hour. It has no events, but
  // must hold the receiver long enough to hear a fresh time-quality report.
  out.keepAwake = elapsedMs >=
                      kHourMs - (uint32_t)DAYTIME_RITUAL_PREWAKE_S * 1000UL ||
                  elapsedMs <= (uint32_t)DAYTIME_RITUAL_END_S * 1000UL;
  out.hourKey = in.utcS / 3600UL;

  uint8_t event = DAYTIME_RITUAL_NONE;
  uint32_t hash = fixtureHash(in.fixtureId);
  if (in.uncertaintyMs <= DAYTIME_RITUAL_UNISON_MAX_UNCERT_MS &&
      due(elapsedMs, kUnisonAtMs)) {
    event = DAYTIME_RITUAL_UNISON;
  } else if (due(elapsedMs,
                 kRollStartMs + (hash % kRollSlots) * kRollSlotMs)) {
    event = DAYTIME_RITUAL_ROLL;
  } else if ((hash & 0x03U) == 0 &&
             due(elapsedMs, kAfterStartMs +
                                ((hash >> 2) % kAfterSlots) * kAfterSlotMs)) {
    event = DAYTIME_RITUAL_AFTER;
  }
  if (event == DAYTIME_RITUAL_NONE) return out;

  uint32_t eventKey = (out.hourKey << 2) | event;
  if (alreadyAttempted(state, event, eventKey)) return out;
  // Mark before the platform attempts the load. A hard mechanism refusal is
  // final for this event; retrying could turn a brownout into a reset loop.
  markAttempted(state, event, eventKey);
  out.strikeRequested = true;
  out.event = event;
  return out;
}

uint16_t daytimeRitualSleepS(uint32_t utcS, uint16_t subMs,
                             uint16_t normalSleepS) {
  if (!normalSleepS || subMs >= 1000) return normalSleepS;
  uint32_t elapsedMs = (utcS % 3600UL) * 1000UL + subMs;
  uint32_t prewakeMs =
      kHourMs - (uint32_t)DAYTIME_RITUAL_PREWAKE_S * 1000UL;
  uint32_t deltaMs = elapsedMs < prewakeMs
                         ? prewakeMs - elapsedMs
                         : kHourMs + prewakeMs - elapsedMs;
  uint32_t deltaS = (deltaMs + 999UL) / 1000UL;
  if (deltaS == 0) deltaS = 1;
  return deltaS < normalSleepS ? (uint16_t)deltaS : normalSleepS;
}

uint16_t daytimeRitualSleepSForHour(uint32_t utcS, uint16_t subMs,
                                    uint16_t normalSleepS,
                                    uint32_t allowedHourKey) {
  if (!allowedHourKey)
    return daytimeRitualSleepS(utcS, subMs, normalSleepS);
  if (!normalSleepS || subMs >= 1000) return normalSleepS;

  uint64_t nowMs = (uint64_t)utcS * 1000ULL + subMs;
  uint64_t prewakeMs = (uint64_t)allowedHourKey * kHourMs -
                       (uint64_t)DAYTIME_RITUAL_PREWAKE_S * 1000ULL;
  if (nowMs >= prewakeMs) return normalSleepS;
  uint64_t deltaMs = prewakeMs - nowMs;
  uint64_t deltaS = (deltaMs + 999ULL) / 1000ULL;
  return deltaS < normalSleepS ? (uint16_t)deltaS : normalSleepS;
}

uint8_t daytimeRitualEventMask(uint8_t event) {
  if (event < DAYTIME_RITUAL_UNISON || event > DAYTIME_RITUAL_AFTER) return 0;
  return (uint8_t)(1U << (event - DAYTIME_RITUAL_UNISON));
}

uint8_t daytimeRitualExpectedMask(const uint8_t fixtureId[3]) {
  uint8_t mask = DAYTIME_RITUAL_MASK_UNISON | DAYTIME_RITUAL_MASK_ROLL;
  if ((fixtureHash(fixtureId) & 0x03U) == 0)
    mask |= DAYTIME_RITUAL_MASK_AFTER;
  return mask;
}

const char *daytimeRitualEventName(uint8_t event) {
  switch (event) {
  case DAYTIME_RITUAL_UNISON: return "unison";
  case DAYTIME_RITUAL_ROLL: return "roll";
  case DAYTIME_RITUAL_AFTER: return "after-ring";
  default: return "none";
  }
}
