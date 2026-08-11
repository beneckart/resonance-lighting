#include "tmf_recovery.h"

void tmfRecoveryInit(TmfRecoveryPolicy &policy) {
  policy.consecutiveFailures = 0;
  policy.domainResetUsed = false;
}

TmfRecoveryAction tmfRecoveryObserve(TmfRecoveryPolicy &policy, bool success) {
  if (success) {
    policy.consecutiveFailures = 0;
    return TMF_RECOVERY_NONE;
  }

  if (policy.consecutiveFailures < UINT8_MAX) policy.consecutiveFailures++;
  if (!policy.domainResetUsed &&
      policy.consecutiveFailures >= TMF_DOMAIN_RESET_FAILURES) {
    policy.consecutiveFailures = 0;
    policy.domainResetUsed = true;
    return TMF_RECOVERY_DOMAIN_RESET;
  }
  return TMF_RECOVERY_NONE;
}
