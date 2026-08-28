#include <lvgl.h>

#include <Arduino.h>

#include "../core/battery_model.h"
#include "../hal/hal_board.h"
#include "../hal/hal_display.h"
#include "../net/census_svc.h"
#include "../net/net_mgr.h"
#include "../net/stream_svc.h"
#include "../store/store.h"
#include "app_ca.h"
#include "app_chat.h"
#include "app_commission.h"
#include "app_contagion.h"
#include "app_fleet.h"
#include "app_health.h"
#include "app_knocker.h"
#include "app_power.h"
#include "app_patterns.h"
#include "app_rf.h"
#include "app_schedule.h"
#include "app_settings.h"
#include "app_zones.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_shell.h"
#include "ui_theme.h"
#include "ui_task.h"

static bool gActive = false;
static lv_obj_t *gLauncher = nullptr;

// ---------------------------------------------------------------- launcher --

struct Tile {
  const char *symbol;
  const char *name;
  void (*open)();
};

static void openSunTest();
static void openPatterns();
static void openLocatePlaceholder();

// The launcher owns registration only. Each app owns its screen lifecycle and
// returns through uiGoHome(); adding an app does not extend a string dispatch
// chain or require a shared base class.
static const Tile kTiles[] = {
    {LV_SYMBOL_KEYBOARD, "Claude", appChatOpen},
    {LV_SYMBOL_LIST, "Fleet", appFleetOpen},
    {LV_SYMBOL_BATTERY_FULL, "Health", appHealthOpen},
    {LV_SYMBOL_TINT, "LEDs", appZonesOpen},
    {LV_SYMBOL_BELL, "Knock", appKnockerOpen},
    {LV_SYMBOL_PLAY, "Patterns", openPatterns},
    {LV_SYMBOL_LOOP, "CA", appCaOpen},
    {LV_SYMBOL_SHUFFLE, "Contagion", appContagionOpen},
    {LV_SYMBOL_HOME, "Default", appCommissionOpen},
    {LV_SYMBOL_GPS, "Wake", appScheduleOpen},
    {LV_SYMBOL_EYE_OPEN, "Locate", openLocatePlaceholder},
    {LV_SYMBOL_WIFI, "RF", appRfOpen},
    {LV_SYMBOL_POWER, "Rest", appPowerOpen},
    {LV_SYMBOL_IMAGE, "SunTest", openSunTest},
    {LV_SYMBOL_SETTINGS, "Settings", appSettingsOpen},
};

static void buildLauncher();

// Launcher trackball semantics: up/down jump a whole grid row (Ben's model);
// left/right step tiles via the default focus walk.
static bool launcherVertical(int dir) {
  lv_group_t *g = lvglGroup();
  lv_obj_t *focused = g ? lv_group_get_focused(g) : nullptr;
  if (!focused || !gLauncher) return false;
  int32_t idx = lv_obj_get_index(focused);
  if (idx < 0) return false;
  int32_t target = idx + dir * 4;  // 4 columns
  int32_t tiles = (int32_t)(sizeof(kTiles) / sizeof(kTiles[0]));
  if (target < 0 || target >= tiles) return true;  // consume at grid edge
  lv_obj_t *btn = lv_obj_get_child(gLauncher, target);
  if (btn) lv_group_focus_obj(btn);
  return true;
}
static const UiNavHooks kLauncherHooks = {launcherVertical, nullptr};

void uiGoHome() {
  lv_obj_t *old = lv_screen_active();
  buildLauncher();
  uiShellSetTitle("Home");
  lvglSetNavHooks(&kLauncherHooks);
  lv_screen_load(gLauncher);
  if (old && old != gLauncher) lv_obj_delete(old);
}

lv_obj_t *uiLauncherScreen() { return gLauncher; }

static void backToLauncher(lv_event_t *) { uiGoHome(); }

static void addBackHandlers(lv_obj_t *screen, lv_obj_t *focusTarget) {
  lv_obj_add_event_cb(screen, backToLauncher, LV_EVENT_CLICKED, nullptr);
  lv_group_remove_all_objs(lvglGroup());
  if (focusTarget) lv_group_add_obj(lvglGroup(), focusTarget);
}

static void openSunTest() {
  uiShellSetTitle("SunTest");
  lvglSetNavHooks(nullptr);
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  halDisplaySetBacklight(255);
  for (int i = 0; i < 8; ++i) {
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_style_bg_color(bar, lv_color_white(), 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_size(bar, 20, 34);
    lv_obj_set_pos(bar, i * 40, UI_SHELL_BAR_HEIGHT);
  }
  static const lv_color_t rgb[3] = {lv_color_hex(0xFF0000),
                                    lv_color_hex(0x00FF00),
                                    lv_color_hex(0x0000FF)};
  for (int i = 0; i < 3; ++i) {
    lv_obj_t *sw = lv_obj_create(scr);
    lv_obj_set_style_bg_color(sw, rgb[i], 0);
    lv_obj_set_style_radius(sw, 0, 0);
    lv_obj_set_style_border_width(sw, 0, 0);
    lv_obj_set_size(sw, 106, 46);
    lv_obj_set_pos(sw, i * 107, 118);
  }
  lv_obj_t *l1 = lv_label_create(scr);
  lv_obj_set_style_text_color(l1, lv_color_white(), 0);
  lv_obj_set_style_text_font(l1, &lv_font_montserrat_24, 0);
  lv_label_set_text(l1, "SUN TEST 0123456789");
  lv_obj_set_pos(l1, 4, 66);
  lv_obj_t *l2 = lv_label_create(scr);
  lv_obj_set_style_text_color(l2, lv_color_white(), 0);
  lv_obj_set_style_text_font(l2, &lv_font_montserrat_14, 0);
  lv_label_set_text(l2, "size1-ish: the quick brown fox jumps over 13 dogs");
  lv_obj_set_pos(l2, 4, 96);
  lv_obj_t *l3 = lv_label_create(scr);
  lv_obj_set_style_text_color(l3, lv_color_hex(0xFFFF00), 0);
  lv_label_set_text(l3, "click / ENTER to exit");
  lv_obj_set_pos(l3, 4, 178);

  lv_obj_t *back = lv_button_create(scr);
  lv_obj_set_pos(back, 4, 204);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, "back");
  lv_obj_add_event_cb(back, [](lv_event_t *) {
    uiApplyDisplayMode();
    backToLauncher(nullptr);
  }, LV_EVENT_CLICKED, nullptr);

  addBackHandlers(scr, back);
  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != gLauncher) lv_obj_delete(old);
}

