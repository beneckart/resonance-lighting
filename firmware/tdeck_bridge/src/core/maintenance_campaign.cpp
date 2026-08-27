#include "maintenance_campaign.h"

#include <string.h>

MaintenanceCampaign::MaintenanceCampaign()
    : mJobId(0),
      mUntilMs(0),
      mNextMs(0),
      mDispatchCount(0),
      mCount(0),
      mCursor(0),
      mPhase(MAINT_CAMPAIGN_IDLE) {
  memset(mTargets, 0, sizeof(mTargets));
}

bool MaintenanceCampaign::targetIsValid(const uint8_t target[3]) const {
  return target && (target[0] != 0 || target[1] != 0 || target[2] != 0);
}

bool MaintenanceCampaign::begin(uint32_t jobId, uint32_t durationMs,
                                uint32_t nowMs) {
  if (jobId == 0 || durationMs == 0 || durationMs > kMaxDurationMs) return false;
  mJobId = jobId;
  mUntilMs = nowMs + durationMs;
  mNextMs = nowMs;
  mDispatchCount = 0;
  mCount = 0;
  mCursor = 0;
  mPhase = MAINT_CAMPAIGN_GATHER;
  memset(mTargets, 0, sizeof(mTargets));
  return true;
}

bool MaintenanceCampaign::add(uint32_t jobId, const uint8_t target[3]) {
  if (mPhase != MAINT_CAMPAIGN_GATHER || jobId != mJobId ||
      !targetIsValid(target))
    return false;
  for (uint16_t i = 0; i < mCount; ++i) {
    if (memcmp(mTargets[i], target, 3) == 0) return true;
  }
  if (mCount >= kCapacity) return false;
  memcpy(mTargets[mCount++], target, 3);
  return true;
}

bool MaintenanceCampaign::freeze(uint32_t jobId, uint32_t nowMs) {
  expireIfDue(nowMs);
  if (jobId == 0 || jobId != mJobId || mPhase != MAINT_CAMPAIGN_GATHER)
    return false;
  mPhase = MAINT_CAMPAIGN_FROZEN;
  mUntilMs = nowMs;
  return true;
}

void MaintenanceCampaign::expireIfDue(uint32_t nowMs) {
  if (mPhase == MAINT_CAMPAIGN_GATHER &&
      (int32_t)(nowMs - mUntilMs) >= 0) {
    mPhase = MAINT_CAMPAIGN_EXPIRED;
  }
}

bool MaintenanceCampaign::next(uint32_t nowMs, uint8_t target[3]) {
  expireIfDue(nowMs);
  if (!target || mPhase != MAINT_CAMPAIGN_GATHER || mCount == 0 ||
      (int32_t)(nowMs - mNextMs) < 0)
    return false;
  memcpy(target, mTargets[mCursor], 3);
  mCursor = (uint16_t)((mCursor + 1U) % mCount);
  ++mDispatchCount;
  mNextMs = nowMs + kDispatchIntervalMs;
  return true;
}

MaintenanceCampaignStatus MaintenanceCampaign::status(uint32_t nowMs) {
  expireIfDue(nowMs);
  MaintenanceCampaignStatus value = {};
  value.jobId = mJobId;
  value.phase = mPhase;
  value.targetCount = mCount;
  value.cursor = mCursor;
  value.dispatchCount = mDispatchCount;
  value.remainingMs =
      mPhase == MAINT_CAMPAIGN_GATHER ? mUntilMs - nowMs : 0;
  value.cycleMs = (uint32_t)mCount * kDispatchIntervalMs;
  return value;
}
