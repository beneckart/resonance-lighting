#include <lvgl.h>

#include <Arduino.h>
#include <string.h>

#include "../core/knock_plan.h"
#include "../net/census_svc.h"
#include "../net/mesh_tx.h"
#include "app_commission.h"
#include "fixture/src/core/fixture_context.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_shell.h"
#include "ui_task.h"

static constexpr uint32_t COMMISSION_ROLL_STEP_MS = 80;
static constexpr uint32_t COMMISSION_ALL = 0;
static constexpr uint32_t COMMISSION_FIRST_TARGET = 1;

static lv_obj_t *gTargetDd = nullptr;
static lv_obj_t *gModeDd = nullptr;
static lv_obj_t *gStorageDd = nullptr;
static lv_obj_t *gInfo = nullptr;
static uint8_t gTargets[CENSUS_MAX_TRACKED][3];
static size_t gTargetCount = 0;

static uint8_t gQueue[CENSUS_MAX_TRACKED][3];
static size_t gQueueLen = 0, gQueueNext = 0, gQueueFailed = 0;
static uint8_t gQueueMode = COMMISSION_DEFAULT_LISTENER;
static bool gQueuePersist = false;
static lv_timer_t *gQueueTimer = nullptr;

static uint8_t gPendingTarget[3] = {};
static uint8_t gPendingMode = COMMISSION_DEFAULT_LISTENER;
static bool gPendingPersist = false;

static size_t snapshotFresh(uint8_t out[][3], size_t outCap, uint32_t now) {
  static CensusView rows[CENSUS_MAX_TRACKED];
  size_t n = censusSnapshotSafe(rows, CENSUS_MAX_TRACKED, now);
  return targetPlanFresh(rows, n, censusFreshMsSafe(), out, outCap);
}

static const char *modeLabel(uint8_t mode) {
  switch (mode) {
  case COMMISSION_DEFAULT_CA: return "wildfire CA";
  case COMMISSION_DEFAULT_DARK: return "rails dark";
  default: return "ready beacon";
  }
}

static void queueTick(lv_timer_t *timer) {
  if (gQueueNext >= gQueueLen) {
    lv_timer_delete(timer);
    gQueueTimer = nullptr;
    if (gInfo)
      lv_label_set_text_fmt(gInfo, "sent %u exact-target settings%s",
                            (unsigned)gQueueLen,
                            gQueueFailed ? "; local send failures occurred" : "");
    return;
  }
  if (!meshCommissionDefault(gQueue[gQueueNext], gQueueMode, gQueuePersist))
    ++gQueueFailed;
  ++gQueueNext;
  if (gInfo)
    lv_label_set_text_fmt(gInfo, "setting... %u/%u", (unsigned)gQueueNext,
                          (unsigned)gQueueLen);
}

static void applyAllYes(void *) {
  if (gQueueTimer) {
    if (gInfo) lv_label_set_text(gInfo, "default campaign already running");
    return;
  }
  gQueueNext = 0;
  gQueueFailed = 0;
  gQueueMode = gPendingMode;
  gQueuePersist = gPendingPersist;
  gQueueLen = snapshotFresh(gQueue, CENSUS_MAX_TRACKED, millis());
  if (gQueueLen == 0) {
    if (gInfo) lv_label_set_text(gInfo, "no fresh fixtures to set");
    return;
  }
  gQueueTimer = lv_timer_create(queueTick, COMMISSION_ROLL_STEP_MS, nullptr);
}

static void applyOneYes(void *) {
  bool sent = meshCommissionDefault(gPendingTarget, gPendingMode,
                                    gPendingPersist);
  if (gInfo)
    lv_label_set_text_fmt(gInfo, "%s %02X%02X%02X -> %s%s",
                          sent ? "sent" : "send refused", gPendingTarget[0],
                          gPendingTarget[1], gPendingTarget[2],
                          modeLabel(gPendingMode),
                          gPendingPersist ? " (persist)" : "");
}

static void applyCb(lv_event_t *) {
  if (!gTargetDd || !gModeDd || !gStorageDd) return;
  uint32_t selected = lv_dropdown_get_selected(gTargetDd);
  gPendingMode = (uint8_t)lv_dropdown_get_selected(gModeDd);
  gPendingPersist = lv_dropdown_get_selected(gStorageDd) == 1;
  const char *lifetime = gPendingPersist ? "persist after reboot" : "until reboot";
  char summary[180];
  if (selected == COMMISSION_ALL) {
    int live = 0;
    censusCountsSafe(&live, nullptr, millis());
    snprintf(summary, sizeof(summary),
             "Set %d fresh fixtures to commission default '%s' %s. Each NVS "
             "change is sent as an exact-target command.",
             live, modeLabel(gPendingMode), lifetime);
    uiConfirm(summary, "Commission Default", applyAllYes, nullptr);
    return;
  }
  size_t target = selected - COMMISSION_FIRST_TARGET;
  if (target >= gTargetCount) return;
  memcpy(gPendingTarget, gTargets[target], sizeof(gPendingTarget));
  snprintf(summary, sizeof(summary),
           "Set %02X%02X%02X commission default to '%s' %s. Field-profile "
           "behavior is unchanged.",
           gPendingTarget[0], gPendingTarget[1], gPendingTarget[2],
           modeLabel(gPendingMode), lifetime);
  uiConfirm(summary, "Commission Default", applyOneYes, nullptr);
}

