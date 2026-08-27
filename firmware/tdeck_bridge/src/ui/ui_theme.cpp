#include "ui_theme.h"

#include "../hal/hal_display.h"
#include "../store/store.h"

bool uiDayMode() { return settings().dayMode; }

uint8_t uiDisplayBacklight() {
  return uiDayMode() ? 255 : settings().backlight;
}

lv_color_t uiScreenColor() {
  return uiDayMode() ? lv_color_hex(0xF7F9FB) : lv_color_hex(0x101418);
}

lv_color_t uiTextColor() {
  return uiDayMode() ? lv_color_hex(0x111827) : lv_color_hex(0xF2F5F7);
}

lv_color_t uiMutedTextColor() {
  return uiDayMode() ? lv_color_hex(0x4B5563) : lv_color_hex(0xB8C0C8);
}

void uiApplyDisplayMode() {
  lv_display_t *display = lv_display_get_default();
  if (display) {
    // Reinitializing the default theme updates its styles in place and reports
    // a global style change, so already-open standard widgets repaint too.
    lv_theme_t *theme = lv_theme_default_init(
        display, lv_color_hex(0x1565C0), lv_color_hex(0xC62828),
        !uiDayMode(), &lv_font_montserrat_18);
    lv_display_set_theme(display, theme);
  }
  halDisplaySetBacklight(uiDisplayBacklight());
}

void uiSetDayMode(bool dayMode) {
  settings().dayMode = dayMode;
  storeSave();
  uiApplyDisplayMode();
}
