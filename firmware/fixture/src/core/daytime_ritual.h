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

enum DaytimeRitualEventMask : uint8_t {
  DAYTIME_RITUAL_MASK_UNISON = 0x01,
  DAYTIME_RITUAL_MASK_ROLL = 0x02,
  DAYTIME_RITUAL_MASK_AFTER = 0x04,
};

// Retain this tiny ledger across timer deep-sleep. A reset during or just
// after a strike must not turn the same hourly event into a reboot loop.
struct DaytimeRitualState {
  uint32_t lastEventKey[3];
};

struct DaytimeRitualInputs {
  bool enabled;          // field profile + cymbal-bearing fixture
  bool scheduledDay;     // site/date schedule, never the panel heuristic
  bool energyReady;      // platform-owned renewable + battery permission
  bool authorityFree;    // no operator/program lease
  bool utcValid;
  uint32_t utcS;
  uint16_t subMs;
  uint16_t uncertaintyMs;
  uint8_t fixtureId[3];
  // Zero means the production every-hour schedule. A target-locked canary
  // supplies one Unix UTC hour key and cannot actuate in any other hour.
  uint32_t allowedHourKey;
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

// Canary form: align only to the named UTC hour. Once that window has passed,
// return the ordinary sleep cadence rather than aligning to another hour.
uint16_t daytimeRitualSleepSForHour(uint32_t utcS, uint16_t subMs,
                                    uint16_t normalSleepS,
                                    uint32_t allowedHourKey);

// The after-ring belongs to a deterministic quarter of fixtures. These
// helpers let a retrieval tool distinguish a correct two-event canary from a
// missed third event without duplicating the hash contract.
uint8_t daytimeRitualExpectedMask(const uint8_t fixtureId[3]);
uint8_t daytimeRitualEventMask(uint8_t event);

const char *daytimeRitualEventName(uint8_t event);

static constexpr uint16_t DAYTIME_RITUAL_PREWAKE_S = 20;
static constexpr uint16_t DAYTIME_RITUAL_END_S = 47;
static constexpr uint16_t DAYTIME_RITUAL_UNISON_MAX_UNCERT_MS = 500;
static constexpr uint16_t DAYTIME_RITUAL_ORGANIC_MAX_UNCERT_MS = 3000;
