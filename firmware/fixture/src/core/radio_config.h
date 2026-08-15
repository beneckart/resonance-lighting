#pragma once

#include <stdint.h>

// Channel 11 is the commissioned fleet and portable-router channel. Builds may
// override RES_CHANNEL for an isolated lab, but an unspecified production
// build must never silently fall back to the historical channel-6 bench value.
#ifndef RES_CHANNEL
#define RES_CHANNEL 11
#endif

#define RES_CHANNEL_POLICY_VERSION 1u

static_assert(RES_CHANNEL >= 1 && RES_CHANNEL <= 13,
              "RES_CHANNEL must be a 2.4 GHz WiFi channel in 1..13");

// Stored 0 means no NVS key. Channel 6 was the pre-production fallback and was
// also written to three perimeter fixtures during a 2026-08-06 bench restore.
// Migrate only that known legacy value; preserve any other explicit lab choice.
constexpr uint8_t resolveRadioChannel(uint8_t stored,
                                      uint32_t policyVersion) {
  if (policyVersion < RES_CHANNEL_POLICY_VERSION &&
      (stored == 0 || stored == 6))
    return RES_CHANNEL;
  return (stored >= 1 && stored <= 13) ? stored : RES_CHANNEL;
}

constexpr bool radioChannelNeedsPersist(uint8_t stored,
                                        uint32_t policyVersion) {
  return policyVersion < RES_CHANNEL_POLICY_VERSION ||
         stored != resolveRadioChannel(stored, policyVersion);
}
