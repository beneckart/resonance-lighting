#include "storage_host_command.h"
#include <string.h>

static int hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool storageHostJobId(const char *text, uint32_t &jobId) {
  jobId = 0;
  if (!text || strlen(text) != 8) return false;
  uint32_t value = 0;
  for (size_t i = 0; i < 8; ++i) {
    int digit = hex(text[i]);
    if (digit < 0) return false;
    value = (value << 4) | (uint32_t)digit;
  }
  if (!value) return false;
  jobId = value;
  return true;
}

bool storageHostRequest(const char *job, const char *ids, const char *confirm,
                        StorageHostRequest &out) {
  out = {};
  StorageHostRequest candidate = {};
  if (!storageHostJobId(job, candidate.jobId) || !ids || !confirm ||
      strcmp(confirm, "CONFIRM-USB-WAKE") != 0) return false;
  size_t len = strlen(ids);
  if (len < 6 || (len + 1) % 7 != 0) return false;
  candidate.count = (len + 1) / 7;
  if (candidate.count > StorageHostRequest::kMaxTargets) return false;
  for (size_t i = 0; i < candidate.count; ++i) {
    const char *p = ids + i * 7;
    for (size_t j = 0; j < 3; ++j) {
      int hi = hex(p[j * 2]), lo = hex(p[j * 2 + 1]);
      if (hi < 0 || lo < 0) return false;
      candidate.targets[i][j] = (uint8_t)((hi << 4) | lo);
    }
    const uint8_t *id = candidate.targets[i];
    if (!(id[0] || id[1] || id[2])) return false;
    const uint8_t wand[3] = {0xF4, 0x03, 0x44};
    if (memcmp(id, wand, 3) == 0) return false;
    if (i + 1 < candidate.count && p[6] != ',') return false;
    for (size_t j = 0; j < i; ++j)
      if (memcmp(id, candidate.targets[j], 3) == 0) return false;
  }
  out = candidate;
  return true;
}
