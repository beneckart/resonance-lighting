#include <lvgl.h>

#include <Arduino.h>
#include <string.h>

#include "../core/fleet_registry_generated.h"
#include "../core/fleet_view_model.h"
#include "../core/health_model.h"
#include "../net/census_svc.h"
#include "../net/mesh_tx.h"
#include "app_fleet.h"
#include "app_power.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_task.h"

static lv_obj_t *gTable = nullptr;
static lv_obj_t *gHeader = nullptr;
static lv_timer_t *gTimer = nullptr;
static CensusView gSnapshot[CENSUS_MAX_TRACKED];
static FleetViewRow gRows[CENSUS_MAX_TRACKED];
static size_t gRowCount = 0;
static uint32_t gSelRow = 1;
static uint8_t gSelectedId[3] = {};
static bool gHasSelectedId = false;
static bool gRestoreSelectionScroll = false;
static FleetViewSettings gViewSettings = fleetViewDefaults();

// A filtered identify is an exact-target roll, not a broadcast: off-air rows
// are never claimed as reached, and the confirmed cohort cannot change while
// the modal is open. Pacing keeps the UI callback bounded and leaves every
// target blinking long enough for a complete full-fleet pass to overlap.
static uint8_t gIdentifyTargets[CENSUS_MAX_TRACKED][3];
static size_t gIdentifyTargetCount = 0;
static size_t gIdentifyTargetIndex = 0;
static lv_timer_t *gIdentifyTimer = nullptr;

static void openDetail(const uint8_t id[3]);
static lv_color_t ledChipColor(const CensusView &v);
static void refreshTable(lv_timer_t *);

static const char *callsignForId(const uint8_t id[3]) {
  const HealthRegistryEntry *entry =
      healthRegistryFind(kHealthRegistry, kHealthRegistryCount, id);
  return entry ? entry->callsign : nullptr;
}

static void rememberSelectedRow() {
  if (gSelRow >= 1 && gSelRow - 1 < gRowCount) {
    memcpy(gSelectedId, gRows[gSelRow - 1].view.id, 3);
    gHasSelectedId = true;
  }
}

// Trackball on the fleet screen: up/down moves the row selection (and keeps
// it in view); click opens the selected node's detail.
static bool fleetVertical(int dir) {
  if (!gTable) return false;
  uint32_t rows = lv_table_get_row_count(gTable);
  if (rows <= 1) return true;
  int32_t sel = (int32_t)gSelRow + dir;
  if (sel < 1) sel = 1;
  if (sel >= (int32_t)rows) sel = (int32_t)rows - 1;
  gSelRow = (uint32_t)sel;
  rememberSelectedRow();
  int32_t rowH = 26;
  lv_obj_scroll_to_y(gTable, gSelRow > 4 ? (int32_t)(gSelRow - 4) * rowH : 0,
                     LV_ANIM_OFF);
  lv_obj_invalidate(gTable);  // repaint the row highlight
  return true;
}

// Row highlight drawn ourselves (draw-task hook): the built-in selected-cell
// style only renders while the table is keypad-focused, and ours is driven
// through the nav hooks instead.
static void tableDrawEvent(lv_event_t *e) {
  lv_draw_task_t *task = lv_event_get_draw_task(e);
  lv_draw_dsc_base_t *base = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(task);
  if (!base || base->part != LV_PART_ITEMS) return;
  uint32_t row = base->id1;
  uint32_t col = base->id2;

  // LVGL emits separate draw tasks for a cell's fill and label. The Bridge
  // screen uses a dark palette, so never inherit the default black table text.
  // Keep retained/off-air rows visibly subdued without sacrificing contrast.
  lv_draw_label_dsc_t *label = lv_draw_task_get_label_dsc(task);
  if (label) {
    bool offAir = row >= 1 && row - 1 < gRowCount &&
                  !gRows[row - 1].fresh;
    label->color = offAir ? lv_color_hex(0xB8C0C8)
                          : lv_color_hex(0xF2F5F7);
    return;
  }

  lv_draw_fill_dsc_t *fill = lv_draw_task_get_fill_dsc(task);
  if (!fill) return;
  if (col == 0 && row >= 1) {  // reported-color chip column
    if (row - 1 < gRowCount) {
      fill->color = ledChipColor(gRows[row - 1].view);
      fill->opa = LV_OPA_COVER;
    }
    return;
  }
  if (row == 0) {
    fill->color = lv_color_hex(0x2A3540);  // header band
    fill->opa = LV_OPA_COVER;
  } else if (row == gSelRow) {
    fill->color = lv_color_hex(0x3A6EA5);  // selected row
    fill->opa = LV_OPA_COVER;
  } else if (row >= 1 && row - 1 < gRowCount && col == 5) {
    switch (gRows[row - 1].batteryBand) {
      case BatteryHealthBand::GOOD: fill->color = lv_color_hex(0x194F2B); break;
      case BatteryHealthBand::NEAR_LOW: fill->color = lv_color_hex(0x66510A); break;
      case BatteryHealthBand::LOW_BATTERY: fill->color = lv_color_hex(0x6A2020); break;
      case BatteryHealthBand::UNKNOWN: fill->color = lv_color_hex(0x214B6A); break;
      case BatteryHealthBand::OFF_AIR: fill->color = lv_color_hex(0x30343A); break;
    }
    fill->opa = LV_OPA_COVER;
  } else if (row >= 1 && row - 1 < gRowCount &&
             !gRows[row - 1].fresh) {
    fill->color = lv_color_hex(0x20242A);  // retained blank/off-air row
    fill->opa = LV_OPA_COVER;
  }
}

