#pragma once
#include <stdint.h>

// Minimum gap between accepted presses. Was 5000 ms (one signature at a time),
// then 250 ms, which still swallowed the second press of a natural double click
// (often 120-250 ms apart). 100 ms lets it through; release-arming plus the
// 35 ms debounce already stop bounce and held presses from repeating. Receivers keep their own
// phrase cooldown, so a fast second PLAY without relay mode is simply refused.
#ifndef ATOM_PRESS_GAP_MS
#define ATOM_PRESS_GAP_MS 100
#endif

struct DecompressionButton {
  static constexpr uint32_t PRESS_GAP_MS = ATOM_PRESS_GAP_MS;
  bool rawReleased = true, stableReleased = true, armed = false;
  uint32_t changedMs = 0, lastPressMs = 0;
  void begin(bool released, uint32_t now) {
    rawReleased = stableReleased = armed = released;
    changedMs = now;
    lastPressMs = now - PRESS_GAP_MS;
  }
  bool tick(bool released, uint32_t now) {
    if (released != rawReleased) { rawReleased = released; changedMs = now; }
    if (released == stableReleased || now - changedMs < 35) return false;
    stableReleased = released;
    if (released) { armed = true; return false; }
    bool fire = armed && now - lastPressMs >= PRESS_GAP_MS;
    armed = false; // a refused held press never becomes a delayed trigger
    if (fire) lastPressMs = now;
    return fire;
  }
};
