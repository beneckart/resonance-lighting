#include <lvgl.h>

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

#include "../net/tonelab_svc.h"
#include "app_tonelab.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_shell.h"
#include "ui_task.h"

// Tone Lab: switch prepared patterns and Atom button rules on site. Three
// views share one list: Play (tap a pattern to play / stop it), Active (tap to
// put a pattern in or out of the double-click rotation) and Rules (tap to turn
// a button rule on or off). Strikes go out over radio; no laptop needed.

enum View : uint8_t { VIEW_PLAY = 0, VIEW_ACTIVE, VIEW_RULES, VIEW_MODE, VIEW_COUNT };
// The first button cycles views; the loop icon marks it as a view switch, not "play".
static const char *kViewName[] = {LV_SYMBOL_LOOP " Play", LV_SYMBOL_LOOP " Active", LV_SYMBOL_LOOP " Rules",
                                  LV_SYMBOL_LOOP " Mode"};
static const char *kModeName[] = {"", "Perimeter cycle", "Lantern dings"};
static const char *kModeHelp[] = {"",
    "Perimeter cycle: each approach to a perimeter light plays the next two lanterns, low to high; then all nine.",
    "Lantern dings: a lantern chimes when someone walks under it; Bidoof's partner Sakura chimes after it."};
static const char *kClick[] = {"any", "single", "double", "triple"};

static View gView = VIEW_PLAY;
static lv_obj_t *gHeader = nullptr;
static lv_obj_t *gList = nullptr;
static lv_obj_t *gViewLabel = nullptr;
static lv_obj_t *gAwakeLabel = nullptr;
static lv_obj_t *gModeBtn = nullptr, *gAwakeBtn = nullptr, *gStopBtn = nullptr, *gBackBtn = nullptr;
static lv_timer_t *gTimer = nullptr;
static int gShownPlaying = -2, gFocus = 0;
static size_t gShownCount = 0;
static uint8_t gShownMode = 0xFF;

static void rebuildList();

static void refreshHeader(lv_timer_t *) {
  if (!gHeader) return;
  TonelabStatus s = tonelabStatus();
  if (!s.loaded) {
    lv_label_set_text(gHeader, "No show yet.\nTone Lab > Interactions > Send to T-Deck.");
  } else {
    char playing[48] = "stopped";
    ShowPattern p;
    if (s.playing >= 0 && tonelabPattern(s.playing, p)) {
      if (s.loopsLeft) snprintf(playing, sizeof(playing), LV_SYMBOL_PLAY " %s %u/%d", p.name, (unsigned)s.loopsDone + 1, s.loopsLeft);
      else snprintf(playing, sizeof(playing), LV_SYMBOL_PLAY " %s", p.name);
    }
    lv_label_set_text_fmt(gHeader, "%s | %s | %u presses\n%s", playing,
                          s.mode ? kModeName[s.mode] : "mode not set", (unsigned)s.presses, s.last);
  }
  if (gAwakeLabel) lv_label_set_text(gAwakeLabel, s.awake ? LV_SYMBOL_EYE_OPEN " awake" : LV_SYMBOL_EYE_CLOSE " sleep");
  size_t count = gView == VIEW_RULES ? s.rules : gView == VIEW_MODE ? 2 : s.patterns;
  if (s.playing != gShownPlaying || count != gShownCount || s.mode != gShownMode) rebuildList();  // keep the playing marker current
}

static void modeYes(void *mode) { tonelabSetMode((uint8_t)(intptr_t)mode); }

static void itemCb(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  gFocus = i;
  if (gView == VIEW_MODE) {  // every lantern and perimeter switches: confirm first
    uiConfirm(kModeHelp[i + 1], "Tone Lab", modeYes, (void *)(intptr_t)(i + 1));
    return;
  }
  if (gView == VIEW_PLAY) {
    TonelabStatus s = tonelabStatus();
    if (s.playing == i) tonelabStop();
    else tonelabPlay(i, 0);
  } else if (gView == VIEW_ACTIVE) {
    ShowPattern p;
    if (tonelabPattern(i, p)) tonelabSetActive(i, !p.active);
  } else {
    ShowRule r;
    if (tonelabRule(i, r)) tonelabSetRuleEnabled(i, !r.enabled);
  }
  // The tapped button is a list child: rebuild after this event finishes.
  lv_async_call([](void *) { rebuildList(); refreshHeader(nullptr); }, nullptr);
}

