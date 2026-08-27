#pragma once

#include <stddef.h>
#include <stdint.h>

enum MaintenanceCampaignPhase : uint8_t {
  MAINT_CAMPAIGN_IDLE = 0,
  MAINT_CAMPAIGN_GATHER = 1,
  MAINT_CAMPAIGN_FROZEN = 2,
  MAINT_CAMPAIGN_EXPIRED = 3,
};

struct MaintenanceCampaignStatus {
  uint32_t jobId;
  MaintenanceCampaignPhase phase;
  uint16_t targetCount;
  uint16_t cursor;
  uint32_t dispatchCount;
  uint32_t remainingMs;
  uint32_t cycleMs;
};

// Fixed-storage round-robin scheduler for exact-target maintenance commands.
// At the 10 ms dispatch interval, a full 160-target roster repeats in 1.6 s,
// inside the fixture's 3 s production timer-wake window.
class MaintenanceCampaign {
 public:
  static constexpr size_t kCapacity = 160;
  static constexpr uint32_t kDispatchIntervalMs = 10;
  static constexpr uint32_t kMaxDurationMs = 3600000UL;

  MaintenanceCampaign();

  bool begin(uint32_t jobId, uint32_t durationMs, uint32_t nowMs);
  bool add(uint32_t jobId, const uint8_t target[3]);
  bool freeze(uint32_t jobId, uint32_t nowMs);
  bool next(uint32_t nowMs, uint8_t target[3]);
  MaintenanceCampaignStatus status(uint32_t nowMs);

 private:
  bool targetIsValid(const uint8_t target[3]) const;
  void expireIfDue(uint32_t nowMs);

  uint8_t mTargets[kCapacity][3];
  uint32_t mJobId;
  uint32_t mUntilMs;
  uint32_t mNextMs;
  uint32_t mDispatchCount;
  uint16_t mCount;
  uint16_t mCursor;
  MaintenanceCampaignPhase mPhase;
};
