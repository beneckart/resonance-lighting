#include <lvgl.h>

#include <Arduino.h>
#include <string.h>

#include "../net/mesh_tx.h"
#include "app_ca.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_task.h"

// GH-CA params[8] semantics (fixture prog_gh_ca.cpp:9-33):
// [0] K excitation threshold, [1] spontaneous activation x/256,
// [2] refractory ticks, [3] tick period in deciseconds, [4] hue 0-255,
// [5] output mode (0=light, 1=knock), [6] hardened ToF edge seed.
struct Knob {
  const char *name;
  int min, max, dflt;
};
static const Knob kKnobs[5] = {
    {"K neighbors", 1, 8, 1},
    {"spark /256", 0, 64, 2},
    {"refractory", 1, 10, 3},
    {"tick (ds)", 1, 50, 10},
    {"light hue", 0, 255, 160},
};
static lv_obj_t *gSliders[5] = {};
static lv_obj_t *gValLabels[5] = {};
static lv_obj_t *gProgDd = nullptr;
static lv_obj_t *gLeaseDd = nullptr;
static lv_obj_t *gTofBtn = nullptr;
static lv_obj_t *gTofLabel = nullptr;
static lv_obj_t *gInfo = nullptr;

static const uint16_t kLeases[4] = {120, 300, 600, 3600};

static bool knockMode() {
  return gProgDd && lv_dropdown_get_selected(gProgDd) == 1;
}

static bool tofSeedEnabled() {
  return gTofBtn && lv_obj_has_state(gTofBtn, LV_STATE_CHECKED);
}

static void tofChanged(lv_event_t *) {
  if (gTofLabel)
    lv_label_set_text(gTofLabel, tofSeedEnabled() ? "ToF seed: ON"
                                                  : "ToF seed: off");
}

static void applyYes(void *) {
  static const uint8_t kAll[3] = {0, 0, 0};
  uint16_t leaseS = kLeases[lv_dropdown_get_selected(gLeaseDd)];
  uint8_t params[8] = {};
  for (int i = 0; i < 5; ++i)
    params[i] = (uint8_t)lv_slider_get_value(gSliders[i]);
  params[5] = knockMode() ? 1 : 0;
  params[6] = tofSeedEnabled() ? 1 : 0;
  meshProgramLease(kAll, 1 /* PROG_GH_CA */, leaseS, 0x01, params);
  if (gInfo) {
    lv_label_set_text_fmt(gInfo, "%s%s, %us", knockMode() ? "knocks" : "lights",
                          tofSeedEnabled() ? " + ToF" : "", leaseS);
  }
}

static void applyCb(lv_event_t *) {
  char summary[160];
  uint16_t leaseS = kLeases[lv_dropdown_get_selected(gLeaseDd)];
  snprintf(summary, sizeof(summary), "%s wildfire on all awake fixtures for "
           "%u s. ToF seed %s; %s", knockMode() ? "Knock" : "Light", leaseS,
           tofSeedEnabled() ? "ON (sensor downlights originate)" : "off",
           knockMode() ? "Local daytime and power gates still decide each knock."
                       : "Local light power gates still apply.");
  uiConfirm(summary, "CA Studio", applyYes, nullptr);
}

static void releaseCb(lv_event_t *) {
  static const uint8_t kAll[3] = {0, 0, 0};
  meshProgramLease(kAll, 0 /* release */, 0, 0x01, nullptr);
  if (gInfo) lv_label_set_text(gInfo, "released -> autonomous");
}

static void sliderChanged(lv_event_t *e) {
  lv_obj_t *s = (lv_obj_t *)lv_event_get_target(e);
  for (int i = 0; i < 5; ++i)
    if (gSliders[i] == s && gValLabels[i])
      lv_label_set_text_fmt(gValLabels[i], "%d", (int)lv_slider_get_value(s));
}

