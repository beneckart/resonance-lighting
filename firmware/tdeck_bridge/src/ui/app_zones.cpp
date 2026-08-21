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
static int gSwatch = 0;

static uint8_t classFilter() {
  return gClassDd ? (uint8_t)lv_dropdown_get_selected(gClassDd) : 0;
}

static void applyStream() {
  const Swatch &s = kSwatches[gSwatch];
  uint8_t dim = gDimSlider ? (uint8_t)lv_slider_get_value(gDimSlider) : 255;
  streamSolid(classFilter(), s.r, s.g, s.b, s.w, dim);
  if (gInfo)
    lv_label_set_text_fmt(gInfo, "streaming %s to %d fixtures @8Hz", s.name,
                          streamTargetCount());
}

static void swatchCb(lv_event_t *e) {
  gSwatch = (int)(intptr_t)lv_event_get_user_data(e);
  applyStream();
}

static void dimChanged(lv_event_t *) {
  if (streamMode() != StreamMode::OFF) applyStream();
}

static void stopCb(lv_event_t *) {
  streamStop();
  if (gInfo)
    lv_label_set_text(gInfo, "stopped; fixtures revert in ~3 s (staleness)");
}

static void backCb(lv_event_t *) {
  // Deliberately KEEPS an active stream (suspended-owner semantics): the
  // color survives app-switch; stop is explicit.
  gClassDd = nullptr;
  gDimSlider = nullptr;
  gInfo = nullptr;
  uiGoHome();
}

void appZonesOpen() {
  lvglSetNavHooks(nullptr);
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(scr);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_label_set_text(title, "Zones");
  lv_obj_set_pos(title, 8, 4);

  gClassDd = lv_dropdown_create(scr);
  lv_dropdown_set_options(gClassDd,
                          "all\ndownlights\nperimeter\nuplights\nchandelier");
  lv_obj_set_pos(gClassDd, 130, 2);
  lv_obj_set_width(gClassDd, 182);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), gClassDd);

  // Swatch grid, 4x2, thumb-sized.
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
  if (streamMode() == StreamMode::OFF)
    lv_label_set_text(gInfo, "pick a color; class map needs ~60 s of census");
  else
    lv_label_set_text_fmt(gInfo, "stream active (%d fixtures)",
                          streamTargetCount());

  lv_obj_t *stop = lv_button_create(scr);
  lv_obj_set_size(stop, 140, 34);
  lv_obj_set_pos(stop, 8, 200);
  lv_obj_t *sl = lv_label_create(stop);
  lv_label_set_text(sl, LV_SYMBOL_STOP " stop stream");
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

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
