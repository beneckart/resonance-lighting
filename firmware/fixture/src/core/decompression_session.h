#pragma once
#include <stdint.h>

struct DecompressionSession {
  uint64_t untilUs = 0;
  bool active(uint64_t now) const { return now < untilUs; }
  bool set(uint64_t now, uint16_t minutes) {
    if (minutes > 120) return false;
    untilUs = minutes ? now + (uint64_t)minutes * 60000000ULL : 0;
    return true;
  }
  uint32_t remainingSeconds(uint64_t now) const {
    return active(now) ? (uint32_t)((untilUs - now + 999999) / 1000000) : 0;
  }
};
