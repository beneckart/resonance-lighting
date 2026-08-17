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

  return testReport("test_deep_recovery");
}
