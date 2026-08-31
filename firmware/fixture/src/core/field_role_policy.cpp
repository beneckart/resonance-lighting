#include "field_role_policy.h"

#include "hex_geometry.h"

bool fieldFrameVisible(const FrameBuffer &frame) {
  uint8_t count =
      frame.count > FRAME_MAX_PIXELS ? FRAME_MAX_PIXELS : frame.count;
  for (uint8_t i = 0; i < count; ++i) {
    if (frame.px[i][0] || frame.px[i][1] || frame.px[i][2] ||
        frame.px[i][3])
      return true;
  }
  return false;
}

static void setRgbWhite(FrameBuffer &frame, uint8_t pixel, uint8_t value) {
  frame.px[pixel][0] = value;
  frame.px[pixel][1] = value;
  frame.px[pixel][2] = value;
  frame.px[pixel][3] = 0;
}

bool fieldNightRoleApply(FrameBuffer &frame, uint8_t fixtureClass, bool) {
  if (fixtureClass == FIXTURE_PERIMETER) {
    frame.count = FRAME_MAX_PIXELS;
    frameClear(frame);
    // spiralOrder[0] is the physical center pixel. One RGB-white pixel sums to
    // 765 channel units, exactly the existing dense-frame current budget.
    setRgbWhite(frame, hexGeometry().spiralOrder[0], 255);
    return true;
  }

  // Downlight/canopy, uplight, chandelier, and unknown all use the safe
  // one-point-source profile until physical identity says otherwise.
  frame.count = 1;
  frameClear(frame);
  setRgbWhite(frame, 0, 255);
  return true;
}
