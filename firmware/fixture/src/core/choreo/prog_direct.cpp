// PROG_DIRECT: consumes bridge NbDirectFrame entries (cambium browser sim,
// ~8 Hz) -- per-fixture RGBW streaming. Channels are slew-limited so packet
// loss never causes color snaps; staleness fallback lives in the runtime.
#include "program.h"

class ProgDirect : public Program {
public:
  uint8_t id() const override { return PROG_DIRECT; }

  void reset(uint32_t, const uint8_t[8], uint8_t fixtureClass, uint16_t pixelCount) override {
    mClass = fixtureClass;
    mPixels = pixelCount;
    mR = mG = mB = mW = 0;
    mSeenRxMs = 0;
  }

  void tick(const ProgramInputs &in, ProgramOutputs &out) override {
    const DirectFrameState *f = in.directFrame;
    uint8_t tr = 0, tg = 0, tb = 0, tw = 0;
    if (f && f->rxMs) {
      uint32_t age = in.nowMs - f->rxMs;
      bool hold = age > RES_SHOWFRAME_HOLD_MS; // hold+half band (runtime falls
                                               // back entirely past 3 s)
      tr = hold ? (uint8_t)(f->r / 2) : f->r;
      tg = hold ? (uint8_t)(f->g / 2) : f->g;
      tb = hold ? (uint8_t)(f->b / 2) : f->b;
      tw = hold ? (uint8_t)(f->w / 2) : f->w;
      if ((f->flags & 0x02) && f->rxMs != mSeenRxMs) {
        // Hard-cut: snap on the frame edge; slew resumes from the new color.
        mR = tr;
        mG = tg;
        mB = tb;
        mW = tw;
      }
      mSeenRxMs = f->rxMs;
    }
    // 32/tick (proposed tunable): full 0->255 traverse in 8 ticks, <=0.8 s at
    // the 10 Hz render rate -- fast enough to track the 8 Hz stream, slow
    // enough that a dropped packet never reads as a flash.
    slew(mR, tr, 32);
    slew(mG, tg, 32);
    slew(mB, tb, 32);
    slew(mW, tw, 32);

    out.frame.count = (uint8_t)mPixels;
    frameClear(out.frame);
    if (mPixels == 1) {
      // Point source: the commanded RGBW, verbatim.
      out.frame.px[0][0] = mR;
      out.frame.px[0][1] = mG;
      out.frame.px[0][2] = mB;
      out.frame.px[0][3] = mW;
    } else {
      // HEX: uniform wash (W ignored by GRB pixels; written for wire truth).
      for (int i = 0; i < (int)mPixels && i < FRAME_MAX_PIXELS; i++) {
        out.frame.px[i][0] = mR;
        out.frame.px[i][1] = mG;
        out.frame.px[i][2] = mB;
        out.frame.px[i][3] = mW;
      }
    }
    uint8_t peak = mR;
    if (mG > peak) peak = mG;
    if (mB > peak) peak = mB;
    if (mW > peak) peak = mW;
    out.txState = 0;
    out.txIntensity = peak;
    out.generation = 0;
    out.phaseMs = (uint16_t)(in.nowMs & 0xFFFF);
  }

private:
  static void slew(uint8_t &cur, uint8_t target, uint8_t maxStep) {
    int d = (int)target - (int)cur;
    if (d > maxStep) d = maxStep;
    if (d < -maxStep) d = -maxStep;
    cur = (uint8_t)((int)cur + d);
  }

  uint8_t mClass = 0;
  uint16_t mPixels = 1;
  uint8_t mR = 0, mG = 0, mB = 0, mW = 0;
  uint32_t mSeenRxMs = 0;
};

Program *newProgDirect() {
  static ProgDirect p;
  return &p;
}
