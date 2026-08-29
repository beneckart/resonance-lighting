#include "frame_budget.h"

static uint16_t livePixelCount(const FrameBuffer &frame,
                               uint16_t physicalPixels) {
  uint16_t n = frame.count;
  if (n > physicalPixels) n = physicalPixels;
  if (n > FRAME_MAX_PIXELS) n = FRAME_MAX_PIXELS;
  return n;
}

FramePowerBudget framePowerBudget(const FrameBuffer &frame,
                                  uint16_t physicalPixels, bool isRgbw,
                                  uint8_t brightnessCap) {
  FramePowerBudget result = {brightnessCap, false};
  if (isRgbw || physicalPixels != FRAME_MAX_PIXELS || brightnessCap == 0)
    return result;

  uint32_t rawSum = 0;
  uint32_t roundedSum = 0;
  uint16_t n = livePixelCount(frame, physicalPixels);
  for (uint16_t i = 0; i < n; ++i) {
    for (uint8_t channel = 0; channel < 3; ++channel) {
      uint8_t value = frame.px[i][channel];
      rawSum += value;
      roundedSum += ((uint32_t)value * brightnessCap + 127u) / 255u;
    }
  }
  if (roundedSum <= RES_HEX_RGB_CHANNEL_BUDGET || rawSum == 0) return result;

  uint32_t allowed = (RES_HEX_RGB_CHANNEL_BUDGET * 255u) / rawSum;
  if (allowed > brightnessCap) allowed = brightnessCap;
  result.scale = (uint8_t)allowed;
  result.currentLimited = true;
  return result;
}

uint8_t framePowerScaleChannel(uint8_t value,
                               const FramePowerBudget &budget) {
  uint16_t product = (uint16_t)value * budget.scale;
  return (uint8_t)((product + (budget.currentLimited ? 0u : 127u)) / 255u);
}
