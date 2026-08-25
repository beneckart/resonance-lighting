#include "pattern_model.h"

#include <string.h>

namespace {

struct Rgbw {
  uint8_t r, g, b, w;
};

static constexpr Rgbw kPalettes[(size_t)PatternPalette::COUNT][4] = {
    // Ember: flame edge, amber, warm white, deep red.
    {{255, 12, 0, 0}, {255, 92, 0, 18}, {220, 60, 8, 96}, {112, 0, 0, 0}},
    // Forest: leaf green through cool moonlit foliage.
    {{0, 72, 8, 0}, {0, 210, 48, 0}, {18, 120, 100, 12}, {80, 180, 36, 0}},
    // Ocean: deep blue through turquoise and pale surf.
    {{0, 8, 92, 0}, {0, 88, 230, 0}, {0, 210, 180, 0}, {28, 90, 180, 48}},
    // Aurora: saturated green, cyan, violet, and magenta.
    {{0, 235, 72, 0}, {0, 148, 255, 0}, {118, 24, 255, 0}, {235, 0, 132, 0}},
    // Moon: point-source W remains useful while HEX safely ignores it.
    {{0, 0, 18, 72}, {12, 28, 62, 132}, {42, 64, 96, 190}, {0, 8, 28, 108}},
};

static bool realId(const uint8_t id[3]) {
  return id && (id[0] != 0 || id[1] != 0 || id[2] != 0);
}

static uint8_t scale8(uint8_t value, uint8_t scale) {
  return (uint8_t)(((uint16_t)value * scale + 127U) / 255U);
}

static uint8_t lerp8(uint8_t a, uint8_t b, uint8_t frac) {
  uint16_t left = (uint16_t)a * (uint16_t)(255U - frac);
  uint16_t right = (uint16_t)b * frac;
  return (uint8_t)((left + right + 127U) / 255U);
}

static uint32_t mix32(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7FEB352DU;
  x ^= x >> 15;
  x *= 0x846CA68BU;
  x ^= x >> 16;
  return x;
}

static uint32_t idWord(const uint8_t id[3]) {
  return ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

static uint16_t cyclePhase(uint32_t elapsedMs, uint32_t cycleMs) {
  if (cycleMs == 0) return 0;
  return (uint16_t)(((uint64_t)(elapsedMs % cycleMs) * 65536ULL) / cycleMs);
}

static Rgbw paletteAt(PatternPalette palette, uint16_t phase) {
  size_t p = (size_t)palette;
  uint32_t scaled = (uint32_t)phase * 4U;
  uint8_t index = (uint8_t)(scaled >> 16);
  uint8_t frac = (uint8_t)((scaled & 0xFFFFU) >> 8);
  const Rgbw &a = kPalettes[p][index & 3U];
  const Rgbw &b = kPalettes[p][(index + 1U) & 3U];
  return {lerp8(a.r, b.r, frac), lerp8(a.g, b.g, frac),
          lerp8(a.b, b.b, frac), lerp8(a.w, b.w, frac)};
}

static void applyScale(PatternFrameEntry &entry, const Rgbw &color,
                       uint8_t scale) {
  entry.r = scale8(color.r, scale);
  entry.g = scale8(color.g, scale);
  entry.b = scale8(color.b, scale);
  entry.w = scale8(color.w, scale);
}

static bool idEqual(const uint8_t a[3], const uint8_t b[3]) {
  return memcmp(a, b, 3) == 0;
}

static bool selected(const PatternNode &node, uint32_t freshMs,
                     const PatternSettings &settings) {
  if (!realId(node.id) || node.ageMs >= freshMs) return false;
  if (settings.classFilter != 0 &&
      node.fixtureClass != settings.classFilter)
    return false;
  if (settings.cohort != PatternCohort::ALL &&
      patternCohortForId(node.id) != settings.cohort)
    return false;
  return true;
}

// Insert a unique ID into a bounded sorted result. Keeping the lowest IDs when
// full makes truncation deterministic even if census input order changes.
static void insertId(const uint8_t id[3], PatternFrameEntry *out, size_t &count,
                     size_t outCap) {
  size_t pos = 0;
  while (pos < count && memcmp(out[pos].id, id, 3) < 0) ++pos;
  if (pos < count && idEqual(out[pos].id, id)) return;
  if (count == outCap && pos == count) return;

  size_t newCount = count < outCap ? count + 1 : count;
  size_t last = newCount - 1;
  while (last > pos) {
    out[last] = out[last - 1];
    --last;
  }
  memcpy(out[pos].id, id, 3);
  out[pos].r = out[pos].g = out[pos].b = out[pos].w = 0;
  count = newCount;
}

}  // namespace

PatternSettings patternDefaultSettings() {
  return {PatternMode::CHASE, PatternPalette::EMBER, PatternCohort::ALL,
          0, 4, 128, PATTERN_DEFAULT_SEED};
}

PatternSettings patternSanitize(const PatternSettings &settings) {
  PatternSettings out = settings;
  if ((uint8_t)out.mode >= (uint8_t)PatternMode::COUNT)
    out.mode = PatternMode::CHASE;
  if ((uint8_t)out.palette >= (uint8_t)PatternPalette::COUNT)
    out.palette = PatternPalette::EMBER;
  if ((uint8_t)out.cohort >= (uint8_t)PatternCohort::COUNT)
    out.cohort = PatternCohort::ALL;
  if (out.classFilter > 4) out.classFilter = 0;
  if (out.speed < 1) out.speed = 1;
  if (out.speed > 8) out.speed = 8;
  return out;
}

PatternCohort patternCohortForId(const uint8_t id[3]) {
  if (!realId(id)) return PatternCohort::ALL;
  uint32_t h = mix32(idWord(id) ^ 0xC04F04A1U);
  return (PatternCohort)(1U + (h & 3U));
}

uint32_t patternStepMs(uint8_t speed) {
  static constexpr uint16_t kStepMs[8] = {2000, 1400, 1000, 700,
                                           500,  350,  250,  175};
  if (speed < 1) speed = 1;
  if (speed > 8) speed = 8;
  return kStepMs[speed - 1];
}

size_t patternPlanFrame(const PatternNode *nodes, size_t nodeCount,
                        uint32_t freshMs, const PatternSettings &rawSettings,
                        uint32_t elapsedMs, PatternFrameEntry *out,
                        size_t outCap) {
  if (!nodes || !out || outCap == 0 || freshMs == 0) return 0;
  PatternSettings settings = patternSanitize(rawSettings);

  size_t count = 0;
  for (size_t i = 0; i < nodeCount; ++i) {
    if (selected(nodes[i], freshMs, settings))
      insertId(nodes[i].id, out, count, outCap);
  }
  if (count == 0) return 0;

  uint32_t stepMs = patternStepMs(settings.speed);
  uint32_t cycleMs = stepMs * 4U;
  uint16_t globalPhase = cyclePhase(elapsedMs, cycleMs);

  for (size_t i = 0; i < count; ++i) {
    uint16_t phase = globalPhase;
    uint8_t level = 255;

    switch (settings.mode) {
      case PatternMode::WASH:
        break;

      case PatternMode::CHASE: {
        uint16_t offset = (uint16_t)(((uint64_t)i * 65536ULL) / count);
        phase = (uint16_t)(globalPhase + offset);
        break;
      }

      case PatternMode::WAVE: {
        uint16_t offset = (uint16_t)(((uint64_t)i * 65536ULL) / count);
        uint16_t wave = (uint16_t)(globalPhase + offset);
        uint8_t ramp = (uint8_t)(wave >> 8);
        uint8_t triangle = ramp < 128 ? (uint8_t)(ramp * 2U)
                                      : (uint8_t)((255U - ramp) * 2U);
        level = (uint8_t)(40U + ((uint16_t)triangle * 215U) / 255U);
        break;
      }

      case PatternMode::TWINKLE: {
        uint32_t slot = elapsedMs / stepMs;
        uint32_t h = mix32(idWord(out[i].id) ^ settings.seed ^
                           (slot * 0x9E3779B9U));
        phase = (uint16_t)(h >> 16);
        level = (uint8_t)(32U + ((h >> 8) & 0xDFU));
        break;
      }

      default:
        break;
    }

    Rgbw color = paletteAt(settings.palette, phase);
    uint8_t finalScale = scale8(settings.intensity, level);
    applyScale(out[i], color, finalScale);
  }
  return count;
}
