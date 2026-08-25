#pragma once

#include <stddef.h>
#include <stdint.h>

// Pure deterministic Patterns v1 model. It emits final per-fixture RGBW so the
// integration layer can feed the existing NB_DIRECT_FRAME streamer without
// relying on fixture-side show fields that are not currently honored.

enum class PatternMode : uint8_t {
  WASH = 0,
  CHASE = 1,
  WAVE = 2,
  TWINKLE = 3,
  COUNT = 4,
};

enum class PatternPalette : uint8_t {
  EMBER = 0,
  FOREST = 1,
  OCEAN = 2,
  AURORA = 3,
  MOON = 4,
  COUNT = 5,
};

// Cohorts are stable quarters of the short-ID space. They do not depend on
// input order, current census membership, palette, mode, or animation seed.
enum class PatternCohort : uint8_t {
  ALL = 0,
  A = 1,
  B = 2,
  C = 3,
  D = 4,
  COUNT = 5,
};

struct PatternSettings {
  PatternMode mode;
  PatternPalette palette;
  PatternCohort cohort;
  uint8_t classFilter;  // 0=all, 1-4=fixture class
  uint8_t speed;        // 1=slowest, 8=fastest
  uint8_t intensity;    // final client-side RGBW scale, 0-255
  uint32_t seed;
};

struct PatternNode {
  uint8_t id[3];
  uint8_t fixtureClass;
  uint32_t ageMs;
};

struct PatternFrameEntry {
  uint8_t id[3];
  uint8_t r, g, b, w;
};

static constexpr uint32_t PATTERN_DEFAULT_SEED = 0x52545245U;  // "RTRE"

PatternSettings patternDefaultSettings();
PatternSettings patternSanitize(const PatternSettings &settings);

// Return the stable cohort (A-D) for one real short ID. An all-zero ID returns
// ALL because it is not a fixture address.
PatternCohort patternCohortForId(const uint8_t id[3]);

// Build one complete direct-frame wave. Selection is fresh/class/cohort
// filtered, de-duplicated, and sorted by short ID. If outCap is smaller than
// the selection, the lexicographically lowest IDs win, independent of input
// order. elapsedMs is relative to the explicit pattern start.
size_t patternPlanFrame(const PatternNode *nodes, size_t nodeCount,
                        uint32_t freshMs, const PatternSettings &settings,
                        uint32_t elapsedMs, PatternFrameEntry *out,
                        size_t outCap);

// Useful to a UI or owner service for rate descriptions and deterministic
// slot tests. The result is always non-zero after speed sanitization.
uint32_t patternStepMs(uint8_t speed);
