#include <lvgl.h>

#include <Arduino.h>

#include "../net/census_svc.h"
#include "../net/mesh_tx.h"
#include "../net/stream_svc.h"
#include "../store/store.h"
#include "app_power.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_task.h"

enum class RestAction : uint8_t { DARK = 0, SLEEP = 1 };

static const uint16_t kDurationsS[] = {
    600,   3600,  7200,  10800, 14400, 18000, 21600,
    25200, 28800, 32400, 36000, 39600, 43200,
};
static const char *kDurationNames[] = {
    "10 min",   "1 hour",   "2 hours",  "3 hours",   "4 hours",
    "5 hours",  "6 hours",  "7 hours", "8 hours",   "9 hours",
    "10 hours", "11 hours", "12 hours",
};
static_assert(sizeof(kDurationsS) / sizeof(kDurationsS[0]) ==
                  sizeof(kDurationNames) / sizeof(kDurationNames[0]),
              "duration labels must match duration values");

static lv_obj_t *gActionDd = nullptr;
static lv_obj_t *gDurationDd = nullptr;
static lv_obj_t *gInfo = nullptr;
static RestAction gPendingAction = RestAction::DARK;
static uint16_t gPendingSeconds = 600;

static RestAction selectedAction() {
  return gActionDd && lv_dropdown_get_selected(gActionDd) == 1
             ? RestAction::SLEEP
             : RestAction::DARK;
}

static uint32_t selectedDurationIndex() {
  uint32_t i = gDurationDd ? lv_dropdown_get_selected(gDurationDd) : 0;
  return i < sizeof(kDurationsS) / sizeof(kDurationsS[0]) ? i : 0;
}

static void refreshExplanation(lv_event_t *) {
  if (!gInfo) return;
  const char *explanation;
  if (selectedAction() == RestAction::SLEEP) {
    explanation = "Lowest draw: rails + radio off. Cannot cancel.";
  } else {
    explanation = "LED rail off; radio reachable and releasable.";
  }
  ActionAuditRecord last;
  char text[220];
  if (storeLatestAction(last)) {
    if (last.action == ACTION_AUDIT_SLEEP_ALL ||
        last.action == ACTION_AUDIT_DARK_ALL)
      snprintf(text, sizeof(text), "%s\nLast: %s %lumin, seq %lu",
               explanation, actionAuditName(last.action),
               (unsigned long)(last.value / 60UL),
               (unsigned long)last.mesh_seq);
    else
      snprintf(text, sizeof(text), "%s\nLast: %s, seq %lu", explanation,
               actionAuditName(last.action), (unsigned long)last.mesh_seq);
  } else {
    snprintf(text, sizeof(text), "%s\nLast: no retained action", explanation);
  }
  lv_label_set_text(gInfo, text);
}

static void applyYes(void *) {
  static const uint8_t kAll[3] = {0, 0, 0};
  // A suspended LED Studio stream would otherwise resume commanding as soon
  // as the sleep/dark action ends (or compete with a dark lease).
  streamStop();
  if (gPendingAction == RestAction::SLEEP) {
    bool sent = meshSleepAll(gPendingSeconds);
    if (gInfo) {
      if (sent)
        lv_label_set_text_fmt(gInfo,
                              "sleep sent: %lu min\nAudit saved; watch live count fall",
                              (unsigned long)gPendingSeconds / 60UL);
      else
        lv_label_set_text(gInfo, "SLEEP NOT SENT\nAudit storage or radio unavailable");
    }
  } else {
    bool sent = meshProgramLease(kAll, 4 /*COMMISSION_DARK*/, gPendingSeconds,
                                 0x01 /*hard cut*/, nullptr);
    if (gInfo) {
      if (sent)
        lv_label_set_text_fmt(gInfo, "dark lease sent: %lu min\nAudit saved",
                              (unsigned long)gPendingSeconds / 60UL);
      else
        lv_label_set_text(gInfo, "DARK NOT SENT\nAudit storage or radio unavailable");
    }
  }
}

