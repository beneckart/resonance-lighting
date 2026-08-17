// Pure authorization gate for a one-target, externally supervised low-LFP
// recovery artifact. This does not identify a battery by itself; the operator
// must still confirm the installed cell and watch temperature/current.
#pragma once

#include <stdint.h>

struct DeepRecoverySample {
  bool build_enabled;
  bool target_matches;
  bool supply_good;
  bool precharge_configured;
  float battery_v;
  float battery_ma;
  float supply_v;
  float supply_ma;
  uint8_t charger_fault;
};

bool deepRecoveryMayEnable(const DeepRecoverySample &s);
