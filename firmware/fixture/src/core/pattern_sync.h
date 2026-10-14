// ADR 0084: shared group clock with an elected conductor, and the hit
// scheduler that plays one lantern's part on that clock. Platform independent:
// every time is passed in (local esp_timer microseconds on the device).
#pragma once

#include <stdint.h>

#include "packet.h"
#include "pattern_part.h"

struct PatternSync {
  static constexpr uint64_t BEAT_PERIOD_US = 500000;
  static constexpr uint64_t LISTEN_US = 1500000;     // before conducting at start
  static constexpr uint64_t TAKEOVER_US = 1600000;   // silence before taking over
  static constexpr uint64_t STAGGER_US = 400000;     // ID-derived spread of the above
  static constexpr uint64_t SETTLE_US = 1200000;     // uncontested conducting before play
  static constexpr uint8_t SAMPLES = 5;              // median window
  static constexpr uint8_t LOCK_SAMPLES = 2;

  enum Role : uint8_t { IDLE = 0, LISTEN = 1, FOLLOW = 2, CONDUCT = 3 };

  Role role = IDLE;
  uint8_t myId[3] = {};
  uint32_t patternId = 0, loopUs = 0, myBoot = 0;
  int64_t offsetUs = 0;     // group = local + offset
  uint64_t roleSinceUs = 0, lastBeatRxUs = 0;
  uint64_t staggerUs = 0;

  // Follower view of the current conductor.
  uint8_t conductor[3] = {};
  uint32_t conductorBoot = 0;
  struct Pending { uint32_t seq; uint64_t rxUs; } pending[4] = {};
  uint8_t pendingNext = 0;
  int64_t samples[SAMPLES] = {};
  uint8_t sampleCount = 0, sampleNext = 0;

  // Conductor bookkeeping.
  uint32_t beatSeq = 0, lastTxSeq = 0;
  uint64_t lastTxGroupUs = 0, nextBeatUs = 0;
  bool haveTx = false;

  // Diagnostics.
  uint32_t beatsRx = 0, beatsTx = 0, samplesTaken = 0, yields = 0, takeovers = 0;
  int64_t lastStepUs = 0;

  void start(uint64_t now, const uint8_t id[3], uint32_t pattern, uint32_t loop, uint32_t boot);
  void stop() { role = IDLE; }
  bool active() const { return role != IDLE; }
  uint64_t groupNow(uint64_t local) const { return (uint64_t)((int64_t)local + offsetUs); }
  // Ready to play: an uncontested conductor, or a follower with a measured offset.
  bool locked(uint64_t now) const;

  // Loop context. Returns true when a beat should be sent now; fills the payload
  // (the caller fills the header and sends, then reports beatSent()).
  bool beatDue(uint64_t now, NbPatternBeat &out);
  void beatSent(uint32_t seq, uint64_t txDoneLocalUs);
  void onBeat(const NbPatternBeat &b, uint64_t rxLocalUs, uint64_t now);
  void tick(uint64_t now);

private:
  void becomeConductor(uint64_t now);
  void follow(const uint8_t src[3], uint32_t boot, uint64_t now);
};

// Plays one part on group time: absolute targets, a lead for slow mechanisms,
// no catch-up, and never the same hit twice after a clock correction.
struct PatternPlayer {
  static constexpr uint64_t MAX_LATE_US = 20000;
  static constexpr uint64_t START_MARGIN_US = 50000;
  enum Action : uint8_t { WAIT = 0, FIRE = 1, SKIP = 2 };

  bool primed = false;
  uint64_t nextTargetUs = 0; // group time the next hit should SOUND (before lead)
  uint32_t hits = 0, lateSkips = 0, blocked = 0, maxLateUs = 0;

  // Smallest k*loop + offset strictly greater than `after` (group us).
  static uint64_t nextHit(const PatternPart &p, uint64_t after);
  void reset() { primed = false; }
  Action poll(const PatternPart &p, uint64_t groupNow, bool playable, bool gateOn);
  void fired(const PatternPart &p, uint64_t groupAtOnset);
  void refused(const PatternPart &p);
};