static bool fleetEnter() {
  if (!gTable) return false;
  if (gSelRow >= 1 && gSelRow - 1 < gRowCount) {
    openDetail(gRows[gSelRow - 1].view.id);
    return true;
  }
  return false;
}

static const UiNavHooks kFleetHooks = {fleetVertical, fleetEnter};

static char classLetter(uint8_t cls) {
  switch (cls) {
    case 1: return 'D';  // downlight
    case 2: return 'P';  // perimeter
    case 3: return 'U';  // uplight
    case 4: return 'C';  // chandelier
    default: return '?';
  }
}

static const char *programName(uint8_t prog) {
  switch (prog) {  // core/choreo/program.h registry
    case 0: return "idle";
    case 1: return "ca";
    case 2: return "brdg";
    case 3: return "dir";
    case 4: return "dark";
    case 5: return "virus";
    default: return "?";
  }
}

// Reported-output chip color: RGBW blended for a display swatch; unlit or
// unknown fixtures show dark grey so "off" and "no data" stay distinguishable
// from black-rendering-as-color.
static lv_color_t ledChipColor(const CensusView &v) {
  if (!v.ledKnown) return lv_color_hex(0x383838);
  if (!v.ledOn || (v.ledR | v.ledG | v.ledB | v.ledW) == 0)
    return lv_color_hex(0x141414);
  uint16_t w = (uint16_t)(v.ledW * 4 / 5);
  uint8_t r = (uint8_t)LV_MIN(255, v.ledR + w);
  uint8_t g = (uint8_t)LV_MIN(255, v.ledG + w);
  uint8_t b = (uint8_t)LV_MIN(255, v.ledB + w);
  return lv_color_make(r, g, b);
}

