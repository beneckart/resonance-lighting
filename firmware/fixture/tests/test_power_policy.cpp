#include "test_util.h"

#include "../src/core/power_policy.h"

static PowerSample sample(uint32_t nowMs, float bv, float ma,
                          bool supplyGood = false, float supplyMa = 0.0f,
                          bool fault = false, bool battValid = true) {
  PowerSample s = {};
  s.now_ms = nowMs;
  s.batt_valid = battValid;
  s.batt_v = bv;
  s.batt_ma = ma;
  s.supply_valid = true;
  s.supply_v = supplyGood ? 5.0f : 0.0f;
  s.supply_ma = supplyMa;
  s.supply_good = supplyGood;
  s.charger_fault = fault;
  return s;
}

int main() {
  PowerConfig c = powerConfigDefaults();

  // --- Downward ladder with confirm times ------------------------------------
  {
    PowerState st;
    powerStateInit(st, LedTier::FULL);
    uint32_t t = 1000;
    // Light load so compensation is negligible (-20 mA -> +3 mV).
    // 9 s below dim: not yet confirmed (10 s required).
    PowerBudget b = {};
    for (int i = 0; i < 10; i++) b = powerPolicyTick(st, sample(t += 1000, 3.14f, -20), c);
    CHECK(b.tier == LedTier::FULL);
    // 1 more second confirms.
    b = powerPolicyTick(st, sample(t += 1000, 3.14f, -20), c);
    CHECK(b.tier == LedTier::DIM);
    CHECK(b.tier_changed);
    CHECK_EQ(b.brightness_cap, 128u);
    // 60 s below off -> OFF.
    for (int i = 0; i < 61; i++) b = powerPolicyTick(st, sample(t += 1000, 3.09f, -20), c);
    CHECK(b.tier == LedTier::OFF);
    CHECK_EQ(b.brightness_cap, 0u);
    CHECK(!b.may_tx_show);
    // Below 3.05 -> PROTECT immediately, no confirm.
    b = powerPolicyTick(st, sample(t += 1000, 3.00f, -20), c);
    CHECK(b.tier == LedTier::PROTECT);
    CHECK(b.must_sleep);
    CHECK_EQ(b.sleep_s, 900u);
  }

  // --- One dip does not dim (confirm interrupted) -----------------------------
  {
    PowerState st;
    powerStateInit(st, LedTier::FULL);
    uint32_t t = 1000;
    PowerBudget b = {};
    for (int i = 0; i < 5; i++) b = powerPolicyTick(st, sample(t += 1000, 3.14f, -20), c);
    b = powerPolicyTick(st, sample(t += 1000, 3.20f, -20), c); // recovers
    for (int i = 0; i < 9; i++) b = powerPolicyTick(st, sample(t += 1000, 3.14f, -20), c);
    CHECK(b.tier == LedTier::FULL); // streak restarted; 9 s < 10 s
  }

  // --- Anti-oscillation: IR-sag rebound after dimming must NOT re-brighten ----
  {
    PowerState st;
    powerStateInit(st, LedTier::FULL);
    uint32_t t = 1000;
    PowerBudget b = {};
    // Full HEX load (860 mA) sags the terminal to 3.02 V -> compensated
    // 3.149 V, below dim -> DIM after the 10 s confirm.
    for (int i = 0; i < 11; i++) b = powerPolicyTick(st, sample(t += 1000, 3.02f, -860), c);
    CHECK(b.tier == LedTier::DIM);
    // Dimming halves the load; the terminal rebounds ~200 mV to 3.23 V, but
    // compensated (3.23 + 0.065 = 3.295) it sits just under dim+150 = 3.30:
    // the rebound alone must not re-brighten, ever.
    for (int i = 0; i < 120; i++) b = powerPolicyTick(st, sample(t += 1000, 3.23f, -430), c);
    CHECK(b.tier == LedTier::DIM); // held: rebound alone can't clear hysteresis
    // A real recovery (charging, voltage up) does clear it after 60 s.
    for (int i = 0; i < 61; i++) b = powerPolicyTick(st, sample(t += 1000, 3.40f, 200), c);
    CHECK(b.tier == LedTier::FULL);
  }

  // --- Load compensation moves a marginal case across the threshold -----------
  {
    PowerState st;
    powerStateInit(st, LedTier::FULL);
    uint32_t t = 1000;
    PowerBudget b = {};
    // 3.10 V at -860 mA: compensated = 3.10 + 0.129 = 3.229 -> NOT below dim.
    for (int i = 0; i < 30; i++) b = powerPolicyTick(st, sample(t += 1000, 3.10f, -860), c);
    CHECK(b.tier == LedTier::FULL);
    // Same terminal voltage nearly unloaded: below dim -> dims.
    for (int i = 0; i < 11; i++) b = powerPolicyTick(st, sample(t += 1000, 3.10f, -20), c);
    CHECK(b.tier == LedTier::DIM);
  }

  // --- Missing data freezes the ladder ---------------------------------------
  {
    PowerState st;
    powerStateInit(st, LedTier::DIM);
    uint32_t t = 1000;
    PowerBudget b = {};
    for (int i = 0; i < 300; i++)
      b = powerPolicyTick(st, sample(t += 1000, 0.0f, 0.0f, false, 0, false, false), c);
    CHECK(b.tier == LedTier::DIM); // no advance, no release, no re-brighten
  }

  // --- Externally powered service while PROTECT remains latched ---------------
  // A persisted PROTECT latch stays electrically parked but must remain awake
  // for explicit USB/VDC bare-board service when battery data is absent.
  {
    PowerState st;
    powerStateInit(st, LedTier::PROTECT);
    PowerBudget b = powerPolicyTick(
        st, sample(1000, 0.0f, 0.0f, true, 0, false, false), c);
    CHECK(b.tier == LedTier::PROTECT);
    CHECK_EQ(b.brightness_cap, 0u);
    CHECK(!b.must_sleep);
    b = powerPolicyTick(st, sample(2000, 0.0f, 0.0f, false, 0, false, false), c);
    CHECK(b.tier == LedTier::PROTECT);
    CHECK(b.must_sleep);
  }

  // --- Compound PROTECT release ----------------------------------------------
  {
    PowerState st;
    powerStateInit(st, LedTier::PROTECT);
    uint32_t t = 1000;
    PowerBudget b = {};
    // A single positive-current blip: stays latched (the +24 mA morning-blip
    // lesson -- rebound evidence is not release evidence).
    b = powerPolicyTick(st, sample(t += 1000, 3.30f, 25, true, 100), c);
    for (int i = 0; i < 30; i++) b = powerPolicyTick(st, sample(t += 1000, 3.20f, -5, false), c);
    CHECK(b.tier == LedTier::PROTECT);
    // Rebound voltage alone (no charge current): stays latched for hours.
    for (int i = 0; i < 3600; i++) b = powerPolicyTick(st, sample(t += 1000, 3.35f, -2, false), c);
    CHECK(b.tier == LedTier::PROTECT);
    // Qualified sustained charging: releases exactly once, to OFF (not FULL --
    // "clear stage once and do NOT immediately start the show").
    bool released = false;
    for (int i = 0; i < 61; i++) {
      b = powerPolicyTick(st, sample(t += 1000, 3.30f, 150, true, 300), c);
      // The release needs 60 s, so qualified recovery must suppress the normal
      // PROTECT sleep throughout the streak or it can never finish.
      if (!b.protect_released) CHECK(!b.must_sleep);
      if (b.protect_released) {
        CHECK(!released); // exactly once
        released = true;
      }
    }
    CHECK(released);
    CHECK(b.tier == LedTier::OFF);
    // Charger fault blocks release.
    PowerState st2;
    powerStateInit(st2, LedTier::PROTECT);
    uint32_t t2 = 1000;
    PowerBudget b2 = {};
    for (int i = 0; i < 300; i++)
      b2 = powerPolicyTick(st2, sample(t2 += 1000, 3.30f, 150, true, 300, true), c);
    CHECK(b2.tier == LedTier::PROTECT);
    // An interrupted streak restarts the sustain window.
    PowerState st3;
    powerStateInit(st3, LedTier::PROTECT);
    uint32_t t3 = 1000;
    PowerBudget b3 = {};
    for (int i = 0; i < 40; i++) b3 = powerPolicyTick(st3, sample(t3 += 1000, 3.30f, 150, true, 300), c);
    b3 = powerPolicyTick(st3, sample(t3 += 1000, 3.30f, 5, true, 300), c); // dip
    for (int i = 0; i < 40; i++) b3 = powerPolicyTick(st3, sample(t3 += 1000, 3.30f, 150, true, 300), c);
    CHECK(b3.tier == LedTier::PROTECT); // 40 s < 60 s after restart
  }

  // --- OFF tier climbs back to DIM with hysteresis ----------------------------
  {
    PowerState st;
    powerStateInit(st, LedTier::OFF);
    uint32_t t = 1000;
    PowerBudget b = {};
    for (int i = 0; i < 61; i++) b = powerPolicyTick(st, sample(t += 1000, 3.27f, 100), c);
    CHECK(b.tier == LedTier::DIM); // 3.27 >= 3.10+0.15 sustained
  }

  // --- Tier -> stage mapping --------------------------------------------------
  CHECK_EQ(powerTierToStage(LedTier::FULL), (uint8_t)STAGE_FULL);
  CHECK_EQ(powerTierToStage(LedTier::DIM), (uint8_t)STAGE_DIM);
  CHECK_EQ(powerTierToStage(LedTier::OFF), (uint8_t)STAGE_LEDS_OFF);
  CHECK_EQ(powerTierToStage(LedTier::PROTECT), (uint8_t)STAGE_PROTECT);

  return testReport("test_power_policy");
}
