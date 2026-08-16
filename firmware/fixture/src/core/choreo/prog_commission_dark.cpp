// PROG_COMMISSION_DARK: electrically quiet fallback for pre-build/build-week
// operation. Fixtures listen and report telemetry but never invent an artistic
// state when the bridge lease expires.
#include "program.h"

class ProgCommissionDark : public Program {
public:
  uint8_t id() const override { return PROG_COMMISSION_DARK; }
  void reset(uint32_t, const uint8_t[8], uint8_t, uint16_t pixelCount) override {
    mPixels = pixelCount;
  }
  void tick(const ProgramInputs &in, ProgramOutputs &out) override {
    out = ProgramOutputs{};
    out.frame.count = (uint8_t)mPixels;
    frameClear(out.frame);
    out.phaseMs = (uint16_t)(in.nowMs & 0xFFFF);
  }

private:
  uint16_t mPixels = 1;
};

Program *newProgCommissionDark() {
  static ProgCommissionDark p;
  return &p;
}
