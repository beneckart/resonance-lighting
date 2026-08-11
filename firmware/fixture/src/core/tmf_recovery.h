// Bounded escalation policy for the TMF8820 cooperative ranging machine.
// A normal failure still gets the inexpensive stop/start retry. Repeated
// consecutive failures request one full shared sensor-domain reset per boot.
#pragma once

#include <stdint.h>

static constexpr uint8_t TMF_DOMAIN_RESET_FAILURES = 3;

enum TmfRecoveryAction : uint8_t {
  TMF_RECOVERY_NONE = 0,
  TMF_RECOVERY_DOMAIN_RESET = 1,
};

struct TmfRecoveryPolicy {
  uint8_t consecutiveFailures;
  bool domainResetUsed;
};

void tmfRecoveryInit(TmfRecoveryPolicy &policy);
TmfRecoveryAction tmfRecoveryObserve(TmfRecoveryPolicy &policy, bool success);
