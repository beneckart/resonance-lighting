#include "storage_campaign.h"
#include <string.h>

bool StorageCampaign::begin(const uint8_t (*targets)[3], size_t count,
                            uint8_t mode, uint32_t sleepS, uint32_t nowMs) {
  if (mActive || !targets || count == 0 || count > kCapacity || mode > 2 ||
      (mode == 0 && (sleepS < 1800 || sleepS > 604800)) ||
      (mode != 0 && sleepS != 0)) return false;
  for (size_t i = 0; i < count; ++i) {
    if (!(targets[i][0] || targets[i][1] || targets[i][2])) return false;
    for (size_t j = 0; j < i; ++j)
      if (memcmp(targets[i], targets[j], 3) == 0) return false;
  }
  memcpy(mTargets, targets, count * 3);
  memset(mStatus, 0, sizeof(mStatus));
  mCount = (uint16_t)count;
  mCursor = 0;
  mMode = mode;
  mUntil = nowMs + 960000UL;
  mWake = nowMs + sleepS * 1000UL;
  mNext = nowMs;
  mDispatches = 0;
  mActive = true;
  return true;
}

bool StorageCampaign::next(uint32_t nowMs, size_t &index, uint8_t id[3],
                           uint32_t &sleepS) {
  if (!status(nowMs).active || (int32_t)(nowMs - mNext) < 0) return false;
  mNext = nowMs + 15;
  for (size_t tried = 0; tried < mCount; ++tried) {
    size_t i = mCursor;
    mCursor = (uint16_t)((mCursor + 1) % mCount);
    if (mStatus[i] != 0) continue;
    index = i;
    memcpy(id, mTargets[i], 3);
    sleepS = mMode == 0 ? (mWake - nowMs + 999UL) / 1000UL : 0;
    ++mDispatches;
    return true;
  }
  return false;
}

void StorageCampaign::receipt(size_t index, uint8_t status) {
  if (index >= mCount || status < 1 || status > 5) return;
  // RF copies can arrive out of order. A later PREPARED copy must never erase
  // a refusal or electrical entry failure already recorded for this request.
  if (mStatus[index] > 1 && status == 1) return;
  mStatus[index] = status;
}
void StorageCampaign::stop() { mActive = false; }
const uint8_t *StorageCampaign::target(size_t i) const {
  return i < mCount ? mTargets[i] : nullptr;
}
StorageCampaignStatus StorageCampaign::status(uint32_t nowMs) {
  if (mActive && (int32_t)(nowMs - mUntil) >= 0) mActive = false;
  StorageCampaignStatus s = {};
  s.active = mActive;
  s.targets = mCount;
  s.dispatches = mDispatches;
  s.remainingMs = mActive ? mUntil - nowMs : 0;
  for (size_t i = 0; i < mCount; ++i) {
    if (mStatus[i] == 1) ++s.prepared;
    if (mStatus[i] > 1) ++s.refused;
  }
  return s;
}