static void backCb(lv_event_t *) {
  if (gQueueTimer) {
    lv_timer_delete(gQueueTimer);
    gQueueTimer = nullptr;
  }
  gTargetDd = nullptr;
  gModeDd = nullptr;
  gStorageDd = nullptr;
  gInfo = nullptr;
  uiGoHome();
}

void appCommissionOpen() {
  lvglSetNavHooks(nullptr);
  uiShellSetTitle("Default");
  lv_obj_t *screen = lv_obj_create(nullptr);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  gTargetCount = snapshotFresh(gTargets, CENSUS_MAX_TRACKED, millis());
  static char options[1600];
  int offset = snprintf(options, sizeof(options), "ALL: targeted fresh");
  for (size_t i = 0; i < gTargetCount && offset > 0 &&
                     (size_t)offset < sizeof(options); ++i)
    offset += snprintf(options + offset, sizeof(options) - (size_t)offset,
                       "\n%02X%02X%02X", gTargets[i][0], gTargets[i][1],
                       gTargets[i][2]);

  lv_obj_t *targetLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(targetLabel, &lv_font_montserrat_14, 0);
  lv_label_set_text(targetLabel, "target");
  lv_obj_set_pos(targetLabel, 8, 40);
  gTargetDd = lv_dropdown_create(screen);
  lv_dropdown_set_options(gTargetDd, options);
  lv_dropdown_set_selected(gTargetDd,
                           gTargetCount ? COMMISSION_FIRST_TARGET : COMMISSION_ALL);
  lv_obj_set_pos(gTargetDd, 8, 58);
  lv_obj_set_width(gTargetDd, 148);

  lv_obj_t *modeLabelObj = lv_label_create(screen);
  lv_obj_set_style_text_font(modeLabelObj, &lv_font_montserrat_14, 0);
  lv_label_set_text(modeLabelObj, "no-command fallback");
  lv_obj_set_pos(modeLabelObj, 164, 40);
  gModeDd = lv_dropdown_create(screen);
  lv_dropdown_set_options(gModeDd, "ready beacon\nwildfire CA\nrails dark");
  lv_obj_set_pos(gModeDd, 164, 58);
  lv_obj_set_width(gModeDd, 148);

  lv_obj_t *storageLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(storageLabel, &lv_font_montserrat_14, 0);
  lv_label_set_text(storageLabel, "lifetime");
  lv_obj_set_pos(storageLabel, 8, 104);
  gStorageDd = lv_dropdown_create(screen);
  lv_dropdown_set_options(gStorageDd, "until reboot\npersist after reboot");
  lv_obj_set_pos(gStorageDd, 8, 122);
  lv_obj_set_width(gStorageDd, 148);

  gInfo = lv_label_create(screen);
  lv_obj_set_style_text_font(gInfo, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gInfo, 8, 166);
  lv_obj_set_width(gInfo, 304);
  lv_label_set_text(gInfo,
                    "commission profile only; leases still override\n"
                    "field schedule and safety stay unchanged");

  lv_obj_t *apply = lv_button_create(screen);
  lv_obj_set_size(apply, 120, 34);
  lv_obj_set_pos(apply, 8, 200);
  lv_obj_t *applyLabel = lv_label_create(apply);
  lv_label_set_text(applyLabel, "set default");
  lv_obj_center(applyLabel);
  lv_obj_add_event_cb(apply, applyCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *back = lv_button_create(screen);
  lv_obj_set_size(back, 100, 34);
  lv_obj_set_pos(back, 212, 200);
  lv_obj_t *backLabel = lv_label_create(back);
  lv_label_set_text(backLabel, LV_SYMBOL_LEFT " back");
  lv_obj_center(backLabel);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), gTargetDd);
  lv_group_add_obj(lvglGroup(), gModeDd);
  lv_group_add_obj(lvglGroup(), gStorageDd);
  lv_group_add_obj(lvglGroup(), apply);
  lv_group_add_obj(lvglGroup(), back);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(screen);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
