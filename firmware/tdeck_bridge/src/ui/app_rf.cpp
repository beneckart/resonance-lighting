#include <lvgl.h>

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../core/fleet_registry_generated.h"
#include "../core/rf_model.h"
#include "../net/census_svc.h"
#include "../net/espnow_link.h"
#include "../net/mesh_tx.h"
#include "../net/net_mgr.h"
#include "../store/store.h"
#include "app_rf.h"
#include "lvgl_glue.h"
#include "ui_theme.h"
#include "ui_task.h"

static lv_obj_t *gScreen = nullptr;
static lv_obj_t *gSummary = nullptr;
static lv_obj_t *gStrong = nullptr;
static lv_obj_t *gWeak = nullptr;
static lv_obj_t *gTail = nullptr;
static lv_obj_t *gPageButtonLabel = nullptr;
static lv_timer_t *gTimer = nullptr;
static bool gTailPage = false;

static CensusView gCensusRows[CENSUS_MAX_TRACKED];
static RfPeerObservation gPeerRows[CENSUS_MAX_TRACKED];

static void setTextColor(lv_obj_t *label, uint32_t rgb) {
  lv_obj_set_style_text_color(label, lv_color_hex(rgb), 0);
}

static void appendText(char *out, size_t cap, const char *format, ...) {
  size_t used = strlen(out);
  if (used >= cap - 1) return;
  va_list args;
  va_start(args, format);
  vsnprintf(out + used, cap - used, format, args);
  va_end(args);
}

static int compareId(const uint8_t a[3], const uint8_t b[3]) {
  return memcmp(a, b, 3);
}

static bool inProductionRoster(const uint8_t id[3]) {
  size_t lo = 0;
  size_t hi = kHealthRegistryCount;
  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    int cmp = compareId(id, kHealthRegistry[mid].id);
    if (cmp == 0) return true;
    if (cmp < 0) hi = mid;
    else lo = mid + 1;
  }
  return false;
}

static RfWifiState wifiState() {
  switch (netState()) {
    case NetState::OFF: return RfWifiState::MESH_ONLY;
    case NetState::CONNECTING: return RfWifiState::CONNECTING;
    case NetState::GUARD_BLOCKED: return RfWifiState::GUARD_BLOCKED;
    case NetState::ONLINE: return RfWifiState::ONLINE;
  }
  return RfWifiState::UNKNOWN;
}

static const char *wifiName(RfWifiState state) {
  switch (state) {
    case RfWifiState::MESH_ONLY: return "mesh-only";
    case RfWifiState::CONNECTING: return "joining";
    case RfWifiState::GUARD_BLOCKED: return "blocked";
    case RfWifiState::ONLINE: return "online";
    case RfWifiState::UNKNOWN: return "unknown";
  }
  return "unknown";
}

static const char *guardName(RfGuardState state) {
  switch (state) {
    case RfGuardState::MESH_ONLY: return "guard n/a";
    case RfGuardState::CHECKING: return "checking";
    case RfGuardState::MATCH: return "guard OK";
    case RfGuardState::BLOCKED: return "GUARD BLOCK";
    case RfGuardState::INCONSISTENT: return "guard ?";
    case RfGuardState::UNAVAILABLE: return "guard n/a";
  }
  return "guard n/a";
}

static void formatAge(uint32_t ageMs, char *out, size_t cap) {
  uint32_t seconds = ageMs / 1000;
  if (seconds < 100) snprintf(out, cap, "%lus", (unsigned long)seconds);
  else if (seconds < 6000)
    snprintf(out, cap, "%lum", (unsigned long)(seconds / 60));
  else
    snprintf(out, cap, "%luh", (unsigned long)(seconds / 3600));
}

static void formatPdr(const RfRankedPeer &peer, char *out, size_t cap) {
  if (peer.pdrSource == RfPdrSource::UNAVAILABLE) {
    snprintf(out, cap, "-");
    return;
  }
  char source = peer.pdrSource == RfPdrSource::WINDOW ? 'w' : 'c';
  snprintf(out, cap, "%u%c", peer.pdrX1000 / 10, source);
}

static void formatRankColumn(const char *heading, const RfRankedPeer *rows,
                             size_t count, char *out, size_t cap) {
  out[0] = '\0';
  appendText(out, cap, "%s\n", heading);
  if (!count) {
    appendText(out, cap, "(no fresh RSSI)");
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    char age[12];
    char pdr[12];
    formatAge(rows[i].ageMs, age, sizeof(age));
    formatPdr(rows[i], pdr, sizeof(pdr));
    appendText(out, cap, "%02X%02X%02X %d %s %s%s", rows[i].id[0],
               rows[i].id[1], rows[i].id[2], rows[i].rssi, pdr, age,
               i + 1 == count ? "" : "\n");
  }
}

