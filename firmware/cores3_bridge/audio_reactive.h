#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>

struct AudioColor {
  uint8_t r, g, b, w;
};

struct AudioEnvelope {
  float rms = 0.0f;
  float noise = 0.0f;
  float ceiling = 512.0f;
  float level = 0.0f;
  uint16_t calibrationFrames = 0;

  bool calibrated() const { return calibrationFrames >= 20; }

  float update(const int16_t *samples, size_t count) {
    if (!samples || !count) return level;

    int64_t sum = 0;
    for (size_t i = 0; i < count; ++i) sum += samples[i];
    float mean = (float)sum / (float)count;

    double sumSquares = 0.0;
    for (size_t i = 0; i < count; ++i) {
      float centered = (float)samples[i] - mean;
      sumSquares += (double)centered * centered;
    }
    rms = sqrtf((float)(sumSquares / (double)count));

    if (!calibrated()) {
      ++calibrationFrames;
      noise += (rms - noise) / (float)calibrationFrames;
      ceiling = noise + 512.0f;
      level = 0.0f;
      return level;
    }

    // Follow a quiet room slowly, but never let a beat drag the floor upward.
    if (rms < noise * 1.5f + 64.0f) noise += (rms - noise) * 0.01f;
    float minimumCeiling = noise + 512.0f;
    ceiling *= 0.985f;
    if (ceiling < minimumCeiling) ceiling = minimumCeiling;
    if (rms > ceiling) ceiling = rms;

    float normalized = (rms - noise) / (ceiling - noise);
    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;
    normalized = sqrtf(normalized); // useful response before clipping

    // Fast attack, slower release: beats remain crisp without display chatter.
    float blend = normalized > level ? 0.65f : 0.15f;
    level += (normalized - level) * blend;
    if (level < 0.002f) level = 0.0f;
    return level;
  }
};

inline AudioColor audioColorForSlot(uint8_t slot, float level) {
  if (level < 0.0f) level = 0.0f;
  if (level > 1.0f) level = 1.0f;
  uint8_t v = (uint8_t)(level * 255.0f + 0.5f);
  switch (slot % 3) {
  case 0: return {v, (uint8_t)(v / 4), 0, 0};
  case 1: return {0, v, (uint8_t)(v / 8), 0};
  default: return {(uint8_t)(v / 8), (uint8_t)(v / 3), v, 0};
  }
}
