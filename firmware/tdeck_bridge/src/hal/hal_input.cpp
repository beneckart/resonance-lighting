#include "hal_input.h"

#include <Arduino.h>
#include <Wire.h>

#include "pins_tdeck.h"

static bool gKbdPresent = false;
static bool gTouchPresent = false;
static uint8_t gGt911Addr = 0;

static volatile uint32_t gTbA = 0, gTbB = 0, gTbC = 0, gTbD = 0;
static void IRAM_ATTR isrTbA() { ++gTbA; }
static void IRAM_ATTR isrTbB() { ++gTbB; }
static void IRAM_ATTR isrTbC() { ++gTbC; }
static void IRAM_ATTR isrTbD() { ++gTbD; }

static bool i2cAck(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void halInputInit() {
  Wire.begin(TDECK_PIN_I2C_SDA, TDECK_PIN_I2C_SCL, 400000);

  gKbdPresent = i2cAck(TDECK_I2C_ADDR_KEYBOARD);
  if (i2cAck(TDECK_I2C_ADDR_GT911_A)) gGt911Addr = TDECK_I2C_ADDR_GT911_A;
  else if (i2cAck(TDECK_I2C_ADDR_GT911_B)) gGt911Addr = TDECK_I2C_ADDR_GT911_B;
  gTouchPresent = gGt911Addr != 0;

  pinMode(TDECK_PIN_TB_A, INPUT_PULLUP);
  pinMode(TDECK_PIN_TB_B, INPUT_PULLUP);
  pinMode(TDECK_PIN_TB_C, INPUT_PULLUP);
  pinMode(TDECK_PIN_TB_D, INPUT_PULLUP);
  pinMode(TDECK_PIN_TB_CLICK, INPUT_PULLUP);
  attachInterrupt(TDECK_PIN_TB_A, isrTbA, FALLING);
  attachInterrupt(TDECK_PIN_TB_B, isrTbB, FALLING);
  attachInterrupt(TDECK_PIN_TB_C, isrTbC, FALLING);
  attachInterrupt(TDECK_PIN_TB_D, isrTbD, FALLING);
}

bool halKeyboardPresent() { return gKbdPresent; }

char halKeyboardRead() {
  if (!gKbdPresent) return 0;
  if (Wire.requestFrom((int)TDECK_I2C_ADDR_KEYBOARD, 1) != 1) return 0;
  int v = Wire.read();
  return (v > 0 && v < 0x80) ? (char)v : 0;
}

TrackballCounts halTrackballRead() {
  TrackballCounts t;
  t.a = gTbA;
  t.b = gTbB;
  t.c = gTbC;
  t.d = gTbD;
  static uint32_t lastLowMs = 0;
  bool raw = digitalRead(TDECK_PIN_TB_CLICK) == LOW;
  uint32_t now = millis();
  if (raw) lastLowMs = now;
  t.click = raw || (now - lastLowMs < 30);  // 30 ms release debounce
  return t;
}

bool halTouchPresent() { return gTouchPresent; }

// Minimal GT911 point read over Wire (16-bit registers). Using the Arduino
// Wire driver for BOTH the keyboard and the touch controller keeps one owner
// on the shared bus — LovyanGFX's own touch driver would be a second I2C
// stack contending with Wire.
static bool gtRead(uint16_t reg, uint8_t *buf, size_t n) {
  Wire.beginTransmission(gGt911Addr);
  Wire.write((uint8_t)(reg >> 8));
  Wire.write((uint8_t)(reg & 0xFF));
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)gGt911Addr, (int)n) != (int)n) return false;
  for (size_t i = 0; i < n; ++i) buf[i] = Wire.read();
  return true;
}

static void gtWrite(uint16_t reg, uint8_t v) {
  Wire.beginTransmission(gGt911Addr);
  Wire.write((uint8_t)(reg >> 8));
  Wire.write((uint8_t)(reg & 0xFF));
  Wire.write(v);
  Wire.endTransmission();
}

bool halTouchRead(int16_t *x, int16_t *y) {
  if (!gTouchPresent) return false;
  uint8_t status = 0;
  if (!gtRead(0x814E, &status, 1)) return false;
  if (!(status & 0x80)) return false;  // no fresh coordinate frame
  uint8_t nPoints = status & 0x0F;
  bool got = false;
  if (nPoints >= 1) {
    uint8_t p[4];
    if (gtRead(0x8150, p, 4)) {
      // Native portrait 240x320 -> rotation-1 landscape 320x240.
      int16_t nx = (int16_t)(p[0] | (p[1] << 8));
      int16_t ny = (int16_t)(p[2] | (p[3] << 8));
      *x = ny;
      *y = (int16_t)(240 - 1 - nx);
      got = true;
    }
  }
  gtWrite(0x814E, 0);  // clear buffer-ready flag
  return got;
}