static void renderLinkPage(uint32_t now) {
  size_t count = censusSnapshotSafe(gCensusRows, CENSUS_MAX_TRACKED, now);
  for (size_t i = 0; i < count; ++i) {
    memcpy(gPeerRows[i].id, gCensusRows[i].id, 3);
    gPeerRows[i].ageMs = gCensusRows[i].ageMs;
    gPeerRows[i].rssiEwma = gCensusRows[i].rssiEwma;
    gPeerRows[i].rssiAvailable = gCensusRows[i].rssiEwma < 0;
    gPeerRows[i].pdrX1000 = gCensusRows[i].pdrX1000;
    gPeerRows[i].windowPdrX1000 = gCensusRows[i].winPdrX1000;
    gPeerRows[i].inProductionRoster =
        inProductionRoster(gCensusRows[i].id);
  }

  RfReport report;
  // The census initializes coverage optimistically; its first real 60-second
  // observation window cannot have closed during the first minute of boot.
  uint16_t observed =
      now < 60000 ? RF_PDR_UNAVAILABLE : censusObservedPermilleSafe();
  rfBuildReport(gPeerRows, count, censusFreshMsSafe(), kHealthRegistryCount,
                observed, &report);
  MeshStats mesh = espnowStats();
  RfWifiState wifi = wifiState();
  uint8_t meshChannel = settings().channel;
  uint8_t apChannel = netApChannel();
  RfGuardState guard = rfGuardState(wifi, meshChannel, apChannel);

  char listen[32];
  if (report.summary.coverage == RfCoverageState::UNAVAILABLE)
    snprintf(listen, sizeof(listen), "unavailable");
  else
    snprintf(listen, sizeof(listen), "%u.%u%% observed",
             report.summary.observedPermille / 10,
             report.summary.observedPermille % 10);

  char ap[8];
  if ((wifi == RfWifiState::ONLINE ||
       wifi == RfWifiState::GUARD_BLOCKED) &&
      apChannel >= 1 && apChannel <= 13)
    snprintf(ap, sizeof(ap), "%u", apChannel);
  else
    snprintf(ap, sizeof(ap), "-");

  char summary[512];
  snprintf(summary, sizeof(summary),
           "census %u live / %u seen / %u stale\n"
           "roster %u/%u; unobs %u; other live %u\n"
           "listen %s | PDR w=60s c=sync\n"
           "mesh %s ch%u | wifi %s AP%s %s\n"
           "rx %lu invalid %lu ring %lu\n"
           "tx ok %lu fail %lu",
           report.summary.live, report.summary.seen, report.summary.stale,
           report.summary.rosterSeen, (unsigned)kHealthRegistryCount,
           report.summary.rosterUnobserved, report.summary.foreignLive, listen,
           espnowUp() ? "UP" : "DOWN", meshChannel, wifiName(wifi), ap,
           guardName(guard), (unsigned long)mesh.frames,
           (unsigned long)mesh.dropped, (unsigned long)censusRingDrops(),
           (unsigned long)meshTxSendOk(), (unsigned long)meshTxSendFail());
  lv_label_set_text(gSummary, summary);

  char strongest[192];
  char weakest[192];
  formatRankColumn("Strong RSSI/PDR/age", report.strongest,
                   report.strongestCount, strongest, sizeof(strongest));
  formatRankColumn("Weak RSSI/PDR/age", report.weakest,
                   report.weakestCount, weakest, sizeof(weakest));
  lv_label_set_text(gStrong, strongest);
  lv_label_set_text(gWeak, weakest);
}

static void renderTailPage(uint32_t now) {
  MeshStats mesh = espnowStats();
  char summary[256];
  snprintf(summary, sizeof(summary),
           "mesh %s ch%u | rx %lu invalid %lu ring %lu\n"
           "tx ok %lu fail %lu | latest first",
           espnowUp() ? "UP" : "DOWN", settings().channel,
           (unsigned long)mesh.frames, (unsigned long)mesh.dropped,
           (unsigned long)censusRingDrops(), (unsigned long)meshTxSendOk(),
           (unsigned long)meshTxSendFail());
  lv_label_set_text(gSummary, summary);

  TailFrame frames[7];
  size_t count = censusTailSafe(frames, sizeof(frames) / sizeof(frames[0]));
  char tail[512] = "ID      type RSSI age  bytes\n";
  if (!count) appendText(tail, sizeof(tail), "(no frames observed)");
  for (size_t i = 0; i < count; ++i) {
    char age[12];
    formatAge(now - frames[i].ms, age, sizeof(age));
    appendText(tail, sizeof(tail), "%02X%02X%02X  t%02u  %4d %-4s %3u%s",
               frames[i].id[0], frames[i].id[1], frames[i].id[2],
               frames[i].type, frames[i].rssi, age, frames[i].len,
               i + 1 == count ? "" : "\n");
  }
  lv_label_set_text(gTail, tail);
}

