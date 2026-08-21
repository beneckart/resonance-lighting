#pragma once

#include <lvgl.h>
#include <stdint.h>

// LVGL 9 <-> T-Deck glue: display flush over LovyanGFX, millis tick,
// trackball as encoder indev (a=UP c=DOWN + click), keyboard as keypad indev
// (press-only aux MCU; release synthesized), GT911 as pointer indev.
// ALL lv_* calls stay on the UI task (LVGL is not thread-safe).

bool lvglGlueInit();  // false = buffer alloc/display failure (plan-B fallback)

// The shared focus group the inputs navigate. Screens swap their widgets
// into this group on load.
lv_group_t *lvglGroup();

// Per-screen trackball semantics (Ben 2026-08-19: left/right = one focus
// step, up/down = something spatial). A screen may install hooks; return true
// = consumed, false = fall back to focus prev/next. Cleared automatically by
// uiGoHome(); apps set their own on open.
struct UiNavHooks {
  bool (*onVertical)(int dir);  // +1 = down
  bool (*onEnter)();            // trackball click
};
void lvglSetNavHooks(const UiNavHooks *hooks);  // nullptr = defaults
