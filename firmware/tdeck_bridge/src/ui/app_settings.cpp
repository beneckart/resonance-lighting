#include <lvgl.h>

#include <Arduino.h>

#include "../hal/hal_display.h"
#include "../store/store.h"
#include "app_settings.h"
#include "lvgl_glue.h"
#include "ui_theme.h"
#include "ui_task.h"

static lv_obj_t *gModeValue = nullptr;
static lv_obj_t *gBacklightLabel = nullptr;

static void updateDisplayLabels() {
  if (gModeValue)
    lv_label_set_text(gModeValue, uiDayMode() ? "DAY  full sun" : "NIGHT");
  if (gBacklightLabel)
    lv_label_set_text_fmt(gBacklightLabel, "night backlight  %u",
                          settings().backlight);
}

static void modeChanged(lv_event_t *e) {
  lv_obj_t *toggle = (lv_obj_t *)lv_event_get_target(e);
  uiSetDayMode(lv_obj_has_state(toggle, LV_STATE_CHECKED));
  updateDisplayLabels();
}

static void blChanged(lv_event_t *e) {
  lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
  int v = (int)lv_slider_get_value(slider);
  settings().backlight = (uint8_t)v;
  if (!uiDayMode()) halDisplaySetBacklight((uint8_t)v);
  updateDisplayLabels();
}

static void blReleased(lv_event_t *) { storeSave(); }

static void chChanged(lv_event_t *e) {
  lv_obj_t *dd = (lv_obj_t *)lv_event_get_target(e);
  settings().channel = (uint8_t)(lv_dropdown_get_selected(dd) + 1);
  storeSave();
}

static void backCb(lv_event_t *) { uiGoHome(); }

void appSettingsOpen() {
  lvglSetNavHooks(nullptr);
  lv_obj_t *scr = lv_obj_create(nullptr);

  lv_obj_t *title = lv_label_create(scr);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_label_set_text(title, "Settings");
  lv_obj_set_pos(title, 8, 6);

  lv_obj_t *modeLabel = lv_label_create(scr);
  lv_label_set_text(modeLabel, "display");
  lv_obj_set_pos(modeLabel, 8, 43);
  lv_obj_t *modeToggle = lv_switch_create(scr);
  lv_obj_set_pos(modeToggle, 112, 37);
  if (uiDayMode()) lv_obj_add_state(modeToggle, LV_STATE_CHECKED);
  lv_obj_add_event_cb(modeToggle, modeChanged, LV_EVENT_VALUE_CHANGED, nullptr);
  gModeValue = lv_label_create(scr);
  lv_obj_set_style_text_font(gModeValue, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gModeValue, 177, 43);

  gBacklightLabel = lv_label_create(scr);
  lv_obj_set_pos(gBacklightLabel, 8, 78);
  lv_obj_t *slider = lv_slider_create(scr);
  lv_obj_set_size(slider, 112, 14);
  lv_obj_set_pos(slider, 192, 81);
  lv_slider_set_range(slider, 10, 255);
  lv_slider_set_value(slider, settings().backlight, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, blChanged, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(slider, blReleased, LV_EVENT_RELEASED, nullptr);

  lv_obj_t *chLabel = lv_label_create(scr);
  lv_label_set_text(chLabel, "mesh channel");
  lv_obj_set_pos(chLabel, 8, 113);
  lv_obj_t *dd = lv_dropdown_create(scr);
  lv_dropdown_set_options(dd, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13");
  lv_dropdown_set_selected(dd, settings().channel - 1);
  lv_obj_set_pos(dd, 145, 104);
  lv_obj_set_width(dd, 80);
  lv_obj_add_event_cb(dd, chChanged, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_t *chNote = lv_label_create(scr);
  lv_obj_set_style_text_font(chNote, &lv_font_montserrat_14, 0);
  lv_label_set_text(chNote, "fleet = 11; reboot applies cleanly");
  lv_obj_set_pos(chNote, 8, 140);

  lv_obj_t *prov = lv_label_create(scr);
  lv_obj_set_style_text_font(prov, &lv_font_montserrat_14, 0);
  char keyMask[16] = "(unset)";
  if (settings().apiKey[0]) snprintf(keyMask, sizeof(keyMask), "set");
  lv_label_set_text_fmt(prov,
                        "wifi: %s (%s)  api: %s\nmodel: %s\n"
                        "secrets: serial CLI only",
                        settings().ssid[0] ? settings().ssid : "(unset)",
                        settings().psk[0] ? "psk set" : "no psk", keyMask,
                        settings().model);
  lv_obj_set_pos(prov, 8, 165);

  lv_obj_t *back = lv_button_create(scr);
  lv_obj_align(back, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, LV_SYMBOL_LEFT " back");
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), modeToggle);
  lv_group_add_obj(lvglGroup(), slider);
  lv_group_add_obj(lvglGroup(), dd);
  lv_group_add_obj(lvglGroup(), back);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
  updateDisplayLabels();
}