static void refreshTable(lv_timer_t *) {
  if (!gTable) return;
  uint32_t now = millis();
  int32_t scrollY = lv_obj_get_scroll_y(gTable);
  rememberSelectedRow();
  size_t snapshotCount =
      censusSnapshotSafe(gSnapshot, CENSUS_MAX_TRACKED, now);
  gRowCount = fleetBuildView(
      kHealthRegistry, kHealthRegistryCount, gSnapshot, snapshotCount,
      censusFreshMsSafe(), gViewSettings, gRows, CENSUS_MAX_TRACKED);

  int selected = gHasSelectedId
                     ? fleetFindRowById(gRows, gRowCount, gSelectedId)
                     : -1;
  if (selected >= 0)
    gSelRow = (uint32_t)selected + 1;
  else if (gRowCount == 0)
    gSelRow = 0;
  else if (gSelRow < 1 || gSelRow > gRowCount)
    gSelRow = 1;

  int live = 0, seen = 0;
  censusCountsSafe(&live, &seen, now);
  if (gIdentifyTimer)
    lv_label_set_text_fmt(gHeader, "Fleet blink %u/%u  %u shown",
                          (unsigned)gIdentifyTargetIndex,
                          (unsigned)gIdentifyTargetCount,
                          (unsigned)gRowCount);
  else
    lv_label_set_text_fmt(gHeader, "Fleet %u shown  %d/%d live",
                          (unsigned)gRowCount, live, seen);

  lv_table_set_row_count(gTable, (uint32_t)gRowCount + 1);
  char buf[24];
  for (size_t i = 0; i < gRowCount; ++i) {
    const FleetViewRow &row = gRows[i];
    const CensusView &v = row.view;
    uint32_t r = (uint32_t)i + 1;
    lv_table_set_cell_value(gTable, r, 0, "");  // chip column (draw hook)
    const char *callsign = row.registry ? row.registry->callsign
                                        : callsignForId(v.id);
    if (callsign)
      snprintf(buf, sizeof(buf), "%s %c", callsign,
               classLetter(row.fixtureClass));
    else
      snprintf(buf, sizeof(buf), "%02X%02X%02X %c", v.id[0], v.id[1], v.id[2],
               classLetter(row.fixtureClass));
    lv_table_set_cell_value(gTable, r, 1, buf);
    uint32_t ageS = v.ageMs / 1000;
    if (!row.observed)
      snprintf(buf, sizeof(buf), "never");
    else if (row.fresh && ageS < 100)
      snprintf(buf, sizeof(buf), "%lus", (unsigned long)ageS);
    else if (ageS < 6000)
      snprintf(buf, sizeof(buf), "%lum", (unsigned long)(ageS / 60));
    else
      snprintf(buf, sizeof(buf), "%luh", (unsigned long)(ageS / 3600));
    lv_table_set_cell_value(gTable, r, 2, buf);
    if (row.observed) snprintf(buf, sizeof(buf), "%d", v.rssiEwma);
    else snprintf(buf, sizeof(buf), "-");
    lv_table_set_cell_value(gTable, r, 3, buf);
    if (row.observed) snprintf(buf, sizeof(buf), "%u%%", v.pdrX1000 / 10);
    else snprintf(buf, sizeof(buf), "-");
    lv_table_set_cell_value(gTable, r, 4, buf);
    if (row.observed && v.battMv > 0)
      snprintf(buf, sizeof(buf), "%d.%02d", v.battMv / 1000,
               (v.battMv % 1000) / 10);
    else
      snprintf(buf, sizeof(buf), "-");
    lv_table_set_cell_value(gTable, r, 5, buf);
    lv_table_set_cell_value(gTable, r, 6,
                            row.observed ? programName(v.activeProgram) : "-");
  }

  int32_t targetY = scrollY;
  if (gRestoreSelectionScroll && gSelRow >= 1) {
    const int32_t rowH = 26;
    targetY = gSelRow > 4 ? (int32_t)(gSelRow - 4) * rowH : 0;
  }
  lv_obj_scroll_to_y(gTable, targetY, LV_ANIM_OFF);
  gRestoreSelectionScroll = false;
  lv_obj_invalidate(gTable);
}

static void stopTimer() {
  if (gTimer) {
    lv_timer_delete(gTimer);
    gTimer = nullptr;
  }
  gTable = nullptr;
  gHeader = nullptr;
}

static void backFromFleet(lv_event_t *) {
  stopTimer();
  uiGoHome();
}

// ------------------------------------------------------------------ detail --

static uint8_t gDetailId[3];

static void identifyCb(lv_event_t *) {
  meshIdentify(gDetailId, 10, 2 /*green*/, 1 /*blink*/, 128);
}

static void backFromDetail(lv_event_t *) {
  gRestoreSelectionScroll = true;
  appFleetOpen();
}

