#include "app_field.h"

#include <lvgl.h>
#include <stdio.h>

#include "../net/mesh_tx.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_shell.h"
#include "ui_task.h"

namespace {

static lv_obj_t *gChime = nullptr;
static lv_obj_t *gShow = nullptr;
static lv_obj_t *gSeed = nullptr;
static lv_obj_t *gClear = nullptr;
static lv_obj_t *gStatus = nullptr;

static const uint8_t kChimeValues[] = {0, 32, 64, 128, 255};
static const uint16_t kSeedValues[] = {60, 120, 300, 600};
static const uint16_t kClearValues[] = {10, 30, 60};

static void applyYes(void *) {
  uint8_t chime = kChimeValues[lv_dropdown_get_selected(gChime)];
  uint8_t show = lv_dropdown_get_selected(gShow) == 0 ? 1 : 0;
  uint16_t seed = kSeedValues[lv_dropdown_get_selected(gSeed)];
  uint16_t clear = kClearValues[lv_dropdown_get_selected(gClear)];
  bool ok = meshFieldTuning(chime, show, seed, clear, true);
  if (gStatus)
    lv_label_set_text(gStatus, ok ? "sent; awake fixtures save"
                                  : "send refused / audit failed");
}

static void applyCb(lv_event_t *) {
  uint8_t chime = kChimeValues[lv_dropdown_get_selected(gChime)];
  uint16_t seed = kSeedValues[lv_dropdown_get_selected(gSeed)];
  uint16_t clear = kClearValues[lv_dropdown_get_selected(gClear)];
  char summary[220];
  snprintf(summary, sizeof(summary),
           "Persist on all awake fixtures: daytime chime chance %s, %s, "
           "presence seed no more than every %u s after %u s continuously "
           "clear. OFF and PROTECT still veto chimes.",
           chime == 255 ? "100%" :
           (chime == 128 ? "50%" :
            (chime == 64 ? "25%" : (chime == 32 ? "12.5%" : "off"))),
           lv_dropdown_get_selected(gShow) == 0 ? "four-mode rotation"
                                                : "current CA only",
           seed, clear);
  uiConfirm(summary, "Field Tune", applyYes, nullptr);
}

static void backCb(lv_event_t *) { uiGoHome(); }

static lv_obj_t *addRow(lv_obj_t *scr, const char *label, const char *options,
                        int y, uint32_t selected) {
  lv_obj_t *name = lv_label_create(scr);
  lv_label_set_text(name, label);
  lv_obj_set_pos(name, 8, y + 8);
  lv_obj_t *dd = lv_dropdown_create(scr);
  lv_dropdown_set_options(dd, options);
  lv_dropdown_set_selected(dd, selected);
  lv_obj_set_pos(dd, 132, y);
  lv_obj_set_width(dd, 178);
  return dd;
}

} // namespace

void appFieldOpen() {
  lvglSetNavHooks(nullptr);
  uiShellSetTitle("Field Tune");
  lv_obj_t *scr = lv_obj_create(nullptr);

  gChime = addRow(scr, "day crickets", "off\n12.5%\n25%\n50%\n100%",
                  29, 4);
  gShow = addRow(scr, "night show", "4-mode rotation\ncurrent CA only",
                 70, 0);
  gSeed = addRow(scr, "seed interval", "60 sec\n120 sec\n300 sec\n600 sec",
                 111, 2);
  gClear = addRow(scr, "clear re-arm", "10 sec\n30 sec\n60 sec", 152, 1);

  lv_obj_t *apply = lv_button_create(scr);
  lv_obj_set_pos(apply, 8, 198);
  lv_obj_set_size(apply, 115, 34);
  lv_obj_t *applyLabel = lv_label_create(apply);
  lv_label_set_text(applyLabel, LV_SYMBOL_OK " apply fleet");
  lv_obj_center(applyLabel);
  lv_obj_add_event_cb(apply, applyCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *back = lv_button_create(scr);
  lv_obj_set_pos(back, 225, 198);
  lv_obj_set_size(back, 85, 34);
  lv_obj_t *backLabel = lv_label_create(back);
  lv_label_set_text(backLabel, LV_SYMBOL_LEFT " back");
  lv_obj_center(backLabel);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  gStatus = lv_label_create(scr);
  lv_obj_set_style_text_font(gStatus, &lv_font_montserrat_14, 0);
  lv_label_set_text(gStatus, "");
  lv_obj_set_pos(gStatus, 126, 207);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), gChime);
  lv_group_add_obj(lvglGroup(), gShow);
  lv_group_add_obj(lvglGroup(), gSeed);
  lv_group_add_obj(lvglGroup(), gClear);
  lv_group_add_obj(lvglGroup(), apply);
  lv_group_add_obj(lvglGroup(), back);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
