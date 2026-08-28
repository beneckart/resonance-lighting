#include "ui_shell.h"

#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "../core/battery_model.h"
#include "../core/control_activity_model.h"
#include "../hal/hal_board.h"
#include "../net/census_svc.h"
#include "../net/contagion_fanout.h"
#include "../net/mesh_tx.h"
#include "../net/net_mgr.h"
#include "../net/stream_svc.h"
#include "../store/store.h"
#include "ui_theme.h"

namespace {

static lv_obj_t *gBar = nullptr;
static lv_obj_t *gTitle = nullptr;
static lv_obj_t *gStatus = nullptr;
static lv_obj_t *gClock = nullptr;
static lv_obj_t *gStop = nullptr;
static char gAppStatus[120] = {};
static uint32_t gAppStatusRgb = 0;
static char gRenderedStatus[180] = {};
static char gRenderedClock[16] = {};

static const char *programName(uint8_t programId) {
  switch (programId) {
    case 1: return "CA";
    case 2: return "BRIDGE";
    case 3: return "DIRECT";
    case 4: return "DARK";
    case 5: return "CONTAGION";
    default: return "PROGRAM";
  }
}

static const char *streamName(StreamMode mode) {
  switch (mode) {
    case StreamMode::SOLID: return "SOLID";
    case StreamMode::BLINK: return "BLINK";
    case StreamMode::PATTERN: return "PATTERN";
    case StreamMode::OFF: return "OFF";
  }
  return "STREAM";
}

static void formatRemaining(char *out, size_t cap, uint32_t remainingMs) {
  uint32_t seconds = (remainingMs + 999UL) / 1000UL;
  uint32_t hours = seconds / 3600UL;
  uint32_t minutes = (seconds / 60UL) % 60UL;
  uint32_t secs = seconds % 60UL;
  if (hours)
    snprintf(out, cap, "%lu:%02lu:%02lu", (unsigned long)hours,
             (unsigned long)minutes, (unsigned long)secs);
  else
    snprintf(out, cap, "%02lu:%02lu", (unsigned long)minutes,
             (unsigned long)secs);
}

static void appendText(char *out, size_t cap, const char *text) {
  size_t used = strlen(out);
  if (used < cap - 1) strncat(out, text, cap - used - 1);
}

static void appendProgram(char *out, size_t cap, const char *prefix,
                          uint8_t programId) {
  char part[64];
  snprintf(part, sizeof(part), "%s%s until expiry", prefix,
           programName(programId));
  appendText(out, cap, part);
}

static void setStatusText(const char *text) {
  if (strncmp(gRenderedStatus, text, sizeof(gRenderedStatus)) == 0) return;
  strlcpy(gRenderedStatus, text, sizeof(gRenderedStatus));
  lv_label_set_text(gStatus, gRenderedStatus);
}

static void setClockText(const char *text) {
  if (strncmp(gRenderedClock, text, sizeof(gRenderedClock)) == 0) return;
  strlcpy(gRenderedClock, text, sizeof(gRenderedClock));
  lv_label_set_text(gClock, gRenderedClock);
}

static void stopCb(lv_event_t *) {
  ProgramLeaseActivity program = meshProgramActivity();
  streamStop();
  if (program.active && program.programId == 5) contagionFanoutDisable();
  meshStopTrackedProgramActivity();
}

static void baseStatus(char *out, size_t cap, uint32_t nowMs) {
  int live = 0, seen = 0;
  censusCountsSafe(&live, &seen, nowMs);
  const char *net = "mesh-only";
  switch (netState()) {
    case NetState::ONLINE: net = "WiFi"; break;
    case NetState::GUARD_BLOCKED: net = "GUARD"; break;
    case NetState::CONNECTING: net = "WiFi joining"; break;
    default: break;
  }
  snprintf(out, cap, "%s | ch%u | live %d/%d | bat %u%%", net,
           settings().channel, live, seen,
           lipoPercentFromMv(halBatteryMv()));
}

static void updateShell(lv_timer_t *) {
  if (!gBar || !gTitle || !gStatus || !gClock || !gStop) return;
  uint32_t nowMs = millis();
  StreamMode stream = streamMode();
  ProgramLeaseActivity program = meshProgramActivity();
  ForeignControlActivity foreign = {};
  bool foreignActive = censusForeignControlSafe(&foreign, nowMs);
  bool localActive = stream != StreamMode::OFF || program.active;
  bool controlActive = localActive || foreignActive;

  char status[180] = {};
  if (stream != StreamMode::OFF) {
    char part[96];
    snprintf(part, sizeof(part), "LOCAL %s until STOP  n=%d",
             streamName(stream), streamTargetCount());
    appendText(status, sizeof(status), part);
  }
  if (program.active) {
    appendProgram(status, sizeof(status), status[0] ? " / LOCAL " : "LOCAL ",
                  program.programId);
  }
  if (foreignActive) {
    char identity[64];
    snprintf(identity, sizeof(identity), "%s! %s %02X%02X%02X ",
             status[0] ? " | " : "", controlPublisherName(foreign.publisher),
             foreign.id[0], foreign.id[1], foreign.id[2]);
    appendText(status, sizeof(status), identity);
    if (foreign.kind == ForeignControlKind::PROGRAM)
      appendProgram(status, sizeof(status), "", foreign.programId);
    else
      appendText(status, sizeof(status), foreignControlKindName(foreign.kind));
  }
  if (!controlActive) {
    if (gAppStatus[0])
      strlcpy(status, gAppStatus, sizeof(status));
    else
      baseStatus(status, sizeof(status), nowMs);
  }
  setStatusText(status);

  char clock[16] = {};
  bool clockVisible = false;
  if (program.active) {
    clock[0] = '-';
    formatRemaining(clock + 1, sizeof(clock) - 1, program.remainingMs);
    clockVisible = true;
  } else if (stream != StreamMode::OFF) {
    clock[0] = '+';
    formatRemaining(clock + 1, sizeof(clock) - 1, streamElapsedMs(nowMs));
    clockVisible = true;
  } else if (foreignActive && foreign.kind == ForeignControlKind::PROGRAM) {
    clock[0] = '-';
    formatRemaining(clock + 1, sizeof(clock) - 1, foreign.remainingMs);
    clockVisible = true;
  }
  setClockText(clock);

  bool day = uiDayMode();
  lv_color_t background =
      localActive && foreignActive
          ? lv_color_hex(0x9E1B32)
          : (localActive
                 ? lv_color_hex(0xB45309)
                 : (foreignActive
                        ? lv_color_hex(0x334E68)
                        : (day ? lv_color_hex(0xE5EDF4)
                               : lv_color_hex(0x202830))));
  lv_color_t tickerText = controlActive ? lv_color_white() : uiTextColor();
  if (!controlActive && gAppStatus[0] && gAppStatusRgb)
    tickerText = lv_color_hex(gAppStatusRgb);
  lv_color_t titleText = controlActive
                             ? lv_color_hex(0xFFE6A7)
                             : (day ? lv_color_hex(0x0B57D0)
                                    : lv_color_hex(0x9CC7FF));
  lv_color_t clockText = controlActive
                             ? lv_color_hex(0xA5F3FC)
                             : (day ? lv_color_hex(0x0F6B78)
                                    : lv_color_hex(0xA5F3FC));
  lv_obj_set_style_bg_color(gBar, background, 0);
  lv_obj_set_style_border_color(
      gBar, day ? lv_color_hex(0x9AA7B2) : lv_color_hex(0x3A4652), 0);
  lv_obj_set_style_text_color(gTitle, titleText, 0);
  lv_obj_set_style_text_color(gStatus, tickerText, 0);
  lv_obj_set_style_text_color(gClock, clockText, 0);

  if (localActive) {
    lv_obj_clear_flag(gStop, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_width(gStatus, clockVisible ? 113 : 167);
    lv_obj_set_pos(gClock, 199, 4);
    lv_obj_set_width(gClock, 50);
  } else if (clockVisible) {
    lv_obj_add_flag(gStop, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_width(gStatus, 175);
    lv_obj_set_pos(gClock, 261, 4);
    lv_obj_set_width(gClock, 55);
  } else {
    lv_obj_add_flag(gStop, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_width(gStatus, 233);
  }
  if (clockVisible)
    lv_obj_clear_flag(gClock, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(gClock, LV_OBJ_FLAG_HIDDEN);
  // Modal dialogs also use the top layer. Shell chrome stays frontmost.
  lv_obj_move_foreground(gBar);
}

}  // namespace

void uiShellStart() {
  if (gBar) return;
  gBar = lv_obj_create(lv_layer_top());
  lv_obj_set_size(gBar, 320, UI_SHELL_BAR_HEIGHT);
  lv_obj_set_pos(gBar, 0, 0);
  lv_obj_set_style_radius(gBar, 0, 0);
  lv_obj_set_style_border_width(gBar, 1, 0);
  lv_obj_set_style_pad_all(gBar, 0, 0);
  lv_obj_set_style_bg_opa(gBar, LV_OPA_COVER, 0);
  lv_obj_clear_flag(gBar, LV_OBJ_FLAG_SCROLLABLE);

  gTitle = lv_label_create(gBar);
  lv_obj_set_pos(gTitle, 4, 4);
  lv_obj_set_width(gTitle, 74);
  lv_obj_set_style_text_font(gTitle, &lv_font_montserrat_14, 0);
  lv_label_set_long_mode(gTitle, LV_LABEL_LONG_CLIP);
  lv_label_set_text(gTitle, "Home");

  gStatus = lv_label_create(gBar);
  lv_obj_set_pos(gStatus, 82, 4);
  lv_obj_set_width(gStatus, 233);
  lv_obj_set_style_text_font(gStatus, &lv_font_montserrat_14, 0);
  lv_label_set_long_mode(gStatus, LV_LABEL_LONG_SCROLL_CIRCULAR);

  gClock = lv_label_create(gBar);
  lv_obj_set_pos(gClock, 199, 4);
  lv_obj_set_width(gClock, 50);
  lv_obj_set_style_text_font(gClock, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_align(gClock, LV_TEXT_ALIGN_RIGHT, 0);
  lv_label_set_long_mode(gClock, LV_LABEL_LONG_CLIP);
  lv_obj_add_flag(gClock, LV_OBJ_FLAG_HIDDEN);

  gStop = lv_button_create(gBar);
  lv_obj_set_size(gStop, 66, 24);
  lv_obj_set_pos(gStop, 253, 1);
  lv_obj_set_style_radius(gStop, 3, 0);
  lv_obj_set_style_pad_all(gStop, 2, 0);
  lv_obj_set_style_bg_color(gStop, lv_color_hex(0xFEE2E2), 0);
  lv_obj_add_event_cb(gStop, stopCb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *stopLabel = lv_label_create(gStop);
  lv_label_set_text(stopLabel, LV_SYMBOL_STOP " STOP");
  lv_obj_set_style_text_color(stopLabel, lv_color_hex(0x7F1D1D), 0);
  lv_obj_center(stopLabel);
  lv_obj_add_flag(gStop, LV_OBJ_FLAG_HIDDEN);

  lv_timer_create(updateShell, 250, nullptr);
  updateShell(nullptr);
}

void uiShellSetTitle(const char *title) {
  if (!title) title = "";
  if (gTitle) lv_label_set_text(gTitle, title);
  uiShellSetAppStatus(nullptr);
}

void uiShellSetAppStatus(const char *text, uint32_t rgb) {
  if (!text) text = "";
  strlcpy(gAppStatus, text, sizeof(gAppStatus));
  gAppStatusRgb = rgb;
}
