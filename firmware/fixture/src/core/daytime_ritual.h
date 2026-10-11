// Deterministic, rootless daytime cymbal ritual. Shared UTC schedules the
// fleet without a bridge command; platform glue retains actuator and energy
// authority. See ADR 0071.
#pragma once

#include <stdint.h>

enum DaytimeRitualEvent : uint8_t {
  DAYTIME_RITUAL_NONE = 0,
  DAYTIME_RITUAL_UNISON = 1,
  DAYTIME_RITUAL_ROLL = 2,
  DAYTIME_RITUAL_AFTER = 3,
};

// Retain this tiny ledger across timer deep-sleep. A reset during or just
// after a strike must not turn the same hourly event into a reboot loop.
struct DaytimeRitualState {
  uint32_t lastEventKey[3];
};

struct DaytimeRitualInputs {
  bool enabled;          // field profile + cymbal-bearing fixture
  bool scheduledDay;     // site/date schedule, never the panel heuristic
  bool batterySafe;      // FULL/DIM only; OFF/PROTECT remain hard vetoes
  bool authorityFree;    // no operator/program lease
  bool utcValid;
  uint32_t utcS;
  uint16_t subMs;
  uint16_t uncertaintyMs;
  uint8_t fixtureId[3];
};

struct DaytimeRitualOutputs {
  bool keepAwake;        // true only inside the hard ritual window
  bool strikeRequested;  // one attempt; platform mechanism gate still wins
  uint8_t event;
  uint32_t hourKey;
};

void daytimeRitualInit(DaytimeRitualState &state);
DaytimeRitualOutputs daytimeRitualTick(DaytimeRitualState &state,
                                       const DaytimeRitualInputs &in);

// Shorten an ordinary daytime sleep only when necessary to wake at the next
// hourly pre-roll boundary. Invalid time keeps the normal cadence.
uint16_t daytimeRitualSleepS(uint32_t utcS, uint16_t subMs,
                             uint16_t normalSleepS);

const char *daytimeRitualEventName(uint8_t event);

static constexpr uint16_t DAYTIME_RITUAL_PREWAKE_S = 20;
static constexpr uint16_t DAYTIME_RITUAL_END_S = 47;
static constexpr uint16_t DAYTIME_RITUAL_UNISON_MAX_UNCERT_MS = 500;
static constexpr uint16_t DAYTIME_RITUAL_ORGANIC_MAX_UNCERT_MS = 3000;
