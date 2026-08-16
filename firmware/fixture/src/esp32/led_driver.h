// Class-profiled LED driver: one Adafruit_NeoPixel on GPIO10/A0 (ADR 0029:
// both roles rail-fed, one harness), constructed at runtime after the class
// probe. Owns the fail-safe order of operations:
//   rail ON  = data LOW -> EN_3V3 raised + pad-verified -> begin -> clear/show
//   rail OFF = all-off frame + show -> data LOW -> EN_3V3 cut + RTC-held
// and the guarded 4-step brightness ramp with VBAT sampling (POR-loop lesson).
#pragma once

#include <stdint.h>
#include "../core/fixture_context.h"

// Ramp abort thresholds (ADR 0023 standard tier; the power policy proper owns
// the running thresholds from P3 -- these only guard the turn-on transient).
#define RES_LED_RAMP_STEPS 4
#define RES_LED_RAMP_STEP_MS 800
#define RES_LED_RAMP_PARK_MV 2900
#define RES_LED_RAMP_DIM_MV 2950

void ledProfileForClass(uint8_t fixtureClass); // (re)construct the strip object
uint16_t ledPixelCount();

bool ledRailOn();  // cleared rail-on; false = pad verify failed (stays parked)
void ledRailOff(); // blank -> data low -> rail cut (safe to call anytime)
bool ledRailIsOn();

// Compact, dashboard-oriented truth about the frame actually handed to the
// pixels after brightness cap and gamma. Multi-pixel fixtures report the mean
// color of nonzero pixels plus their count; this describes the visible look
// without pretending all 37 HEX pixels share one value.
struct LedOutputSnapshot {
  uint8_t railOn;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t w;
  uint8_t litPixels;
};
LedOutputSnapshot ledOutputSnapshot();

// Render a frame (applies gamma + the global brightness cap).
void ledRender(const FrameBuffer &f, uint8_t brightnessCap);

// Guarded turn-on: rail on + 4-step ramp of `f` sampling VBAT between steps.
// Returns false (and parks the rail) on a low sample; caps the final level to
// dim on a marginal one.
bool ledRailOnWithRamp(const FrameBuffer &f, uint8_t targetBrightness);

// Bench smoke-render toggle (serial 'L1'/'L0'); consumed by the loop's render
// tick until the P5 program runtime replaces it.
extern bool gSmokeRender;

// Class-appropriate smoke/identify frames.
void ledSmokeFrame(FrameBuffer &f, uint32_t nowMs);
// Rig color-identify: color 1=R 2=G 3=B 4=Y 5=W; blink toggles at 1 Hz.
void ledIdentifyFrame(FrameBuffer &f, uint8_t color, uint8_t blink, uint32_t nowMs);
