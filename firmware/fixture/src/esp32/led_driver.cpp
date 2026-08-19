#include "led_driver.h"

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "../core/fixture_context.h"
#include "../core/hex_geometry.h"
#include "board_power.h"
#include "boot_guard_io.h"
#include "boot_park.h"

// Runtime re-profiling via updateType/updateLength (the led_studio hot-swap
// pattern): one static strip object serves every class.
bool gSmokeRender = false;
bool gBenchRailForcedOff = false;

static Adafruit_NeoPixel gStrip(1, RES_LED_DATA_PIN, NEO_RGBW + NEO_KHZ800);
static bool gBegun = false;
static bool gRailOn = false;
static uint16_t gCount = 1;
static bool gIsRgbw = true;
static LedOutputSnapshot gOutput = {};

LedOutputSnapshot ledOutputSnapshot() { return gOutput; }

void ledProfileForClass(uint8_t fixtureClass) {
  // Production 4 W point source decodes RGBW, NOT GRBW (led_sol_bench /raw
  // slot-injection proved it; GRBW silently swaps R/G).
  switch (fixtureClass) {
  case FIXTURE_PERIMETER:
    gCount = HEX_NUMPIXELS;
    gIsRgbw = false;
    gStrip.updateType(NEO_GRB + NEO_KHZ800);
    break;
  default: // downlight / uplight / chandelier(safe default) / unknown
    gCount = 1;
    gIsRgbw = true;
    gStrip.updateType(NEO_RGBW + NEO_KHZ800);
    break;
  }
  gStrip.updateLength(gCount);
  // setPin AFTER begin() detaches the RMT peripheral via the library's own
  // pinMode calls (Adafruit_NeoPixel::setPin does pinMode(INPUT)+pinMode(OUTPUT)
  // when begun) — the same ghost the 08-15 rail fix guards against, one layer
  // deeper. The data pin never changes, so skipping the call when the strip
  // is live is lossless. (Bench: "one red and four dark", 2026-08-15 night.)
  if (!gBegun) gStrip.setPin(RES_LED_DATA_PIN);
  gStrip.setBrightness(255); // brightness composes in float math, not here
}

uint16_t ledPixelCount() { return gCount; }
bool ledRailIsOn() { return gRailOn; }

static void stripBegin() {
  if (gBegun) return;
  gStrip.begin();
  gStrip.setBrightness(255);
  gStrip.clear();
  gStrip.show();
  gBegun = true;
}

bool ledRailOn() {
  // Before the first show(), GPIO owns the pin and boot parking keeps it LOW.
  // After the first show(), Adafruit_NeoPixel's ESP32 backend owns GPIO10 via
  // RMT and leaves its EOT level LOW. Do NOT call pinMode() again: Arduino-ESP32
  // 3.x's peripheral manager would detach RMT, while Adafruit_NeoPixel caches
  // the same rmtPin and would not reinitialize it on the next show().
  if (!gBegun) {
    pinMode(RES_LED_DATA_PIN, OUTPUT);
    digitalWrite(RES_LED_DATA_PIN, LOW);
  }
  // ADR 0051: persist the load-armed marker BEFORE the rail can energize; an
  // unpersistable marker refuses the rail (stage-persist doctrine).
  if (!bootGuardLoadArm()) {
    Serial.println("led: load-arm persist FAILED -> rail stays off");
    return false;
  }
  if (!railEnable3V3(true)) {
    railEnable3V3(false);
    return false;
  }
  delay(20);
  stripBegin();
  // Deterministic rail-on: clear any frame retained across a warm reset before
  // anything can call this healthy.
  gStrip.setBrightness(255);
  gStrip.clear();
  gStrip.show();
  delay(5);
  gRailOn = true;
  gOutput = {};
  gOutput.railOn = 1;
  return true;
}

void ledRailOff() {
  if (gBegun) {
    gStrip.setBrightness(255);
    gStrip.clear();
    gStrip.show(); // latches all-off and leaves RMT EOT LOW before rail cut
  } else {
    // RMT has never claimed the pin, so a plain GPIO park is safe here.
    pinMode(RES_LED_DATA_PIN, OUTPUT);
    digitalWrite(RES_LED_DATA_PIN, LOW);
  }
  railEnable3V3(false);
  gRailOn = false;
  gOutput = {};
}

