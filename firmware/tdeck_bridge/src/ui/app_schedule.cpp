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
enum ScheduleAction : uint8_t {
  SCHEDULE_AUTO = 0,
  SCHEDULE_WAKE = 1,
  SCHEDULE_HOLD = 2,
  SCHEDULE_NIGHT = 3,
};
static ScheduleAction gPendingAction = SCHEDULE_AUTO;

static uint8_t lifecycleMode(ScheduleAction action) {
  if (action == SCHEDULE_NIGHT) return 1;
  if (action == SCHEDULE_AUTO) return 2;
  return 0;
}

static const char *actionName(ScheduleAction action) {
  if (action == SCHEDULE_WAKE) return "WAKE FLEET";
  if (action == SCHEDULE_HOLD) return "PERFORMANCE HOLD";
  return action == SCHEDULE_NIGHT ? "NIGHT SHOW" : "AUTO";
}

static void refresh(lv_timer_t *) {
  if (!gStatus || lv_screen_active() != gScreen) return;
  GpsUtcObservation utc = halGpsUtc();
  uint32_t ageS = utc.valid ? (millis() - utc.receivedMs) / 1000UL : 0;
  MeshLifecycleCampaignStatus campaign = meshLifecycleCampaignStatus();
  char campaignLine[96];
  if (campaign.active && campaign.mode == 0) {
    uint32_t remainingS = (campaign.remainingMs + 999UL) / 1000UL;
    snprintf(campaignLine, sizeof(campaignLine), "%s active: %lu:%02lu left",
             campaign.durationMs > 360000UL ? "PERFORMANCE HOLD" : "WAKE",
             (unsigned long)(remainingS / 60UL),
             (unsigned long)(remainingS % 60UL));
  } else {
    snprintf(campaignLine, sizeof(campaignLine),
             "Wake 16 min; Performance Hold 1 hour.");
  }
  if (utc.valid)
    lv_label_set_text_fmt(gStatus, "%s\nUTC: GPS, %lus old\n%s",
                          halGpsSummary(), (unsigned long)ageS, campaignLine);
  else
    lv_label_set_text_fmt(gStatus, "%s\nUTC: waiting\n%s", halGpsSummary(),
                          campaignLine);
}

static void applyYes(void *) {
  uint8_t mode = lifecycleMode(gPendingAction);
  bool sent = gPendingAction == SCHEDULE_HOLD
                  ? meshPerformanceHold()
                  : meshForceLifecycle(mode);
  if (gStatus) {
    if (sent)
      if (gPendingAction == SCHEDULE_HOLD)
        lv_label_set_text(gStatus,
                          "PERFORMANCE HOLD started for 1 hour.\n"
                          "Sleeping fixtures gather; LED/audio stays armed.\n"
                          "Auto or Night Show cancels the hold.");
      else if (gPendingAction == SCHEDULE_WAKE)
        lv_label_set_text(gStatus,
                          "WAKE FLEET campaign started.\n"
                          "Catches timer wakes for 6 min; each radio stays up 10 min.\n"
                          "Inspection image: LED/audio control armed, AUTO retained.");
      else
        lv_label_set_text_fmt(gStatus, "%s field baseline sent fleet-wide.\n"
                                      "RAM-only; reboot returns a fixture to AUTO.",
                              actionName(gPendingAction));
    else
      lv_label_set_text(gStatus,
                        "NOT SENT: action audit storage or radio unavailable.");
  }
}

static void modeCb(lv_event_t *e) {
  gPendingAction =
      (ScheduleAction)(uintptr_t)lv_event_get_user_data(e);
  char summary[240];
  if (gPendingAction == SCHEDULE_WAKE)
    snprintf(summary, sizeof(summary),
             "Catch sleepers for 6 min and keep each captured radio awake for 10 "
             "min. The inspection image arms bounded LED/audio control and keeps "
             "AUTO; older images use the dark DAY baseline.");
  else if (gPendingAction == SCHEDULE_HOLD)
    snprintf(summary, sizeof(summary),
             "Arm inspection LED/audio control for one hour. Repeated Wake copies "
             "gather sleepers and refresh their bounded control window. Battery "
             "safety still wins; Auto or Night Show cancels the hold.");
  else
    snprintf(summary, sizeof(summary),
             "%s field baseline. On the inspection image this closes manual "
             "control and restores static safety light; battery safety wins.",
             actionName(gPendingAction));
  uiConfirm(summary, "Schedule", applyYes, nullptr);
}

static void backCb(lv_event_t *) {
  if (gTimer) lv_timer_delete(gTimer);
  gTimer = nullptr;
  gScreen = nullptr;
  gStatus = nullptr;
  uiGoHome();
}

static lv_obj_t *addModeButton(lv_obj_t *parent, const char *name,
                               ScheduleAction action, int x) {
  lv_obj_t *button = lv_button_create(parent);
  lv_obj_set_size(button, 75, 42);
  lv_obj_set_pos(button, x, 145);
  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, name);
  lv_obj_center(label);
  lv_obj_add_event_cb(button, modeCb, LV_EVENT_CLICKED,
                      (void *)(uintptr_t)action);
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

  lv_obj_t *autoBtn = addModeButton(gScreen, "auto", SCHEDULE_AUTO, 3);
  lv_obj_t *wakeBtn = addModeButton(gScreen, "wake 16m", SCHEDULE_WAKE, 83);
  lv_obj_t *holdBtn = addModeButton(gScreen, "hold 1h", SCHEDULE_HOLD, 163);
  lv_obj_t *nightBtn = addModeButton(gScreen, "night", SCHEDULE_NIGHT, 243);

  lv_obj_t *back = lv_button_create(gScreen);
  lv_obj_set_size(back, 96, 36);
  lv_obj_set_pos(back, 6, 198);
  lv_obj_t *backLabel = lv_label_create(back);
  lv_label_set_text(backLabel, LV_SYMBOL_LEFT " back");
  lv_obj_center(backLabel);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), autoBtn);
  lv_group_add_obj(lvglGroup(), wakeBtn);
  lv_group_add_obj(lvglGroup(), holdBtn);
  lv_group_add_obj(lvglGroup(), nightBtn);
  lv_group_add_obj(lvglGroup(), back);

  gTimer = lv_timer_create(refresh, 1000, nullptr);
  refresh(nullptr);
  lv_obj_t *old = lv_screen_active();
  lv_screen_load(gScreen);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
