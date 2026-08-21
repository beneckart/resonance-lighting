#include <lvgl.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "../hal/hal_display.h"
#include "../hal/hal_input.h"
#include "lvgl_glue.h"

static lv_group_t *gGroup = nullptr;

static void flushCb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  lgfx::LGFX_Device *d = halDisplayDevice();
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;
  d->startWrite();
  // rgb565_t source: LovyanGFX handles the panel byte order.
  d->pushImage(area->x1, area->y1, w, h, (const lgfx::rgb565_t *)px_map);
  d->endWrite();
  lv_display_flush_ready(disp);
}

static uint32_t tickCb() { return millis(); }

// Trackball semantics (pins_tdeck.h calibration: a=UP b=RIGHT c=DOWN d=LEFT):
//  - navigate mode: left/right = focus prev/next; up/down = screen hook
//    (launcher row-jump, table row-scroll) or focus prev/next fallback;
//    click = screen hook (open selection) or ENTER key.
//  - edit mode (slider/dropdown opened with ENTER): pulses become arrow KEYS
//    into the widget; click = ENTER (commit/close).
static const UiNavHooks *gNavHooks = nullptr;
void lvglSetNavHooks(const UiNavHooks *hooks) { gNavHooks = hooks; }

static void trackballRead(lv_indev_t *, lv_indev_data_t *data) {
  static uint32_t pu = 0, pd = 0, pl = 0, pr = 0;
  static int32_t remV = 0, remH = 0;
  static bool prevClick = false;
  static uint32_t pendingKey = 0;
  static int pendingPhase = 0;  // 0 idle, 1 deliver press, 2 deliver release

  data->key = pendingKey;
  data->state = LV_INDEV_STATE_RELEASED;
  if (pendingPhase == 1) {
    data->state = LV_INDEV_STATE_PRESSED;
    pendingPhase = 2;
    return;
  }
  if (pendingPhase == 2) {
    pendingPhase = 0;
    return;
  }

  TrackballCounts t = halTrackballRead();
  remV += (int32_t)(t.c - pd) - (int32_t)(t.a - pu);
  remH += (int32_t)(t.b - pr) - (int32_t)(t.d - pl);
  pu = t.a;
  pd = t.c;
  pl = t.d;
  pr = t.b;
  bool clickEdge = t.click && !prevClick;
  prevClick = t.click;

  lv_group_t *g = lvglGroup();
  bool editing = g && lv_group_get_editing(g);

  auto queueKey = [&](uint32_t k) {
    pendingKey = k;
    pendingPhase = 1;
  };

  if (clickEdge) {
    if (!editing && gNavHooks && gNavHooks->onEnter && gNavHooks->onEnter()) {
      remV = remH = 0;
      return;
    }
    queueKey(LV_KEY_ENTER);
    return;
  }

  if (editing) {  // pulses feed the widget as arrow keys
    if (remH > 0) { --remH; queueKey(LV_KEY_RIGHT); }
    else if (remH < 0) { ++remH; queueKey(LV_KEY_LEFT); }
    else if (remV > 0) { --remV; queueKey(LV_KEY_DOWN); }
    else if (remV < 0) { ++remV; queueKey(LV_KEY_UP); }
    return;
  }

  if (remH != 0) {
    int dir = remH > 0 ? 1 : -1;
    remH -= dir;
    if (g) { if (dir > 0) lv_group_focus_next(g); else lv_group_focus_prev(g); }
    return;
  }
  if (remV != 0) {
    int dir = remV > 0 ? 1 : -1;
    remV -= dir;
    if (gNavHooks && gNavHooks->onVertical && gNavHooks->onVertical(dir)) return;
    if (g) { if (dir > 0) lv_group_focus_next(g); else lv_group_focus_prev(g); }
    return;
  }
}

// Keyboard aux MCU reports ASCII on press only; synthesize the release on the
// following poll so LVGL sees clean press/release pairs.
static void keypadRead(lv_indev_t *, lv_indev_data_t *data) {
  static uint32_t lastKey = 0;
  static bool pressPending = false;
  if (pressPending) {
    pressPending = false;
    data->key = lastKey;
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }
  char c = halKeyboardRead();
  if (!c) {
    data->key = lastKey;
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }
  uint32_t k = (uint8_t)c;
  if (c == '\r' || c == '\n') k = LV_KEY_ENTER;
  else if (c == 0x08) k = LV_KEY_BACKSPACE;
  lastKey = k;
  data->key = k;
  data->state = LV_INDEV_STATE_PRESSED;
  pressPending = true;
}

static void pointerRead(lv_indev_t *, lv_indev_data_t *data) {
  static int16_t lastX = 0, lastY = 0;
  int16_t x, y;
  // GT911 fresh-frame reads only: a tap registers; hold-tracking is a later
  // refinement (buttons and lists only need taps).
  if (halTouchRead(&x, &y)) {
    lastX = x;
    lastY = y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
  data->point.x = lastX;
  data->point.y = lastY;
}

bool lvglGlueInit() {
  lv_init();
  lv_tick_set_cb(tickCb);

  // Two partial draw buffers (320x40 px) in internal DMA-capable RAM.
  const size_t bufPx = 320 * 40;
  const size_t bufBytes = bufPx * 2;
  void *buf1 = heap_caps_malloc(bufBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  void *buf2 = heap_caps_malloc(bufBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!buf1 || !buf2) return false;

  lv_display_t *disp = lv_display_create(320, 240);
  if (!disp) return false;
  lv_display_set_flush_cb(disp, flushCb);
  lv_display_set_buffers(disp, buf1, buf2, bufBytes,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  gGroup = lv_group_create();
  lv_group_set_default(gGroup);

  lv_indev_t *tb = lv_indev_create();
  lv_indev_set_type(tb, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(tb, trackballRead);
  lv_indev_set_group(tb, gGroup);

  lv_indev_t *kbd = lv_indev_create();
  lv_indev_set_type(kbd, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(kbd, keypadRead);
  lv_indev_set_group(kbd, gGroup);

  lv_indev_t *ptr = lv_indev_create();
  lv_indev_set_type(ptr, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(ptr, pointerRead);

  return true;
}

lv_group_t *lvglGroup() { return gGroup; }
