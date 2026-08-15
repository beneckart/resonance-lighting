// ADR 0023 LFP power policy as a pure state machine: injected samples in,
// tier + budget out. No Arduino, no SDK, no NVS -- natively tested against the
// threshold ladder, hysteresis, and the compound PROTECT release.
//
// The ladder (standard tier, voltages UNDER LOAD, per-unit NVS-overridable):
//   FULL --(<3.00 V, 10 s confirm)--> DIM --(<2.95 V, 60 s confirm)--> OFF
//   any --(<2.90 V, IMMEDIATE)--> PROTECT (durable; compound release only)
// Re-brighten requires +150 mV over the threshold sustained 60 s. Voltage is
// load-compensated (~150 mOhm source path). Gauge SOC is NOT an input: RepSOC
// parks at 1% from ~60% delivered (ADR 0023 addendum) and is forbidden as a
// gate -- the struct deliberately has no SOC field.
#pragma once

#include <stdint.h>
#include "fixture_context.h"

struct PowerSample {
  uint32_t now_ms;
  bool batt_valid;   // a read actually succeeded this tick
  float batt_v;
  float batt_ma;     // corrected (/1.08); + = charging, - = discharging
  bool supply_valid;
  float supply_v;
  float supply_ma;
  bool supply_good;
  bool charger_fault; // any BQ fault bit set (blocks PROTECT release)
};

struct PowerConfig {
  uint16_t dim_mv;          // default 3000
  uint16_t off_mv;          // default 2950
  uint16_t protect_mv;      // default 2900 (immediate)
  uint16_t clear_delta_mv;  // default 150 (re-brighten hysteresis)
  uint16_t dim_confirm_s;   // default 10
  uint16_t low_confirm_s;   // default 60 (off tier + re-brighten confirms)
  float load_comp_ohm;      // default 0.15 (bv_comp = bv + I_load*R)
  // Compound PROTECT release (ALL required, sustained):
  uint16_t release_ma;      // default 20: corrected charge current floor
  uint16_t release_s;       // default 60: sustained duration
  uint16_t release_floor_mv;// default 3100: recovered voltage floor
  uint16_t protect_sleep_s; // default 900
};

PowerConfig powerConfigDefaults();

struct PowerState {
  LedTier tier;
  // confirm timers (0 = condition not currently held)
  uint32_t belowDimSinceMs;
  uint32_t belowOffSinceMs;
  uint32_t aboveClearSinceMs;
  uint32_t releaseHeldSinceMs;
  bool initialized;
};

// What the rest of the firmware consumes (the "power veto" -- programs and
// lifecycle read this, never raw voltages).
struct PowerBudget {
  LedTier tier;
  uint8_t brightness_cap; // 0-255 multiplier for every rendered frame
  uint8_t tick_divider;   // program ticks every Nth frame
  bool may_tx_show;       // choreo-state sends allowed
  bool may_strike;        // solar-surplus daytime gate ANDs with this
  bool must_sleep;        // PROTECT: park now
  uint16_t sleep_s;
  bool tier_changed;      // this tick
  bool protect_released;  // compound release fired this tick (exactly once)
};

void powerStateInit(PowerState &st, LedTier startTier);
PowerBudget powerPolicyTick(PowerState &st, const PowerSample &s, const PowerConfig &c);

// Tier -> boot-guard stage for NVS persistence ordering.
uint8_t powerTierToStage(LedTier t);
