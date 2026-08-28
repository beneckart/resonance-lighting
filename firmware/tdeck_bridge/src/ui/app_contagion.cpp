#include "app_contagion.h"

#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "../core/fleet_registry_generated.h"
#include "../core/health_model.h"
#include "../core/knock_plan.h"
#include "../core/target_picker_model.h"
#include "../net/census_svc.h"
#include "../net/contagion_fanout.h"
#include "../net/mesh_tx.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_shell.h"
#include "ui_task.h"

namespace {

static constexpr uint8_t PROG_CONTAGION_ID = 5;
static constexpr uint16_t CONTAGION_LEASE_S = 600;

struct Tempo {
  uint8_t infectedTicks;
  uint8_t immuneTicks;
  uint8_t tickDs;
};

static const Tempo kTempos[] = {
    {4, 8, 20}, // slow
    {3, 5, 10}, // medium
    {2, 3, 5},  // fast
};

static const uint8_t kHues[] = {8, 8, 85, 150, 205};

static lv_obj_t *gModeDd = nullptr;
static lv_obj_t *gOutputDd = nullptr;
static lv_obj_t *gTempoDd = nullptr;
static lv_obj_t *gColorDd = nullptr;
static lv_obj_t *gTargetSearch = nullptr;
static lv_obj_t *gTargetDd = nullptr;
static lv_obj_t *gTofButton = nullptr;
static lv_obj_t *gTofLabel = nullptr;
static lv_obj_t *gInfo = nullptr;
static TargetPickerItem gAllTargets[CENSUS_MAX_TRACKED] = {};
static size_t gAllTargetCount = 0;
static uint8_t gTargets[CENSUS_MAX_TRACKED][3] = {};
static size_t gTargetCount = 0;
static char gTargetOptions[4096] = {};

static const char *callsignForId(const uint8_t id[3]) {
  const HealthRegistryEntry *entry =
      healthRegistryFind(kHealthRegistry, kHealthRegistryCount, id);
  return entry ? entry->callsign : nullptr;
}

static size_t snapshotFresh(uint8_t out[][3], size_t outCap, uint32_t nowMs) {
  static CensusView rows[CENSUS_MAX_TRACKED];
  size_t count = censusSnapshotSafe(rows, CENSUS_MAX_TRACKED, nowMs);
  return targetPlanFresh(rows, count, censusFreshMsSafe(), out, outCap);
}

static void rebuildTargetOptions(const char *query) {
  uint8_t selectedId[3] = {};
  bool hadSelection = false;
  if (gTargetDd && gTargetCount > 0) {
    size_t selected = lv_dropdown_get_selected(gTargetDd);
    if (selected < gTargetCount) {
      memcpy(selectedId, gTargets[selected], sizeof(selectedId));
      hadSelection = true;
    }
  }

  gTargetCount = 0;
  int offset = 0;
  for (size_t i = 0; i < gAllTargetCount; ++i) {
    const TargetPickerItem &item = gAllTargets[i];
    if (!targetPickerMatches(item, query)) continue;
    if ((size_t)offset >= sizeof(gTargetOptions)) break;
    memcpy(gTargets[gTargetCount], item.id, sizeof(item.id));
    int wrote;
    if (item.callsign)
      wrote = snprintf(gTargetOptions + offset,
                       sizeof(gTargetOptions) - (size_t)offset,
                       "%s%s [%02X%02X%02X]", gTargetCount ? "\n" : "",
                       item.callsign, item.id[0], item.id[1], item.id[2]);
    else
      wrote = snprintf(gTargetOptions + offset,
                       sizeof(gTargetOptions) - (size_t)offset,
                       "%s%02X%02X%02X", gTargetCount ? "\n" : "",
                       item.id[0], item.id[1], item.id[2]);
    if (wrote < 0 || (size_t)wrote >=
                         sizeof(gTargetOptions) - (size_t)offset)
      break;
    offset += wrote;
    ++gTargetCount;
  }
  if (gTargetCount == 0)
    snprintf(gTargetOptions, sizeof(gTargetOptions), "no matching fixtures");

  if (!gTargetDd) return;
  lv_dropdown_set_options(gTargetDd, gTargetOptions);
  if (hadSelection) {
    for (size_t i = 0; i < gTargetCount; ++i) {
      if (memcmp(gTargets[i], selectedId, sizeof(selectedId)) == 0) {
        lv_dropdown_set_selected(gTargetDd, (uint32_t)i);
        break;
      }
    }
  }
}

static void targetSearchChanged(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_FOCUSED) {
    lv_group_set_editing(lvglGroup(), true);
  } else if (code == LV_EVENT_DEFOCUSED) {
    lv_group_set_editing(lvglGroup(), false);
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    rebuildTargetOptions(lv_textarea_get_text(gTargetSearch));
  } else if (code == LV_EVENT_READY) {
    lv_group_set_editing(lvglGroup(), false);
    if (gTargetDd) lv_group_focus_obj(gTargetDd);
  }
}

