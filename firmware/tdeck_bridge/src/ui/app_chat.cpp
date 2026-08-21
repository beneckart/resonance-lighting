#include <lvgl.h>

#include <Arduino.h>
#include <string.h>

#include "../net/census_svc.h"
#include "../net/claude_client.h"
#include "../net/espnow_link.h"
#include "../net/net_mgr.h"
#include "../store/store.h"
#include "app_chat.h"
#include "lvgl_glue.h"
#include "ui_task.h"

#define SCROLLBACK_CAP 6144

static lv_obj_t *gStatus = nullptr;
static lv_obj_t *gScrollBox = nullptr;
static lv_obj_t *gScrollLabel = nullptr;
static lv_obj_t *gInput = nullptr;
static lv_timer_t *gTimer = nullptr;
static char *gScrollback = nullptr;  // PSRAM, allocated once
static size_t gLen = 0;
static uint32_t gLastGen = 0;

static void sbAppendRaw(const char *s, size_t n) {
  if (!gScrollback) return;
  if (gLen + n + 1 > SCROLLBACK_CAP) {  // trim oldest third
    size_t cut = SCROLLBACK_CAP / 3 + n;
    if (cut > gLen) cut = gLen;
    memmove(gScrollback, gScrollback + cut, gLen - cut);
    gLen -= cut;
  }
  memcpy(gScrollback + gLen, s, n);
  gLen += n;
  gScrollback[gLen] = 0;
  lv_label_set_text_static(gScrollLabel, gScrollback);
}

// The Montserrat fonts ship ASCII only — fold common Unicode punctuation to
// ASCII lookalikes and everything else to '?', and escape '#' (recolor markup)
// so streamed text can't open a color span.
static void sbAppend(const char *s, size_t n) {
  char out[300];
  size_t o = 0;
  for (size_t i = 0; i < n;) {
    if (o + 4 >= sizeof(out)) {
      sbAppendRaw(out, o);
      o = 0;
    }
    unsigned char c = (unsigned char)s[i];
    if (c == '#') {
      out[o++] = '#';
      out[o++] = '#';
      ++i;
    } else if (c < 0x80) {
      out[o++] = (char)c;
      ++i;
    } else if (c == 0xE2 && i + 2 < n && (unsigned char)s[i + 1] == 0x80) {
      unsigned char t = (unsigned char)s[i + 2];
      if (t == 0x93 || t == 0x94) out[o++] = '-';        // en/em dash
      else if (t == 0x98 || t == 0x99) out[o++] = '\'';  // curly single
      else if (t == 0x9C || t == 0x9D) out[o++] = '"';   // curly double
      else if (t == 0xA6) { out[o++] = '.'; out[o++] = '.'; out[o++] = '.'; }
      else if (t == 0xA2) out[o++] = '*';                // bullet
      else out[o++] = '?';
      i += 3;
    } else {  // any other multibyte sequence -> one '?', skip continuations
      out[o++] = '?';
      ++i;
      while (i < n && ((unsigned char)s[i] & 0xC0) == 0x80) ++i;
    }
  }
  if (o) sbAppendRaw(out, o);
}

static void chatTick(lv_timer_t *) {
  // New response generation -> colored prefix, then PARK the view at the
  // start of the response (no tail-follow; read from the top as it streams).
  uint32_t gen = claudeGeneration();
  if (gen != gLastGen) {
    gLastGen = gen;
    lv_obj_update_layout(gScrollBox);
    int32_t parkY = lv_obj_get_height(gScrollLabel);
    static const char kPrefix[] = "\n\n#fbbc7f claude:# ";
    sbAppendRaw(kPrefix, sizeof(kPrefix) - 1);
    lv_obj_scroll_to_y(gScrollBox, parkY > 8 ? parkY - 8 : 0, LV_ANIM_OFF);
  }
  char buf[256];
  size_t n;
  while ((n = claudeReadDeltas(buf, sizeof(buf))) > 0) sbAppend(buf, n);

  // Status strip: state + the mesh-silent flag (mesh trouble must be visible
  // even mid-chat; the mesh is the primary function).
  MeshStats ms = espnowStats();
  bool meshSilent = ms.frames == 0 || millis() - ms.lastFrameMs > 15000;
  lv_color_t color = lv_color_hex(0x808890);
  const char *txt = claudeStatusLine();
  switch (claudeState()) {
    case ChatState::QUEUED: color = lv_color_hex(0xE8A33D); break;     // amber
    case ChatState::CONNECTING: color = lv_color_hex(0xE8D44D); break; // yellow
    case ChatState::STREAMING: color = lv_color_hex(0x4DC3E8); break;  // cyan
    case ChatState::BACKOFF: color = lv_color_hex(0xE8A33D); break;
    case ChatState::FAILED: color = lv_color_hex(0xE85D5D); break;     // red
    default: break;
  }
  char line[120];
  snprintf(line, sizeof(line), "%s%s", txt, meshSilent ? "  | MESH SILENT" : "");
  lv_label_set_text(gStatus, line);
  lv_obj_set_style_text_color(gStatus, meshSilent && claudeState() == ChatState::IDLE
                                           ? lv_color_hex(0xE85D5D)
                                           : color, 0);
}

