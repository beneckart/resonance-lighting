#pragma once
#include <stdint.h>

// Absolute deadlines, never a chain of relative delays or catch-up strikes.
// The platform arms the durable load marker before starting this state machine.
struct SignatureSequence {
  static constexpr uint8_t COUNT = 7;
  static constexpr uint32_t PULSE_US = 40000;
  static constexpr uint32_t MAX_LATE_US = 20000;
  static constexpr uint32_t COOLDOWN_US = 1000000;
  enum Result : uint8_t { IDLE, RUNNING, COMPLETE, CANCELLED, LATE, MECHANISM };
  enum Action : uint8_t { WAIT, FIRE, DONE, ABORT };
  uint64_t startUs = 0, readyUs = 0;
  uint32_t offsetsUs[COUNT] = {};
  uint32_t maxLateUs = 0;
  uint8_t hits = 0;
  Result result = IDLE;

  static uint32_t beatUs(uint8_t i) {
    constexpr uint32_t beats[COUNT] = {0, 500000, 750000, 1000000,
                                     1500000, 2500000, 3000000};
    return beats[i];
  }
  bool active() const { return result == RUNNING; }
  bool ready(uint64_t now) const { return !active() && now >= readyUs; }
  bool start(uint64_t now) {
    if (!ready(now)) return false;
    startUs = now + 20000; // allow timer setup; not part of the musical phrase
    hits = 0;
    maxLateUs = 0;
    for (auto &offset : offsetsUs) offset = 0;
    result = RUNNING;
    return true;
  }
  void cancel(uint64_t now, Result why = CANCELLED) {
    if (!active()) return;
    result = why;
    readyUs = now + COOLDOWN_US;
  }
  Action poll(uint64_t now, bool gateOn) {
    if (!active()) return WAIT;
    if (hits == COUNT) {
      if (gateOn) return WAIT; // final pulse must actually end
      result = COMPLETE;
      readyUs = now + COOLDOWN_US;
      return DONE;
    }
    uint64_t due = startUs + beatUs(hits);
    if (now < due) return WAIT;
    uint64_t late = now - due;
    if (late > MAX_LATE_US) {
      cancel(now, LATE);
      return ABORT;
    }
    if (gateOn) {
      cancel(now, MECHANISM);
      return ABORT;
    }
    return FIRE;
  }
  // The platform acknowledges only a successfully driven pulse, timestamped
  // immediately after enabling the GPIO. A refused pulse is never a hit.
  void fired(uint64_t now) {
    if (!active() || hits >= COUNT) return;
    uint64_t late = now - (startUs + beatUs(hits));
    offsetsUs[hits++] = (uint32_t)(now - startUs);
    if (late > maxLateUs) maxLateUs = (uint32_t)late;
  }
};
