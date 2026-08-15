// PROG_GH_CA: Greenberg-Hastings excitable media -- the one real CA for
// milestone 1. Why GH (vs Life/parity/Lenia): 3-state fits 4 bits; traveling
// pulses/spirals read as alive on a sparse irregular outdoor graph; a lost
// neighbor sample delays or locally stops a wave (looks organic) instead of
// turning to noise; async-native (works off most-recent neighbor state, no
// generation lock-step -- `generation` is carried so the sync A/B stays cheap).
//
// States: 0 = quiescent, 1 = excited, 2..(1+refractory) = refractory countdown.
// params[8]: [0] K excitation threshold (default 1 fresh excited neighbor)
//            [1] p_x256 spontaneous excitation per tick (default 2 ~ 0.8%)
//            [2] refractory length in ticks (default 3)
//            [3] tick period in deciseconds (default 10 = 1 s)
//            [4] hue 0-255 on a warm->cool wheel (default class-picked)
#include "program.h"

#include "../hex_geometry.h"

class ProgGhCa : public Program {
public:
  uint8_t id() const override { return PROG_GH_CA; }

  void reset(uint32_t seed, const uint8_t params[8], uint8_t fixtureClass,
             uint16_t pixelCount) override {
    mClass = fixtureClass;
    mPixels = pixelCount;
    mRng = seed ? seed : 0xA5A5A5A5u;
    mK = params && params[0] ? params[0] : 1;
    mPx256 = params ? params[1] : 2;
    if (!params || !params[1]) mPx256 = 2;
    mRefractory = params && params[2] ? params[2] : 3;
    mTickMs = (params && params[3]) ? (uint32_t)params[3] * 100 : 1000;
    mHue = (params && params[4]) ? params[4]
           : (fixtureClass == FIXTURE_PERIMETER ? 30 : 160);
    mState = 0;
    mGeneration = 0;
    mLastStepMs = 0;
    mExcitedAtMs = 0;
  }

  void tick(const ProgramInputs &in, ProgramOutputs &out) override {
    out.sendNow = false;
    if (mLastStepMs == 0) mLastStepMs = in.nowMs;
    uint32_t effTick = mTickMs * (in.tickDivider ? in.tickDivider : 1);
    if (in.nowMs - mLastStepMs >= effTick) {
      mLastStepMs = in.nowMs;
      step(in);
      if (mState == 1) out.sendNow = true; // edge: excitation announces itself
    }
    render(in, out);
    out.txState = mState & 0x0F;
    out.generation = mGeneration;
    out.phaseMs = (uint16_t)((in.nowMs - mLastStepMs) & 0xFFFF);
  }

private:
  uint32_t rnd() { // xorshift32: pure, deterministic per seed
    mRng ^= mRng << 13;
    mRng ^= mRng >> 17;
    mRng ^= mRng << 5;
    return mRng;
  }

  void step(const ProgramInputs &in) {
    mGeneration++;
    if (mState == 0) {
      // Excite on K fresh excited neighbors, or spontaneously ("lightning" --
      // the forest-fire seed that keeps a quiet graph alive).
      uint8_t excited = 0;
      for (uint8_t i = 0; i < in.neighborCount; i++)
        if ((in.neighbors[i].state & 0x0F) == 1) excited++;
      bool spark = (rnd() & 0xFF) < mPx256;
      if (excited >= mK || spark) {
        mState = 1;
        mExcitedAtMs = in.nowMs;
      }
    } else if (mState >= 1 + mRefractory) {
      mState = 0;
    } else {
      mState++;
    }
  }

  static void hueToRgb(uint8_t hue, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b) {
    // Cheap 6-segment wheel, enough for class tinting.
    uint8_t seg = hue / 43, rem = (hue % 43) * 6;
    uint8_t q = (uint8_t)((uint16_t)v * (255 - rem) / 255);
    uint8_t t = (uint8_t)((uint16_t)v * rem / 255);
    switch (seg) {
    case 0: r = v; g = t; b = 0; break;
    case 1: r = q; g = v; b = 0; break;
    case 2: r = 0; g = v; b = t; break;
    case 3: r = 0; g = q; b = v; break;
    case 4: r = t; g = 0; b = v; break;
    default: r = v; g = 0; b = q; break;
    }
  }

  void render(const ProgramInputs &in, ProgramOutputs &out) {
    out.frame.count = (uint8_t)mPixels;
    frameClear(out.frame);
    uint8_t intensity = 0;
    if (mState == 1) {
      // Excited: ~200 ms attack to full, hold for the tick.
      uint32_t dt = in.nowMs - mExcitedAtMs;
      intensity = dt >= 200 ? 255 : (uint8_t)(dt * 255 / 200);
    } else if (mState >= 2) {
      // Refractory: decaying tail across the countdown.
      intensity = (uint8_t)(140 / mState);
    } else {
      intensity = 25; // quiescent: dim base presence
    }
    out.txIntensity = intensity;

    if (mPixels == 1) {
      uint8_t r, g, b;
      hueToRgb(mHue, intensity, r, g, b);
      out.frame.px[0][0] = r;
      out.frame.px[0][1] = g;
      out.frame.px[0][2] = b;
      if (mState == 0) { // quiescent point source leans warm white
        out.frame.px[0][3] = intensity;
        out.frame.px[0][0] = out.frame.px[0][1] = out.frame.px[0][2] = 0;
      }
      return;
    }
    // HEX: the wavefront travels center->rings across the excited tick; the
    // refractory tail dims ring by ring outward.
    const HexGeometry &geo = hexGeometry();
    if (mState == 1) {
      uint32_t dt = in.nowMs - mExcitedAtMs;
      int front = (int)(dt * 4 / (mTickMs ? mTickMs : 1000)); // ring 0..3 sweep
      for (int ring = 0; ring < 4; ring++) {
        if (ring > front) break;
        uint8_t v = (ring == front) ? intensity : intensity / (2 + ring);
        uint8_t r, g, b;
        hueToRgb(mHue, v, r, g, b);
        for (int i = 0; i < geo.ringSize[ring]; i++) {
          uint8_t p = geo.ringMembers[ring][i];
          out.frame.px[p][0] = r;
          out.frame.px[p][1] = g;
          out.frame.px[p][2] = b;
        }
      }
    } else {
      uint8_t v = intensity;
      uint8_t r, g, b;
      hueToRgb(mHue, v, r, g, b);
      uint8_t c = geo.spiralOrder[0];
      out.frame.px[c][0] = r;
      out.frame.px[c][1] = g;
      out.frame.px[c][2] = b;
    }
  }

  uint8_t mClass = 0;
  uint16_t mPixels = 1;
  uint32_t mRng = 1;
  uint8_t mK = 1, mPx256 = 2, mRefractory = 3, mHue = 160;
  uint32_t mTickMs = 1000;
  uint8_t mState = 0;
  uint16_t mGeneration = 0;
  uint32_t mLastStepMs = 0, mExcitedAtMs = 0;
};

Program *newProgGhCa() {
  static ProgGhCa p;
  return &p;
}
