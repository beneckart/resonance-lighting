#include "lifecycle.h"

LifeConfig lifeConfigDefaults(bool devProfile) {
  LifeConfig c;
  c.usefulSupplyMa = 20;
  c.surplusMa = 150;
  c.surplusConfirmS = 60;
  c.noSurplusConfirmS = 300;
  c.surplusBattV = 3.40f;
  c.deficitBattV = 3.30f;
  c.nightMaxMin = 630;
  c.daySleepS = 300;
  if (devProfile) {
    // Bench iteration: fast dusk/dawn for demos, never duty-cycle the radio.
    c.duskConfirmS = 60;
    c.dawnConfirmS = 60;
    c.devNoSleep = true;
  } else {
    c.duskConfirmS = 1800; // bare-peer fallback (donor: 30 min without input)
    c.dawnConfirmS = 300;
    c.devNoSleep = false;
  }
  return c;
}

void lifeInit(LifeState_t &st) {
  st = LifeState_t{};
  st.state = LIFE_DAY_CHARGE; // conservative: day posture until evidence
  st.initialized = true;
}

static bool heldFor(bool cond, uint32_t nowMs, uint32_t &sinceMs, uint16_t holdS) {
  if (!cond) {
    sinceMs = 0;
    return false;
  }
  if (sinceMs == 0) sinceMs = nowMs ? nowMs : 1;
  return (nowMs - sinceMs) >= (uint32_t)holdS * 1000UL;
}

LifeOutputs lifeTick(LifeState_t &st, const LifeInputs &in, const LifeConfig &c) {
  LifeOutputs out = {};
  if (!st.initialized) lifeInit(st);
  uint8_t prev = st.state;

  bool dayEvidence = in.supplyGood && in.supplyMa >= (float)c.usefulSupplyMa;
  bool surplus = (in.supplyGood && in.supplyMa >= (float)c.surplusMa) ||
                 in.battV >= c.surplusBattV;

  // Day evidence clears the bounded-night latch.
  if (dayEvidence) st.nightDone = false;

  if (in.forceNight == 1) {
    if (st.state != LIFE_NIGHT_SHOW) {
      st.state = LIFE_NIGHT_SHOW;
      st.nightStartMs = in.nowMs;
    }
  } else if (in.forceNight == 0) {
    if (st.state == LIFE_NIGHT_SHOW) st.state = LIFE_DAY_CHARGE;
  } else if (st.state == LIFE_NIGHT_SHOW) {
    // Bounded night: the hard stop that no missing sensor can defeat.
    uint32_t nightMin = (in.nowMs - st.nightStartMs) / 60000UL;
    if (nightMin >= c.nightMaxMin) {
      st.state = LIFE_DAY_CHARGE;
      st.nightDone = true;
    } else if (heldFor(dayEvidence, in.nowMs, st.dawnHeldSinceMs, c.dawnConfirmS)) {
      st.state = LIFE_DAY_CHARGE; // affirmative dawn: end the show exactly once
      st.dawnHeldSinceMs = 0;
    }
  } else {
    // Day states: dusk gate (supply absent, sustained). The nightDone latch
    // blocks an immediate re-night after a bounded-night exit.
    if (!st.nightDone &&
        heldFor(!dayEvidence, in.nowMs, st.duskHeldSinceMs, c.duskConfirmS)) {
      st.state = LIFE_NIGHT_SHOW;
      st.nightStartMs = in.nowMs;
      st.duskHeldSinceMs = 0;
    } else if (st.state == LIFE_DAY_CHARGE || st.state == LIFE_BOOT) {
      if (heldFor(surplus, in.nowMs, st.surplusHeldSinceMs, c.surplusConfirmS)) {
        st.state = LIFE_DAY_ACTIVE;
        st.surplusHeldSinceMs = 0;
      }
    } else if (st.state == LIFE_DAY_ACTIVE) {
      bool fade = !surplus && in.battV < c.deficitBattV && in.battV > 0.5f;
      if (heldFor(fade, in.nowMs, st.noSurplusHeldSinceMs, c.noSurplusConfirmS)) {
        st.state = LIFE_DAY_CHARGE;
        st.noSurplusHeldSinceMs = 0;
      }
    }
  }

  out.state = st.state;
  out.stateChanged = (st.state != prev);
  // LedTier: 0 FULL, 1 DIM, 2 OFF, 3 PROTECT -- the power veto over art.
  out.showActive = (st.state == LIFE_NIGHT_SHOW) && in.tier <= 1;
  out.strikesAllowed = (st.state == LIFE_DAY_ACTIVE) && in.tier == 0 &&
                       in.supplyGood && in.supplyMa >= (float)c.surplusMa;
  out.nightMin = (st.state == LIFE_NIGHT_SHOW)
                     ? (uint16_t)((in.nowMs - st.nightStartMs) / 60000UL)
                     : 0;
  // Day-charge duty cycle (prod only): sleep unless recently booted/woken or
  // anything was heard on the radio (bridge-hold keeps a poked fleet awake).
  bool rxHold = in.lastRxMs && (in.nowMs - in.lastRxMs) < in.rxHoldMs;
  out.wantSleep = (st.state == LIFE_DAY_CHARGE) && !c.devNoSleep && !rxHold &&
                  (int32_t)(in.nowMs - in.awakeGraceUntilMs) >= 0;
  out.sleepS = c.daySleepS;
  return out;
}
