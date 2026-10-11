// Offline injected-sample reproduction. No hardware access or policy changes.
#include "power_policy.h"
#include <cstdio>
#include <cassert>
static PowerSample sample(unsigned now, float v, float ma, unsigned phase=1) {
  PowerSample s{};s.now_ms=now;s.batt_valid=true;s.batt_corroborated=true;
  s.batt_v=v;s.batt_ma=ma;s.supply_valid=true;s.supply_good=true;
  s.supply_v=5;s.charger_valid=true;s.charging_enabled=true;s.charge_phase=phase;
  return s;
}
static unsigned run(float v,float ma,unsigned phase,bool intermittent) {
  auto c=powerConfigDefaults();PowerState st{};powerStateInit(st,LedTier::PROTECT);
  for(unsigned t=1000;t<=600000;t+=1000) {
    float current=(intermittent && ((t/1000)%50)>=45)?0:ma;
    auto b=powerPolicyTick(st,sample(t,v,current,phase),c);
    if(b.protect_released)return t;
  }
  return 0;
}
int main() {
  auto strong=run(3.30f,50,1,false);
  auto weak=run(3.30f,-80,1,false);
  auto flicker=run(3.30f,50,1,true);
  auto full=run(3.55f,2,2,false);
  assert(strong==61000);assert(!weak);assert(!flicker);assert(full==61000);
  // Actual 9-second wake / 900-second sleep, RAM proof resets on each wake.
  // Illustrative external power model: +50 mA into the battery while asleep,
  // but 130 mA added radio/system draw produces -80 mA at the awake sample.
  for(int cycle=0;cycle<20;cycle++) {
    PowerState st{};powerStateInit(st,LedTier::PROTECT);
    for(unsigned t=1000;t<=9000;t+=1000) {
      auto b=powerPolicyTick(st,sample(t,3.30f,-80),powerConfigDefaults());
      assert(!b.protect_released);assert(b.must_sleep);
    }
  }
  double net=(50.0*900.0-80.0*9.0)/(900.0+9.0);
  std::printf("{\"stable_charge_release_ms\":%u,\"weak_source_release_ms\":%u,"
              "\"intermittent_charge_release_ms\":%u,\"full_cv_release_ms\":%u,"
              "\"weak_cycle_mean_battery_ma_model\":%.3f,\"weak_cycles_checked\":20}\n",
              strong,weak,flicker,full,net);
}