// Local slash commands — handled on-device, never sent to the API.
static bool handleSlash(const char *text) {
  if (text[0] != '/') return false;
  static const char kSys[] = "\n\n#9a9fa6 sys:# ";
  sbAppendRaw(kSys, sizeof(kSys) - 1);
  char line[160];
  if (strncmp(text, "/model", 6) == 0) {
    const char *arg = text + 6;
    while (*arg == ' ') ++arg;
    if (*arg) {
      strlcpy(settings().model, arg, sizeof(settings().model));
      storeSave();
    }
    snprintf(line, sizeof(line), "model = %s", settings().model);
  } else if (strcmp(text, "/clear") == 0) {
    if (claudeClearHistory()) {
      gLen = 0;
      gScrollback[0] = 0;
      lv_label_set_text_static(gScrollLabel, gScrollback);
      sbAppendRaw(kSys, sizeof(kSys) - 1);
      snprintf(line, sizeof(line), "history cleared");
    } else {
      snprintf(line, sizeof(line), "busy - try after the response finishes");
    }
  } else if (strcmp(text, "/status") == 0) {
    int live = 0, seen = 0;
    censusCountsSafe(&live, &seen, millis());
    snprintf(line, sizeof(line), "net=%s ip=%s | chat=%s | mesh %d/%d live",
             netStateName(), netIp(), claudeStatusLine(), live, seen);
  } else {
    snprintf(line, sizeof(line),
             "/model [id]  /clear  /status  /help");
  }
  sbAppend(line, strlen(line));
  lv_obj_scroll_to_y(gScrollBox, LV_COORD_MAX, LV_ANIM_OFF);
  return true;
}

static void submitCb(lv_event_t *) {
  const char *text = lv_textarea_get_text(gInput);
  if (!text || !text[0]) return;
  if (handleSlash(text)) {
    lv_textarea_set_text(gInput, "");
    return;
  }
  if (!claudeSubmit(text)) return;  // busy: status strip already says so
  static const char kYou[] = "\n\n#8ab4f8 you:# ";
  sbAppendRaw(kYou, sizeof(kYou) - 1);
  sbAppend(text, strlen(text));
  lv_textarea_set_text(gInput, "");
  lv_obj_scroll_to_y(gScrollBox, LV_COORD_MAX, LV_ANIM_OFF);
}

static void backCb(lv_event_t *) {
  if (gTimer) {
    lv_timer_delete(gTimer);
    gTimer = nullptr;
  }
  gStatus = nullptr;
  gScrollBox = nullptr;
  gScrollLabel = nullptr;
  gInput = nullptr;
  uiGoHome();
}

// Trackball: vertical scrolls the transcript; click focuses the input line.
static bool chatVertical(int dir) {
  if (!gScrollBox) return false;
  lv_obj_scroll_by_bounded(gScrollBox, 0, (int32_t)(-dir * 24), LV_ANIM_OFF);
  return true;
}
static bool chatEnter() {
  if (!gInput) return false;
  lv_group_focus_obj(gInput);
  return true;
}
static const UiNavHooks kChatHooks = {chatVertical, chatEnter};

void appChatOpen() {
  if (!gScrollback) {
    gScrollback = (char *)ps_malloc(SCROLLBACK_CAP);
    if (gScrollback) {
      gScrollback[0] = 0;
      gLen = 0;
    }
  }
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  gStatus = lv_label_create(scr);
  lv_obj_set_style_text_font(gStatus, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gStatus, 6, 4);
  lv_label_set_text(gStatus, "ready");

  lv_obj_t *back = lv_button_create(scr);
  lv_obj_set_size(back, 44, 22);
  lv_obj_set_pos(back, 272, 0);
  lv_obj_t *bl = lv_label_create(back);
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
  lv_label_set_text(bl, LV_SYMBOL_LEFT);
  lv_obj_center(bl);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  gScrollBox = lv_obj_create(scr);
  lv_obj_set_pos(gScrollBox, 0, 24);
  lv_obj_set_size(gScrollBox, 320, 178);
  lv_obj_set_style_bg_color(gScrollBox, lv_color_hex(0x14181C), 0);
  lv_obj_set_style_border_width(gScrollBox, 0, 0);
  lv_obj_set_style_pad_all(gScrollBox, 6, 0);
  gScrollLabel = lv_label_create(gScrollBox);
  lv_obj_set_width(gScrollLabel, 296);
  lv_label_set_long_mode(gScrollLabel, LV_LABEL_LONG_WRAP);
  lv_label_set_recolor(gScrollLabel, true);  // #RRGGBB spans for you:/claude:
  lv_obj_set_style_text_color(gScrollLabel, lv_color_hex(0xD8DDE2), 0);
  lv_label_set_text_static(gScrollLabel, gScrollback ? gScrollback : "");

  gInput = lv_textarea_create(scr);
  lv_textarea_set_one_line(gInput, true);
  lv_textarea_set_placeholder_text(gInput, "type; ENTER sends");
  lv_obj_set_pos(gInput, 0, 204);
  lv_obj_set_size(gInput, 320, 36);
  lv_obj_add_event_cb(gInput, submitCb, LV_EVENT_READY, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), gInput);
  lv_group_add_obj(lvglGroup(), back);
  lv_group_focus_obj(gInput);
  // Keep the group in "editing" so keystrokes land in the textarea directly.
  lv_group_set_editing(lvglGroup(), true);
  lvglSetNavHooks(&kChatHooks);

  gLastGen = claudeGeneration();
  gTimer = lv_timer_create(chatTick, 100, nullptr);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