static void backCb(lv_event_t *) {
  gProgDd = nullptr;
  gLeaseDd = nullptr;
  gTofBtn = nullptr;
  gTofLabel = nullptr;
  gInfo = nullptr;
  memset(gSliders, 0, sizeof(gSliders));
  uiGoHome();
}

void appCaOpen() {
  lvglSetNavHooks(nullptr);
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(scr);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_label_set_text(title, "Wildfire CA");
  lv_obj_set_pos(title, 8, 4);

  gProgDd = lv_dropdown_create(scr);
  lv_dropdown_set_options(gProgDd, "lights\nknocks");
  lv_obj_set_pos(gProgDd, 150, 2);
  lv_obj_set_width(gProgDd, 88);
  gLeaseDd = lv_dropdown_create(scr);
  lv_dropdown_set_options(gLeaseDd, "120s\n300s\n600s\n1h");
  lv_dropdown_set_selected(gLeaseDd, 2);
  lv_obj_set_pos(gLeaseDd, 244, 2);
  lv_obj_set_width(gLeaseDd, 72);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), gProgDd);
  lv_group_add_obj(lvglGroup(), gLeaseDd);

  for (int i = 0; i < 5; ++i) {
    lv_obj_t *lbl = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl, kKnobs[i].name);
    lv_obj_set_pos(lbl, 8, 42 + i * 26);
    gSliders[i] = lv_slider_create(scr);
    lv_obj_set_size(gSliders[i], 150, 12);
    lv_obj_set_pos(gSliders[i], 110, 46 + i * 26);
    lv_slider_set_range(gSliders[i], kKnobs[i].min, kKnobs[i].max);
    lv_slider_set_value(gSliders[i], kKnobs[i].dflt, LV_ANIM_OFF);
    lv_obj_add_event_cb(gSliders[i], sliderChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    gValLabels[i] = lv_label_create(scr);
    lv_obj_set_style_text_font(gValLabels[i], &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(gValLabels[i], "%d", kKnobs[i].dflt);
    lv_obj_set_pos(gValLabels[i], 272, 42 + i * 26);
    lv_group_add_obj(lvglGroup(), gSliders[i]);
  }

  gTofBtn = lv_button_create(scr);
  lv_obj_add_flag(gTofBtn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_size(gTofBtn, 108, 25);
  lv_obj_set_pos(gTofBtn, 8, 170);
  gTofLabel = lv_label_create(gTofBtn);
  lv_label_set_text(gTofLabel, "ToF seed: off");
  lv_obj_center(gTofLabel);
  lv_obj_add_event_cb(gTofBtn, tofChanged, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_group_add_obj(lvglGroup(), gTofBtn);

  gInfo = lv_label_create(scr);
  lv_obj_set_style_text_font(gInfo, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gInfo, 124, 174);
  lv_obj_set_width(gInfo, 188);
  lv_label_set_text(gInfo, "hardened rising edge");

  lv_obj_t *apply = lv_button_create(scr);
  lv_obj_set_size(apply, 100, 34);
  lv_obj_set_pos(apply, 8, 200);
  lv_obj_t *al = lv_label_create(apply);
  lv_label_set_text(al, "apply");
  lv_obj_center(al);
  lv_obj_add_event_cb(apply, applyCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *rel = lv_button_create(scr);
  lv_obj_set_size(rel, 100, 34);
  lv_obj_set_pos(rel, 114, 200);
  lv_obj_t *rl = lv_label_create(rel);
  lv_label_set_text(rl, "release");
  lv_obj_center(rl);
  lv_obj_add_event_cb(rel, releaseCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *back = lv_button_create(scr);
  lv_obj_set_size(back, 92, 34);
  lv_obj_set_pos(back, 220, 200);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, LV_SYMBOL_LEFT " back");
  lv_obj_center(bl);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  lv_group_add_obj(lvglGroup(), apply);
  lv_group_add_obj(lvglGroup(), rel);
  lv_group_add_obj(lvglGroup(), back);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
