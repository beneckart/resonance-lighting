#include "deep_recovery.h"
#include "test_util.h"

static DeepRecoverySample good() {
  DeepRecoverySample s = {};
  s.build_enabled = true;
  s.target_matches = true;
  s.supply_good = true;
  s.precharge_configured = true;
  s.battery_v = 2.48f;
  s.battery_ma = 0.0f;
  s.supply_v = 4.86f;
  s.supply_ma = 140.0f;
  s.charger_fault = 0;
  return s;
}

int main() {
  auto s = good();
  CHECK(deepRecoveryMayEnable(s));

  s = good(); s.build_enabled = false; CHECK(!deepRecoveryMayEnable(s));
  s = good(); s.target_matches = false; CHECK(!deepRecoveryMayEnable(s));
  s = good(); s.battery_v = 2.249f; CHECK(!deepRecoveryMayEnable(s));
  s = good(); s.battery_v = 4.4f; CHECK(!deepRecoveryMayEnable(s));
  s = good(); s.battery_ma = -50.0f; CHECK(!deepRecoveryMayEnable(s));
  s = good(); s.battery_ma = 50.0f; CHECK(!deepRecoveryMayEnable(s));
  s = good(); s.supply_good = false; CHECK(!deepRecoveryMayEnable(s));
  s = good(); s.supply_v = 4.599f; CHECK(!deepRecoveryMayEnable(s));
  s = good(); s.supply_ma = 49.9f; CHECK(!deepRecoveryMayEnable(s));
  s = good(); s.charger_fault = 1; CHECK(!deepRecoveryMayEnable(s));
  s = good(); s.precharge_configured = false; CHECK(!deepRecoveryMayEnable(s));

  FleetRecoverySample f = {};
  f.supply_good = true;
  f.precharge_configured = true;
  f.battery_v = 2.42f;
  f.battery_ma = 0.0f;
  f.supply_v = 4.85f;
  f.supply_ma = 120.0f;
  f.charger_fault = 0;
  CHECK(fleetRecoveryMayTest(f));
  f.battery_v = 2.19f; CHECK(!fleetRecoveryMayTest(f));
  f.battery_v = 2.42f; f.supply_good = false; CHECK(!fleetRecoveryMayTest(f));
  f.supply_good = true; f.charger_fault = 1; CHECK(!fleetRecoveryMayTest(f));
  CHECK(fleetRecoveryBatteryDetected(2199)); // real cell sagging under 30 mA
  CHECK(fleetRecoveryBatteryDetected(2200));
  CHECK(fleetRecoveryBatteryDetected(2000));  // floor: real sagged 2.2 V cell
  CHECK(!fleetRecoveryBatteryDetected(1999)); // collapsed floating node
  CHECK(fleetRecoveryBatteryDetected(2450));
  CHECK(!fleetRecoveryBatteryDetected(4400));

  return testReport("test_deep_recovery");
}
