#include "deep_recovery.h"

bool deepRecoveryMayEnable(const DeepRecoverySample &s) {
  return s.build_enabled && s.target_matches &&
         s.battery_v >= 2.25f && s.battery_v < 4.4f &&
         s.battery_ma > -50.0f && s.battery_ma < 50.0f &&
         s.supply_good && s.supply_v >= 4.6f && s.supply_ma >= 50.0f &&
         s.charger_fault == 0x00 && s.precharge_configured;
}

bool fleetRecoveryMayTest(const FleetRecoverySample &s) {
  return s.battery_v >= 2.20f && s.battery_v <= 2.50f &&
         s.battery_ma > -100.0f && s.battery_ma < 100.0f &&
         s.supply_good && s.supply_v >= 4.6f && s.supply_ma >= 50.0f &&
         s.charger_fault == 0x00 && s.precharge_configured;
}

bool fleetRecoveryBatteryDetected(uint16_t bqBatteryMv) {
  // The default VBAT_UVLO falling threshold is nominally 2.2 V. A missing
  // BAT node is discharged toward zero; an attached low LFP remains above it.
  return bqBatteryMv >= 2200 && bqBatteryMv < 4400;
}
