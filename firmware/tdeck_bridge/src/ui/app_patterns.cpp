#include "app_patterns.h"

#include <Arduino.h>
#include <lvgl.h>

#include "lvgl_glue.h"
#include "ui_task.h"

namespace {

static PatternStreamHooks gHooks = {};
static lv_obj_t *gModeDd = nullptr;
static lv_obj_t *gPaletteDd = nullptr;
static lv_obj_t *gClassDd = nullptr;
static lv_obj_t *gCohortDd = nullptr;
static lv_obj_t *gSpeedSlider = nullptr;
static lv_obj_t *gIntensitySlider = nullptr;
static lv_obj_t *gSpeedLabel = nullptr;
static lv_obj_t *gIntensityLabel = nullptr;
static lv_obj_t *gInfo = nullptr;
static lv_timer_t *gInfoTimer = nullptr;
static bool gRequestedActive = false;

static PatternSettings settingsFromUi() {
  PatternSettings settings = patternDefaultSettings();
  if (gModeDd)
    settings.mode = (PatternMode)lv_dropdown_get_selected(gModeDd);
  if (gPaletteDd)
    settings.palette = (PatternPalette)lv_dropdown_get_selected(gPaletteDd);
  if (gClassDd)
    settings.classFilter = (uint8_t)lv_dropdown_get_selected(gClassDd);
  if (gCohortDd)
    settings.cohort = (PatternCohort)lv_dropdown_get_selected(gCohortDd);
  if (gSpeedSlider)
    settings.speed = (uint8_t)lv_slider_get_value(gSpeedSlider);
  if (gIntensitySlider)
    settings.intensity = (uint8_t)lv_slider_get_value(gIntensitySlider);
  return patternSanitize(settings);
}

static bool streamIsActive() {
  return gRequestedActive && gHooks.active && gHooks.active();
}

static void refreshInfo(lv_timer_t *) {
  if (!gInfo) return;
  if (!gRequestedActive) {
    lv_label_set_text(gInfo, "stopped; fixtures revert in about 3 s");
    return;
  }
  if (!streamIsActive()) {
    gRequestedActive = false;
    lv_label_set_text(gInfo, "stopped or replaced by another stream");
    return;
  }
  int targets = gHooks.targetCount ? gHooks.targetCount() : 0;
  lv_label_set_text_fmt(gInfo, "running -> %d fresh fixtures", targets);
}

static void startPattern() {
  PatternSettings settings = settingsFromUi();
  if (!gHooks.start || !gHooks.active) {
    gRequestedActive = false;
    if (gInfo) lv_label_set_text(gInfo, "stream integration pending");
    return;
  }
  gRequestedActive = gHooks.start(settings, millis());
  if (!gRequestedActive && gInfo)
    lv_label_set_text(gInfo, "stream start refused");
  refreshInfo(nullptr);
}

static void controlsChanged(lv_event_t *) {
  if (gSpeedLabel && gSpeedSlider)
    lv_label_set_text_fmt(gSpeedLabel, "speed %d",
                          (int)lv_slider_get_value(gSpeedSlider));
  if (gIntensityLabel && gIntensitySlider)
    lv_label_set_text_fmt(gIntensityLabel, "intensity %d",
                          (int)lv_slider_get_value(gIntensitySlider));
  if (streamIsActive()) startPattern();
}

static void startCb(lv_event_t *) { startPattern(); }

static void stopCb(lv_event_t *) {
  if (gHooks.stop) gHooks.stop();
  gRequestedActive = false;
  refreshInfo(nullptr);
}

static void backCb(lv_event_t *) {
  // Like LED Studio, an active pattern survives app switching. Stop is always
  // explicit, while a different stream owner may replace it.
  if (gInfoTimer) {
    lv_timer_delete(gInfoTimer);
    gInfoTimer = nullptr;
  }
  gModeDd = nullptr;
  gPaletteDd = nullptr;
  gClassDd = nullptr;
  gCohortDd = nullptr;
  gSpeedSlider = nullptr;
  gIntensitySlider = nullptr;
  gSpeedLabel = nullptr;
  gIntensityLabel = nullptr;
  gInfo = nullptr;
  uiGoHome();
}

static lv_obj_t *makeDropdown(lv_obj_t *screen, const char *options, int x,
                              int y) {
  lv_obj_t *dropdown = lv_dropdown_create(screen);
  lv_dropdown_set_options(dropdown, options);
  lv_obj_set_pos(dropdown, x, y);
  lv_obj_set_width(dropdown, 145);
  lv_obj_add_event_cb(dropdown, controlsChanged, LV_EVENT_VALUE_CHANGED,
                      nullptr);
  lv_group_add_obj(lvglGroup(), dropdown);
  return dropdown;
}

}  // namespace