static bool tofEnabled() {
  return gTofButton && lv_obj_has_state(gTofButton, LV_STATE_CHECKED);
}

static uint8_t outputMode() {
  return gOutputDd ? (uint8_t)lv_dropdown_get_selected(gOutputDd) : 0;
}

static bool legacyFanout() { return outputMode() == 3; }

static uint8_t fixtureOutputMode() {
  return legacyFanout() ? 1 : outputMode();
}

static const char *outputName() {
  switch (outputMode()) {
  case 1: return "knocks";
  case 2: return "lights + knocks";
  case 3: return "legacy fleet roll";
  default: return "lights";
  }
}

static const char *modelName() {
  return gModeDd && lv_dropdown_get_selected(gModeDd) == 1 ? "Epidemic"
                                                           : "Color Virus";
}

static void paramsFromUi(uint8_t params[8], bool seedNow) {
  memset(params, 0, 8);
  uint8_t tempoIndex = gTempoDd ? (uint8_t)lv_dropdown_get_selected(gTempoDd) : 1;
  if (tempoIndex >= sizeof(kTempos) / sizeof(kTempos[0])) tempoIndex = 1;
  uint8_t colorIndex = gColorDd ? (uint8_t)lv_dropdown_get_selected(gColorDd) : 0;
  if (colorIndex >= sizeof(kHues) / sizeof(kHues[0])) colorIndex = 0;
  params[0] = gModeDd ? (uint8_t)lv_dropdown_get_selected(gModeDd) : 0;
  params[1] = fixtureOutputMode();
  params[2] = kTempos[tempoIndex].infectedTicks;
  params[3] = kTempos[tempoIndex].immuneTicks;
  params[4] = kTempos[tempoIndex].tickDs;
  params[5] = kHues[colorIndex];
  params[6] = tofEnabled() ? 1 : 0;
  params[7] = (uint8_t)((seedNow ? 0x01 : 0) |
                        (colorIndex == 0 ? 0x02 : 0));
}

static void tofChanged(lv_event_t *) {
  if (gTofLabel)
    lv_label_set_text(gTofLabel, tofEnabled() ? "ToF: ON" : "ToF: off");
}

static void startYes(void *) {
  static const uint8_t all[3] = {0, 0, 0};
  uint8_t params[8];
  paramsFromUi(params, false);
  if (legacyFanout() && gTargetDd && gTargetCount > 0) {
    size_t selected = lv_dropdown_get_selected(gTargetDd);
    if (selected < gTargetCount)
      contagionFanoutConfigure(gTargets[selected], CONTAGION_LEASE_S * 1000UL,
                               40);
  } else {
    contagionFanoutDisable();
  }
  meshProgramLease(all, PROG_CONTAGION_ID, CONTAGION_LEASE_S, 0x01, params);
  if (gInfo)
    if (legacyFanout()) {
      size_t selected = lv_dropdown_get_selected(gTargetDd);
      lv_label_set_text_fmt(gInfo, "legacy roll armed on %02X%02X%02X",
                            gTargets[selected][0], gTargets[selected][1],
                            gTargets[selected][2]);
    } else {
      lv_label_set_text_fmt(gInfo, "%s / %s%s - seed one fixture",
                            modelName(), outputName(),
                            tofEnabled() ? " / ToF" : "");
    }
}

