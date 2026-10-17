#pragma once
#include <stdint.h>
#include <string.h>

// Shared Atom/perimeter/lantern roster, generated from the Tone Lab tone map
// (tone order, highest pitch first; each lantern's tuned pulse).
// partner: roster index of the lantern that chimes after this one in lantern-
// dings mode (a sensorless lantern paired with a sensing one), or -1.
struct SquadMember { const char *name; uint8_t id[3]; uint8_t pulseMs; int8_t partner; };
#include "squad_roster.h"
static constexpr unsigned SQUAD_COUNT = sizeof(SQUAD) / sizeof(SQUAD[0]);
static constexpr uint16_t SQUAD_ALL = (1U << SQUAD_COUNT) - 1;
inline int squadIndex(const uint8_t id[3]) {
  for (unsigned i=0; i<SQUAD_COUNT; ++i)
    if (!memcmp(id, SQUAD[i].id, 3)) return (int)i;
  return -1;
}
// Event cycle, one step per approach (Brian, 2026-10-17): rising pitch two
// lanterns at a time (the lower, then the higher SQUAD_PAIR_GAP_MS later),
// then a lone top lantern if the count is odd, then every lantern at once;
// repeat. For nine: (9,8) (7,6) (5,4) (3,2) (1) (all), by tone number.
static constexpr uint32_t SQUAD_PAIR_GAP_MS = 400;
static constexpr uint32_t SQUAD_COOLDOWN_MS = 2500;
static constexpr uint8_t SQUAD_STEPS = (SQUAD_COUNT + 1) / 2 + 1;
// Roster index of the k-th lantern in rising pitch (SQUAD is highest first).
constexpr unsigned squadRising(unsigned k) { return SQUAD_COUNT - 1 - k; }
constexpr uint16_t squadStepMask(uint8_t step) {
  if (step >= SQUAD_STEPS - 1) return SQUAD_ALL;
  uint16_t m = (uint16_t)(1U << squadRising(2U * step));
  if (2U * step + 1 < SQUAD_COUNT) m |= (uint16_t)(1U << squadRising(2U * step + 1));
  return m;
}
// Strike offset within a step: the higher lantern of a pair sounds after the lower.
constexpr uint32_t squadStepOffsetMs(uint8_t step, unsigned rosterIndex) {
  return (step < SQUAD_STEPS - 1 && 2U * step + 1 < SQUAD_COUNT &&
          rosterIndex == squadRising(2U * step + 1)) ? SQUAD_PAIR_GAP_MS : 0;
}
struct SquadCycle {
  uint8_t next = 0;
  bool fired = false;
  uint32_t lastMs = 0, count = 0;
  uint16_t targets() const { return squadStepMask(next); }
  // Fires with whichever of the step's lanterns are ready: one tired lantern
  // must not stall the cycle at an event. Nothing ready = nothing to play.
  bool canFire(uint32_t now, uint16_t ready) const {
    return (ready & targets()) != 0 && (!fired || now-lastMs >= SQUAD_COOLDOWN_MS);
  }
  void sent(uint32_t now) {
    lastMs=now; fired=true; ++count; next=(uint8_t)((next+1) % SQUAD_STEPS);
  }
};
struct SquadRadioRecovery {
  uint8_t attempts=0;
  bool paused=false;
  uint32_t resumeAt=0;
  bool start(uint32_t now) {
    if (paused || attempts>=3) return false;
    ++attempts; paused=true; resumeAt=now+200; return true;
  }
  bool due(uint32_t now) const { return paused && (int32_t)(now-resumeAt)>=0; }
  void resumed() { paused=false; }
};
static constexpr uint16_t SQUAD_PRESENCE_MIN_MM = 50;
static constexpr uint16_t SQUAD_PRESENCE_NEAR_MM = 400;
static constexpr uint16_t SQUAD_PRESENCE_CLEAR_MM = 600;
constexpr bool squadPresenceRangeValueValid(uint16_t mm) {
  // Match the existing VL53 valid-nearest-zone selection window.
  return mm >= 30 && mm <= 4000;
}
// Clear space after boot/fault and between approaches. Two fresh near frames;
// stale/failed frames never count as clearance. Zero is an invalid/no return.
struct SquadPresence {
  uint16_t minMm=SQUAD_PRESENCE_MIN_MM;
  uint16_t nearMm=SQUAD_PRESENCE_NEAR_MM, clearMm=SQUAD_PRESENCE_CLEAR_MM;
  bool armed=false, clearing=false;
  uint8_t nearFrames=0;
  uint32_t clearSince=0, lastMs=0;
  constexpr void invalidate() { armed=false; clearing=false; nearFrames=0; }
  constexpr bool setRange(uint16_t minimum, uint16_t near, uint16_t clear) {
    if (!squadPresenceRangeValueValid(minimum) ||
        !squadPresenceRangeValueValid(clear) || minimum>=near || near>=clear)
      return false;
    if (minimum!=minMm || near!=nearMm || clear!=clearMm) {
      minMm=minimum; nearMm=near; clearMm=clear; invalidate();
    }
    return true;
  }
  constexpr bool setMinMm(uint16_t mm) { return setRange(mm,nearMm,clearMm); }
  constexpr bool setNearMm(uint16_t mm) { return setRange(minMm,mm,clearMm); }
  constexpr bool setClearMm(uint16_t mm) { return setRange(minMm,nearMm,mm); }
  constexpr bool sample(uint32_t now, bool valid, const uint16_t zones[16]) {
    if (!valid || (lastMs && now-lastMs > 1000)) invalidate();
    lastMs=now;
    if (!valid) return false;
    bool near=false, occupied=false;
    for (unsigned i=0;i<16;++i) {
      if (zones[i]>=minMm && zones[i]<=nearMm) near=true;
      if (zones[i]>=minMm && zones[i]<=clearMm) occupied=true;
    }
    if (!occupied) {
      nearFrames=0;
      if (!clearing) { clearing=true; clearSince=now; }
      if (now-clearSince>=800) armed=true;
      return false;
    }
    clearing=false;
    if (!near) { nearFrames=0; return false; }
    if (nearFrames<2) ++nearFrames;
    if (!armed || nearFrames<2) return false;
    armed=false;
    return true;
  }
};
