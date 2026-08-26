// PROG_CONTAGION: infection-family choreography, intentionally separate from
// Greenberg-Hastings CA. Color Virus spreads a hue and remains infected for
// the lease; a re-armed local seed introduces a newer strain and recolors the
// infected graph. Epidemic cycles infected -> immune -> susceptible and recurs.
//
// params[8]: [0] model: 0 Color Virus, 1 Epidemic
//            [1] output: 0 lights, 1 knocks, 2 lights + knocks
//            [2] infected ticks (Epidemic, default 3)
//            [3] immune ticks (Epidemic, default 5)
//            [4] tick period in deciseconds (default 10 = 1 s)
//            [5] fixed hue (ignored for an adopted neighbor hue)
//            [6] local ToF seed: 0 off, 1 enabled
//            [7] bit0 seed this fixture now, bit1 random local seed hue
//
// NbChoreoState.state: bits 0..1 status (0 susceptible, 1 infected,
// 2 immune), bits 2..7 hue bucket. For Color Virus, generation is a 16-bit
// serial strain sequence. Newer sequences win; an equal-sequence hue
// tie-break makes simultaneous strains converge instead of ping-ponging.
// Only fresh PROG_CONTAGION neighbors count.
#include "program.h"

#include "../hex_geometry.h"

enum : uint8_t {
  CONTAGION_SUSCEPTIBLE = 0,
  CONTAGION_INFECTED = 1,
  CONTAGION_IMMUNE = 2,
};

class ProgContagion : public Program {
public:
  uint8_t id() const override { return PROG_CONTAGION; }

  void reset(uint32_t seed, const uint8_t params[8], uint8_t fixtureClass,
             uint16_t pixelCount) override {
    mClass = fixtureClass;
    mPixels = pixelCount;
    mRng = seed ? seed : 0xC01A51A5u;
    mEpidemic = params && params[0] == 1;
    mOutput = params && params[1] <= 2 ? params[1] : 0;
    mInfectedTicks = params && params[2] ? params[2] : 3;
    mImmuneTicks = params && params[3] ? params[3] : 5;
    mTickMs = params && params[4] ? (uint32_t)params[4] * 100 : 1000;
    mFixedHue = params ? params[5] : 8;
    mTofSeed = params && params[6] == 1;
    mSeedNow = params && (params[7] & 0x01);
    mRandomLocalHue = params && (params[7] & 0x02);
    mState = CONTAGION_SUSCEPTIBLE;
    mHue = mFixedHue;
    mTicksInState = 0;
    mGeneration = 0;
    mLastStepMs = 0;
    mInfectedAtMs = 0;
    mLocalSeedPending = false;
  }

  void tick(const ProgramInputs &in, ProgramOutputs &out) override {
    out.sendNow = false;
    out.strikeRequested = false;
    out.strikePulseMs = 0;
    out.suppressLight = mOutput == 1;

    bool infectedEdge = false;
    if (mTofSeed && in.tofPresenceRising) {
      if (!mEpidemic && mState == CONTAGION_INFECTED) {
        infectLocal(in, true);
        infectedEdge = true;
      } else {
        mLocalSeedPending = true;
      }
    }

    if (mState == CONTAGION_SUSCEPTIBLE &&
        (mSeedNow || mLocalSeedPending)) {
      infectLocal(in, false);
      mSeedNow = false;
      mLocalSeedPending = false;
      infectedEdge = true;
    }

    if (mLastStepMs == 0) mLastStepMs = in.nowMs;
    uint32_t effectiveTick = mTickMs * (in.tickDivider ? in.tickDivider : 1);
    if (!infectedEdge && in.nowMs - mLastStepMs >= effectiveTick) {
      mLastStepMs = in.nowMs;
      infectedEdge = step(in);
    }

    if (infectedEdge) {
      out.sendNow = true;
      // Production mallets belong to downlights. Perimeter fixtures can seed
      // and relay the infection, but never request a pulse on their unused D7.
      if ((mOutput == 1 || mOutput == 2) && mClass == FIXTURE_DOWNLIGHT) {
        out.strikeRequested = true;
        out.strikePulseMs = 40;
      }
    }

    render(in, out);
    if (mOutput == 1) frameClear(out.frame);
    out.txState = (uint8_t)((mHue & 0xFC) | (mState & 0x03));
    out.generation = mGeneration;
    out.phaseMs = (uint16_t)((in.nowMs - mLastStepMs) & 0xFFFF);
  }

private:
  uint32_t rnd() {
    mRng ^= mRng << 13;
    mRng ^= mRng >> 17;
    mRng ^= mRng << 5;
    return mRng;
  }

