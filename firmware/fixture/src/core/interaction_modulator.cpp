#include "interaction_modulator.h"

#include "hex_geometry.h"

static bool frameVisible(const FrameBuffer &frame) {
  uint8_t n = frame.count > FRAME_MAX_PIXELS ? FRAME_MAX_PIXELS : frame.count;
  for (uint8_t i = 0; i < n; ++i)
    if (frame.px[i][0] || frame.px[i][1] || frame.px[i][2] || frame.px[i][3])
      return true;
  return false;
}

static void hueToRgb(uint8_t hue, uint8_t &r, uint8_t &g, uint8_t &b) {
  uint8_t seg = hue / 43;
  uint8_t rem = (hue % 43) * 6;
  uint8_t q = 255 - rem;
  switch (seg) {
  case 0: r = 255; g = rem; b = 0; break;
  case 1: r = q; g = 255; b = 0; break;
  case 2: r = 0; g = 255; b = rem; break;
  case 3: r = 0; g = q; b = 255; break;
  case 4: r = rem; g = 0; b = 255; break;
  default: r = 255; g = 0; b = q; break;
  }
}

static uint8_t distanceHue(uint16_t distanceMm) {
  uint32_t offset = distanceMm - RES_TOF_INTERACTION_NEAR_MM;
  uint32_t span = RES_TOF_INTERACTION_MAX_MM - RES_TOF_INTERACTION_NEAR_MM;
  return (uint8_t)(offset * 224u / span);
}

static bool applyTof(FrameBuffer &frame,
                     const LocalInteractionInputs &inputs) {
  if (!inputs.tofValid ||
      inputs.tofDistanceMm < RES_TOF_INTERACTION_NEAR_MM ||
      inputs.tofDistanceMm > RES_TOF_INTERACTION_MAX_MM)
    return false;

  uint8_t r, g, b;
  hueToRgb(distanceHue(inputs.tofDistanceMm), r, g, b);

  if (inputs.fixtureClass == FIXTURE_PERIMETER &&
      frame.count == FRAME_MAX_PIXELS) {
    uint8_t maxRing = 3;
    if (inputs.tofDistanceMm <= RES_TOF_INTERACTION_CLOSE_MM)
      maxRing = 0;
    else if (inputs.tofDistanceMm <= RES_TOF_INTERACTION_MID_MM)
      maxRing = 1;
    else if (inputs.tofDistanceMm <= RES_TOF_INTERACTION_FAR_RING_MM)
      maxRing = 2;

    frameClear(frame);
    const HexGeometry &geo = hexGeometry();
    for (uint8_t ring = 0; ring <= maxRing; ++ring) {
      for (uint8_t i = 0; i < geo.ringSize[ring]; ++i) {
        uint8_t pixel = geo.ringMembers[ring][i];
        frame.px[pixel][0] = r;
        frame.px[pixel][1] = g;
        frame.px[pixel][2] = b;
      }
    }
    return true;
  }

  if (inputs.fixtureClass == FIXTURE_DOWNLIGHT && frame.count == 1) {
    frameClear(frame);
    if (inputs.tofDistanceMm <= RES_TOF_INTERACTION_CLOSE_MM) {
      // At intimate range, pop the dedicated white point source through its
      // physical gobo instead of trying to express closeness as dimness.
      frame.px[0][3] = 255;
    } else {
      frame.px[0][0] = r;
      frame.px[0][1] = g;
      frame.px[0][2] = b;
    }
    return true;
  }
  return false;
}

static bool applyMsa(FrameBuffer &frame,
                     const LocalInteractionInputs &inputs) {
  if (!inputs.msaInteractionEnabled || !inputs.msaValid ||
      inputs.msaSwayMg < RES_MSA_INTERACTION_TRIGGER_MG)
    return false;

  uint8_t peak = 0;
  uint8_t n = frame.count > FRAME_MAX_PIXELS ? FRAME_MAX_PIXELS : frame.count;
  for (uint8_t i = 0; i < n; ++i)
    for (uint8_t channel = 0; channel < 4; ++channel)
      if (frame.px[i][channel] > peak) peak = frame.px[i][channel];
  if (!peak || peak == 255) return false;

  // A deliberate shake/climb accent raises the existing color to full scale;
  // it does not invent a new hue or awaken a dark frame.
  for (uint8_t i = 0; i < n; ++i)
    for (uint8_t channel = 0; channel < 4; ++channel)
      frame.px[i][channel] =
          (uint8_t)((uint16_t)frame.px[i][channel] * 255u / peak);
  return true;
}

bool interactionApply(FrameBuffer &frame, const LocalInteractionInputs &inputs) {
  if (!frameVisible(frame)) return false;
  if (applyTof(frame, inputs)) return true;
  return applyMsa(frame, inputs);
}
