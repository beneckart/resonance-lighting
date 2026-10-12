#pragma once
#include <stdint.h>

struct DecompressionButton {
  bool rawReleased = true, stableReleased = true, armed = false;
  uint32_t changedMs = 0, lastPressMs = 0;
  void begin(bool released, uint32_t now) {
    rawReleased = stableReleased = armed = released;
    changedMs = now;
    lastPressMs = now - 5000;
  }
  bool tick(bool released, uint32_t now) {
    if (released != rawReleased) { rawReleased = released; changedMs = now; }
    if (released == stableReleased || now - changedMs < 35) return false;
    stableReleased = released;
    if (released) { armed = true; return false; }
    bool fire = armed && now - lastPressMs >= 5000;
    armed = false; // a refused held press never becomes a delayed trigger
    if (fire) lastPressMs = now;
    return fire;
  }
};