  static bool serialNewer(uint16_t candidate, uint16_t current) {
    uint16_t delta = (uint16_t)(candidate - current);
    return delta != 0 && delta < 0x8000;
  }

  static uint8_t hueDistance(uint8_t a, uint8_t b) {
    uint8_t direct = a > b ? (uint8_t)(a - b) : (uint8_t)(b - a);
    return direct <= 128 ? direct : (uint8_t)(256 - direct);
  }

  uint8_t localSeedHue(bool requireChange) {
    if (!mRandomLocalHue) return (uint8_t)(mFixedHue & 0xFC);
    for (uint8_t attempt = 0; attempt < 4; ++attempt) {
      uint8_t candidate = (uint8_t)(rnd() & 0xFC);
      if (!requireChange || hueDistance(candidate, mHue) >= 32)
        return candidate;
    }
    return (uint8_t)((mHue + 84) & 0xFC);
  }

  void infect(uint8_t hue, uint16_t generation, uint32_t nowMs) {
    mState = CONTAGION_INFECTED;
    mHue = (uint8_t)(hue & 0xFC);
    mTicksInState = 0;
    mInfectedAtMs = nowMs;
    mGeneration = generation;
  }

  void infectLocal(const ProgramInputs &in, bool requireColorChange) {
    uint16_t newest = mGeneration;
    for (uint8_t i = 0; i < in.neighborCount; ++i) {
      const NeighborView &neighbor = in.neighbors[i];
      if (neighbor.programId == PROG_CONTAGION &&
          (neighbor.state & 0x03) == CONTAGION_INFECTED &&
          serialNewer(neighbor.generation, newest))
        newest = neighbor.generation;
    }
    infect(localSeedHue(requireColorChange), (uint16_t)(newest + 1), in.nowMs);
  }

  bool virusNeighborWins(const NeighborView &neighbor) const {
    if (serialNewer(neighbor.generation, mGeneration)) return true;
    return neighbor.generation == mGeneration &&
           (neighbor.state & 0xFC) > (mHue & 0xFC);
  }

  bool step(const ProgramInputs &in) {
    if (mState == CONTAGION_SUSCEPTIBLE) {
      if (mLocalSeedPending) {
        infectLocal(in, false);
        mLocalSeedPending = false;
        return true;
      }
      for (uint8_t i = 0; i < in.neighborCount; ++i) {
        const NeighborView &neighbor = in.neighbors[i];
        if (neighbor.programId != PROG_CONTAGION ||
            (neighbor.state & 0x03) != CONTAGION_INFECTED)
          continue;
        uint16_t generation = mEpidemic ? (uint16_t)(mGeneration + 1)
                                        : neighbor.generation;
        infect(neighbor.state & 0xFC, generation, in.nowMs);
        return true;
      }
      if (mEpidemic) ++mGeneration;
      return false;
    }

    if (!mEpidemic) {
      for (uint8_t i = 0; i < in.neighborCount; ++i) {
        const NeighborView &neighbor = in.neighbors[i];
        if (neighbor.programId != PROG_CONTAGION ||
            (neighbor.state & 0x03) != CONTAGION_INFECTED ||
            !virusNeighborWins(neighbor))
          continue;
        infect(neighbor.state & 0xFC, neighbor.generation, in.nowMs);
        return true;
      }
      return false;
    }

    ++mGeneration;
    if (++mTicksInState < (mState == CONTAGION_INFECTED ? mInfectedTicks
                                                        : mImmuneTicks))
      return false;
    mTicksInState = 0;
    if (mState == CONTAGION_INFECTED)
      mState = CONTAGION_IMMUNE;
    else
      mState = CONTAGION_SUSCEPTIBLE;
    return false;
  }