static void openPlaceholder(const char *name) {
  uiShellSetTitle(name);
  lvglSetNavHooks(nullptr);
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_t *title = lv_label_create(scr);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_label_set_text(title, name);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
  lv_obj_t *sub = lv_label_create(scr);
  lv_label_set_text(sub, "app lands in a later milestone");
  lv_obj_align(sub, LV_ALIGN_CENTER, 0, -10);

  lv_obj_t *back = lv_button_create(scr);
  lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -16);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, LV_SYMBOL_LEFT " back");
  lv_obj_add_event_cb(back, backToLauncher, LV_EVENT_CLICKED, nullptr);

  addBackHandlers(scr, back);
  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != gLauncher) lv_obj_delete(old);
}

static bool startPatternStream(const PatternSettings &settings,
                               uint32_t startedMs) {
  return streamPattern(settings, startedMs);
}

static void openPatterns() {
  static const PatternStreamHooks hooks = {
      startPatternStream, streamPatternStop, streamPatternActive,
      streamTargetCount};
  appPatternsSetStreamHooks(&hooks);
  appPatternsOpen();
}

static void openLocatePlaceholder() { openPlaceholder("Locate"); }

static void tileClicked(lv_event_t *e) {
  const Tile *tile = (const Tile *)lv_event_get_user_data(e);
  if (tile && tile->open) {
    uiShellSetTitle(tile->name);
    tile->open();
  }
}

static void styleLauncher() {
  if (!gLauncher) return;
  bool day = uiDayMode();
  lv_obj_set_style_bg_color(gLauncher, uiScreenColor(), 0);
  uint32_t count = lv_obj_get_child_count(gLauncher);
  for (uint32_t i = 0; i < count; ++i) {
    lv_obj_t *child = lv_obj_get_child(gLauncher, (int32_t)i);
    if (!lv_obj_check_type(child, &lv_button_class)) continue;
    lv_obj_set_style_bg_color(
        child, day ? lv_color_hex(0xE5EDF4) : lv_color_hex(0x202830), 0);
    lv_obj_set_style_bg_color(
        child, day ? lv_color_hex(0xFFD54F) : lv_color_hex(0x3A6EA5),
        LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(
        child, day ? lv_color_hex(0x9AA7B2) : lv_color_hex(0x3A4652), 0);
    lv_obj_set_style_border_color(
        child, day ? lv_color_hex(0x0B57D0) : lv_color_hex(0x9CC7FF),
        LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
    lv_obj_t *label = lv_obj_get_child(child, 0);
    if (label) lv_obj_set_style_text_color(label, uiTextColor(), 0);
  }
}

static void buildLauncher() {
  if (gLauncher) {
    styleLauncher();
    // Rebuild focus into the existing launcher's tiles.
    lv_group_remove_all_objs(lvglGroup());
    uint32_t n = lv_obj_get_child_count(gLauncher);
    for (uint32_t i = 0; i < n; ++i) {
      lv_obj_t *child = lv_obj_get_child(gLauncher, (int32_t)i);
      if (lv_obj_check_type(child, &lv_button_class))
        lv_group_add_obj(lvglGroup(), child);
    }
    return;
  }
  gLauncher = lv_obj_create(nullptr);

  lv_obj_t *grid = gLauncher;
  const int cols = 4, cellW = 78, cellH = 52, x0 = 5, y0 = 26;
  lv_group_remove_all_objs(lvglGroup());
  for (size_t i = 0; i < sizeof(kTiles) / sizeof(kTiles[0]); ++i) {
    lv_obj_t *btn = lv_button_create(grid);
    lv_obj_set_size(btn, cellW - 4, cellH - 4);
    lv_obj_set_pos(btn, x0 + (int)(i % cols) * cellW, y0 + (int)(i / cols) * cellH);
    lv_obj_t *lbl = lv_label_create(btn);
    char txt[32];
    snprintf(txt, sizeof(txt), "%s\n%s", kTiles[i].symbol, kTiles[i].name);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);
    lv_obj_add_event_cb(btn, tileClicked, LV_EVENT_CLICKED, (void *)&kTiles[i]);
    lv_group_add_obj(lvglGroup(), btn);
  }

  styleLauncher();
}

// -------------------------------------------------------------------- task --

static void confirmPollTimer(lv_timer_t *) { uiConfirmPollTick(); }

static void uiTask(void *) {
  lv_timer_create(confirmPollTimer, 200, nullptr);
  for (;;) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

bool uiStart() {
  if (!lvglGlueInit()) return false;
  uiApplyDisplayMode();
  buildLauncher();
  lvglSetNavHooks(&kLauncherHooks);
  lv_screen_load(gLauncher);
  uiShellStart();
  uiShellSetTitle("Home");
  BaseType_t ok = xTaskCreatePinnedToCore(uiTask, "ui", 12288, nullptr, 4,
                                          nullptr, 1);
  gActive = ok == pdPASS;
  return gActive;
}

bool uiActive() { return gActive; }
