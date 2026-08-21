/* Bridge OS LVGL configuration (LVGL 9.5.x, found via -DLV_CONF_INCLUDE_SIMPLE
 * + -I<sketch dir> in build.sh). Overrides only — every option not named here
 * takes the lv_conf_internal.h default. */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16

/* 256 KB LVGL heap, placed in PSRAM (internal SRAM is reserved for the DMA
 * draw buffers, TLS, and task stacks — see the plan's memory budget). */
#define LV_MEM_SIZE (256 * 1024U)
#define LV_MEM_ADR 0
#define LV_MEM_POOL_INCLUDE <esp_heap_caps.h>
#define LV_MEM_POOL_ALLOC(size) heap_caps_malloc(size, MALLOC_CAP_SPIRAM)

/* Sun rule (M0 field verdict): default UI text ~= legacy "size 2". Smaller
 * fonts stay available for dense diagnostic tables only. */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_DEFAULT &lv_font_montserrat_18

#define LV_USE_LOG 0

#endif /* LV_CONF_H */