static void startCb(lv_event_t *) {
  if (legacyFanout() && (!gTargetDd || gTargetCount == 0)) {
    if (gInfo) lv_label_set_text(gInfo, "legacy roll needs one fresh source");
    return;
  }
  char summary[280];
  if (legacyFanout()) {
    size_t selected = lv_dropdown_get_selected(gTargetDd);
    const char *callsign = callsignForId(gTargets[selected]);
    char source[48];
    if (callsign)
      snprintf(source, sizeof(source), "%s [%02X%02X%02X]", callsign,
               gTargets[selected][0], gTargets[selected][1],
               gTargets[selected][2]);
    else
      snprintf(source, sizeof(source), "%02X%02X%02X", gTargets[selected][0],
               gTargets[selected][1], gTargets[selected][2]);
    snprintf(summary, sizeof(summary),
             "Arm %s as the Contagion source for 10 minutes. Its infection "
             "starts one proven 40 ms targeted roll over fresh downlights. "
             "Hard mechanism gates still decide each strike.", source);
  } else {
    snprintf(summary, sizeof(summary),
             "Start %s on all awake updated fixtures for 10 minutes: %s%s. "
             "%s",
             modelName(), outputName(), tofEnabled() ? ", ToF seeds ON" : "",
             outputMode() == 0
                 ? "The fleet stays susceptible until a manual or ToF seed."
                 : "Each downlight infection may request one 40 ms knock; autonomous energy and mechanism gates still decide.");
  }
  uiConfirm(summary, "Contagion", startYes, nullptr);
}

static void seedYes(void *) {
  if (!gTargetDd || gTargetCount == 0) return;
  size_t selected = lv_dropdown_get_selected(gTargetDd);
  if (selected >= gTargetCount) return;
  uint8_t params[8];
  paramsFromUi(params, true);
  meshProgramLease(gTargets[selected], PROG_CONTAGION_ID, CONTAGION_LEASE_S,
                   0x01, params);
  if (gInfo)
    lv_label_set_text_fmt(gInfo, "seeded %02X%02X%02X - watch the spread",
                          gTargets[selected][0], gTargets[selected][1],
                          gTargets[selected][2]);
}

static void seedCb(lv_event_t *) {
  if (!gTargetDd || gTargetCount == 0) {
    if (gInfo) lv_label_set_text(gInfo, "no fresh fixture available to seed");
    return;
  }
  size_t selected = lv_dropdown_get_selected(gTargetDd);
  if (selected >= gTargetCount) return;
  const char *callsign = callsignForId(gTargets[selected]);
  char target[48];
  if (callsign)
    snprintf(target, sizeof(target), "%s [%02X%02X%02X]", callsign,
             gTargets[selected][0], gTargets[selected][1],
             gTargets[selected][2]);
  else
    snprintf(target, sizeof(target), "%02X%02X%02X", gTargets[selected][0],
             gTargets[selected][1], gTargets[selected][2]);
  char summary[200];
  snprintf(summary, sizeof(summary),
           "Seed %s with %s / %s. %s",
           target, modelName(), outputName(),
           outputMode() == 0
               ? "This is one exact-target infection."
               : legacyFanout()
                     ? "If this is the armed source, it starts one targeted downlight roll."
                     : "Each downlight infection may request one 40 ms knock; autonomous gates still decide.");
  uiConfirm(summary, "Contagion", seedYes, nullptr);
}

static void stopCb(lv_event_t *) {
  static const uint8_t all[3] = {0, 0, 0};
  contagionFanoutDisable();
  meshProgramLease(all, 0, 0, 0x01, nullptr);
  if (gInfo) lv_label_set_text(gInfo, "stopped -> autonomous behavior");
}

static lv_obj_t *makeDropdown(lv_obj_t *screen, const char *options, int x,
                              int y, int width) {
  lv_obj_t *dropdown = lv_dropdown_create(screen);
  lv_dropdown_set_options(dropdown, options);
  lv_obj_set_pos(dropdown, x, y);
  lv_obj_set_width(dropdown, width);
  lv_group_add_obj(lvglGroup(), dropdown);
  return dropdown;
}

static void backCb(lv_event_t *) {
  gModeDd = nullptr;
  gOutputDd = nullptr;
  gTempoDd = nullptr;
  gColorDd = nullptr;
  gTargetSearch = nullptr;
  gTargetDd = nullptr;
  gTofButton = nullptr;
  gTofLabel = nullptr;
  gInfo = nullptr;
  lv_group_set_editing(lvglGroup(), false);
  uiGoHome();
}

} // namespace