void ledRender(const FrameBuffer &f, uint8_t brightnessCap) {
  if (!gRailOn) return;
  uint16_t n = min((uint16_t)f.count, gCount);
  uint32_t sumR = 0, sumG = 0, sumB = 0, sumW = 0;
  uint8_t lit = 0;
  for (uint16_t i = 0; i < gCount; i++) {
    if (i >= n) {
      gStrip.setPixelColor(i, 0);
      continue;
    }
    // Supervised commissioning uses direct linear 8-bit levels: 0 is off,
    // 128 is dim, and 255 is bright. Keep only the hard power-policy cap.
    uint8_t r = (uint8_t)(((uint16_t)f.px[i][0] * brightnessCap + 127) / 255);
    uint8_t g = (uint8_t)(((uint16_t)f.px[i][1] * brightnessCap + 127) / 255);
    uint8_t b = (uint8_t)(((uint16_t)f.px[i][2] * brightnessCap + 127) / 255);
    uint8_t w = 0;
    if (gIsRgbw) {
      w = (uint8_t)(((uint16_t)f.px[i][3] * brightnessCap + 127) / 255);
      gStrip.setPixelColor(i, gStrip.Color(r, g, b, w));
    } else {
      gStrip.setPixelColor(i, gStrip.Color(r, g, b));
    }
    if (r || g || b || w) {
      sumR += r;
      sumG += g;
      sumB += b;
      sumW += w;
      ++lit;
    }
  }
  gStrip.show();
  gOutput.railOn = 1;
  gOutput.r = lit ? (uint8_t)(sumR / lit) : 0;
  gOutput.g = lit ? (uint8_t)(sumG / lit) : 0;
  gOutput.b = lit ? (uint8_t)(sumB / lit) : 0;
  gOutput.w = lit ? (uint8_t)(sumW / lit) : 0;
  gOutput.litPixels = lit;
}

bool ledRailOnWithRamp(const FrameBuffer &f, uint8_t targetBrightness) {
  if (!ledRailOn()) {
    Serial.println("led: rail-on pad verify FAILED -> parked");
    return false;
  }
  // Four-step ramp exposes delayed harness/cell sag before full load; a low
  // VBAT sample between steps parks, a dim-region sample caps the target.
  uint8_t target = targetBrightness;
  for (int step = 1; step <= RES_LED_RAMP_STEPS; step++) {
    uint8_t level = (uint8_t)((uint32_t)target * step / RES_LED_RAMP_STEPS);
    ledRender(f, level);
    delay(RES_LED_RAMP_STEP_MS);
    readBatteryNow();
    int mv = (int)(batteryVolts() * 1000.0f + 0.5f);
    if (mv > 500 && mv < RES_LED_RAMP_PARK_MV) {
      Serial.printf("led: ramp abort at %dmV -> rail off\n", mv);
      ledRailOff();
      return false;
    }
    if (mv > 500 && mv < RES_LED_RAMP_DIM_MV && target > 128) {
      Serial.printf("led: ramp sag at %dmV -> capping to dim\n", mv);
      target = 128;
    }
  }
  ledRender(f, target);
  return true;
}

// Class-appropriate smoke pattern: perimeter walks one white pixel along the
// spiral (the "dancing gobo" motion, single-px so the gobo stays crisp);
// point sources breathe warm white.
void ledSmokeFrame(FrameBuffer &f, uint32_t nowMs) {
  f.count = (uint8_t)gCount;
  frameClear(f);
  if (gCount > 1) {
    const HexGeometry &geo = hexGeometry();
    int pos = hexPathIndex((long)(nowMs / 290), HEX_NUMPIXELS, false);
    uint8_t p = geo.spiralOrder[pos];
    f.px[p][0] = f.px[p][1] = f.px[p][2] = 255;
  } else {
    float ph = 0.5f + 0.5f * sinf((float)nowMs / 800.0f);
    f.px[0][3] = (uint8_t)(40 + 180 * ph); // W channel: warm, gobo-friendly
  }
}

void ledIdentifyFrame(FrameBuffer &f, uint8_t color, uint8_t blink,
                      uint8_t value, uint32_t nowMs) {
  f.count = (uint8_t)gCount;
  frameClear(f);
  if (blink && ((nowMs / 500) & 1)) return; // off half-cycle
  uint8_t r = 0, g = 0, b = 0, w = 0;
  if (value == 0) value = 255;
  switch (color) {
  case 1: r = value; break;
  case 2: g = value; break;
  case 3: b = value; break;
  case 4: r = value; g = (uint8_t)((uint16_t)value * 180 / 255); break;
  case 5: w = value; r = g = b = value; break;
  default: return;
  }
  for (uint16_t i = 0; i < gCount; i++) {
    f.px[i][0] = r;
    f.px[i][1] = g;
    f.px[i][2] = b;
    f.px[i][3] = gIsRgbw ? w : 0;
  }
}
