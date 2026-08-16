#pragma once

#include <stdint.h>

// Solenoid capability is a fleet-wide PowerFeather default. Boards without a
// capboard simply leave D7 unconnected; armed idle is INPUT/high-Z so rev-1
// receiver/manual sources on the shared net remain usable.
#define RES_SOLENOID_DEFAULT_ENABLED 1u
#define RES_SOLENOID_POLICY_VERSION 1u

// Policy v1 deliberately migrates every historical state to enabled. Earlier
// firmware used absent/zero as its default, so a stored zero cannot reliably
// distinguish an intentional disarm from the old build posture. Once v1 is
// recorded, explicit runtime arm/disarm choices are preserved.
constexpr uint8_t resolveSolenoidEnabled(uint8_t stored,
                                         uint32_t policyVersion) {
  if (policyVersion < RES_SOLENOID_POLICY_VERSION)
    return RES_SOLENOID_DEFAULT_ENABLED;
  return stored ? 1u : 0u;
}

constexpr bool solenoidPolicyNeedsPersist(uint8_t stored,
                                          uint32_t policyVersion) {
  return policyVersion < RES_SOLENOID_POLICY_VERSION || stored > 1u;
}
