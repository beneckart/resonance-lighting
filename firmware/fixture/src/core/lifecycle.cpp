#include "lifecycle.h"

#include "fixture_context.h"

LifeConfig lifeConfigDefaults(bool devProfile) {
  LifeConfig c;
  c.usefulSupplyMa = 20;
  c.surplusMa = 150;
  c.surplusExitMa = 100;
  c.surplusConfirmS = 60;
  c.noSurplusConfirmS = 300;
  c.nightMaxMin = 630;
  c.daySleepS = 300;
  if (devProfile) {
    // Commissioning: lifecycle does not infer intent from panel current. The
    // bridge owns output leases and command loss returns to the dark program.
    c.duskConfirmS = 60;
    c.dawnConfirmS = 60;
    c.devNoSleep = true;
    c.commissioning = true;
  } else {
    c.duskConfirmS = 1800; // bare-peer fallback (donor: 30 min without input)
    c.dawnConfirmS = 300;
    c.devNoSleep = false;
    c.commissioning = false;
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

  // COMMISSION is deliberately boring: keep the command runtime available,
  // never infer night/show intent from solar current, and never day-sleep.
  // Power tiers still veto rendering in the ordinary output fields below.
  if (c.commissioning) {
    st.state = LIFE_COMMISSION;
    out.state = st.state;
    out.stateChanged = (st.state != prev);
    out.showActive = in.tier <= 1;
    out.strikesAllowed = false;
    out.wantSleep = false;
    out.sleepS = c.daySleepS;
    out.nightMin = 0;
    return out;
  }

  bool dayEvidence = in.supplyGood && in.supplyMa >= (float)c.usefulSupplyMa;
  // DAY_ACTIVE means measured renewable/input surplus, not merely a charged
  // battery. Requiring FULL tier before entry prevents the radio/actuator
  // posture from competing with battery recovery. A lower current threshold
  // holds an already-active fixture through ordinary solar variation.
  bool solarEnter = in.supplyGood && in.supplyMa >= (float)c.surplusMa &&
                    in.tier == 0;
  bool solarStay = in.supplyGood &&
                   in.supplyMa >= (float)c.surplusExitMa;

  // Day evidence clears the bounded-night latch.
  if (dayEvidence) st.nightDone = false;

  if (in.forceNight == 1) {
    if (st.state != LIFE_NIGHT_SHOW) {
      st.state = LIFE_NIGHT_SHOW;
      st.nightStartMs = in.nowMs;
      st.surplusHeldSinceMs = 0;
      st.noSurplusHeldSinceMs = 0;
    }
  } else {
    bool forceDay = in.forceNight == 0;
    if (!forceDay && st.state == LIFE_NIGHT_SHOW) {
      // Bounded night: the hard stop that no missing sensor can defeat.
      uint32_t nightMin = (in.nowMs - st.nightStartMs) / 60000UL;
      if (nightMin >= c.nightMaxMin) {
        st.state = LIFE_DAY_CHARGE;
        st.nightDone = true;
        st.surplusHeldSinceMs = 0;
        st.noSurplusHeldSinceMs = 0;
      } else if (heldFor(dayEvidence, in.nowMs, st.dawnHeldSinceMs,
                         c.dawnConfirmS)) {
        st.state = LIFE_DAY_CHARGE; // affirmative dawn: end the show exactly once
        st.dawnHeldSinceMs = 0;
        st.surplusHeldSinceMs = 0;
        st.noSurplusHeldSinceMs = 0;
      }
    } else {
      // Scheduled/explicit day suppresses dusk but still runs the normal
      // charge <-> surplus-active policy. Daytime strikes therefore remain
      // possible only after the ordinary surplus confirmation and power veto.
      if (forceDay) {
        st.duskHeldSinceMs = 0;
        if (st.state == LIFE_NIGHT_SHOW) {
          st.state = LIFE_DAY_CHARGE;
          st.surplusHeldSinceMs = 0;
          st.noSurplusHeldSinceMs = 0;
        }
      }

      // Day states: dusk gate (supply absent, sustained). The nightDone latch
      // blocks an immediate re-night after a bounded-night exit.
      if (!forceDay && !st.nightDone &&
          heldFor(!dayEvidence, in.nowMs, st.duskHeldSinceMs,
                  c.duskConfirmS)) {
        st.state = LIFE_NIGHT_SHOW;
        st.nightStartMs = in.nowMs;
        st.duskHeldSinceMs = 0;
        st.surplusHeldSinceMs = 0;
        st.noSurplusHeldSinceMs = 0;
      } else if (st.state == LIFE_DAY_CHARGE || st.state == LIFE_BOOT) {
        if (heldFor(solarEnter, in.nowMs, st.surplusHeldSinceMs,
                    c.surplusConfirmS)) {
          st.state = LIFE_DAY_ACTIVE;
          st.surplusHeldSinceMs = 0;
          st.noSurplusHeldSinceMs = 0;
        }
      } else if (st.state == LIFE_DAY_ACTIVE) {
        if (heldFor(!solarStay, in.nowMs, st.noSurplusHeldSinceMs,
                    c.noSurplusConfirmS)) {
          st.state = LIFE_DAY_CHARGE;
          st.noSurplusHeldSinceMs = 0;
        }
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
  // The ordinary deep-sleep wake grace is only 15 s. Once genuine solar
  // surplus starts the 60 s confirmation timer, suppress sleep until that
  // continuous probe either confirms DAY_ACTIVE or the current falls away.
  out.solarProbeActive = st.state == LIFE_DAY_CHARGE && solarEnter &&
                         st.surplusHeldSinceMs != 0;
  // Day-charge duty cycle (prod only): sleep unless recently booted/woken or
  // an actual operator command was accepted (ordinary fleet heartbeats and
  // time beacons must never keep every fixture awake indefinitely).
  bool rxHold = in.lastRxMs && (in.nowMs - in.lastRxMs) < in.rxHoldMs;
  // PROTECT sleep belongs exclusively to power_policy. In particular, its
  // qualified release path deliberately stays awake for a continuous 60 s
  // evidence window. Letting the independent DAY_CHARGE cadence sleep here
  // resets that RAM-only window on every wake and makes release impossible.
  bool powerOwnsSleep = in.tier == (uint8_t)LedTier::PROTECT;
  out.wantSleep = (st.state == LIFE_DAY_CHARGE) && !c.devNoSleep && !rxHold &&
                  !out.solarProbeActive && !powerOwnsSleep &&
                  (int32_t)(in.nowMs - in.awakeGraceUntilMs) >= 0;
  out.sleepS = c.daySleepS;
  return out;
}
