#include "deep_recovery.h"

bool deepRecoveryMayEnable(const DeepRecoverySample &s) {
  return s.build_enabled && s.target_matches &&
         s.battery_v >= 2.25f && s.battery_v < 4.4f &&
         s.battery_ma > -50.0f && s.battery_ma < 50.0f &&
         s.supply_good && s.supply_v >= 4.6f && s.supply_ma >= 50.0f &&
         s.charger_fault == 0x00 && s.precharge_configured;
}
