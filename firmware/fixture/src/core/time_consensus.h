// Bounded, platform-independent UTC selector for ADR 0031 sparse anchors.
// It consumes the fleet's one NbTimeQuality wire type and exposes a monotonic
// wall-clock estimate. No source is allowed to move accepted time backward.
#pragma once

#include <stdint.h>

#include "packet.h"

#define TIME_CONSENSUS_SOURCES 8

struct TimeEstimate {
  bool valid;
  uint32_t utcS;
  uint16_t subMs;
  uint16_t uncertaintyMs;
  uint8_t source;
  uint8_t votes;
  uint8_t hops;
  uint32_t anchorAgeMs;
};

struct TimeCandidate {
  bool used;
  uint8_t id[3];
  uint16_t bootId;
  uint8_t source;
  uint8_t hops;
  uint64_t utcMsAtRx;
  uint32_t rxMs;
  uint16_t uncertaintyMs;
};

struct TimeConsensus {
  TimeCandidate candidates[TIME_CONSENSUS_SOURCES];
  bool accepted;
  uint64_t acceptedUtcMs;
  uint32_t acceptedMonoMs;
  uint32_t acceptedSourceRxMs;
  uint16_t acceptedUncertaintyMs;
  uint8_t acceptedSource;
  uint8_t acceptedHops;
  uint8_t acceptedVotes;
};

void timeConsensusInit(TimeConsensus &tc);

// Returns false for malformed, implausible, stale, or untrusted reports.
bool timeConsensusObserve(TimeConsensus &tc, const NbTimeQuality &q,
                          const uint8_t immediateSrc[3], uint32_t rxMs);

// Direct anchors are usable alone. Peer-relayed time requires two agreeing
// reports. Estimates expire without a fresh anchor so solar/power fallback can
// safely regain authority.
TimeEstimate timeConsensusEstimate(TimeConsensus &tc, uint32_t nowMs);