void appContagionOpen() {
  lvglSetNavHooks(nullptr);
  uiShellSetTitle("Contagion");
  lv_obj_t *screen = lv_obj_create(nullptr);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_group_remove_all_objs(lvglGroup());
  gModeDd = makeDropdown(screen, "Color Virus\nEpidemic", 8, 31, 148);
  gOutputDd =
      makeDropdown(screen, "lights\nknocks\nlights + knocks\nlegacy fleet roll",
                   164, 31, 148);
  gTempoDd = makeDropdown(screen, "slow\nmedium\nfast", 8, 73, 148);
  lv_dropdown_set_selected(gTempoDd, 1);
  gColorDd =
      makeDropdown(screen, "random\nember\ngreen\nocean\nviolet", 164, 73, 148);

  uint8_t fresh[CENSUS_MAX_TRACKED][3] = {};
  gAllTargetCount = snapshotFresh(fresh, CENSUS_MAX_TRACKED, millis());
  for (size_t i = 0; i < gAllTargetCount; ++i) {
    memcpy(gAllTargets[i].id, fresh[i], sizeof(gAllTargets[i].id));
    gAllTargets[i].callsign = callsignForId(fresh[i]);
  }
  targetPickerSort(gAllTargets, gAllTargetCount);

  gTargetSearch = lv_textarea_create(screen);
  lv_textarea_set_one_line(gTargetSearch, true);
  lv_textarea_set_max_length(gTargetSearch, 20);
  lv_textarea_set_placeholder_text(gTargetSearch, "find name / ID");
  lv_obj_set_pos(gTargetSearch, 8, 116);
  lv_obj_set_size(gTargetSearch, 110, 34);
  lv_obj_add_event_cb(gTargetSearch, targetSearchChanged, LV_EVENT_ALL, nullptr);
  lv_group_add_obj(lvglGroup(), gTargetSearch);

  gTargetDd = makeDropdown(screen, "no fresh fixtures", 124, 116, 188);
  rebuildTargetOptions("");
  gTofButton = lv_button_create(screen);
  lv_obj_add_flag(gTofButton, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_size(gTofButton, 100, 34);
  lv_obj_set_pos(gTofButton, 8, 157);
  gTofLabel = lv_label_create(gTofButton);
  lv_label_set_text(gTofLabel, "ToF: off");
  lv_obj_center(gTofLabel);
  lv_obj_add_event_cb(gTofButton, tofChanged, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_group_add_obj(lvglGroup(), gTofButton);

  gInfo = lv_label_create(screen);
  lv_obj_set_style_text_font(gInfo, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gInfo, 116, 155);
  lv_obj_set_width(gInfo, 196);
  lv_label_set_text(gInfo, "start, then seed one fixture\nperimeter ToF: hover palm");

  lv_obj_t *start = lv_button_create(screen);
  lv_obj_set_size(start, 72, 32);
  lv_obj_set_pos(start, 8, 202);
  lv_obj_t *startLabel = lv_label_create(start);
  lv_label_set_text(startLabel, LV_SYMBOL_PLAY " start");
  lv_obj_center(startLabel);
  lv_obj_add_event_cb(start, startCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *seed = lv_button_create(screen);
  lv_obj_set_size(seed, 72, 32);
  lv_obj_set_pos(seed, 86, 202);
  lv_obj_t *seedLabel = lv_label_create(seed);
  lv_label_set_text(seedLabel, "seed");
  lv_obj_center(seedLabel);
  lv_obj_add_event_cb(seed, seedCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *stop = lv_button_create(screen);
  lv_obj_set_size(stop, 72, 32);
  lv_obj_set_pos(stop, 164, 202);
  lv_obj_t *stopLabel = lv_label_create(stop);
  lv_label_set_text(stopLabel, LV_SYMBOL_STOP " stop");
  lv_obj_center(stopLabel);
  lv_obj_add_event_cb(stop, stopCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *back = lv_button_create(screen);
  lv_obj_set_size(back, 72, 32);
  lv_obj_set_pos(back, 242, 202);
  lv_obj_t *backLabel = lv_label_create(back);
  lv_label_set_text(backLabel, LV_SYMBOL_LEFT " back");
  lv_obj_center(backLabel);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  lv_group_add_obj(lvglGroup(), start);
  lv_group_add_obj(lvglGroup(), seed);
  lv_group_add_obj(lvglGroup(), stop);
  lv_group_add_obj(lvglGroup(), back);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(screen);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
