// Program runtime skeleton (milestone 1 of the choreography concept doc):
// three programs behind one interface, bridge authority as an expiring lease,
// smooth fallback, power veto applied by the caller. Pure/native.
//
// Registry:
//   0 = PROG_IDLE         class-tinted slow breathe (field-selectable look)
//   1 = PROG_GH_CA        Greenberg-Hastings excitable media (the one real CA)
//   2 = PROG_BRIDGE_SHOW  NbShowFrame consumer (DJ/bench multicast)
//   3 = PROG_DIRECT       NbDirectFrame consumer (cambium browser-sim streaming)
//   4 = PROG_COMMISSION_DARK safe dark/listening commissioning fallback
//   5 = PROG_CONTAGION    Color Virus / Epidemic infection family
//   6+ reserved (timeline, ripple, Lenia, easter eggs -- M2).
#pragma once

#include <stdint.h>
#include "../fixture_context.h"
#include "../neighbor_table.h"

#define PROG_IDLE 0
#define PROG_GH_CA 1
#define PROG_BRIDGE_SHOW 2
#define PROG_DIRECT 3
#define PROG_COMMISSION_DARK 4
#define PROG_CONTAGION 5
#define PROG_COUNT 6

// Latest bridge show frame, as received (staleness judged by the runtime).
struct ShowFrameState {
  uint32_t rxMs; // 0 = never
  uint16_t phase;
  uint8_t hue, flags, val, bright, effect, beatPhase, energy;
};

// Latest direct color naming THIS fixture, as received (staleness judged by
// the runtime; the entry scan happened in net_peer).
struct DirectFrameState {
  uint32_t rxMs; // 0 = never
  uint8_t r, g, b, w;
  uint8_t flags; // NbDirectFrame flags: bit0=micro-lease bit1=hard-cut
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
  // Valid shared UTC lets autonomous programs make fleet-synchronous choices.
  // Bridge-owned programs keep their requested palette exactly.
  bool synchronizedPalette;
  bool utcValid;
  uint32_t utcS;
  // One fresh, debounced local ToF presence edge. Programs must explicitly
  // opt in; it is never a continuing level or actuator permission.
  bool tofPresenceRising;
  const DirectFrameState *directFrame; // runtime-owned; callers leave it null
                                       // (ChoreoRuntime::tick patches it in)
};

struct ProgramOutputs {
  FrameBuffer frame;
  uint8_t txState;     // compact state for NbChoreoState.state (GH state)
  uint8_t txIntensity; // render energy for neighbors/bridge viz
  uint16_t generation;
  uint16_t phaseMs;
  bool sendNow;        // edge-triggered choreo send (quiescent->excited)
  // A choreography program may request one physical knock, but it never owns
  // the mechanism. Platform glue applies the autonomous lifecycle/power gate
  // plus the hard arm/rest/maintenance/failsafe mechanism gates. Deliberate
  // operator radio knocks have a separate best-effort policy. `suppressLight`
  // keeps a sound-only program electrically dark.
  bool strikeRequested;
  uint16_t strikePulseMs;
  bool suppressLight;
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
  void init(uint8_t fixtureClass, uint16_t pixelCount, uint32_t seed,
            uint8_t autonomousProgram = PROG_GH_CA);
  // Bridge NB_PROGRAM_SET: returns false on unknown program (fails closed).
  bool applyProgramSet(uint8_t programId, uint16_t leaseS, uint32_t seed,
                       uint8_t flags, const uint8_t params[8], uint32_t nowMs);
  // An extended ShowFrame with flags bit0 grants a 10 s micro-lease.
  void noteShowFrame(const ShowFrameState &f, uint32_t nowMs);
  // A DirectFrame naming us; flags bit0 grants the same 10 s micro-lease.
  void noteDirectFrame(const DirectFrameState &f, uint32_t nowMs);
  void tick(const ProgramInputs &in, ProgramOutputs &out);
  // Profile flips change only the fallback. An active bridge lease remains
  // authoritative until release/expiry; otherwise switch immediately.
  bool setAutonomousProgram(uint8_t programId, uint32_t nowMs, bool hardCut);
  // UTC field scheduling uses the same programs with explicit autonomous
  // params. Passing nullptr restores that program's compiled defaults.
  bool setAutonomousPreset(uint8_t programId, const uint8_t params[8],
                           uint32_t seed, uint32_t nowMs, bool hardCut);

  uint8_t activeProgram() const { return mActive; }
  bool leaseActive() const { return mLease.active; }
  bool darkLeaseActive() const {
    return mLease.active && mActive == PROG_COMMISSION_DARK;
  }
  uint16_t leaseRemainingS(uint32_t nowMs) const;

private:
  void selectAutonomous(uint32_t nowMs, bool hardCut);
  Program *byId(uint8_t id);

  uint8_t mClass = 0;
  uint16_t mPixels = 1;
  uint32_t mSeed = 1;
  uint8_t mActive = PROG_IDLE;
  uint8_t mAutonomous = PROG_GH_CA; // class default (all classes: GH at night)
  uint8_t mAutonomousParams[8] = {};
  bool mAutonomousHasParams = false;
  uint32_t mAutonomousSeed = 1;
  ChoreoLease mLease = {};
  DirectFrameState mDirect = {}; // latest direct color (unlike showFrame, the
                                 // runtime owns this storage, not net_peer)
  // Crossfade: render both for RES_CHOREO_FADE_MS after a soft switch.
  uint8_t mPrev = PROG_IDLE;
  uint32_t mFadeStartMs = 0;
  bool mFading = false;
};

#define RES_CHOREO_FADE_MS 2000
#define RES_SHOWFRAME_HOLD_MS 1000  // >1 s without frames: hold + fade begins
#define RES_SHOWFRAME_STALE_MS 3000 // >3 s: autonomous fallback even in-lease
#define RES_SHOWFRAME_MICROLEASE_MS 10000
