#include <assert.h>
#include <stdio.h>

#include "field_role_policy.h"
#include "frame_budget.h"
#include "hex_geometry.h"

static uint8_t litPixels(const FrameBuffer &frame) {
  uint8_t lit = 0;
  for (uint8_t i = 0; i < frame.count; ++i) {
    if (frame.px[i][0] || frame.px[i][1] || frame.px[i][2] ||
        frame.px[i][3])
      ++lit;
  }
  return lit;
}

int main() {
  FrameBuffer frame = {};
  frame.count = 1;

  assert(fieldNightRoleApply(frame, FIXTURE_UPLIGHT, false));
  assert(frame.count == 1);
  assert(frame.px[0][0] == 255 && frame.px[0][1] == 255 &&
         frame.px[0][2] == 255 && frame.px[0][3] == 0);
  assert(fieldFrameVisible(frame));

  assert(fieldNightRoleApply(frame, FIXTURE_UPLIGHT, true));
  assert(frame.px[0][0] == 255 && frame.px[0][1] == 255 &&
         frame.px[0][2] == 255 && frame.px[0][3] == 0);

  for (uint8_t i = 0; i < FRAME_MAX_PIXELS; ++i)
    for (uint8_t channel = 0; channel < 4; ++channel)
      frame.px[i][channel] = 255;
  assert(fieldNightRoleApply(frame, FIXTURE_PERIMETER, false));
  assert(frame.count == FRAME_MAX_PIXELS);
  assert(litPixels(frame) == 1);
  uint8_t center = hexGeometry().spiralOrder[0];
  assert(frame.px[center][0] == 255 && frame.px[center][1] == 255 &&
         frame.px[center][2] == 255 && frame.px[center][3] == 0);
  FramePowerBudget fullBudget = framePowerBudget(frame, 37, false, 255);
  assert(!fullBudget.currentLimited && fullBudget.scale == 255);
  FramePowerBudget dimBudget = framePowerBudget(frame, 37, false, 128);
  assert(!dimBudget.currentLimited && dimBudget.scale == 128);

  frameClear(frame);
  frame.count = 1;
  assert(fieldNightRoleApply(frame, FIXTURE_DOWNLIGHT, false));
  assert(frame.px[0][0] == 255 && frame.px[0][1] == 255 &&
         frame.px[0][2] == 255 && frame.px[0][3] == 0);

  frameClear(frame);
  assert(fieldNightRoleApply(frame, FIXTURE_UNKNOWN, false));
  assert(frame.count == 1);
  assert(frame.px[0][0] == 255 && frame.px[0][1] == 255 &&
         frame.px[0][2] == 255 && frame.px[0][3] == 0);

  frameClear(frame);
  assert(fieldNightRoleApply(frame, FIXTURE_CHANDELIER, false));
  assert(frame.px[0][0] == 255 && frame.px[0][1] == 255 &&
         frame.px[0][2] == 255 && frame.px[0][3] == 0);

  frame.count = FRAME_MAX_PIXELS;
  frameClear(frame);
  for (uint8_t i = 0; i < FRAME_MAX_PIXELS; ++i) {
    frame.px[i][0] = 11;
    frame.px[i][1] = 22;
    frame.px[i][2] = 33;
    frame.px[i][3] = 44;
  }
  assert(fieldDirectRoleApply(frame, FIXTURE_PERIMETER));
  assert(frame.count == FRAME_MAX_PIXELS);
  center = hexGeometry().spiralOrder[0];
  for (uint8_t i = 0; i < FRAME_MAX_PIXELS; ++i) {
    if (i == center) {
      assert(frame.px[i][0] == 11 && frame.px[i][1] == 22 &&
             frame.px[i][2] == 33 && frame.px[i][3] == 0);
    } else {
      assert(frame.px[i][0] == 0 && frame.px[i][1] == 0 &&
             frame.px[i][2] == 0 && frame.px[i][3] == 0);
    }
  }

  frame.count = 1;
  frameClear(frame);
  frame.px[0][0] = 9;
  frame.px[0][1] = 19;
  frame.px[0][2] = 29;
  frame.px[0][3] = 39;
  assert(fieldDirectRoleApply(frame, FIXTURE_DOWNLIGHT));
  assert(frame.count == 1);
  assert(frame.px[0][0] == 9 && frame.px[0][1] == 19 &&
         frame.px[0][2] == 29 && frame.px[0][3] == 39);

  printf("field role policy tests passed\n");
  return 0;
}
