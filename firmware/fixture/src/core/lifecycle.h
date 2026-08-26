// Day/night lifecycle (milestone 1): pure transitions, supply-based dusk/dawn
// (production classes carry no lux sensor -- TSL2591 was bench-only), bounded
// night (the 13-15 h no-lux artifact fix), and field energy-gated
// wakefulness. A timer wake normally listens for 15 s, but measured input
// surplus holds a bounded solar probe awake long enough to earn DAY_ACTIVE;
// battery voltage alone is not evidence of renewable surplus.
// ADR 0031's scheduled-UTC gate replaces the dusk heuristic in M2 via the same
// inputs (the TimePhase seam feeds `forceNight`).
#pragma once

#include <stdint.h>

enum LifeState : uint8_t {
  LIFE_BOOT = 0,
  LIFE_DAY_CHARGE = 1, // day, no surplus: prod duty-cycles the radio
  LIFE_DAY_ACTIVE = 2, // day, surplus: fully awake, strikes permitted
  LIFE_NIGHT_SHOW = 3, // program runtime drives the LEDs (power veto applies)
  LIFE_COMMISSION = 4, // bridge-commanded runtime; no solar/autonomous transition
};

struct LifeConfig {
  uint16_t duskConfirmS;      // supply absent this long -> night (prod 1800)
  uint16_t dawnConfirmS;      // supply useful this long -> day (prod 300)
  uint16_t usefulSupplyMa;    // input current that counts as "day evidence" (20)
  uint16_t surplusMa;         // enter/strike threshold for DAY_ACTIVE (150)
  uint16_t surplusExitMa;     // remain-active hysteresis threshold (100)
  uint16_t surplusConfirmS;   // 60
  uint16_t noSurplusConfirmS; // 300 (fall back to DAY_CHARGE)
  uint16_t nightMaxMin;       // bounded night force-exit (630)
  uint16_t daySleepS;         // prod DAY_CHARGE sleep period (300)
  bool devNoSleep;            // commissioning: never take lifecycle day-sleep
  bool commissioning;         // command-only; no dusk/dawn/autonomous lifecycle
};

LifeConfig lifeConfigDefaults(bool devProfile);

struct LifeInputs {
  uint32_t nowMs;
  bool supplyGood;
  float supplyMa;
  float battV;
  uint8_t tier;        // LedTier byte; PROTECT/OFF suppress the show
  uint32_t lastRxMs;   // last accepted operator command (not peer/time traffic)
  uint32_t awakeGraceUntilMs; // no day-sleep before this (boot/wake windows)
  uint32_t rxHoldMs;   // stay awake this long after a control command (600000)
  int8_t forceNight;   // -1 auto, 0 force day, 1 force night (serial/bridge)
};

struct LifeState_t {
  uint8_t state;
  uint32_t duskHeldSinceMs;
  uint32_t dawnHeldSinceMs;
  uint32_t surplusHeldSinceMs;
  uint32_t noSurplusHeldSinceMs;
  uint32_t nightStartMs;
  bool nightDone; // bounded-night latch: needs day evidence before re-night
  bool initialized;
};

struct LifeOutputs {
  uint8_t state;
  bool stateChanged;
  bool showActive;     // run the program runtime + choreo tx
  bool strikesAllowed; // daytime-surplus gate (ANDed with power.may_strike)
  bool solarProbeActive; // measured surplus is extending a DAY_CHARGE wake
  bool wantSleep;      // prod DAY_CHARGE duty cycle
  uint16_t sleepS;
  uint16_t nightMin;   // minutes into the current night (heartbeat tail 13)
};

void lifeInit(LifeState_t &st);
LifeOutputs lifeTick(LifeState_t &st, const LifeInputs &in, const LifeConfig &c);