static void refresh(lv_timer_t *) {
  if (!gScreen || lv_screen_active() != gScreen) return;
  uint32_t now = millis();
  if (gTailPage) renderTailPage(now);
  else renderLinkPage(now);
}

static void showPage(bool tail) {
  gTailPage = tail;
  if (tail) {
    lv_obj_add_flag(gStrong, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(gWeak, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(gTail, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(gPageButtonLabel, "links");
  } else {
    lv_obj_clear_flag(gStrong, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(gWeak, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(gTail, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(gPageButtonLabel, "frames");
  }
  refresh(nullptr);
}

static void pageCb(lv_event_t *) { showPage(!gTailPage); }

static void backCb(lv_event_t *) {
  if (gTimer) lv_timer_delete(gTimer);
  gTimer = nullptr;
  gScreen = nullptr;
  gSummary = nullptr;
  gStrong = nullptr;
  gWeak = nullptr;
  gTail = nullptr;
  gPageButtonLabel = nullptr;
  uiGoHome();
}

void appRfOpen() {
  lvglSetNavHooks(nullptr);
  gScreen = lv_obj_create(nullptr);
  lv_obj_clear_flag(gScreen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(gScreen, uiScreenColor(), 0);

  lv_obj_t *title = lv_label_create(gScreen);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  setTextColor(title, uiDayMode() ? 0x111827 : 0xFFFFFF);
  lv_label_set_text(title, "RF Diagnostics");
  lv_obj_set_pos(title, 5, 3);

  gSummary = lv_label_create(gScreen);
  lv_obj_set_style_text_font(gSummary, &lv_font_montserrat_14, 0);
  setTextColor(gSummary, uiDayMode() ? 0x111827 : 0xF4F7FA);
  lv_obj_set_width(gSummary, 310);
  lv_obj_set_pos(gSummary, 5, 27);

  gStrong = lv_label_create(gScreen);
  lv_obj_set_style_text_font(gStrong, &lv_font_montserrat_14, 0);
  setTextColor(gStrong, uiDayMode() ? 0x166534 : 0xC8F7D4);
  lv_obj_set_width(gStrong, 153);
  lv_obj_set_pos(gStrong, 5, 128);

  gWeak = lv_label_create(gScreen);
  lv_obj_set_style_text_font(gWeak, &lv_font_montserrat_14, 0);
  setTextColor(gWeak, uiDayMode() ? 0x8A4B00 : 0xFFE0A3);
  lv_obj_set_width(gWeak, 153);
  lv_obj_set_pos(gWeak, 162, 128);

  gTail = lv_label_create(gScreen);
  lv_obj_set_style_text_font(gTail, &lv_font_montserrat_14, 0);
  setTextColor(gTail, uiDayMode() ? 0x111827 : 0xF4F7FA);
  lv_obj_set_width(gTail, 310);
  lv_obj_set_pos(gTail, 5, 66);

  lv_obj_t *page = lv_button_create(gScreen);
  lv_obj_set_size(page, 98, 29);
  lv_obj_set_pos(page, 5, 207);
  gPageButtonLabel = lv_label_create(page);
  setTextColor(gPageButtonLabel, 0xFFFFFF);
  lv_obj_center(gPageButtonLabel);
  lv_obj_add_event_cb(page, pageCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *back = lv_button_create(gScreen);
  lv_obj_set_size(back, 98, 29);
  lv_obj_set_pos(back, 217, 207);
  lv_obj_t *backLabel = lv_label_create(back);
  setTextColor(backLabel, 0xFFFFFF);
  lv_label_set_text(backLabel, LV_SYMBOL_LEFT " back");
  lv_obj_center(backLabel);
  lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), page);
  lv_group_add_obj(lvglGroup(), back);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(gScreen);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);

  showPage(false);
  gTimer = lv_timer_create(refresh, 1000, nullptr);
}