static void openDetail(const uint8_t id[3]) {
  memcpy(gDetailId, id, 3);
  memcpy(gSelectedId, id, 3);
  gHasSelectedId = true;
  stopTimer();
  lvglSetNavHooks(nullptr);  // default linear nav between identify/back
  PeerStat p;
  bool ok = censusPeerSafe(id, &p);

  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_t *title = lv_label_create(scr);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  const char *callsign = callsignForId(id);
  if (callsign)
    lv_label_set_text_fmt(title, "%s  %02X%02X%02X", callsign, id[0], id[1],
                          id[2]);
  else
    lv_label_set_text_fmt(title, "%02X%02X%02X", id[0], id[1], id[2]);
  lv_obj_set_pos(title, 8, 6);

  lv_obj_t *body = lv_label_create(scr);
  lv_obj_set_style_text_font(body, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(body, 8, 40);
  if (ok) {
    uint32_t now = millis();
    uint32_t total = p.recv + p.gaps;
    char win[16];
    if (p.winPdrX1000 == 0xFFFF) snprintf(win, sizeof(win), "-");
    else snprintf(win, sizeof(win), "%u.%u%%", p.winPdrX1000 / 10, p.winPdrX1000 % 10);
    lv_label_set_text_fmt(
        body,
        "age %lus   rssi %d (ewma %d)\n"
        "pdr %lu/%lu   win %s\n"
        "batt %dmV  soc %d%%  supply %dmV\n"
        "class %c  prog %s  life %u  tier %u\n"
        "led %s r%u g%u b%u w%u (%u px)\n"
        "fw %s",
        (unsigned long)((now - p.lastHeardMs) / 1000), p.rssi, p.rssiEwma,
        (unsigned long)p.recv, (unsigned long)total, win, p.battMv,
        p.soc == 255 ? -1 : p.soc, p.supplyMv, classLetter(p.classLatched),
        programName(p.hasFixtureState ? p.activeProgram : 0),
        p.hasFixtureState ? p.lifeState : 0,
        p.hasFixtureState ? p.powerTier : 0,
        p.ledRailOn ? "on" : "off", p.ledR, p.ledG, p.ledB, p.ledW,
        p.ledLitPixels, p.hasFw ? p.fwRev : "(unknown)");
  } else {
    lv_label_set_text(body, "(no longer in census)");
  }

  lv_obj_t *ident = lv_button_create(scr);
  lv_obj_align(ident, LV_ALIGN_BOTTOM_LEFT, 8, -10);
  lv_obj_t *il = lv_label_create(ident);
  lv_label_set_text(il, LV_SYMBOL_EYE_OPEN " identify");
  lv_obj_add_event_cb(ident, identifyCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *back = lv_button_create(scr);
  lv_obj_align(back, LV_ALIGN_BOTTOM_RIGHT, -8, -10);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, LV_SYMBOL_LEFT " back");
  lv_obj_add_event_cb(back, backFromDetail, LV_EVENT_CLICKED, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), ident);
  lv_group_add_obj(lvglGroup(), back);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}

// Touch: the table clears its selection on finger-up, before CLICKED fires —
// so capture the row at selection time and open on the (non-scroll) release.
static uint32_t gTouchRow = 0;

static void tableSelChanged(lv_event_t *e) {
  lv_obj_t *table = (lv_obj_t *)lv_event_get_target(e);
  uint32_t row, col;
  lv_table_get_selected_cell(table, &row, &col);
  if (row != LV_TABLE_CELL_NONE && row > 0) {
    gTouchRow = row;
    gSelRow = row;  // trackball selection follows the finger
    rememberSelectedRow();
    lv_obj_invalidate(table);
  }
}

static void tableClicked(lv_event_t *) {
  if (gTouchRow >= 1 && gTouchRow - 1 < gRowCount)
    openDetail(gRows[gTouchRow - 1].view.id);
  gTouchRow = 0;
}

// ------------------------------------------------------------- fleet-wide --

static void powerCb(lv_event_t *) {
  stopTimer();
  appPowerOpen();
}

static void identifyRollTick(lv_timer_t *) {
  if (gIdentifyTargetIndex >= gIdentifyTargetCount) {
    size_t sent = gIdentifyTargetCount;
    if (gIdentifyTimer) lv_timer_delete(gIdentifyTimer);
    gIdentifyTimer = nullptr;
    gIdentifyTargetCount = 0;
    gIdentifyTargetIndex = 0;
    if (gHeader)
      lv_label_set_text_fmt(gHeader, "Blink sent to %u filtered live",
                            (unsigned)sent);
    return;
  }
  meshIdentify(gIdentifyTargets[gIdentifyTargetIndex], 30, 2 /*green*/,
               1 /*blink*/, 128);
  ++gIdentifyTargetIndex;
  if (gHeader)
    lv_label_set_text_fmt(gHeader, "Fleet blink %u/%u",
                          (unsigned)gIdentifyTargetIndex,
                          (unsigned)gIdentifyTargetCount);
}

static void identifyFilteredYes(void *) {
  if (gIdentifyTimer || gIdentifyTargetCount == 0) return;
  gIdentifyTargetIndex = 0;
  gIdentifyTimer = lv_timer_create(identifyRollTick, 80, nullptr);
  identifyRollTick(nullptr);
}

