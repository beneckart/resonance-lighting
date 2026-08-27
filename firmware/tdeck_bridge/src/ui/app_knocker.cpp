#include <lvgl.h>

#include <Arduino.h>
#include <string.h>

#include "../core/fleet_registry_generated.h"
#include "../core/health_model.h"
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

static constexpr uint32_t KNOCK_SYNC_DELAY_MS = 1000;
static constexpr uint32_t KNOCK_MODE_ROLL = 0;
static constexpr uint32_t KNOCK_MODE_BROADCAST = 1;
static constexpr uint32_t KNOCK_MODE_SYNC = 2;
static constexpr uint32_t KNOCK_FIRST_TARGET = 3;

// The legacy roll remains targeted and intentionally staggered: one request is
// dispatched every 80 ms in deterministic ID order. The two new fleet modes
// use a separately deduplicated broadcast event.
static constexpr uint32_t KNOCK_ROLL_STEP_MS = 80;
static uint8_t gQueue[CENSUS_MAX_TRACKED][3];
static size_t gQueueLen = 0, gQueueNext = 0;
static uint16_t gQueuePulse = 40;
static lv_timer_t *gQueueTimer = nullptr;

static uint16_t pulseMs() {
  return gPulseSlider ? (uint16_t)lv_slider_get_value(gPulseSlider) : 40;
}

static const char *callsignForId(const uint8_t id[3]) {
  const HealthRegistryEntry *entry =
      healthRegistryFind(kHealthRegistry, kHealthRegistryCount, id);
  return entry ? entry->callsign : nullptr;
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

static void knockBroadcastYes(void *) {
  if (meshStrikeBroadcast(pulseMs(), 0)) {
    if (gInfo) lv_label_set_text(gInfo, "immediate multicast sent");
  } else if (gInfo) {
    lv_label_set_text(gInfo, "multicast send refused");
  }
}

static void knockSyncYes(void *) {
  if (meshStrikeBroadcast(pulseMs(), KNOCK_SYNC_DELAY_MS)) {
    if (gInfo) lv_label_set_text(gInfo, "sync event armed: fire in 1.0 s");
  } else if (gInfo) {
    lv_label_set_text(gInfo, "sync event send refused");
  }
}

static void knockCb(lv_event_t *) {
  uint32_t sel = lv_dropdown_get_selected(gTargetDd);
  if (sel == KNOCK_MODE_ROLL) {
    if (gQueueTimer) {
      if (gInfo) lv_label_set_text(gInfo, "roll already in progress");
      return;
    }
    int live = 0;
    censusCountsSafe(&live, nullptr, millis());
    unsigned seconds = (unsigned)(((uint32_t)(live > 0 ? live : 0) *
                                   KNOCK_ROLL_STEP_MS + 999) /
                                  1000);
    char summary[144];
    snprintf(summary, sizeof(summary),
             "Targeted roll over %d fresh fixtures, %u ms each, about %u s. "
             "Every received command attempts the mechanism.",
             live, pulseMs(), seconds);
    uiConfirm(summary, "Knocker", knockAllYes, nullptr);
    return;
  }
  if (sel == KNOCK_MODE_BROADCAST) {
    char summary[128];
    snprintf(summary, sizeof(summary),
             "Immediate multicast to all updated awake fixtures, %u ms. "
             "Reception is asynchronous; hard mechanism gates remain.", pulseMs());
    uiConfirm(summary, "Knocker", knockBroadcastYes, nullptr);
    return;
  }
  if (sel == KNOCK_MODE_SYNC) {
    char summary[128];
    snprintf(summary, sizeof(summary),
             "Multicast to all updated awake fixtures, %u ms, firing at one "
             "shared +1.0 s deadline. Hard mechanism gates remain.", pulseMs());
    uiConfirm(summary, "Knocker", knockSyncYes, nullptr);
    return;
  }
  if (sel >= KNOCK_FIRST_TARGET && sel - KNOCK_FIRST_TARGET < gTargetCount) {
    size_t target = sel - KNOCK_FIRST_TARGET;
    meshStrike(gTargets[target], pulseMs());
    if (gInfo)
      lv_label_set_text_fmt(gInfo, "struck %02X%02X%02X (%u ms)",
                            gTargets[target][0], gTargets[target][1],
                            gTargets[target][2], pulseMs());
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

  // Fleet modes followed by every fresh census entry for one-device strikes.
  gTargetCount = snapshotFresh(gTargets, CENSUS_MAX_TRACKED, millis());
  // 192 entries x (7-char callsign + " [ABCDEF]" + newline), plus fleet modes.
  static char opts[4096];
  int o = snprintf(opts, sizeof(opts),
                   "ALL: targeted roll\nALL: broadcast now\nALL: sync +1.0s");
  for (size_t i = 0; i < gTargetCount && o > 0 && (size_t)o < sizeof(opts); ++i) {
    const char *callsign = callsignForId(gTargets[i]);
    if (callsign)
      o += snprintf(opts + o, sizeof(opts) - (size_t)o,
                    "\n%s [%02X%02X%02X]", callsign, gTargets[i][0],
                    gTargets[i][1], gTargets[i][2]);
    else
      o += snprintf(opts + o, sizeof(opts) - (size_t)o, "\n%02X%02X%02X",
                    gTargets[i][0], gTargets[i][1], gTargets[i][2]);
  }
  gTargetDd = lv_dropdown_create(scr);
  lv_dropdown_set_options(gTargetDd, opts);
  lv_dropdown_set_selected(gTargetDd, gTargetCount ? KNOCK_FIRST_TARGET
                                                   : KNOCK_MODE_ROLL);
  lv_obj_set_pos(gTargetDd, 8, 44);
  lv_obj_set_width(gTargetDd, 200);

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
                    "operator knock ignores solar/tier/lifecycle\n"
                    "all: roll | broadcast | sync +1s");

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
