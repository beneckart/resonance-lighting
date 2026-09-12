#pragma once
#include <stddef.h>
#include <stdint.h>

struct StorageCampaignStatus {
  bool active;
  uint16_t targets, prepared, refused;
  uint32_t dispatches, remainingMs;
};

// Immutable exact roster, one complete PROTECT cadence, monotonic wake deadline.
// Mode 0 uses legacy timed transport; 1/2 use the new explicit storage request.
class StorageCampaign {
 public:
  static constexpr size_t kCapacity = 192;
  bool begin(const uint8_t (*targets)[3], size_t count, uint8_t mode,
             uint32_t sleepS, uint32_t nowMs);
  bool next(uint32_t nowMs, size_t &index, uint8_t target[3], uint32_t &sleepS);
  void receipt(size_t index, uint8_t status);
  void stop();
  StorageCampaignStatus status(uint32_t nowMs);
  const uint8_t *target(size_t i) const;
  uint8_t mode() const { return mMode; }
 private:
  uint8_t mTargets[kCapacity][3] = {};
  uint8_t mStatus[kCapacity] = {};
  uint16_t mCount = 0, mCursor = 0;
  uint8_t mMode = 0;
  bool mActive = false;
  uint32_t mUntil = 0, mWake = 0, mNext = 0, mDispatches = 0;
};
