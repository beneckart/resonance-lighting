#pragma once

#include <stdint.h>

// Keyboard (I2C 0x55 aux MCU), trackball (4 pulse GPIOs + click), GT911 touch.
// All polled cooperatively from loop(); no tasks in M0.

void halInputInit();

// Keyboard: returns 0 when no key. The aux MCU reports ASCII on press only
// (no key-up, no modifiers); alt-layers are resolved in its own firmware.
char halKeyboardRead();
bool halKeyboardPresent();

// Trackball: cumulative pulse counts per line since boot, plus click state.
struct TrackballCounts {
  uint32_t a, b, c, d;  // TDECK_PIN_TB_A/B/C/D edges; direction map is an M0
                        // hardware calibration, recorded in README
  bool click;           // center press (active low), debounced
};
TrackballCounts halTrackballRead();

// GT911 touch: true if a point is currently pressed; fills x/y (rotated frame).
bool halTouchRead(int16_t *x, int16_t *y);
bool halTouchPresent();
