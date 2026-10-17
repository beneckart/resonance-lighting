#pragma once

#include <stddef.h>
#include <stdint.h>

// Atom decompression button clicks, heard directly over radio. Each Atom press
// is one NB_DECOMPRESSION_CONTROL PLAY sequence number, sent as 3 copies to
// each of nine lanterns, so many frames arrive per press. A press is counted
// once per (atom, controller boot, seq). Presses closer than kWindowMs form one
// click: one or two settle after the window, a third fires at once (nothing is
// longer than a triple). Same timing as the Tone Lab's Atom listener.

struct ClickEvent {
  uint8_t atom[3];
  uint8_t clicks;  // 1, 2 or 3
};

class AtomClicks {
 public:
  static constexpr uint32_t kWindowMs = 600;
  static constexpr size_t kAtoms = 4;
  static constexpr size_t kSeen = 16;

  // Feed one received PLAY frame. Returns true when it was a new press.
  bool observe(const uint8_t atom[3], uint32_t boot, uint32_t seq, uint32_t nowMs, ClickEvent *fired);
  // Settle clicks whose window has passed. Returns true and fills one event.
  bool tick(uint32_t nowMs, ClickEvent *fired);

 private:
  struct Seen { uint8_t atom[3]; uint32_t boot, seq; bool used; };
  struct Pending { uint8_t atom[3]; uint8_t n; uint32_t lastMs; bool used; };
  Seen seen_[kSeen] = {};
  size_t seenNext_ = 0;
  Pending pending_[kAtoms] = {};
};