  static void hueToRgb(uint8_t hue, uint8_t value, uint8_t &r, uint8_t &g,
                       uint8_t &b) {
    uint8_t segment = hue / 43;
    uint8_t rem = (uint8_t)((hue % 43) * 6);
    uint8_t q = (uint8_t)((uint16_t)value * (255 - rem) / 255);
    uint8_t t = (uint8_t)((uint16_t)value * rem / 255);
    switch (segment) {
    case 0: r = value; g = t; b = 0; break;
    case 1: r = q; g = value; b = 0; break;
    case 2: r = 0; g = value; b = t; break;
    case 3: r = 0; g = q; b = value; break;
    case 4: r = t; g = 0; b = value; break;
    default: r = value; g = 0; b = q; break;
    }
  }

  void render(const ProgramInputs &in, ProgramOutputs &out) const {
    out.frame.count = (uint8_t)mPixels;
    frameClear(out.frame);
    uint8_t value = 0;
    if (mState == CONTAGION_INFECTED) {
      uint32_t attackMs = in.nowMs - mInfectedAtMs;
      value = attackMs >= 180 ? 255 : (uint8_t)(attackMs * 255 / 180);
    } else if (mState == CONTAGION_IMMUNE) {
      value = 22;
    }
    out.txIntensity = value;
    if (!value || mOutput == 1) return;

    uint8_t r, g, b;
    hueToRgb(mHue, value, r, g, b);
    if (mPixels == 1) {
      if (mState == CONTAGION_IMMUNE) {
        out.frame.px[0][3] = value;
      } else {
        out.frame.px[0][0] = r;
        out.frame.px[0][1] = g;
        out.frame.px[0][2] = b;
      }
      return;
    }

    const HexGeometry &geo = hexGeometry();
    if (mState == CONTAGION_INFECTED) {
      uint32_t attackMs = in.nowMs - mInfectedAtMs;
      uint8_t front = attackMs >= 360 ? 3 : (uint8_t)(attackMs / 90);
      for (uint8_t ring = 0; ring <= front; ++ring) {
        uint8_t ringValue = ring == front ? value : (uint8_t)(value / 2);
        uint8_t rr, gg, bb;
        hueToRgb(mHue, ringValue, rr, gg, bb);
        for (uint8_t i = 0; i < geo.ringSize[ring]; ++i) {
          uint8_t pixel = geo.ringMembers[ring][i];
          out.frame.px[pixel][0] = rr;
          out.frame.px[pixel][1] = gg;
          out.frame.px[pixel][2] = bb;
        }
      }
    } else {
      uint8_t center = geo.spiralOrder[0];
      out.frame.px[center][0] = r;
      out.frame.px[center][1] = g;
      out.frame.px[center][2] = b;
    }
  }

  uint8_t mClass = 0;
  uint16_t mPixels = 1;
  uint32_t mRng = 1;
  bool mEpidemic = false;
  uint8_t mOutput = 0;
  uint8_t mInfectedTicks = 3;
  uint8_t mImmuneTicks = 5;
  uint32_t mTickMs = 1000;
  uint8_t mFixedHue = 8;
  bool mTofSeed = false;
  bool mSeedNow = false;
  bool mRandomLocalHue = false;
  bool mLocalSeedPending = false;
  uint8_t mState = CONTAGION_SUSCEPTIBLE;
  uint8_t mHue = 8;
  uint8_t mTicksInState = 0;
  uint16_t mGeneration = 0;
  uint32_t mLastStepMs = 0;
  uint32_t mInfectedAtMs = 0;
};

Program *newProgContagion() {
  static ProgContagion program;
  return &program;
}
