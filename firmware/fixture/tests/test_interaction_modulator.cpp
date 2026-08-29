#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "interaction_modulator.h"

static FrameBuffer solid(uint8_t count, uint8_t r, uint8_t g, uint8_t b,
                         uint8_t w = 0) {
  FrameBuffer frame = {};
  frame.count = count;
  frameClear(frame);
  for (uint8_t i = 0; i < count; ++i) {
    frame.px[i][0] = r;
    frame.px[i][1] = g;
    frame.px[i][2] = b;
    frame.px[i][3] = w;
  }
  return frame;
}

static uint8_t litPixels(const FrameBuffer &frame) {
  uint8_t lit = 0;
  for (uint8_t i = 0; i < frame.count; ++i)
    if (frame.px[i][0] || frame.px[i][1] || frame.px[i][2] || frame.px[i][3])
      ++lit;
  return lit;
}

int main() {
  LocalInteractionInputs perimeter = {};
  perimeter.fixtureClass = FIXTURE_PERIMETER;
  perimeter.tofValid = true;

  FrameBuffer frame = solid(37, 20, 0, 0);
  FrameBuffer original = frame;
  perimeter.tofDistanceMm = RES_TOF_INTERACTION_MAX_MM + 1;
  assert(!interactionApply(frame, perimeter));
  assert(memcmp(&frame, &original, sizeof(frame)) == 0);

  perimeter.tofDistanceMm = 1500;
  assert(interactionApply(frame, perimeter));
  assert(litPixels(frame) == 37);

  frame = solid(37, 20, 0, 0);
  perimeter.tofDistanceMm = 900;
  assert(interactionApply(frame, perimeter));
  assert(litPixels(frame) == 19); // center + first two outer rings

  frame = solid(37, 20, 0, 0);
  perimeter.tofDistanceMm = 500;
  assert(interactionApply(frame, perimeter));
  assert(litPixels(frame) == 7);

  frame = solid(37, 20, 0, 0);
  perimeter.tofDistanceMm = 250;
  assert(interactionApply(frame, perimeter));
  assert(litPixels(frame) == 1);

  LocalInteractionInputs downlight = {};
  downlight.fixtureClass = FIXTURE_DOWNLIGHT;
  downlight.tofValid = true;
  downlight.tofDistanceMm = 3000;
  frame = solid(1, 0, 0, 0, 25);
  original = frame;
  assert(!interactionApply(frame, downlight));
  assert(memcmp(&frame, &original, sizeof(frame)) == 0);
  downlight.tofPresenceActive = true;
  assert(interactionApply(frame, downlight));
  assert(frame.px[0][3] == 255);
  assert(frame.px[0][0] == 0 && frame.px[0][1] == 0 &&
         frame.px[0][2] == 0);

  // A blackout cannot be awakened by either sensor path.
  frame = solid(37, 0, 0, 0);
  perimeter.tofDistanceMm = 250;
  perimeter.msaValid = true;
  perimeter.msaSwayMg = 500;
  perimeter.msaInteractionEnabled = true;
  assert(!interactionApply(frame, perimeter));
  assert(litPixels(frame) == 0);

  // MSA plumbing is canary-gated. When enabled it boosts the existing color,
  // while the fleet-default disabled posture remains unchanged.
  LocalInteractionInputs uplight = {};
  uplight.fixtureClass = FIXTURE_UPLIGHT;
  uplight.msaValid = true;
  uplight.msaSwayMg = RES_MSA_INTERACTION_TRIGGER_MG;
  frame = solid(1, 20, 10, 0);
  original = frame;
  assert(!interactionApply(frame, uplight));
  assert(memcmp(&frame, &original, sizeof(frame)) == 0);
  uplight.msaInteractionEnabled = true;
  assert(interactionApply(frame, uplight));
  assert(frame.px[0][0] == 255);
  assert(frame.px[0][1] == 127);

  printf("interaction modulator tests passed\n");
  return 0;
}
