#include "power_policy.h"

PowerConfig powerConfigDefaults() {
  PowerConfig c;
  c.dim_mv = 3000;
  c.off_mv = 2950;
  c.protect_mv = 2900;
  c.clear_delta_mv = 150;
  c.dim_confirm_s = 10;
  c.low_confirm_s = 60;
  c.load_comp_ohm = 0.15f;
  c.release_ma = 20;
  c.release_s = 60;
  c.release_floor_mv = 3100;
  c.protect_sleep_s = 900;
  return c;
}

void powerStateInit(PowerState &st, LedTier startTier) {
  st = PowerState{};
  st.tier = startTier;
  st.initialized = true;
}

uint8_t powerTierToStage(LedTier t) {
  switch (t) {
  case LedTier::FULL: return STAGE_FULL;
  case LedTier::DIM: return STAGE_DIM;
  case LedTier::OFF: return STAGE_LEDS_OFF;
  case LedTier::PROTECT: return STAGE_PROTECT;
  }
  return STAGE_PROTECT;
}

static void fillBudget(PowerBudget &b, LedTier tier, const PowerConfig &c) {
  b.tier = tier;
  switch (tier) {
  case LedTier::FULL:
    b.brightness_cap = 255;
    b.tick_divider = 1;
    b.may_tx_show = true;
    b.may_strike = true;
    b.must_sleep = false;
    break;
  case LedTier::DIM:
    b.brightness_cap = 128;
    b.tick_divider = 2;
    b.may_tx_show = true;
    b.may_strike = false;
    b.must_sleep = false;
    break;
  case LedTier::OFF:
    b.brightness_cap = 0;
    b.tick_divider = 4;   // program state advances slowly, frozen not reset
    b.may_tx_show = false; // hb-short continues; choreo sends stop
    b.may_strike = false;
    b.must_sleep = false;
    break;
  case LedTier::PROTECT:
    b.brightness_cap = 0;
    b.tick_divider = 0;
    b.may_tx_show = false;
    b.may_strike = false;
    b.must_sleep = true;
    break;
  }
  b.sleep_s = c.protect_sleep_s;
}

// Held-for helper: returns true when `cond` has been continuously true for
// holdS seconds; sinceMs tracks the streak start (0 = not held).
static bool heldFor(bool cond, uint32_t nowMs, uint32_t &sinceMs, uint16_t holdS) {
  if (!cond) {
    sinceMs = 0;
    return false;
  }
  if (sinceMs == 0) sinceMs = nowMs ? nowMs : 1;
  return (nowMs - sinceMs) >= (uint32_t)holdS * 1000UL;
}

PowerBudget powerPolicyTick(PowerState &st, const PowerSample &s, const PowerConfig &c) {
  PowerBudget b = {};
  if (!st.initialized) powerStateInit(st, LedTier::FULL);

  // Missing data is NEVER healthy: freeze the ladder in place (no tier
  // advance in either direction, no release). Phase-4 row "I2C/BQ/gauge read
  // failure -> never interpret missing data as healthy/daylight/charged".
  if (!s.batt_valid || s.batt_v < 0.5f) {
    fillBudget(b, st.tier, c);
    // A USB/VDC-powered service session must remain reachable even when a
    // previously persisted PROTECT stage has parked the rails. This does not
    // clear or weaken PROTECT: brightness stays zero and the durable stage is
    // unchanged. It only suppresses the 900 s sleep while verified external
    // power is present so an explicit bare-board recovery can be issued.
    if (st.tier == LedTier::PROTECT && s.supply_valid && s.supply_good)
      b.must_sleep = false;
    return b;
  }

  // Load compensation (ADR 0023 req 2): thresholds were measured under load;
  // compensate the IR sag out of the comparison when discharging.
  float loadA = (s.batt_ma < 0.0f) ? (-s.batt_ma / 1000.0f) : 0.0f;
  int mv = (int)((s.batt_v + c.load_comp_ohm * loadA) * 1000.0f + 0.5f);

  LedTier prev = st.tier;

  if (st.tier == LedTier::PROTECT) {
    // Compound release ONLY (addendum 2026-07-13): rebound voltage or a single
    // positive-current sample must not release. All conditions, sustained.
    bool qualified = s.supply_valid && s.supply_good && !s.charger_fault &&
                     s.batt_ma >= (float)c.release_ma &&
                     mv >= (int)c.release_floor_mv;
    if (heldFor(qualified, s.now_ms, st.releaseHeldSinceMs, c.release_s)) {
      // Release to OFF, not FULL: clearing PROTECT is permission to exist,
      // not to start the show; the ladder climbs on real voltage recovery.
      st.tier = LedTier::OFF;
      st.releaseHeldSinceMs = 0;
      st.belowDimSinceMs = st.belowOffSinceMs = st.aboveClearSinceMs = 0;
      fillBudget(b, st.tier, c);
      b.tier_changed = true;
      b.protect_released = true;
      return b;
    }
    fillBudget(b, st.tier, c);
    return b;
  }

  // Immediate critical floor: a reset can erase a timer, not the sample.
  if (mv < (int)c.protect_mv) {
    st.tier = LedTier::PROTECT;
    st.belowDimSinceMs = st.belowOffSinceMs = st.aboveClearSinceMs = 0;
    st.releaseHeldSinceMs = 0;
    fillBudget(b, st.tier, c);
    b.tier_changed = true;
    return b;
  }

  // Downward ladder (confirmed).
  if (st.tier == LedTier::FULL &&
      heldFor(mv < (int)c.dim_mv, s.now_ms, st.belowDimSinceMs, c.dim_confirm_s)) {
    st.tier = LedTier::DIM;
    st.belowDimSinceMs = 0;
    st.aboveClearSinceMs = 0;
  } else if (st.tier == LedTier::DIM &&
             heldFor(mv < (int)c.off_mv, s.now_ms, st.belowOffSinceMs, c.low_confirm_s)) {
    st.tier = LedTier::OFF;
    st.belowOffSinceMs = 0;
    st.aboveClearSinceMs = 0;
  }

  // Upward ladder (hysteresis: +150 mV over the tier's entry threshold,
  // sustained low_confirm_s -- dimming sheds 100+ mV of IR sag, so a naive
  // re-brighten oscillates).
  if (st.tier == LedTier::DIM) {
    if (heldFor(mv >= (int)(c.dim_mv + c.clear_delta_mv), s.now_ms,
                st.aboveClearSinceMs, c.low_confirm_s)) {
      st.tier = LedTier::FULL;
      st.aboveClearSinceMs = 0;
    }
  } else if (st.tier == LedTier::OFF) {
    if (heldFor(mv >= (int)(c.off_mv + c.clear_delta_mv), s.now_ms,
                st.aboveClearSinceMs, c.low_confirm_s)) {
      st.tier = LedTier::DIM;
      st.aboveClearSinceMs = 0;
    }
  }

  fillBudget(b, st.tier, c);
  b.tier_changed = (st.tier != prev);
  return b;
}
