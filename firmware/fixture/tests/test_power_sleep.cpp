#include "test_util.h"
#include <initializer_list>
#include "../src/core/power_policy.h"
#include "../src/core/lifecycle.h"
#include "../src/core/boot_guard.h"

static PowerSample sample(uint32_t t, float v) {
  PowerSample s{};
  s.now_ms=t; s.batt_v=v; s.batt_valid=true; s.batt_corroborated=true;
  return s;
}

int main() {
  const auto config=powerConfigDefaults();
  // A real 12 s daytime wake: sleep must wait for BOTH OFF->DIM->FULL
  // confirmations, without crediting any part of the previous sleep.
  for (auto start : {LedTier::DIM, LedTier::OFF}) {
    PowerState p{}; powerStateInit(p,start);
    LifeState_t life{}; lifeInit(life);
    LifeInputs in{}; in.forceNight=0; in.awakeGraceUntilMs=12000;
    auto lc=lifeConfigDefaults(false);
    PowerBudget b{};
    uint32_t fullAt=0;
    for (uint32_t t=1000;t<=125000;t+=1000) {
      b=powerPolicyTick(p,sample(t,3.4f),config);
      in.nowMs=t; in.tier=(uint8_t)b.tier; in.powerConfirmAwake=b.confirm_awake;
      auto l=lifeTick(life,in,lc);
      if (b.tier!=LedTier::FULL) CHECK(!l.wantSleep);
      else { fullAt=t; CHECK(l.wantSleep); break; }
    }
    CHECK_EQ(fullAt,start==LedTier::DIM ? 61000u : 121000u);
  }
  // Chatter cannot renew the one 150-second extension; missing data cannot
  // be counted as healthy time, and the next wake starts a fresh proof.
  PowerState p{}; powerStateInit(p,LedTier::DIM);
  PowerBudget b{};
  for (uint32_t t=1000;t<=181000;t+=1000) {
    float v=t%30000==0 ? 3.2f : 3.4f;
    b=powerPolicyTick(p,sample(t,v),config);
    CHECK(b.tier==LedTier::DIM);
    if (t>=151000) CHECK(!b.confirm_awake);
  }
  powerStateInit(p,LedTier::DIM);
  for (uint32_t t=1000;t<=50000;t+=1000) b=powerPolicyTick(p,sample(t,3.4f),config);
  auto missing=sample(51000,3.4f); missing.batt_valid=false;
  b=powerPolicyTick(p,missing,config); CHECK(!b.confirm_awake);
  b=powerPolicyTick(p,sample(100000,3.4f),config); CHECK(b.tier==LedTier::DIM);
  b=powerPolicyTick(p,sample(151000,3.4f),config); CHECK(b.tier==LedTier::DIM);
  // Critical voltage always wins over the extension, without waiting.
  b=powerPolicyTick(p,sample(152000,3.0f),config);
  CHECK(b.tier==LedTier::PROTECT); CHECK(b.must_sleep); CHECK(!b.confirm_awake);
  // Verified service disarm preserves FULL; actual armed collapses keep
  // the existing FULL->DIM->PROTECT ladder, including software latch survival.
  auto d=bootGuardDecide(true,STAGE_FULL,true,false); CHECK_EQ(d.stage,STAGE_FULL);
  d=bootGuardDecide(true,STAGE_FULL,true,true); CHECK_EQ(d.stage,STAGE_DIM);
  d=bootGuardDecide(true,STAGE_DIM,true,true); CHECK_EQ(d.stage,STAGE_PROTECT);
  d=bootGuardDecide(true,STAGE_PROTECT,false,false); CHECK(d.park);
  return testReport("power_sleep");
}
