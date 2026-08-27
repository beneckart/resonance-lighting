#pragma once

#include <lvgl.h>

#include <stdint.h>

// Bridge-wide sunlight/night display posture. Day mode is deliberately full
// backlight; the stored backlight slider is the operator's night-mode level.
bool uiDayMode();
uint8_t uiDisplayBacklight();
void uiApplyDisplayMode();
void uiSetDayMode(bool dayMode);

// Common colors for apps that intentionally override LVGL's default theme.
lv_color_t uiScreenColor();
lv_color_t uiTextColor();
lv_color_t uiMutedTextColor();
