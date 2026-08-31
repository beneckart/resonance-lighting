#include <lvgl.h>

#include <Arduino.h>

#include "../hal/hal_board.h"
#include "../net/mesh_tx.h"
#include "app_schedule.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_shell.h"
#include "ui_task.h"

static lv_obj_t *gScreen = nullptr;
static lv_obj_t *gStatus = nullptr;
static lv_timer_t *gTimer = nullptr;
static uint8_t gPendingMode = 2;

static const char *modeName(uint8_t mode) {
  return mode == 0 ? "WAKE FLEET" : (mode == 1 ? "NIGHT SHOW" : "AUTO");
}

static void refresh(lv_timer_t *) {
  if (!gStatus || lv_screen_active() != gScreen) return;
  GpsUtcObservation utc = halGpsUtc();
  uint32_t ageS = utc.valid ? (millis() - utc.receivedMs) / 1000UL : 0;
  if (utc.valid)
    lv_label_set_text_fmt(gStatus, "%s\nUTC source: GPS, %lus old\n"
                                   "Wake campaign catches a full sleep cycle.",
                          halGpsSummary(), (unsigned long)ageS);
  else
    lv_label_set_text_fmt(gStatus, "%s\nUTC source: waiting\n"
                                   "Wake campaign catches a full sleep cycle.",
                          halGpsSummary());
}

static void applyYes(void *) {
  bool sent = meshForceLifecycle(gPendingMode);
  if (gStatus) {
    if (sent)
      if (gPendingMode == 0)
        lv_label_set_text(gStatus,
                          "WAKE FLEET campaign started.\n"
                          "Catches timer wakes for 6 min; each radio stays up 10 min.\n"
                          "Inspection image: LED/audio control armed, AUTO retained.");
      else
        lv_label_set_text_fmt(gStatus, "%s field baseline sent fleet-wide.\n"
                                      "RAM-only; reboot returns a fixture to AUTO.",
                              modeName(gPendingMode));
    else
      lv_label_set_text(gStatus,
                        "NOT SENT: action audit storage or radio unavailable.");
  }
}

static void modeCb(lv_event_t *e) {
  gPendingMode = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  char summary[240];
  if (gPendingMode == 0)
    snprintf(summary, sizeof(summary),
             "Catch sleepers for 6 min and keep each captured radio awake for 10 "
             "min. The inspection image arms bounded LED/audio control and keeps "
             "AUTO; older images use the dark DAY baseline.");
    else
      snprintf(summary, sizeof(summary),
             "%s field baseline. On the inspection image this closes manual "
             "control and restores static safety light; battery safety wins.",
             modeName(gPendingMode));
  uiConfirm(summary, "Schedule", applyYes, nullptr);
}

static void backCb(lv_event_t *) {
  if (gTimer) lv_timer_delete(gTimer);
  gTimer = nullptr;
  gScreen = nullptr;
  gStatus = nullptr;
  uiGoHome();
}

static lv_obj_t *addModeButton(lv_obj_t *parent, const char *name, uint8_t mode,
                               int x) {
  lv_obj_t *button = lv_button_create(parent);
  lv_obj_set_size(button, 96, 42);
  lv_obj_set_pos(button, x, 145);
  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, name);
  lv_obj_center(label);
  lv_obj_add_event_cb(button, modeCb, LV_EVENT_CLICKED,
                      (void *)(uintptr_t)mode);
  return button;
}

void appScheduleOpen() {
  lvglSetNavHooks(nullptr);
  uiShellSetTitle("Schedule");
  gScreen = lv_obj_create(nullptr);
  lv_obj_clear_flag(gScreen, LV_OBJ_FLAG_SCROLLABLE);

  gStatus = lv_label_create(gScreen);
  lv_obj_set_style_text_font(gStatus, &lv_font_montserrat_14, 0);
  lv_obj_set_width(gStatus, 304);
  lv_obj_set_pos(gStatus, 8, 42);

  lv_obj_t *autoBtn = addModeButton(gScreen, "auto", 2, 6);
  lv_obj_t *dayBtn = addModeButton(gScreen, "wake fleet", 0, 111);
  lv_obj_t *nightBtn = addModeButton(gScreen, "night show", 1, 216);

  lv_obj_t *back = lv_button_create(gScreen);
  lv_obj_set_size(back, 96, 36);
  lv_obj_set_pos(back, 6, 198);
  lv_obj_t *backLabel = lv_label_create(back);
  lv_label_set_text(backLabel, LV_SYMBOL_LEFT " back");
  lv_obj_center(backLabel);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), autoBtn);
  lv_group_add_obj(lvglGroup(), dayBtn);
  lv_group_add_obj(lvglGroup(), nightBtn);
  lv_group_add_obj(lvglGroup(), back);

  gTimer = lv_timer_create(refresh, 1000, nullptr);
  refresh(nullptr);
  lv_obj_t *old = lv_screen_active();
  lv_screen_load(gScreen);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
