#include <lvgl.h>

#include <Arduino.h>
#include <string.h>

#include "../core/knock_plan.h"
#include "../net/census_svc.h"
#include "../net/mesh_tx.h"
#include "app_knocker.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_task.h"

static lv_obj_t *gTargetDd = nullptr;
static lv_obj_t *gPulseSlider = nullptr;
static lv_obj_t *gPulseLabel = nullptr;
static lv_obj_t *gInfo = nullptr;
static uint8_t gTargets[CENSUS_MAX_TRACKED][3];
static size_t gTargetCount = 0;

// A roll remains targeted because fixtures reject broadcast strikes. It is not
// synchronized: one request is dispatched every 80 ms in deterministic ID
// order. True synchronized fire needs the separately tracked scheduled-event
// fixture seam.
static constexpr uint32_t KNOCK_ROLL_STEP_MS = 80;
static uint8_t gQueue[CENSUS_MAX_TRACKED][3];
static size_t gQueueLen = 0, gQueueNext = 0;
static uint16_t gQueuePulse = 40;
static lv_timer_t *gQueueTimer = nullptr;

static uint16_t pulseMs() {
  return gPulseSlider ? (uint16_t)lv_slider_get_value(gPulseSlider) : 40;
}

static void queueTick(lv_timer_t *t) {
  if (gQueueNext >= gQueueLen) {
    lv_timer_delete(t);
    gQueueTimer = nullptr;
    if (gInfo)
      lv_label_set_text_fmt(gInfo, "sent %u targeted strike requests",
                            (unsigned)gQueueLen);
    return;
  }
  meshStrike(gQueue[gQueueNext], gQueuePulse);
  ++gQueueNext;
  if (gInfo)
    lv_label_set_text_fmt(gInfo, "rolling... %u/%u", (unsigned)gQueueNext,
                          (unsigned)gQueueLen);
}

static size_t snapshotFresh(uint8_t out[][3], size_t outCap, uint32_t now) {
  static CensusView rows[CENSUS_MAX_TRACKED];
  size_t n = censusSnapshotSafe(rows, CENSUS_MAX_TRACKED, now);
  return knockPlanFresh(rows, n, censusFreshMsSafe(), out, outCap);
}

static void knockAllYes(void *) {
  if (gQueueTimer) {
    if (gInfo) lv_label_set_text(gInfo, "roll already in progress");
    return;
  }
  uint32_t now = millis();
  gQueueNext = 0;
  gQueuePulse = pulseMs();
  gQueueLen = snapshotFresh(gQueue, CENSUS_MAX_TRACKED, now);
  if (gQueueLen == 0) {
    if (gInfo) lv_label_set_text(gInfo, "no fresh fixtures to knock");
    return;
  }
  gQueueTimer = lv_timer_create(queueTick, KNOCK_ROLL_STEP_MS, nullptr);
}

static void knockCb(lv_event_t *) {
  uint32_t sel = lv_dropdown_get_selected(gTargetDd);
  if (sel == 0) {  // targeted roll over all fresh fixtures
    if (gQueueTimer) {
      if (gInfo) lv_label_set_text(gInfo, "roll already in progress");
      return;
    }
    int live = 0;
    censusCountsSafe(&live, nullptr, millis());
    unsigned seconds = (unsigned)(((uint32_t)(live > 0 ? live : 0) *
                                   KNOCK_ROLL_STEP_MS + 999) /
                                  1000);
    char summary[112];
    snprintf(summary, sizeof(summary),
             "Targeted roll over %d fresh fixtures, %u ms each, about %u s. "
             "Not synchronized.",
             live, pulseMs(), seconds);
    uiConfirm(summary, "Knocker", knockAllYes, nullptr);
    return;
  }
  if (sel - 1 < gTargetCount) {
    meshStrike(gTargets[sel - 1], pulseMs());
    if (gInfo)
      lv_label_set_text_fmt(gInfo, "struck %02X%02X%02X (%u ms)",
                            gTargets[sel - 1][0], gTargets[sel - 1][1],
                            gTargets[sel - 1][2], pulseMs());
  }
}

static void pulseChanged(lv_event_t *) {
  if (gPulseLabel)
    lv_label_set_text_fmt(gPulseLabel, "pulse %u ms", pulseMs());
}

static void backCb(lv_event_t *) {
  if (gQueueTimer) {
    lv_timer_delete(gQueueTimer);
    gQueueTimer = nullptr;
  }
  gTargetDd = nullptr;
  gPulseSlider = nullptr;
  gPulseLabel = nullptr;
  gInfo = nullptr;
  uiGoHome();
}

void appKnockerOpen() {
  lvglSetNavHooks(nullptr);
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(scr);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_label_set_text(title, "Knocker");
  lv_obj_set_pos(title, 8, 6);

  // Target list: explicit targeted roll + every fresh census entry.
  gTargetCount = snapshotFresh(gTargets, CENSUS_MAX_TRACKED, millis());
  static char opts[1600];
  int o = snprintf(opts, sizeof(opts), "ROLL ALL (targeted)");
  for (size_t i = 0; i < gTargetCount && o > 0 && (size_t)o < sizeof(opts); ++i)
    o += snprintf(opts + o, sizeof(opts) - (size_t)o, "\n%02X%02X%02X",
                  gTargets[i][0], gTargets[i][1], gTargets[i][2]);
  gTargetDd = lv_dropdown_create(scr);
  lv_dropdown_set_options(gTargetDd, opts);
  lv_dropdown_set_selected(gTargetDd, gTargetCount ? 1 : 0);
  lv_obj_set_pos(gTargetDd, 8, 44);
  lv_obj_set_width(gTargetDd, 150);

  gPulseLabel = lv_label_create(scr);
  lv_obj_set_pos(gPulseLabel, 8, 92);
  lv_label_set_text(gPulseLabel, "pulse 40 ms");
  gPulseSlider = lv_slider_create(scr);
  lv_obj_set_size(gPulseSlider, 200, 14);
  lv_obj_set_pos(gPulseSlider, 8, 116);
  lv_slider_set_range(gPulseSlider, 5, 300);
  lv_slider_set_value(gPulseSlider, 40, LV_ANIM_OFF);
  lv_obj_add_event_cb(gPulseSlider, pulseChanged, LV_EVENT_VALUE_CHANGED, nullptr);

  gInfo = lv_label_create(scr);
  lv_obj_set_style_text_font(gInfo, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gInfo, 8, 144);
  lv_label_set_text(gInfo,
                    "fixtures refuse at night / low power\n"
                    "all = targeted 80 ms roll, not synchronized");

  lv_obj_t *knock = lv_button_create(scr);
  lv_obj_set_size(knock, 140, 34);
  lv_obj_set_pos(knock, 8, 200);
  lv_obj_t *kl = lv_label_create(knock);
  lv_label_set_text(kl, LV_SYMBOL_BELL " knock");
  lv_obj_center(kl);
  lv_obj_add_event_cb(knock, knockCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *back = lv_button_create(scr);
  lv_obj_set_size(back, 100, 34);
  lv_obj_set_pos(back, 212, 200);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, LV_SYMBOL_LEFT " back");
  lv_obj_center(bl);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), gTargetDd);
  lv_group_add_obj(lvglGroup(), gPulseSlider);
  lv_group_add_obj(lvglGroup(), knock);
  lv_group_add_obj(lvglGroup(), back);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
