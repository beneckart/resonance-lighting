#pragma once

#include <stdint.h>

namespace lgfx {
inline namespace v1 {
class LGFX_Sprite;
class LGFX_Device;
}
}  // namespace lgfx

// Raw LovyanGFX ST7789 bring-up (M0). LVGL rides on top of this driver in M2.
bool halDisplayInit();                  // panel up, backlight ON at stored level
void halDisplaySetBacklight(uint8_t v); // 0..255
lgfx::LGFX_Device *halDisplayDevice();  // raw panel for the LVGL flush path
lgfx::LGFX_Sprite *halCanvas();         // full-screen PSRAM sprite (320x240), or
                                        // nullptr if allocation failed (headless)
void halCanvasPush();                   // pushSprite to the panel