void appPatternsSetStreamHooks(const PatternStreamHooks *hooks) {
  gHooks = hooks ? *hooks : PatternStreamHooks{};
}

void appPatternsOpen() {
  lvglSetNavHooks(nullptr);
  lv_obj_t *screen = lv_obj_create(nullptr);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(screen);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_label_set_text(title, "Patterns");
  lv_obj_set_pos(title, 8, 2);

  gInfo = lv_label_create(screen);
  lv_obj_set_style_text_font(gInfo, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gInfo, 126, 8);

  lv_group_remove_all_objs(lvglGroup());
  gPaletteDd = makeDropdown(screen, "Ember\nForest\nOcean\nAurora\nMoon", 8,
                            32);
  gModeDd = makeDropdown(screen, "Wash\nChase\nWave\nTwinkle", 167, 32);
  lv_dropdown_set_selected(gModeDd, (uint16_t)PatternMode::CHASE);

  gClassDd = makeDropdown(
      screen, "all classes\ndownlights\nperimeter\nuplights\nchandelier", 8,
      75);
  gCohortDd = makeDropdown(screen, "all cohorts\ncohort A\ncohort B\ncohort C\ncohort D",
                           167, 75);

  gSpeedLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(gSpeedLabel, &lv_font_montserrat_14, 0);
  lv_label_set_text(gSpeedLabel, "speed 4");
  lv_obj_set_pos(gSpeedLabel, 8, 119);
  gSpeedSlider = lv_slider_create(screen);
  lv_obj_set_size(gSpeedSlider, 210, 12);
  lv_obj_set_pos(gSpeedSlider, 98, 122);
  lv_slider_set_range(gSpeedSlider, 1, 8);
  lv_slider_set_value(gSpeedSlider, 4, LV_ANIM_OFF);
  lv_obj_add_event_cb(gSpeedSlider, controlsChanged, LV_EVENT_VALUE_CHANGED,
                      nullptr);
  lv_group_add_obj(lvglGroup(), gSpeedSlider);

  gIntensityLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(gIntensityLabel, &lv_font_montserrat_14, 0);
  lv_label_set_text(gIntensityLabel, "intensity 128");
  lv_obj_set_pos(gIntensityLabel, 8, 153);
  gIntensitySlider = lv_slider_create(screen);
  lv_obj_set_size(gIntensitySlider, 190, 12);
  lv_obj_set_pos(gIntensitySlider, 118, 156);
  lv_slider_set_range(gIntensitySlider, 8, 255);
  lv_slider_set_value(gIntensitySlider, 128, LV_ANIM_OFF);
  lv_obj_add_event_cb(gIntensitySlider, controlsChanged,
                      LV_EVENT_VALUE_CHANGED, nullptr);
  lv_group_add_obj(lvglGroup(), gIntensitySlider);

  lv_obj_t *start = lv_button_create(screen);
  lv_obj_set_size(start, 96, 34);
  lv_obj_set_pos(start, 8, 200);
  lv_obj_t *startLabel = lv_label_create(start);
  lv_label_set_text(startLabel, LV_SYMBOL_PLAY " start");
  lv_obj_center(startLabel);
  lv_obj_add_event_cb(start, startCb, LV_EVENT_CLICKED, nullptr);
  lv_group_add_obj(lvglGroup(), start);

  lv_obj_t *stop = lv_button_create(screen);
  lv_obj_set_size(stop, 96, 34);
  lv_obj_set_pos(stop, 112, 200);
  lv_obj_t *stopLabel = lv_label_create(stop);
  lv_label_set_text(stopLabel, LV_SYMBOL_STOP " stop");
  lv_obj_center(stopLabel);
  lv_obj_add_event_cb(stop, stopCb, LV_EVENT_CLICKED, nullptr);
  lv_group_add_obj(lvglGroup(), stop);

  lv_obj_t *back = lv_button_create(screen);
  lv_obj_set_size(back, 96, 34);
  lv_obj_set_pos(back, 216, 200);
  lv_obj_t *backLabel = lv_label_create(back);
  lv_label_set_text(backLabel, LV_SYMBOL_LEFT " back");
  lv_obj_center(backLabel);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);
  lv_group_add_obj(lvglGroup(), back);

  gRequestedActive = streamIsActive();
  refreshInfo(nullptr);
  gInfoTimer = lv_timer_create(refreshInfo, 500, nullptr);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(screen);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
