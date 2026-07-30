// PROG_BRIDGE_SHOW: consumes bridge NbShowFrame multicast (10-20 Hz) -- the
// DJ / sound-reactive / bench mode. Hue and value are slew-limited so packet
// loss never causes color snaps; staleness fallback lives in the runtime.
#include "program.h"

#include "../hex_geometry.h"

class ProgBridge : public Program {
public:
  uint8_t id() const override { return PROG_BRIDGE_SHOW; }

  void reset(uint32_t, const uint8_t[8], uint8_t fixtureClass, uint16_t pixelCount) override {
    mClass = fixtureClass;
    mPixels = pixelCount;
    mHue = 0;
    mVal = 0;
  }

  void tick(const ProgramInputs &in, ProgramOutputs &out) override {
    const ShowFrameState *f = in.showFrame;
    uint8_t targetHue = f && f->rxMs ? f->hue : mHue;
    uint8_t targetVal = 0;
    if (f && f->rxMs) {
      uint32_t age = in.nowMs - f->rxMs;
      targetVal = (age <= RES_SHOWFRAME_HOLD_MS)
                      ? f->val
                      : (uint8_t)(f->val / 2); // hold+fade band (runtime falls
                                               // back entirely past 3 s)
    }
    slew(mHue, targetHue, 8);
    slew(mVal, targetVal, 12);

    out.frame.count = (uint8_t)mPixels;
    frameClear(out.frame);
    uint16_t phase = f ? f->phase : 0;
    if (mPixels == 1) {
      // Point source: hue + intensity LFO from the phase counter.
      uint8_t r, g, b;
      hueToRgb(mHue, mVal, r, g, b);
      out.frame.px[0][0] = r;
      out.frame.px[0][1] = g;
      out.frame.px[0][2] = b;
    } else {
      // HEX: radial ripple -- ring index vs phase.
      const HexGeometry &geo = hexGeometry();
      for (int ring = 0; ring < 4; ring++) {
        // 50 ms phase ticks; each ring offset a quarter cycle.
        uint8_t ringPhase = (uint8_t)((phase + ring * 4) & 0x0F);
        uint8_t v = (uint8_t)((uint16_t)mVal * (16 - ringPhase) / 16);
        uint8_t r, g, b;
        hueToRgb(mHue, v, r, g, b);
        for (int i = 0; i < geo.ringSize[ring]; i++) {
          uint8_t p = geo.ringMembers[ring][i];
          out.frame.px[p][0] = r;
          out.frame.px[p][1] = g;
          out.frame.px[p][2] = b;
        }
      }
    }
    out.txState = 0;
    out.txIntensity = mVal;
    out.generation = phase;
    out.phaseMs = phase;
  }

private:
  static void slew(uint8_t &cur, uint8_t target, uint8_t maxStep) {
    int d = (int)target - (int)cur;
    if (d > maxStep) d = maxStep;
    if (d < -maxStep) d = -maxStep;
    cur = (uint8_t)((int)cur + d);
  }
  static void hueToRgb(uint8_t hue, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b) {
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

  uint8_t mClass = 0;
  uint16_t mPixels = 1;
  uint8_t mHue = 0, mVal = 0;
};

Program *newProgBridge() {
  static ProgBridge p;
  return &p;
}
