#include <Arduino.h>
#include <lvgl.h>
#include <initializer_list>
#include "../core/storage_calendar.h"
#include "../hal/hal_board.h"
#include "../net/census_svc.h"
#include "../net/mesh_tx.h"
#include "../net/stream_svc.h"
#include "app_power.h"
#include "app_storage.h"
#include "lvgl_glue.h"
#include "ui_confirm.h"
#include "ui_shell.h"

static lv_obj_t *gScreen, *gMode, *gTarget, *gDay, *gHour, *gMinute, *gStatus;
static lv_timer_t *gTimer;
static uint8_t gCandidates[192][3], gPending[192][3];
static size_t gCandidateCount, gPendingCount;
static uint8_t gPendingMode;
static uint32_t gPendingWakeUtc;
static StorageDate gDays[8];
static bool gDatesReady;
static bool gShowingCampaign;

static uint8_t mode() {
  uint32_t i = lv_dropdown_get_selected(gMode);
  return i == 0 ? NB_STORAGE_USB_WAKE : (i == 1 ? NB_STORAGE_RESET_WAKE : 0);
}
static bool utcNow(uint32_t &utc) {
  GpsUtcObservation gps = halGpsUtc();
  uint32_t age = millis() - gps.receivedMs;
  if (!gps.valid || age > 10000UL || gps.utcS < 1735689600UL) return false;
  utc = gps.utcS + (gps.subMs + age) / 1000UL;
  return true;
}

static void populateTargets() {
  static CensusView rows[192];
  static PeerStat peer;
  size_t n = censusSnapshotSafe(rows, 192, millis());
  String options = "All supported fixtures";
  gCandidateCount = 0;
  for (size_t i = 0; i < n; ++i) {
    if (rows[i].ageMs > 1800000UL || !censusPeerSafe(rows[i].id, &peer) ||
        !peer.hasFw || strncmp(peer.fwRev, "fx-", 3) != 0) continue;
    const uint8_t wand[3] = {0xF4, 0x03, 0x44};
    if (memcmp(rows[i].id, wand, 3) == 0) continue;
    if (mode() && (!(peer.storageCapabilities & (mode() == 1 ? 1 : 2)) ||
                   millis() - peer.storageCapabilitiesHeardMs > 1800000UL)) continue;
    // Type 27 arrived in the August 17 transport image. Older images are not
    // assumed capable simply because an fx-style revision string exists.
    if (!mode() && strncmp(peer.fwRev + 3, "260817", 6) < 0) continue;
    memcpy(gCandidates[gCandidateCount++], rows[i].id, 3);
    char id[12];
    snprintf(id, sizeof(id), "\n%02X%02X%02X", rows[i].id[0], rows[i].id[1], rows[i].id[2]);
    options += id;
  }
  lv_dropdown_set_options(gTarget, options.c_str());
  lv_dropdown_set_selected(gTarget, 0);
}

