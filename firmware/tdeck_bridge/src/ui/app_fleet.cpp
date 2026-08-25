#include <lvgl.h>

#include <Arduino.h>
#include <string.h>

#include "../core/fleet_registry_generated.h"
#include "../core/health_model.h"
#include "../net/census_svc.h"
#include "../net/mesh_tx.h"
#include "app_fleet.h"
#include "app_power.h"
#include "lvgl_glue.h"
#include "ui_task.h"

static lv_obj_t *gTable = nullptr;
static lv_obj_t *gHeader = nullptr;
static lv_timer_t *gTimer = nullptr;
static CensusView gRows[64];
static size_t gRowCount = 0;
static uint32_t gSelRow = 1;

static void openDetail(const uint8_t id[3]);
static lv_color_t ledChipColor(const CensusView &v);

static const char *callsignForId(const uint8_t id[3]) {
  const HealthRegistryEntry *entry =
      healthRegistryFind(kHealthRegistry, kHealthRegistryCount, id);
  return entry ? entry->callsign : nullptr;
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
  lv_draw_fill_dsc_t *fill = lv_draw_task_get_fill_dsc(task);
  if (!fill) return;
  uint32_t row = base->id1;
  uint32_t col = base->id2;
  if (col == 0 && row >= 1) {  // reported-color chip column
    if (row - 1 < gRowCount) {
      fill->color = ledChipColor(gRows[row - 1]);
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
  }
}

static bool fleetEnter() {
  if (!gTable) return false;
  if (gSelRow >= 1 && gSelRow - 1 < gRowCount) {
    openDetail(gRows[gSelRow - 1].id);
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
  gRowCount = censusSnapshotSafe(gRows, 64, now);
  // Live-first (age ascending), then strongest EWMA.
  for (size_t i = 1; i < gRowCount; ++i)
    for (size_t j = i; j > 0; --j) {
      bool swap = gRows[j].ageMs < gRows[j - 1].ageMs ||
                  (gRows[j].ageMs / 5000 == gRows[j - 1].ageMs / 5000 &&
                   gRows[j].rssiEwma > gRows[j - 1].rssiEwma);
      if (!swap) break;
      CensusView t = gRows[j];
      gRows[j] = gRows[j - 1];
      gRows[j - 1] = t;
    }

  int live = 0, seen = 0;
  censusCountsSafe(&live, &seen, now);
  lv_label_set_text_fmt(gHeader, "Fleet  %d/%d live", live, seen);

  lv_table_set_row_count(gTable, (uint32_t)gRowCount + 1);
  char buf[24];
  for (size_t i = 0; i < gRowCount; ++i) {
    const CensusView &v = gRows[i];
    uint32_t r = (uint32_t)i + 1;
    lv_table_set_cell_value(gTable, r, 0, "");  // chip column (draw hook)
    const char *callsign = callsignForId(v.id);
    if (callsign)
      snprintf(buf, sizeof(buf), "%s %c", callsign,
               classLetter(v.fixtureClass));
    else
      snprintf(buf, sizeof(buf), "%02X%02X%02X %c", v.id[0], v.id[1], v.id[2],
               classLetter(v.fixtureClass));
    lv_table_set_cell_value(gTable, r, 1, buf);
    uint32_t ageS = v.ageMs / 1000;
    if (ageS < 100) snprintf(buf, sizeof(buf), "%lus", (unsigned long)ageS);
    else snprintf(buf, sizeof(buf), "%lum", (unsigned long)(ageS / 60));
    lv_table_set_cell_value(gTable, r, 2, buf);
    snprintf(buf, sizeof(buf), "%d", v.rssiEwma);
    lv_table_set_cell_value(gTable, r, 3, buf);
    snprintf(buf, sizeof(buf), "%u%%", v.pdrX1000 / 10);
    lv_table_set_cell_value(gTable, r, 4, buf);
    if (v.soc == 255) snprintf(buf, sizeof(buf), "-");
    else snprintf(buf, sizeof(buf), "%u%%", v.soc);
    lv_table_set_cell_value(gTable, r, 5, buf);
    lv_table_set_cell_value(gTable, r, 6, programName(v.activeProgram));
  }
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

static void backFromDetail(lv_event_t *) { appFleetOpen(); }

static void openDetail(const uint8_t id[3]) {
  memcpy(gDetailId, id, 3);
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
    lv_obj_invalidate(table);
  }
}

static void tableClicked(lv_event_t *) {
  if (gTouchRow >= 1 && gTouchRow - 1 < gRowCount)
    openDetail(gRows[gTouchRow - 1].id);
  gTouchRow = 0;
}

// ------------------------------------------------------------- fleet-wide --

static void powerCb(lv_event_t *) {
  stopTimer();
  appPowerOpen();
}

static void releaseCb(lv_event_t *) {
  static const uint8_t kAll[3] = {0, 0, 0};
  // Release returns the fleet to autonomous — restorative, no confirm needed.
  meshProgramLease(kAll, 4, 0, 0x01, nullptr);
}

// -------------------------------------------------------------------- open --

// Bottom button bar: header text owns the top line, actions live where the
// thumb is (touch-first per Ben), and nothing overlaps.
static lv_obj_t *makeBarButton(lv_obj_t *scr, int slot, const char *text,
                               lv_event_cb_t cb) {
  lv_obj_t *btn = lv_button_create(scr);
  lv_obj_set_size(btn, 100, 30);
  lv_obj_set_pos(btn, 4 + slot * 106, 208);
  lv_obj_t *l = lv_label_create(btn);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
  lv_label_set_text(l, text);
  lv_obj_center(l);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  return btn;
}

void appFleetOpen() {
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  gHeader = lv_label_create(scr);
  lv_obj_set_pos(gHeader, 8, 6);
  lv_label_set_text(gHeader, "Fleet");

  lv_obj_t *dark = makeBarButton(scr, 0, "sleep/dark", powerCb);
  lv_obj_t *rel = makeBarButton(scr, 1, "release", releaseCb);
  lv_obj_t *back = makeBarButton(scr, 2, LV_SYMBOL_LEFT " back", backFromFleet);

  gTable = lv_table_create(scr);
  lv_obj_set_pos(gTable, 0, 30);
  lv_obj_set_size(gTable, 320, 174);
  lv_obj_set_style_text_font(gTable, &lv_font_montserrat_14, 0);
  // Default cell padding wraps 3-char headers ("age" -> "ag/e"); tighten it.
  lv_obj_set_style_pad_hor(gTable, 4, LV_PART_ITEMS);
  lv_obj_set_style_pad_ver(gTable, 4, LV_PART_ITEMS);
  lv_table_set_column_count(gTable, 7);
  lv_table_set_column_width(gTable, 0, 16);  // reported-color chip
  lv_table_set_column_width(gTable, 1, 82);
  lv_table_set_column_width(gTable, 2, 40);
  lv_table_set_column_width(gTable, 3, 44);
  lv_table_set_column_width(gTable, 4, 44);
  lv_table_set_column_width(gTable, 5, 42);
  lv_table_set_column_width(gTable, 6, 52);
  lv_table_set_cell_value(gTable, 0, 0, "");
  lv_table_set_cell_value(gTable, 0, 1, "id cls");
  lv_table_set_cell_value(gTable, 0, 2, "age");
  lv_table_set_cell_value(gTable, 0, 3, "dBm");
  lv_table_set_cell_value(gTable, 0, 4, "pdr");
  lv_table_set_cell_value(gTable, 0, 5, "soc");
  lv_table_set_cell_value(gTable, 0, 6, "prog");
  lv_obj_add_event_cb(gTable, tableSelChanged, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(gTable, tableClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(gTable, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  lv_obj_add_event_cb(gTable, tableDrawEvent, LV_EVENT_DRAW_TASK_ADDED, nullptr);

  // Table stays OUT of the focus group: the trackball drives its selection
  // through the nav hooks; left/right walks the bottom-bar buttons.
  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), dark);
  lv_group_add_obj(lvglGroup(), rel);
  lv_group_add_obj(lvglGroup(), back);
  gSelRow = 1;
  lvglSetNavHooks(&kFleetHooks);

  gTimer = lv_timer_create(refreshTable, 2000, nullptr);
  refreshTable(nullptr);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(scr);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}
