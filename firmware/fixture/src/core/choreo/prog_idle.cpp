// PROG_IDLE: class-tinted slow breathe -- the "nothing selected / fallback"
// look. Deliberately never blank: a fixture that lost its bridge or program
// still reads as alive.
#include "program.h"

#include <math.h>
#include "../hex_geometry.h"

class ProgIdle : public Program {
public:
  uint8_t id() const override { return PROG_IDLE; }
  void reset(uint32_t, const uint8_t[8], uint8_t fixtureClass, uint16_t pixelCount) override {
    mClass = fixtureClass;
    mPixels = pixelCount;
  }
  void tick(const ProgramInputs &in, ProgramOutputs &out) override {
    out.frame.count = (uint8_t)mPixels;
    frameClear(out.frame);
    float ph = 0.5f + 0.5f * sinf((float)in.nowMs / 2500.0f); // ~4 s breathe
    uint8_t v = (uint8_t)(30 + 90 * ph);
    if (mPixels == 1) {
      // Point sources: warm W with a faint class tint on the color die.
      out.frame.px[0][3] = v;
      if (mClass == FIXTURE_UPLIGHT) out.frame.px[0][2] = v / 4; // cool cast up the shaft
      else out.frame.px[0][0] = v / 5; // warm cast
    } else {
      // HEX: center + inner ring breathe (single-digit pixel count keeps the
      // gobo crisp and the load in the preferred-look regime).
      const HexGeometry &g = hexGeometry();
      uint8_t c = g.spiralOrder[0];
      out.frame.px[c][0] = v;
      out.frame.px[c][1] = (uint8_t)(v * 2 / 3);
      for (int i = 0; i < g.ringSize[1]; i++) {
        uint8_t p = g.ringMembers[1][i];
        out.frame.px[p][0] = v / 3;
        out.frame.px[p][1] = v / 5;
      }
    }
    out.txState = 0;
    out.txIntensity = v;
    out.generation = 0;
    out.phaseMs = (uint16_t)(in.nowMs & 0xFFFF);
  }

private:
  uint8_t mClass = 0;
  uint16_t mPixels = 1;
};

Program *newProgIdle() {
  static ProgIdle p;
  return &p;
}
