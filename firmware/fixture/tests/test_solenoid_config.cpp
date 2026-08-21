#include "test_util.h"

#include "solenoid_config.h"

int main() {
  CHECK_EQ(RES_SOLENOID_DEFAULT_ENABLED, 1u);

  // Historical absent/disabled state migrates to the universal capability.
  CHECK_EQ(resolveSolenoidEnabled(0, 0), 1);
  CHECK_EQ(resolveSolenoidEnabled(1, 0), 1);
  CHECK(solenoidPolicyNeedsPersist(0, 0));
  CHECK(solenoidPolicyNeedsPersist(1, 0));

  // After migration, a deliberate runtime disarm or re-arm is preserved.
  CHECK_EQ(resolveSolenoidEnabled(0, RES_SOLENOID_POLICY_VERSION), 0);
  CHECK_EQ(resolveSolenoidEnabled(1, RES_SOLENOID_POLICY_VERSION), 1);
  CHECK(!solenoidPolicyNeedsPersist(0, RES_SOLENOID_POLICY_VERSION));
  CHECK(!solenoidPolicyNeedsPersist(1, RES_SOLENOID_POLICY_VERSION));

  // Corrupt values normalize to enabled and are repaired in NVS.
  CHECK_EQ(resolveSolenoidEnabled(2, RES_SOLENOID_POLICY_VERSION), 1);
  CHECK(solenoidPolicyNeedsPersist(2, RES_SOLENOID_POLICY_VERSION));

  return testReport("solenoid_config");
}