static void identifyFilteredCb(lv_event_t *) {
  if (gIdentifyTimer) {
    if (gHeader) lv_label_set_text(gHeader, "Filtered blink already running");
    return;
  }
  gIdentifyTargetCount = 0;
  for (size_t i = 0;
       i < gRowCount && gIdentifyTargetCount < CENSUS_MAX_TRACKED; ++i) {
    if (!gRows[i].fresh) continue;
    memcpy(gIdentifyTargets[gIdentifyTargetCount++], gRows[i].view.id, 3);
  }
  if (gIdentifyTargetCount == 0) {
    if (gHeader) lv_label_set_text(gHeader, "No live fixtures in this view");
    return;
  }
  char summary[120];
  snprintf(summary, sizeof(summary),
           "Blink %u LIVE fixtures in the current filtered view green for 30 s?",
           (unsigned)gIdentifyTargetCount);
  uiConfirm(summary, "Fleet filter", identifyFilteredYes, nullptr);
}

// ------------------------------------------------------------ view options --

static lv_obj_t *gScopeDd = nullptr;
static lv_obj_t *gClassDd = nullptr;
static lv_obj_t *gBatteryDd = nullptr;
static lv_obj_t *gSortDd = nullptr;

static void readViewSettings() {
  if (gScopeDd)
    gViewSettings.scope = (FleetRowScope)lv_dropdown_get_selected(gScopeDd);
  if (gClassDd)
    gViewSettings.classFilter =
        (FleetClassFilter)lv_dropdown_get_selected(gClassDd);
  if (gBatteryDd)
    gViewSettings.batteryFilter =
        (FleetBatteryFilter)lv_dropdown_get_selected(gBatteryDd);
  if (gSortDd)
    gViewSettings.sort = (FleetSortMode)lv_dropdown_get_selected(gSortDd);
}

static void closeViewSettings() {
  gScopeDd = nullptr;
  gClassDd = nullptr;
  gBatteryDd = nullptr;
  gSortDd = nullptr;
  gRestoreSelectionScroll = true;
  appFleetOpen();
}

static void viewDoneCb(lv_event_t *) {
  readViewSettings();
  closeViewSettings();
}

static void viewDefaultsCb(lv_event_t *) {
  gViewSettings = fleetViewDefaults();
  if (gScopeDd) lv_dropdown_set_selected(gScopeDd, 0);
  if (gClassDd) lv_dropdown_set_selected(gClassDd, 0);
  if (gBatteryDd) lv_dropdown_set_selected(gBatteryDd, 0);
  if (gSortDd) lv_dropdown_set_selected(gSortDd, 0);
}

static lv_obj_t *viewDropdown(lv_obj_t *screen, const char *options, int y,
                              uint16_t selected) {
  lv_obj_t *dropdown = lv_dropdown_create(screen);
  lv_dropdown_set_options(dropdown, options);
  lv_dropdown_set_selected(dropdown, selected);
  lv_obj_set_pos(dropdown, 92, y);
  lv_obj_set_width(dropdown, 220);
  lv_group_add_obj(lvglGroup(), dropdown);
  return dropdown;
}

static lv_obj_t *viewLabel(lv_obj_t *screen, const char *text, int y) {
  lv_obj_t *label = lv_label_create(screen);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_label_set_text(label, text);
  lv_obj_set_pos(label, 8, y);
  return label;
}

static void viewCb(lv_event_t *) {
  stopTimer();
  lvglSetNavHooks(nullptr);
  lv_obj_t *screen = lv_obj_create(nullptr);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(screen);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_label_set_text(title, "Fleet view");
  lv_obj_set_pos(title, 8, 3);

  lv_group_remove_all_objs(lvglGroup());
  viewLabel(screen, "rows", 43);
  gScopeDd = viewDropdown(screen,
                          "roster + live\nseen since boot\nlive now", 32,
                          (uint16_t)gViewSettings.scope);
  viewLabel(screen, "class", 80);
  gClassDd = viewDropdown(
      screen,
      "all light types\ndownlights\nperimeter\nuplights\nchandelier\nunknown",
      69, (uint16_t)gViewSettings.classFilter);
  viewLabel(screen, "battery", 117);
  gBatteryDd = viewDropdown(
      screen,
      "all battery\ngood >3.20 V\nnear low 3.10-3.20\nlow <=3.10 V\noff air\nno valid VBAT",
      106, (uint16_t)gViewSettings.batteryFilter);
  viewLabel(screen, "sort", 154);
  gSortDd = viewDropdown(
      screen,
      "callsign (stable)\nshort ID (stable)\nvoltage low first\nvoltage high first\nmost recent\nstrongest signal",
      143, (uint16_t)gViewSettings.sort);

  lv_obj_t *defaults = lv_button_create(screen);
  lv_obj_set_size(defaults, 100, 34);
  lv_obj_set_pos(defaults, 4, 201);
  lv_obj_t *defaultsLabel = lv_label_create(defaults);
  lv_label_set_text(defaultsLabel, "defaults");
  lv_obj_center(defaultsLabel);
  lv_obj_add_event_cb(defaults, viewDefaultsCb, LV_EVENT_CLICKED, nullptr);
  lv_group_add_obj(lvglGroup(), defaults);

  lv_obj_t *done = lv_button_create(screen);
  lv_obj_set_size(done, 204, 34);
  lv_obj_set_pos(done, 112, 201);
  lv_obj_t *doneLabel = lv_label_create(done);
  lv_label_set_text(doneLabel, LV_SYMBOL_OK " apply to Fleet");
  lv_obj_center(doneLabel);
  lv_obj_add_event_cb(done, viewDoneCb, LV_EVENT_CLICKED, nullptr);
  lv_group_add_obj(lvglGroup(), done);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(screen);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}