static void refresh(lv_timer_t *) {
  if (!gScreen || lv_screen_active() != gScreen) return;
  StorageCampaignStatus s = meshStorageStatus();
  if (gShowingCampaign) {
    lv_label_set_text_fmt(gStatus, "%u targets: %u prepared, %u refused\n"
                                  "%lus left. Power-off unverified.",
        s.targets, s.prepared, s.refused, (unsigned long)(s.remainingMs / 1000));
  }
}
static void modeChanged(lv_event_t *) {
  if (uiConfirmIsOpen()) return;
  if (meshStorageStatus().active) return;
  gShowingCampaign = false;
  bool timed = mode() == 0;
  for (lv_obj_t *obj : {gDay, gHour, gMinute}) {
    if (timed) lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
  populateTargets();
  lv_label_set_text_fmt(gStatus, "%u supported targets. Listen first.\n%s",
      (unsigned)gCandidateCount,
      timed ? "Pacific time; 7-day max; timer can drift." :
      mode() == NB_STORAGE_USB_WAKE ? "USB/solar wakes; RESET will not. No timer." :
                                     "Physical RESET wakes. USB alone will not.");
}

static void applyYes(void *) {
  if (!gScreen || lv_screen_active() != gScreen) return;
  uint32_t seconds = 0;
  if (gPendingMode == 0) {
    uint32_t now;
    if (!utcNow(now) || gPendingWakeUtc <= now ||
        gPendingWakeUtc - now < 1800 || gPendingWakeUtc - now > 604800) {
      lv_label_set_text(gStatus, "NOT SENT: time changed or GPS is stale.");
      return;
    }
    seconds = gPendingWakeUtc - now;
  }
  streamStop();
  bool ok = meshStorageBegin(gPending, gPendingCount, gPendingMode, seconds);
  gShowingCampaign = ok;
  lv_label_set_text(gStatus, ok ? "Sending for 16 minutes.\n"
                                "Keep open; Back stops further sends."
                              : "NOT SENT: invalid roster or campaign already active.");
}

static void applyCb(lv_event_t *) {
  // Keyboard focus can reach background controls while the modal is open.
  // Never replace the target/mode/date represented by its visible summary.
  if (uiConfirmIsOpen()) return;
  if (meshStorageStatus().active) {
    lv_label_set_text(gStatus, "Campaign is running. Stop it before starting another.");
    return;
  }
  uint32_t selected = lv_dropdown_get_selected(gTarget);
  gPendingCount = selected ? 1 : gCandidateCount;
  if (!gPendingCount || selected > gCandidateCount) {
    lv_label_set_text(gStatus, "No supported targets. Update fixtures, then reopen.");
    return;
  }
  if (selected) memcpy(gPending[0], gCandidates[selected - 1], 3);
  else memcpy(gPending, gCandidates, gCandidateCount * 3);
  gPendingMode = mode();
  char summary[270];
  char target[32];
  if (gPendingCount == 1)
    snprintf(target, sizeof(target), "%02X%02X%02X", gPending[0][0], gPending[0][1], gPending[0][2]);
  else snprintf(target, sizeof(target), "%u listed fixtures", (unsigned)gPendingCount);
  if (gPendingMode == 0) {
    uint32_t now;
    if (!gDatesReady || !utcNow(now)) {
      lv_label_set_text(gStatus, "NOT SENT: fresh GPS time required. Reopen after fix.");
      return;
    }
    StorageDate date = gDays[lv_dropdown_get_selected(gDay)];
    date.hour = (int)lv_dropdown_get_selected(gHour);
    date.minute = (int)lv_dropdown_get_selected(gMinute) * 15;
    if (!storagePacificToUtc(date, gPendingWakeUtc) || gPendingWakeUtc <= now ||
        gPendingWakeUtc - now < 1800 || gPendingWakeUtc - now > 604800) {
      lv_label_set_text(gStatus, "NOT SENT: choose 30 min to 7 days ahead.\nDST gaps/repeated hours are refused.");
      return;
    }
    StorageDate actual = storagePacificAt(gPendingWakeUtc);
    snprintf(summary, sizeof(summary), "Sleep %s until %04d-%02d-%02d %02d:%02d %s. "
        "Wake stays LED-dark, radio resumes. Timer can drift; cannot recall asleep fixtures. "
        "Catch-up sends run 16 min.", target, date.year, date.month, date.day,
        date.hour, date.minute, actual.daylight ? "PDT" : "PST");
  } else {
    snprintf(summary, sizeof(summary), "Store %s with no timer. %s "
        "Cannot recall by radio. Sends run 16 min. Physical service will be needed.",
        target, gPendingMode == NB_STORAGE_USB_WAKE ?
        "USB/solar power or QON wakes; RESET does not." :
        "Physical RESET wakes; USB power alone does not.");
  }
  uiConfirm(summary, "Storage", applyYes, nullptr);
}

static void stopCb(lv_event_t *) {
  if (uiConfirmIsOpen()) return;
  meshStorageStop();
  gShowingCampaign = false;
  lv_label_set_text(gStatus, "Further sends stopped.\nAlready sleeping fixtures remain asleep.");
}
static void backCb(lv_event_t *) {
  if (uiConfirmIsOpen()) return;
  meshStorageStop();
  if (gTimer) lv_timer_delete(gTimer);
  gTimer = nullptr;
  gScreen = nullptr;
  appPowerOpen();
}
static lv_obj_t *dropdown(int x, int y, int width, const char *options) {
  lv_obj_t *obj = lv_dropdown_create(gScreen);
  lv_obj_set_pos(obj, x, y);
  lv_obj_set_width(obj, width);
  lv_dropdown_set_options(obj, options);
  return obj;
}
static lv_obj_t *button(int x, const char *label, lv_event_cb_t cb) {
  lv_obj_t *obj = lv_button_create(gScreen);
  lv_obj_set_pos(obj, x, 198);
  lv_obj_set_size(obj, 96, 36);
  lv_obj_t *text = lv_label_create(obj);
  lv_label_set_text(text, label);
  lv_obj_center(text);
  lv_obj_add_event_cb(obj, cb, LV_EVENT_CLICKED, nullptr);
  return obj;
}
static void screenDeleted(lv_event_t *event) {
  if (lv_event_get_target(event) != gScreen) return;
  meshStorageStop();
  if (gTimer) lv_timer_delete(gTimer);
  gTimer = nullptr;
  gScreen = nullptr;
}
void appStorageOpen() {
  lvglSetNavHooks(nullptr);
  uiShellSetTitle("Storage");
  gScreen = lv_obj_create(nullptr);
  lv_obj_add_event_cb(gScreen, screenDeleted, LV_EVENT_DELETE, nullptr);
  lv_obj_clear_flag(gScreen, LV_OBJ_FLAG_SCROLLABLE);
  gMode = dropdown(8, 39, 304, "Until USB power (ship)\nUntil physical RESET\nWake on date (max 7 days)");
  uint32_t now = 0;
  gDatesReady = utcNow(now);
  String dates;
  for (unsigned i = 0; i < 8; ++i) {
    gDays[i] = gDatesReady ? storagePacificDay(now, i) : StorageDate{2026, 1, 1, 0, 0, false};
    char label[20];
    snprintf(label, sizeof(label), "%s%04d-%02d-%02d", i ? "\n" : "", gDays[i].year, gDays[i].month, gDays[i].day);
    dates += label;
  }
  gDay = dropdown(8, 83, 148, dates.c_str());
  lv_dropdown_set_selected(gDay, 7);
  String hours;
  for (int i = 0; i < 24; ++i) { if (i) hours += '\n'; if (i < 10) hours += '0'; hours += i; }
  gHour = dropdown(162, 83, 72, hours.c_str());
  lv_dropdown_set_selected(gHour, 9);
  gMinute = dropdown(240, 83, 72, "00\n15\n30\n45");
  gTarget = dropdown(8, 124, 304, "All supported fixtures");
  gStatus = lv_label_create(gScreen);
  lv_obj_set_pos(gStatus, 8, 163);
  lv_obj_set_width(gStatus, 304);
  lv_obj_set_style_text_font(gStatus, &lv_font_montserrat_14, 0);
  lv_obj_t *apply = button(8, "Review", applyCb);
  lv_obj_t *stop = button(112, "Stop sends", stopCb);
  lv_obj_t *back = button(216, "Back", backCb);
  lv_obj_add_event_cb(gMode, modeChanged, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_group_remove_all_objs(lvglGroup());
  for (lv_obj_t *obj : {gMode, gDay, gHour, gMinute, gTarget, apply, stop, back})
    lv_group_add_obj(lvglGroup(), obj);
  modeChanged(nullptr);
  lv_obj_t *old = lv_screen_active();
  lv_screen_load(gScreen);
  lv_obj_delete(old);
  gTimer = lv_timer_create(refresh, 1000, nullptr);
}
