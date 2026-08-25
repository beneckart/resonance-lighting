#include <lvgl.h>

#include <Arduino.h>

#include "../net/stream_svc.h"
#include "app_zones.h"
#include "lvgl_glue.h"
#include "ui_task.h"

struct Swatch {
  const char *name;
  uint8_t r, g, b, w;
};
static const Swatch kSwatches[8] = {
    {"red", 255, 0, 0, 0},      {"amber", 255, 96, 0, 40},
    {"green", 0, 255, 0, 0},    {"cyan", 0, 200, 200, 0},
    {"blue", 0, 0, 255, 0},     {"purple", 160, 0, 255, 0},
    {"white", 0, 0, 0, 255},    {"pink", 255, 40, 120, 0},
};

static lv_obj_t *gClassDd = nullptr;
static lv_obj_t *gDimSlider = nullptr;
static lv_obj_t *gInfo = nullptr;
static lv_obj_t *gModeLabel = nullptr;
static lv_timer_t *gInfoTimer = nullptr;
static int gSwatch = 0;
static bool gBlink = false;

static uint8_t classFilter() {
  return gClassDd ? (uint8_t)lv_dropdown_get_selected(gClassDd) : 0;
}

static void refreshInfo(lv_timer_t *) {
  if (!gInfo) return;
  if (streamMode() == StreamMode::OFF) {
    lv_label_set_text(gInfo, "stopped; fixtures revert in about 3 s");
    return;
  }
  const Swatch &s = kSwatches[gSwatch];
  lv_label_set_text_fmt(gInfo, "%s %s -> %d fresh fixtures",
                        streamMode() == StreamMode::BLINK ? "blink" : "solid",
                        s.name, streamTargetCount());
}

static void applyStream() {
  const Swatch &s = kSwatches[gSwatch];
  uint8_t dim = gDimSlider ? (uint8_t)lv_slider_get_value(gDimSlider) : 255;
  if (gBlink)
    streamBlink(classFilter(), s.r, s.g, s.b, s.w, dim);
  else
    streamSolid(classFilter(), s.r, s.g, s.b, s.w, dim);
  refreshInfo(nullptr);
}

static void swatchCb(lv_event_t *e) {
  gSwatch = (int)(intptr_t)lv_event_get_user_data(e);
  applyStream();
}

static void dimChanged(lv_event_t *) {
  if (streamMode() != StreamMode::OFF) applyStream();
}

static void classChanged(lv_event_t *) {
  if (streamMode() != StreamMode::OFF) applyStream();
}

static void modeCb(lv_event_t *) {
  gBlink = !gBlink;
  if (gModeLabel)
    lv_label_set_text(gModeLabel, gBlink ? "blink 1Hz" : "solid");
  applyStream();
}

static void stopCb(lv_event_t *) {
  streamStop();
  refreshInfo(nullptr);
}

static void backCb(lv_event_t *) {
  // Deliberately KEEPS an active stream: the color/blink survives app-switch;
  // stop is explicit. Sleep / Dark always stops it before a rest action.
  if (gInfoTimer) {
    lv_timer_delete(gInfoTimer);
    gInfoTimer = nullptr;
  }
  gClassDd = nullptr;
  gDimSlider = nullptr;
  gInfo = nullptr;
  gModeLabel = nullptr;
  uiGoHome();
}

void appZonesOpen() {
  lvglSetNavHooks(nullptr);
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(scr);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_label_set_text(title, "LED Studio");
  lv_obj_set_pos(title, 8, 4);

  gClassDd = lv_dropdown_create(scr);
  lv_dropdown_set_options(gClassDd,
                          "all\ndownlights\nperimeter\nuplights\nchandelier");
  lv_obj_set_pos(gClassDd, 130, 2);
  lv_obj_set_width(gClassDd, 182);
  lv_obj_add_event_cb(gClassDd, classChanged, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), gClassDd);

  // Swatch grid, 4x2, thumb-sized and text-labelled for field use.
  for (int i = 0; i < 8; ++i) {
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 72, 44);
    lv_obj_set_pos(btn, 6 + (i % 4) * 78, 44 + (i / 4) * 50);
    const Swatch &s = kSwatches[i];
    uint16_t wboost = (uint16_t)(s.w * 4 / 5);
    lv_obj_set_style_bg_color(
        btn,
        lv_color_make((uint8_t)LV_MIN(255, s.r + wboost),
                      (uint8_t)LV_MIN(255, s.g + wboost),
                      (uint8_t)LV_MIN(255, s.b + wboost)),
        0);
    lv_obj_add_event_cb(btn, swatchCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *name = lv_label_create(btn);
    lv_label_set_text(name, s.name);
    lv_obj_set_style_text_color(name,
                                (i == 1 || i == 2 || i == 3 || i == 6)
                                    ? lv_color_black()
                                    : lv_color_white(),
                                0);
    lv_obj_center(name);
    lv_group_add_obj(lvglGroup(), btn);
  }

  lv_obj_t *dimLbl = lv_label_create(scr);
  lv_obj_set_style_text_font(dimLbl, &lv_font_montserrat_14, 0);
  lv_label_set_text(dimLbl, "dim");
  lv_obj_set_pos(dimLbl, 8, 150);
  gDimSlider = lv_slider_create(scr);
  lv_obj_set_size(gDimSlider, 250, 14);
  lv_obj_set_pos(gDimSlider, 46, 152);
  lv_slider_set_range(gDimSlider, 8, 255);
  lv_slider_set_value(gDimSlider, 255, LV_ANIM_OFF);
  lv_obj_add_event_cb(gDimSlider, dimChanged, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_group_add_obj(lvglGroup(), gDimSlider);

  gInfo = lv_label_create(scr);
  lv_obj_set_style_text_font(gInfo, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gInfo, 8, 176);
  if (streamMode() == StreamMode::BLINK) gBlink = true;
  refreshInfo(nullptr);

  lv_obj_t *mode = lv_button_create(scr);
  lv_obj_set_size(mode, 100, 34);
  lv_obj_set_pos(mode, 8, 200);
  gModeLabel = lv_label_create(mode);
  lv_label_set_text(gModeLabel, gBlink ? "blink 1Hz" : "solid");
  lv_obj_center(gModeLabel);
  lv_obj_add_event_cb(mode, modeCb, LV_EVENT_CLICKED, nullptr);
  lv_group_add_obj(lvglGroup(), mode);

  lv_obj_t *stop = lv_button_create(scr);
  lv_obj_set_size(stop, 100, 34);
  lv_obj_set_pos(stop, 114, 200);
  lv_obj_t *sl = lv_label_create(stop);
  lv_label_set_text(sl, LV_SYMBOL_STOP " stop");
  lv_obj_center(sl);
  lv_obj_add_event_cb(stop, stopCb, LV_EVENT_CLICKED, nullptr);
  lv_group_add_obj(lvglGroup(), stop);

  lv_obj_t *back = lv_button_create(scr);
  lv_obj_set_size(back, 92, 34);
  lv_obj_set_pos(back, 220, 200);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, LV_SYMBOL_LEFT " back");
  lv_obj_center(bl);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);
  lv_group_add_obj(lvglGroup(), back);

  gInfoTimer = lv_timer_create(refreshInfo, 500, nullptr);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
