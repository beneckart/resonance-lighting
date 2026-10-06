#pragma once
#include <stddef.h>
#include <stdint.h>

struct StorageHostRequest {
  static constexpr size_t kMaxTargets = 20;
  uint32_t jobId;
  size_t count;
  uint8_t targets[kMaxTargets][3];
};

bool storageHostJobId(const char *text, uint32_t &jobId);
bool storageHostRequest(const char *job, const char *ids, const char *confirm,
                        StorageHostRequest &out);
