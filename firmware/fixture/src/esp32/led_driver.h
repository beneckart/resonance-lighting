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

// Render a frame (applies the global brightness cap).
void ledRender(const FrameBuffer &f, uint8_t brightnessCap);

// Guarded turn-on: rail on + 4-step ramp of `f` sampling VBAT between steps.
// Returns false (and parks the rail) on a low sample; caps the final level to
// dim on a marginal one.
bool ledRailOnWithRamp(const FrameBuffer &f, uint8_t targetBrightness);

// Bench LED override. L0 forces the rail off until L1 or reset; L1 clears the
// forced-off state and renders the class smoke pattern. Keeping the off state
// separate from gSmokeRender prevents an autonomous/basic fallback from
// immediately powering the rail again.
extern bool gSmokeRender;
extern bool gBenchRailForcedOff;

// Class-appropriate smoke/identify frames.
void ledSmokeFrame(FrameBuffer &f, uint32_t nowMs);
// Rig color-identify: color 1=R 2=G 3=B 4=Y 5=W; blink toggles at 1 Hz.
void ledIdentifyFrame(FrameBuffer &f, uint8_t color, uint8_t blink, uint32_t nowMs);
