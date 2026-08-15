#include "test_util.h"

#include "../src/core/tmf_recovery.h"

int main() {
  TmfRecoveryPolicy p;
  tmfRecoveryInit(p);
  CHECK_EQ(p.consecutiveFailures, 0);
  CHECK(!p.domainResetUsed);

  // Isolated failures use the normal stop/start retry and a good report clears
  // the streak.
  CHECK_EQ(tmfRecoveryObserve(p, false), TMF_RECOVERY_NONE);
  CHECK_EQ(p.consecutiveFailures, 1);
  CHECK_EQ(tmfRecoveryObserve(p, true), TMF_RECOVERY_NONE);
  CHECK_EQ(p.consecutiveFailures, 0);

  // Three consecutive failures request exactly one full domain reset.
  CHECK_EQ(tmfRecoveryObserve(p, false), TMF_RECOVERY_NONE);
  CHECK_EQ(tmfRecoveryObserve(p, false), TMF_RECOVERY_NONE);
  CHECK_EQ(tmfRecoveryObserve(p, false), TMF_RECOVERY_DOMAIN_RESET);
  CHECK(p.domainResetUsed);
  CHECK_EQ(p.consecutiveFailures, 0);

  // A persistent hardware fault must not flap the shared rail indefinitely.
  for (int i = 0; i < 12; i++)
    CHECK_EQ(tmfRecoveryObserve(p, false), TMF_RECOVERY_NONE);
  CHECK(p.domainResetUsed);

  return testReport("test_tmf_recovery");
}
