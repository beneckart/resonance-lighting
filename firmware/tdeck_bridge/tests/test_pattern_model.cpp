#include <cstdio>
#include <cstring>

#include "core/pattern_model.h"

static int checks = 0;
static int fails = 0;

#define CHECK(expr)                                                            \
  do {                                                                         \
    ++checks;                                                                  \
    if (!(expr)) {                                                             \
      std::printf("FAIL line %d: %s\n", __LINE__, #expr);                     \
      ++fails;                                                                 \
    }                                                                          \
  } while (0)

static PatternNode node(uint32_t id, uint8_t fixtureClass = 1,
                        uint32_t ageMs = 0) {
  PatternNode out = {};
  out.id[0] = (uint8_t)(id >> 16);
  out.id[1] = (uint8_t)(id >> 8);
  out.id[2] = (uint8_t)id;
  out.fixtureClass = fixtureClass;
  out.ageMs = ageMs;
  return out;
}

static bool sameFrame(const PatternFrameEntry *a, const PatternFrameEntry *b,
                      size_t n) {
  return std::memcmp(a, b, n * sizeof(*a)) == 0;
}

static bool dark(const PatternFrameEntry &entry) {
  return entry.r == 0 && entry.g == 0 && entry.b == 0 && entry.w == 0;
}

static bool sameColor(const PatternFrameEntry &a,
                      const PatternFrameEntry &b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.w == b.w;
}

int main() {
  PatternSettings defaults = patternDefaultSettings();
  CHECK(defaults.mode == PatternMode::CHASE);
  CHECK(defaults.palette == PatternPalette::EMBER);
  CHECK(defaults.cohort == PatternCohort::ALL);
  CHECK(defaults.classFilter == 0);
  CHECK(defaults.speed == 4);
  CHECK(defaults.intensity == 128);
  CHECK(defaults.seed == PATTERN_DEFAULT_SEED);

  PatternSettings invalid = {
      (PatternMode)99, (PatternPalette)99, (PatternCohort)99,
      99,              0,                  77,
      123,
  };
  PatternSettings clean = patternSanitize(invalid);
  CHECK(clean.mode == PatternMode::CHASE);
  CHECK(clean.palette == PatternPalette::EMBER);
  CHECK(clean.cohort == PatternCohort::ALL);
  CHECK(clean.classFilter == 0);
  CHECK(clean.speed == 1);
  CHECK(clean.intensity == 77);
  CHECK(clean.seed == 123);
  invalid.speed = 255;
  CHECK(patternSanitize(invalid).speed == 8);
  CHECK(patternStepMs(0) == patternStepMs(1));
  CHECK(patternStepMs(255) == patternStepMs(8));
  CHECK(patternStepMs(1) > patternStepMs(8));

  const uint8_t zero[3] = {0, 0, 0};
  CHECK(patternCohortForId(zero) == PatternCohort::ALL);
  for (uint32_t id = 1; id <= 64; ++id) {
    PatternNode a = node(id);
    PatternNode b = node(id);
    PatternCohort ca = patternCohortForId(a.id);
    CHECK(ca >= PatternCohort::A && ca <= PatternCohort::D);
    CHECK(ca == patternCohortForId(b.id));
  }

  PatternNode rows[] = {
      node(0x000003, 2, 10), node(0x000001, 1, 10),
      node(0x000002, 2, 999), node(0x000001, 1, 12),
      node(0x000000, 1, 0), node(0x000004, 1, 1000),
  };
  PatternFrameEntry frame[8] = {};
  size_t n = patternPlanFrame(rows, 6, 1000, defaults, 0, frame, 8);
  CHECK(n == 3);  // duplicate collapsed; age==fresh and zero ID excluded
  CHECK(frame[0].id[2] == 1);
  CHECK(frame[1].id[2] == 2);
  CHECK(frame[2].id[2] == 3);

  PatternSettings classTwo = defaults;
  classTwo.classFilter = 2;
  n = patternPlanFrame(rows, 6, 1000, classTwo, 0, frame, 8);
  CHECK(n == 2);
  CHECK(frame[0].id[2] == 2 && frame[1].id[2] == 3);

  PatternSettings cohort = defaults;
  PatternNode cohortRows[16];
  size_t expected = 0;
  cohort.cohort = PatternCohort::B;
  for (size_t i = 0; i < 16; ++i) {
    cohortRows[i] = node((uint32_t)(i + 1));
    if (patternCohortForId(cohortRows[i].id) == cohort.cohort) ++expected;
  }
  n = patternPlanFrame(cohortRows, 16, 100, cohort, 0, frame, 8);
  CHECK(n == expected);
  for (size_t i = 0; i < n; ++i)
    CHECK(patternCohortForId(frame[i].id) == PatternCohort::B);

  // A bounded plan keeps the same lowest IDs regardless of input order.
  PatternNode forward[] = {node(5), node(1), node(4), node(2), node(3)};
  PatternNode reverse[] = {node(3), node(2), node(4), node(1), node(5)};
  PatternFrameEntry boundedA[3] = {}, boundedB[3] = {};
  CHECK(patternPlanFrame(forward, 5, 100, defaults, 42, boundedA, 3) == 3);
  CHECK(patternPlanFrame(reverse, 5, 100, defaults, 42, boundedB, 3) == 3);
  CHECK(sameFrame(boundedA, boundedB, 3));
  CHECK(boundedA[0].id[2] == 1 && boundedA[1].id[2] == 2 &&
        boundedA[2].id[2] == 3);

  CHECK(patternPlanFrame(nullptr, 1, 100, defaults, 0, frame, 8) == 0);
  CHECK(patternPlanFrame(rows, 6, 0, defaults, 0, frame, 8) == 0);
  CHECK(patternPlanFrame(rows, 6, 100, defaults, 0, nullptr, 8) == 0);
  CHECK(patternPlanFrame(rows, 6, 100, defaults, 0, frame, 0) == 0);

  // Every mode is deterministic and repeats exactly at its defined cycle/slot.
  PatternNode animationRows[] = {node(0x010101), node(0x020202),
                                 node(0x030303), node(0x040404)};
  PatternFrameEntry a[4] = {}, b[4] = {}, c[4] = {};
  for (uint8_t rawMode = 0; rawMode < (uint8_t)PatternMode::COUNT; ++rawMode) {
    PatternSettings s = defaults;
    s.mode = (PatternMode)rawMode;
    size_t an = patternPlanFrame(animationRows, 4, 100, s, 1234, a, 4);
    size_t bn = patternPlanFrame(animationRows, 4, 100, s, 1234, b, 4);
    CHECK(an == 4 && bn == 4);
    CHECK(sameFrame(a, b, 4));
  }

  PatternSettings wash = defaults;
  wash.mode = PatternMode::WASH;
  wash.intensity = 255;
  uint32_t washCycle = patternStepMs(wash.speed) * 4U;
  CHECK(patternPlanFrame(animationRows, 4, 100, wash, 177, a, 4) == 4);
  CHECK(patternPlanFrame(animationRows, 4, 100, wash, 177 + washCycle, b, 4) ==
        4);
  CHECK(sameFrame(a, b, 4));
  CHECK(a[0].r == a[1].r && a[0].g == a[1].g && a[0].b == a[1].b &&
        a[0].w == a[1].w);

  PatternSettings chase = defaults;
  chase.mode = PatternMode::CHASE;
  chase.intensity = 255;
  patternPlanFrame(animationRows, 4, 100, chase, 0, a, 4);
  CHECK(!sameColor(a[0], a[1]));

  PatternSettings wave = defaults;
  wave.mode = PatternMode::WAVE;
  wave.intensity = 255;
  patternPlanFrame(animationRows, 4, 100, wave, 0, a, 4);
  bool differentLevels = false;
  for (size_t i = 1; i < 4; ++i)
    if (a[i].r != a[0].r || a[i].g != a[0].g || a[i].b != a[0].b ||
        a[i].w != a[0].w)
      differentLevels = true;
  CHECK(differentLevels);

  PatternSettings twinkle = defaults;
  twinkle.mode = PatternMode::TWINKLE;
  twinkle.intensity = 255;
  patternPlanFrame(animationRows, 4, 100, twinkle, 0, a, 4);
  patternPlanFrame(animationRows, 4, 100, twinkle,
                   patternStepMs(twinkle.speed), b, 4);
  CHECK(!sameFrame(a, b, 4));
  twinkle.seed++;
  patternPlanFrame(animationRows, 4, 100, twinkle, 0, c, 4);
  CHECK(!sameFrame(a, c, 4));

  // Intensity zero is electrically dark; full intensity emits some light for
  // every palette, including Moon's W-first point-source palette.
  for (uint8_t rawPalette = 0;
       rawPalette < (uint8_t)PatternPalette::COUNT; ++rawPalette) {
    PatternSettings s = defaults;
    s.mode = PatternMode::WASH;
    s.palette = (PatternPalette)rawPalette;
    s.intensity = 0;
    patternPlanFrame(animationRows, 4, 100, s, 0, a, 4);
    CHECK(dark(a[0]));
    s.intensity = 255;
    patternPlanFrame(animationRows, 4, 100, s, 0, b, 4);
    CHECK(!dark(b[0]));
  }

  std::printf("pattern model: %d checks, %d failures\n", checks, fails);
  return fails ? 1 : 0;
}
