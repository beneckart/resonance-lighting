#include "time_consensus.h"

#include <string.h>

static const uint32_t kMinUtc = 1735689600UL; // 2025-01-01
static const uint32_t kMaxUtc = 2082758400UL; // 2036-01-01
static const uint32_t kCandidateFreshMs = 10UL * 60UL * 1000UL;
static const uint32_t kEstimateHoldMs = 30UL * 60UL * 1000UL;
static const uint32_t kAgreementMs = 3000UL;

void timeConsensusInit(TimeConsensus &tc) { memset(&tc, 0, sizeof(tc)); }

static uint8_t sourceRank(uint8_t source) {
  switch (source) {
  case NB_TIME_GPS: return 0;
  case NB_TIME_BRIDGE: return 1;
  case NB_TIME_RTC: return 2;
  case NB_TIME_PEER: return 3;
  default: return 255;
  }
}

static uint64_t absDiff(uint64_t a, uint64_t b) { return a > b ? a - b : b - a; }

bool timeConsensusObserve(TimeConsensus &tc, const NbTimeQuality &q,
                          const uint8_t immediateSrc[3], uint32_t rxMs) {
  if (!(q.flags & NB_TIME_FLAG_VALID) || !(q.flags & NB_TIME_FLAG_DATE_VALID))
    return false;
  if (q.utc_s < kMinUtc || q.utc_s >= kMaxUtc || q.sub_ms >= 1000 ||
      sourceRank(q.source) == 255 || q.hops > 8 || q.age_s > 21600)
    return false;
  if (q.source == NB_TIME_PEER && q.hops == 0) return false;
  if (q.source != NB_TIME_PEER && q.hops != 0) return false;

  int slot = -1;
  int oldest = 0;
  for (int i = 0; i < TIME_CONSENSUS_SOURCES; ++i) {
    if (tc.candidates[i].used &&
        memcmp(tc.candidates[i].id, immediateSrc, 3) == 0) {
      slot = i;
      break;
    }
    if (!tc.candidates[i].used && slot < 0) slot = i;
    if ((uint32_t)(rxMs - tc.candidates[i].rxMs) >
        (uint32_t)(rxMs - tc.candidates[oldest].rxMs))
      oldest = i;
  }
  if (slot < 0) slot = oldest;

  TimeCandidate &c = tc.candidates[slot];
  c.used = true;
  memcpy(c.id, immediateSrc, 3);
  c.bootId = q.boot_id;
  c.source = q.source;
  c.hops = q.hops;
  c.utcMsAtRx = (uint64_t)q.utc_s * 1000ULL + q.sub_ms +
                 (uint64_t)q.age_s * 1000ULL;
  c.rxMs = rxMs;
  uint32_t uncertainty = (uint32_t)q.uncert_ms + q.hops * 25U;
  c.uncertaintyMs = uncertainty > 65535U ? 65535U : (uint16_t)uncertainty;
  return true;
}

TimeEstimate timeConsensusEstimate(TimeConsensus &tc, uint32_t nowMs) {
  TimeEstimate out = {};
  int best = -1;
  uint8_t bestVotes = 0;

  for (int i = 0; i < TIME_CONSENSUS_SOURCES; ++i) {
    TimeCandidate &a = tc.candidates[i];
    if (!a.used || (uint32_t)(nowMs - a.rxMs) > kCandidateFreshMs) continue;
    uint64_t aNow = a.utcMsAtRx + (uint32_t)(nowMs - a.rxMs);
    uint8_t votes = 0;
    for (int j = 0; j < TIME_CONSENSUS_SOURCES; ++j) {
      TimeCandidate &b = tc.candidates[j];
      if (!b.used || (uint32_t)(nowMs - b.rxMs) > kCandidateFreshMs) continue;
      uint64_t bNow = b.utcMsAtRx + (uint32_t)(nowMs - b.rxMs);
      if (absDiff(aNow, bNow) <= kAgreementMs) ++votes;
    }
    if (a.source == NB_TIME_PEER && votes < 2) continue;
    if (best < 0 || votes > bestVotes ||
        (votes == bestVotes && sourceRank(a.source) < sourceRank(tc.candidates[best].source)) ||
        (votes == bestVotes && sourceRank(a.source) == sourceRank(tc.candidates[best].source) &&
         a.uncertaintyMs < tc.candidates[best].uncertaintyMs)) {
      best = i;
      bestVotes = votes;
    }
  }

  if (best >= 0) {
    TimeCandidate &c = tc.candidates[best];
    uint64_t chosen = c.utcMsAtRx + (uint32_t)(nowMs - c.rxMs);
    uint64_t current = tc.accepted
                           ? tc.acceptedUtcMs + (uint32_t)(nowMs - tc.acceptedMonoMs)
                           : 0;
    // A source that disagrees with already accepted wall time by more than
    // five minutes is not a correction; it is a mis-set RTC or corrupt date.
    if (tc.accepted && absDiff(chosen, current) > 300000ULL) {
      best = -1;
    } else if (!tc.accepted || chosen >= current) {
      tc.acceptedUtcMs = chosen;
      tc.acceptedMonoMs = nowMs;
    } else {
      // Never step backward. A slightly slow source can refresh quality while
      // the monotonic estimate coasts until wall time catches up.
      tc.acceptedUtcMs = current;
      tc.acceptedMonoMs = nowMs;
    }
    if (best >= 0) {
      tc.accepted = true;
      tc.acceptedSourceRxMs = c.rxMs;
      tc.acceptedUncertaintyMs = c.uncertaintyMs;
      tc.acceptedSource = c.source;
      tc.acceptedHops = c.hops;
      tc.acceptedVotes = bestVotes;
    }
  }

  if (!tc.accepted ||
      (uint32_t)(nowMs - tc.acceptedSourceRxMs) > kEstimateHoldMs)
    return out;
  uint64_t utcMs = tc.acceptedUtcMs + (uint32_t)(nowMs - tc.acceptedMonoMs);
  out.valid = true;
  out.utcS = (uint32_t)(utcMs / 1000ULL);
  out.subMs = (uint16_t)(utcMs % 1000ULL);
  out.uncertaintyMs = tc.acceptedUncertaintyMs;
  out.source = tc.acceptedSource;
  out.votes = tc.acceptedVotes;
  out.hops = tc.acceptedHops;
  out.anchorAgeMs = (uint32_t)(nowMs - tc.acceptedSourceRxMs);
  return out;
}