static void rebuildList() {
  if (!gList) return;
  TonelabStatus s = tonelabStatus();
  lv_obj_clean(gList);
  lv_group_remove_all_objs(lvglGroup());
  gShownPlaying = s.playing;
  gShownMode = s.mode;
  gShownCount = gView == VIEW_RULES ? s.rules : gView == VIEW_MODE ? 2 : s.patterns;
  lv_obj_t *focus = nullptr;
  char text[72];
  for (size_t i = 0; i < gShownCount; ++i) {
    const char *sym = nullptr;
    if (gView == VIEW_MODE) {
      sym = s.mode == (int)i + 1 ? LV_SYMBOL_OK : LV_SYMBOL_RIGHT;
      snprintf(text, sizeof(text), "%s", kModeName[i + 1]);
    } else if (gView == VIEW_RULES) {
      ShowRule r;
      if (!tonelabRule((int)i, r)) continue;
      sym = r.enabled ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE;
      snprintf(text, sizeof(text), "%s (%s)", r.name, kClick[r.click > 3 ? 0 : r.click]);
    } else {
      ShowPattern p;
      if (!tonelabPattern((int)i, p)) continue;
      if (gView == VIEW_PLAY) sym = s.playing == (int)i ? LV_SYMBOL_STOP : LV_SYMBOL_PLAY;
      else sym = p.active ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE;
      snprintf(text, sizeof(text), "%s%s  %.0f bpm", p.name, (gView == VIEW_PLAY && p.active) ? " *" : "", p.bpm);
    }
    lv_obj_t *btn = lv_list_add_button(gList, sym, text);
    lv_obj_set_style_text_font(btn, &lv_font_montserrat_14, 0);
    lv_obj_add_event_cb(btn, itemCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_group_add_obj(lvglGroup(), btn);
    if ((int)i == gFocus) focus = btn;
  }
  lv_group_add_obj(lvglGroup(), gModeBtn);
  lv_group_add_obj(lvglGroup(), gAwakeBtn);
  lv_group_add_obj(lvglGroup(), gStopBtn);
  lv_group_add_obj(lvglGroup(), gBackBtn);
  if (focus) {
    lv_group_focus_obj(focus);
    lv_obj_scroll_to_view(focus, LV_ANIM_OFF);
  }
}

static void modeCb(lv_event_t *) {
  gView = (View)((gView + 1) % VIEW_COUNT);
  gFocus = 0;
  lv_label_set_text(gViewLabel, kViewName[gView]);
  rebuildList();
}

static void awakeCb(lv_event_t *) {
  tonelabSetAwake(!tonelabStatus().awake);
  refreshHeader(nullptr);
}

static void stopCb(lv_event_t *) {
  tonelabStop();
  refreshHeader(nullptr);
}

static void backCb(lv_event_t *) {
  // Leaving the app never stops a pattern or keep-awake: they run in the background.
  if (gTimer) {
    lv_timer_delete(gTimer);
    gTimer = nullptr;
  }
  gHeader = gList = gViewLabel = gAwakeLabel = nullptr;
  gModeBtn = gAwakeBtn = gStopBtn = gBackBtn = nullptr;
  uiGoHome();
}

static lv_obj_t *button(lv_obj_t *parent, int x, const char *text, lv_event_cb_t cb, lv_obj_t **labelOut) {
  lv_obj_t *b = lv_button_create(parent);
  lv_obj_set_size(b, 75, 32);
  lv_obj_set_pos(b, x, 204);
  lv_obj_t *l = lv_label_create(b);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
  lv_label_set_text(l, text);
  lv_obj_center(l);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
  if (labelOut) *labelOut = l;
  return b;
}

void appTonelabOpen() {
  lvglSetNavHooks(nullptr);
  uiShellSetTitle("Tone Lab");
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  gHeader = lv_label_create(scr);
  lv_obj_set_style_text_font(gHeader, &lv_font_montserrat_14, 0);
  lv_label_set_long_mode(gHeader, LV_LABEL_LONG_MODE_DOTS);
  lv_obj_set_size(gHeader, 312, 36);
  lv_obj_set_pos(gHeader, 4, UI_SHELL_BAR_HEIGHT + 2);

  gList = lv_list_create(scr);
  lv_obj_set_size(gList, 316, 134);
  lv_obj_set_pos(gList, 2, UI_SHELL_BAR_HEIGHT + 40);

  gModeBtn = button(scr, 2, kViewName[gView], modeCb, &gViewLabel);
  gAwakeBtn = button(scr, 81, LV_SYMBOL_EYE_CLOSE " sleep", awakeCb, &gAwakeLabel);
  gStopBtn = button(scr, 160, LV_SYMBOL_STOP " stop", stopCb, nullptr);
  gBackBtn = button(scr, 239, LV_SYMBOL_LEFT " back", backCb, nullptr);

  gShownPlaying = -2;
  rebuildList();
  refreshHeader(nullptr);
  gTimer = lv_timer_create(refreshHeader, 500, nullptr);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
