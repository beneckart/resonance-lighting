#include <assert.h>
#include <stdio.h>

#include "frame_budget.h"

static FrameBuffer blank(uint8_t count) {
  FrameBuffer frame = {};
  frame.count = count;
  frameClear(frame);
  return frame;
}

static uint32_t scaledRgbSum(const FrameBuffer &frame,
                             const FramePowerBudget &budget) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < frame.count; ++i)
    for (uint8_t channel = 0; channel < 3; ++channel)
      sum += framePowerScaleChannel(frame.px[i][channel], budget);
  return sum;
}

int main() {
  FrameBuffer sparse = blank(37);
  sparse.px[18][0] = sparse.px[18][1] = sparse.px[18][2] = 255;
  FramePowerBudget budget = framePowerBudget(sparse, 37, false, 255);
  assert(!budget.currentLimited);
  assert(budget.scale == 255);
  assert(scaledRgbSum(sparse, budget) == RES_HEX_RGB_CHANNEL_BUDGET);

  FrameBuffer dense = blank(37);
  for (uint8_t i = 0; i < dense.count; ++i)
    dense.px[i][0] = dense.px[i][1] = dense.px[i][2] = 255;
  budget = framePowerBudget(dense, 37, false, 255);
  assert(budget.currentLimited);
  assert(budget.scale < 255);
  assert(scaledRgbSum(dense, budget) <= RES_HEX_RGB_CHANNEL_BUDGET);

  FrameBuffer threeColors = blank(37);
  threeColors.px[0][0] = 255;
  threeColors.px[1][1] = 255;
  threeColors.px[2][2] = 255;
  budget = framePowerBudget(threeColors, 37, false, 255);
  assert(!budget.currentLimited);
  assert(scaledRgbSum(threeColors, budget) == RES_HEX_RGB_CHANNEL_BUDGET);

  // The battery policy still composes first when its cap is more restrictive.
  budget = framePowerBudget(sparse, 37, false, 128);
  assert(!budget.currentLimited);
  assert(budget.scale == 128);
  assert(framePowerScaleChannel(255, budget) == 128);

  // Point fixtures are unchanged, including their dedicated white channel.
  FrameBuffer point = blank(1);
  point.px[0][3] = 255;
  budget = framePowerBudget(point, 1, true, 200);
  assert(!budget.currentLimited);
  assert(budget.scale == 200);
  assert(framePowerScaleChannel(point.px[0][3], budget) == 200);

  // Quantization is included in the decision: a dense low-level frame stays
  // under budget even when rounded 8-bit scaling would have exceeded it.
  FrameBuffer low = blank(37);
  for (uint8_t i = 0; i < low.count; ++i)
    low.px[i][0] = low.px[i][1] = low.px[i][2] = 13;
  budget = framePowerBudget(low, 37, false, 128);
  assert(budget.currentLimited);
  assert(budget.scale == 128);
  assert(scaledRgbSum(low, budget) <= RES_HEX_RGB_CHANNEL_BUDGET);

  printf("frame budget tests passed\n");
  return 0;
}