// -------------------------------------------------------------------- open --

// Bottom button bar: header text owns the top line, actions live where the
// thumb is (touch-first per Ben), and nothing overlaps.
static lv_obj_t *makeBarButton(lv_obj_t *scr, int slot, const char *text,
                               lv_event_cb_t cb) {
  lv_obj_t *btn = lv_button_create(scr);
  lv_obj_set_size(btn, 76, 30);
  lv_obj_set_pos(btn, 2 + slot * 80, 208);
  lv_obj_t *l = lv_label_create(btn);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
  lv_label_set_text(l, text);
  lv_obj_center(l);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  return btn;
}

void appFleetOpen() {
  stopTimer();
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  gHeader = lv_label_create(scr);
  lv_obj_set_pos(gHeader, 8, 6);
  lv_label_set_text(gHeader, "Fleet");

  lv_obj_t *view = makeBarButton(scr, 0, "view", viewCb);
  lv_obj_t *identify =
      makeBarButton(scr, 1, LV_SYMBOL_EYE_OPEN " blink", identifyFilteredCb);
  lv_obj_t *power = makeBarButton(scr, 2, "power", powerCb);
  lv_obj_t *back = makeBarButton(scr, 3, LV_SYMBOL_LEFT " back", backFromFleet);

  gTable = lv_table_create(scr);
  lv_obj_set_pos(gTable, 0, 30);
  lv_obj_set_size(gTable, 320, 174);
  lv_obj_set_style_text_font(gTable, &lv_font_montserrat_14, 0);
  // Default cell padding wraps 3-char headers ("age" -> "ag/e"); tighten it.
  lv_obj_set_style_pad_hor(gTable, 4, LV_PART_ITEMS);
  lv_obj_set_style_pad_ver(gTable, 4, LV_PART_ITEMS);
  lv_table_set_column_count(gTable, 7);
  lv_table_set_column_width(gTable, 0, 16);  // reported-color chip
  lv_table_set_column_width(gTable, 1, 80);
  lv_table_set_column_width(gTable, 2, 40);
  lv_table_set_column_width(gTable, 3, 38);
  lv_table_set_column_width(gTable, 4, 38);
  lv_table_set_column_width(gTable, 5, 48);
  lv_table_set_column_width(gTable, 6, 60);
  lv_table_set_cell_value(gTable, 0, 0, "");
  lv_table_set_cell_value(gTable, 0, 1, "id cls");
  lv_table_set_cell_value(gTable, 0, 2, "age");
  lv_table_set_cell_value(gTable, 0, 3, "dBm");
  lv_table_set_cell_value(gTable, 0, 4, "pdr");
  lv_table_set_cell_value(gTable, 0, 5, "vbat");
  lv_table_set_cell_value(gTable, 0, 6, "prog");
  lv_obj_add_event_cb(gTable, tableSelChanged, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(gTable, tableClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(gTable, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  lv_obj_add_event_cb(gTable, tableDrawEvent, LV_EVENT_DRAW_TASK_ADDED, nullptr);

  // Table stays OUT of the focus group: the trackball drives its selection
  // through the nav hooks; left/right walks the bottom-bar buttons.
  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), view);
  lv_group_add_obj(lvglGroup(), identify);
  lv_group_add_obj(lvglGroup(), power);
  lv_group_add_obj(lvglGroup(), back);
  if (!gHasSelectedId) gSelRow = 1;
  lvglSetNavHooks(&kFleetHooks);

  gTimer = lv_timer_create(refreshTable, 2000, nullptr);
  refreshTable(nullptr);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
