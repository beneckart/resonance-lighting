// Program runtime skeleton (milestone 1 of the choreography concept doc):
// three programs behind one interface, bridge authority as an expiring lease,
// smooth fallback, power veto applied by the caller. Pure/native.
//
// Registry:
//   0 = PROG_IDLE         class-tinted slow breathe (fallback look, never blank)
//   1 = PROG_GH_CA        Greenberg-Hastings excitable media (the one real CA)
//   2 = PROG_BRIDGE_SHOW  NbShowFrame consumer (DJ/bench multicast)
//   3+ reserved (timeline, ripple, Lenia, easter eggs -- M2).
#pragma once

#include <stdint.h>
#include "../fixture_context.h"
#include "../neighbor_table.h"

#define PROG_IDLE 0
#define PROG_GH_CA 1
#define PROG_BRIDGE_SHOW 2
#define PROG_COUNT 3

// Latest bridge show frame, as received (staleness judged by the runtime).
struct ShowFrameState {
  uint32_t rxMs; // 0 = never
  uint16_t phase;
  uint8_t hue, flags, val, bright, effect, beatPhase, energy;
};

struct ProgramInputs {
  uint32_t nowMs;
  uint32_t dtMs;
  uint8_t fixtureClass;
  uint16_t pixelCount;
  const NeighborView *neighbors;
  uint8_t neighborCount;
  const ShowFrameState *showFrame;
  uint8_t tier;        // LedTier as byte; programs may adapt artistically
  uint8_t tickDivider; // power throttle (runtime pre-applies; informational)
};

struct ProgramOutputs {
  FrameBuffer frame;
  uint8_t txState;     // compact state for NbChoreoState.state (GH state)
  uint8_t txIntensity; // render energy for neighbors/bridge viz
  uint16_t generation;
  uint16_t phaseMs;
  bool sendNow;        // edge-triggered choreo send (quiescent->excited)
};

class Program {
public:
  virtual ~Program() {}
  virtual uint8_t id() const = 0;
  virtual void reset(uint32_t seed, const uint8_t params[8], uint8_t fixtureClass,
                     uint16_t pixelCount) = 0;
  virtual void tick(const ProgramInputs &in, ProgramOutputs &out) = 0;
};

// ---- runtime (selection + lease + crossfade) --------------------------------

struct ChoreoLease {
  bool active;
  uint8_t programId;
  uint32_t deadlineMs;
};

class ChoreoRuntime {
public:
  void init(uint8_t fixtureClass, uint16_t pixelCount, uint32_t seed);
  // Bridge NB_PROGRAM_SET: returns false on unknown program (fails closed).
  bool applyProgramSet(uint8_t programId, uint16_t leaseS, uint32_t seed,
                       uint8_t flags, const uint8_t params[8], uint32_t nowMs);
  // An extended ShowFrame with flags bit0 grants a 10 s micro-lease.
  void noteShowFrame(const ShowFrameState &f, uint32_t nowMs);
  void tick(const ProgramInputs &in, ProgramOutputs &out);

  uint8_t activeProgram() const { return mActive; }
  bool leaseActive() const { return mLease.active; }
  uint16_t leaseRemainingS(uint32_t nowMs) const;

private:
  void selectAutonomous(uint32_t nowMs, bool hardCut);
  Program *byId(uint8_t id);

  uint8_t mClass = 0;
  uint16_t mPixels = 1;
  uint32_t mSeed = 1;
  uint8_t mActive = PROG_IDLE;
  uint8_t mAutonomous = PROG_GH_CA; // class default (all classes: GH at night)
  ChoreoLease mLease = {};
  // Crossfade: render both for RES_CHOREO_FADE_MS after a soft switch.
  uint8_t mPrev = PROG_IDLE;
  uint32_t mFadeStartMs = 0;
  bool mFading = false;
};

#define RES_CHOREO_FADE_MS 2000
#define RES_SHOWFRAME_HOLD_MS 1000  // >1 s without frames: hold + fade begins
#define RES_SHOWFRAME_STALE_MS 3000 // >3 s: autonomous fallback even in-lease
#define RES_SHOWFRAME_MICROLEASE_MS 10000