static void applyCb(lv_event_t *) {
  uint32_t i = selectedDurationIndex();
  gPendingAction = selectedAction();
  gPendingSeconds = kDurationsS[i];
  int live = 0, seen = 0;
  censusCountsSafe(&live, &seen, millis());
  char summary[120];
  if (gPendingAction == RestAction::SLEEP) {
    snprintf(summary, sizeof(summary),
             "SLEEP %d live (%d seen) for %s. Radio OFF; cannot cancel.",
             live, seen, kDurationNames[i]);
  } else {
    snprintf(summary, sizeof(summary),
             "DARK %d live (%d seen) for %s. Radio stays awake.", live, seen,
             kDurationNames[i]);
  }
  uiConfirm(summary, "Sleep / Dark", applyYes, nullptr);
}

static void releaseCb(lv_event_t *) {
  static const uint8_t kAll[3] = {0, 0, 0};
  streamStop();
  bool sent = meshProgramLease(kAll, 4, 0, 0x01, nullptr);
  if (gInfo) lv_label_set_text(gInfo, "dark lease released -> autonomous");
  if (!sent && gInfo) lv_label_set_text(gInfo, "release not sent: radio unavailable");
}

static void backCb(lv_event_t *) {
  gActionDd = nullptr;
  gDurationDd = nullptr;
  gInfo = nullptr;
  uiGoHome();
}

void appPowerOpen() {
  lvglSetNavHooks(nullptr);
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(scr);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_label_set_text(title, "Sleep / Dark");
  lv_obj_set_pos(title, 8, 5);

  lv_obj_t *actionLabel = lv_label_create(scr);
  lv_label_set_text(actionLabel, "action");
  lv_obj_set_pos(actionLabel, 8, 48);
  gActionDd = lv_dropdown_create(scr);
  lv_dropdown_set_options(gActionDd, "dark (radio awake)\nlow-power sleep");
  lv_dropdown_set_selected(gActionDd, 0);
  lv_obj_set_pos(gActionDd, 92, 39);
  lv_obj_set_width(gActionDd, 220);
  lv_obj_add_event_cb(gActionDd, refreshExplanation, LV_EVENT_VALUE_CHANGED,
                      nullptr);

  lv_obj_t *durationLabel = lv_label_create(scr);
  lv_label_set_text(durationLabel, "duration");
  lv_obj_set_pos(durationLabel, 8, 92);
  gDurationDd = lv_dropdown_create(scr);
  lv_dropdown_set_options(gDurationDd,
                          "10 min\n1 hour\n2 hours\n3 hours\n4 hours\n"
                          "5 hours\n6 hours\n7 hours\n8 hours\n9 hours\n"
                          "10 hours\n11 hours\n12 hours");
  lv_dropdown_set_selected(gDurationDd, 0);
  lv_obj_set_pos(gDurationDd, 92, 83);
  lv_obj_set_width(gDurationDd, 140);

  gInfo = lv_label_create(scr);
  lv_obj_set_style_text_font(gInfo, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gInfo, 8, 132);

  lv_obj_t *apply = lv_button_create(scr);
  lv_obj_set_size(apply, 100, 36);
  lv_obj_set_pos(apply, 8, 198);
  lv_obj_t *al = lv_label_create(apply);
  lv_label_set_text(al, LV_SYMBOL_POWER " apply");
  lv_obj_center(al);
  lv_obj_add_event_cb(apply, applyCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *release = lv_button_create(scr);
  lv_obj_set_size(release, 100, 36);
  lv_obj_set_pos(release, 114, 198);
  lv_obj_t *rl = lv_label_create(release);
  lv_label_set_text(rl, "release dark");
  lv_obj_center(rl);
  lv_obj_add_event_cb(release, releaseCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *back = lv_button_create(scr);
  lv_obj_set_size(back, 92, 36);
  lv_obj_set_pos(back, 220, 198);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, LV_SYMBOL_LEFT " back");
  lv_obj_center(bl);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), gActionDd);
  lv_group_add_obj(lvglGroup(), gDurationDd);
  lv_group_add_obj(lvglGroup(), apply);
  lv_group_add_obj(lvglGroup(), release);
  lv_group_add_obj(lvglGroup(), back);
  refreshExplanation(nullptr);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
