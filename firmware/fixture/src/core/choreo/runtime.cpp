#include "program.h"

Program *newProgIdle();
Program *newProgGhCa();
Program *newProgBridge();
Program *newProgDirect();

Program *ChoreoRuntime::byId(uint8_t id) {
  switch (id) {
  case PROG_IDLE: return newProgIdle();
  case PROG_GH_CA: return newProgGhCa();
  case PROG_BRIDGE_SHOW: return newProgBridge();
  case PROG_DIRECT: return newProgDirect();
  default: return nullptr;
  }
}

void ChoreoRuntime::init(uint8_t fixtureClass, uint16_t pixelCount, uint32_t seed) {
  mClass = fixtureClass;
  mPixels = pixelCount;
  mSeed = seed ? seed : 1;
  mAutonomous = PROG_GH_CA; // class default (M1: every class runs GH at night)
  mActive = mAutonomous;
  mLease = {};
  mFading = false;
  static const uint8_t noParams[8] = {};
  byId(PROG_IDLE)->reset(mSeed, noParams, mClass, mPixels);
  byId(PROG_GH_CA)->reset(mSeed, noParams, mClass, mPixels);
  byId(PROG_BRIDGE_SHOW)->reset(mSeed, noParams, mClass, mPixels);
  byId(PROG_DIRECT)->reset(mSeed, noParams, mClass, mPixels);
}

bool ChoreoRuntime::applyProgramSet(uint8_t programId, uint16_t leaseS, uint32_t seed,
                                    uint8_t flags, const uint8_t params[8],
                                    uint32_t nowMs) {
  if (programId == 0 || leaseS == 0) { // explicit release
    if (mLease.active) selectAutonomous(nowMs, false);
    mLease = {};
    return true;
  }
  Program *p = byId(programId);
  if (!p) return false; // unknown program: reject, stay autonomous (fails closed)
  bool hardCut = flags & 0x01;
  if (mActive != programId) {
    p->reset(seed ? seed : mSeed, params, mClass, mPixels);
    if (!hardCut) {
      mPrev = mActive;
      mFadeStartMs = nowMs;
      mFading = true;
    } else {
      mFading = false;
    }
    mActive = programId;
  }
  mLease.active = true;
  mLease.programId = programId;
  mLease.deadlineMs = nowMs + (uint32_t)leaseS * 1000UL;
  return true;
}

void ChoreoRuntime::noteShowFrame(const ShowFrameState &f, uint32_t nowMs) {
  // Extended frames with flags bit0 carry an implicit micro-lease so today's
  // bench master (streams frames, never sends PROGRAM_SET) keeps driving.
  if (!(f.flags & 0x01)) return;
  if (mLease.active && mLease.programId != PROG_BRIDGE_SHOW) return; // explicit lease wins
  if (mActive != PROG_BRIDGE_SHOW) {
    static const uint8_t noParams[8] = {};
    byId(PROG_BRIDGE_SHOW)->reset(mSeed, noParams, mClass, mPixels);
    mPrev = mActive;
    mFadeStartMs = nowMs;
    mFading = true;
    mActive = PROG_BRIDGE_SHOW;
  }
  mLease.active = true;
  mLease.programId = PROG_BRIDGE_SHOW;
  mLease.deadlineMs = nowMs + RES_SHOWFRAME_MICROLEASE_MS;
}

void ChoreoRuntime::noteDirectFrame(const DirectFrameState &f, uint32_t nowMs) {
  mDirect = f; // latest command wins; ProgDirect + the staleness check read this
  // Frames with flags bit0 carry the same implicit micro-lease as extended
  // ShowFrames, so a cambium bridge that only streams frames keeps driving.
  if (!(f.flags & 0x01)) return;
  if (mLease.active && mLease.programId != PROG_DIRECT) return; // explicit lease wins
  if (mActive != PROG_DIRECT) {
    static const uint8_t noParams[8] = {};
    byId(PROG_DIRECT)->reset(mSeed, noParams, mClass, mPixels);
    mPrev = mActive;
    mFadeStartMs = nowMs;
    mFading = true;
    mActive = PROG_DIRECT;
  }
  mLease.active = true;
  mLease.programId = PROG_DIRECT;
  mLease.deadlineMs = nowMs + RES_SHOWFRAME_MICROLEASE_MS;
}

void ChoreoRuntime::selectAutonomous(uint32_t nowMs, bool hardCut) {
  if (mActive == mAutonomous) return;
  static const uint8_t noParams[8] = {};
  byId(mAutonomous)->reset(mSeed, noParams, mClass, mPixels);
  if (!hardCut) {
    mPrev = mActive;
    mFadeStartMs = nowMs;
    mFading = true;
  }
  mActive = mAutonomous;
}

uint16_t ChoreoRuntime::leaseRemainingS(uint32_t nowMs) const {
  if (!mLease.active) return 0;
  int32_t rem = (int32_t)(mLease.deadlineMs - nowMs);
  return rem > 0 ? (uint16_t)(rem / 1000) : 0;
}

void ChoreoRuntime::tick(const ProgramInputs &in, ProgramOutputs &out) {
  // Lease expiry -> smooth return to autonomy (never freeze, never blank).
  if (mLease.active && (int32_t)(in.nowMs - mLease.deadlineMs) >= 0) {
    mLease = {};
    selectAutonomous(in.nowMs, false);
  }
  // ShowFrame staleness inside a bridge lease: >3 s silent -> autonomous even
  // though the lease clock is still running.
  if (mActive == PROG_BRIDGE_SHOW && in.showFrame &&
      (in.showFrame->rxMs == 0 ||
       in.nowMs - in.showFrame->rxMs > RES_SHOWFRAME_STALE_MS)) {
    mLease = {};
    selectAutonomous(in.nowMs, false);
  }
  // DirectFrame staleness inside a direct lease: same >3 s ladder.
  if (mActive == PROG_DIRECT &&
      (mDirect.rxMs == 0 || in.nowMs - mDirect.rxMs > RES_SHOWFRAME_STALE_MS)) {
    mLease = {};
    selectAutonomous(in.nowMs, false);
  }

  // ProgDirect reads its target from runtime-owned storage (net_peer owns the
  // ShowFrame): patch the pointer into a local copy of the inputs.
  ProgramInputs pin = in;
  pin.directFrame = &mDirect;

  Program *active = byId(mActive);
  active->tick(pin, out);

  if (mFading) {
    uint32_t dt = in.nowMs - mFadeStartMs;
    if (dt >= RES_CHOREO_FADE_MS || mPrev == mActive) {
      mFading = false;
    } else {
      // Render the outgoing program too and blend (linear alpha).
      ProgramOutputs prevOut = {};
      byId(mPrev)->tick(pin, prevOut);
      uint32_t a = dt * 255 / RES_CHOREO_FADE_MS; // 0 -> new fully in at 255
      for (int i = 0; i < FRAME_MAX_PIXELS; i++)
        for (int ch = 0; ch < 4; ch++)
          out.frame.px[i][ch] =
              (uint8_t)(((uint32_t)out.frame.px[i][ch] * a +
                         (uint32_t)prevOut.frame.px[i][ch] * (255 - a)) / 255);
    }
  }
}
