#include "led_driver.h"

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "../core/fixture_context.h"
#include "../core/gamma.h"
#include "../core/hex_geometry.h"
#include "board_power.h"
#include "boot_park.h"

// Runtime re-profiling via updateType/updateLength (the led_studio hot-swap
// pattern): one static strip object serves every class.
bool gSmokeRender = false;

static Adafruit_NeoPixel gStrip(1, RES_LED_DATA_PIN, NEO_RGBW + NEO_KHZ800);
static bool gBegun = false;
static bool gRailOn = false;
static uint16_t gCount = 1;
static bool gIsRgbw = true;

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
}

void ledRender(const FrameBuffer &f, uint8_t brightnessCap) {
  if (!gRailOn) return;
  float s = (float)brightnessCap / 255.0f;
  uint16_t n = min((uint16_t)f.count, gCount);
  for (uint16_t i = 0; i < gCount; i++) {
    if (i >= n) {
      gStrip.setPixelColor(i, 0);
      continue;
    }
    uint8_t r = resGamma8((uint8_t)(f.px[i][0] * s));
    uint8_t g = resGamma8((uint8_t)(f.px[i][1] * s));
    uint8_t b = resGamma8((uint8_t)(f.px[i][2] * s));
    if (gIsRgbw) {
      uint8_t w = resGamma8((uint8_t)(f.px[i][3] * s));
      gStrip.setPixelColor(i, gStrip.Color(r, g, b, w));
    } else {
      gStrip.setPixelColor(i, gStrip.Color(r, g, b));
    }
  }
  gStrip.show();
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

void ledIdentifyFrame(FrameBuffer &f, uint8_t color, uint8_t blink, uint32_t nowMs) {
  f.count = (uint8_t)gCount;
  frameClear(f);
  if (blink && ((nowMs / 500) & 1)) return; // off half-cycle
  uint8_t r = 0, g = 0, b = 0, w = 0;
  switch (color) {
  case 1: r = 255; break;
  case 2: g = 255; break;
  case 3: b = 255; break;
  case 4: r = 255; g = 180; break; // yellow
  case 5: w = 255; r = g = b = 255; break;
  default: return;
  }
  for (uint16_t i = 0; i < gCount; i++) {
    f.px[i][0] = r;
    f.px[i][1] = g;
    f.px[i][2] = b;
    f.px[i][3] = gIsRgbw ? w : 0;
  }
}
