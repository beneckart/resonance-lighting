#include <lvgl.h>

#include <Arduino.h>
#include <string.h>

#include "../core/fleet_registry_generated.h"
#include "../core/health_model.h"
#include "../net/census_svc.h"
#include "app_health.h"
#include "lvgl_glue.h"
#include "ui_task.h"

static lv_obj_t *gScreen = nullptr;
static lv_obj_t *gHeader = nullptr;
static lv_obj_t *gCaption = nullptr;
static lv_obj_t *gBack = nullptr;
static lv_timer_t *gTimer = nullptr;

static CensusView gCensusRows[CENSUS_MAX_TRACKED];
static HealthObservation gObservations[CENSUS_MAX_TRACKED];
static HealthTile gTiles[CENSUS_MAX_TRACKED];
static lv_obj_t *gTileObjects[CENSUS_MAX_TRACKED];
static size_t gTileCount = 0;
static size_t gUiTileCount = 0;
static uint8_t gColumns = 16;

static void openDetail(const uint8_t id[3]);

static const char *bandName(BatteryHealthBand band) {
  switch (band) {
    case BatteryHealthBand::GOOD: return "GOOD";
    case BatteryHealthBand::NEAR_LOW: return "NEAR LOW";
    case BatteryHealthBand::LOW_BATTERY: return "LOW";
    case BatteryHealthBand::OFF_AIR: return "OFF AIR";
    case BatteryHealthBand::UNKNOWN: return "NO VBAT";
  }
  return "?";
}

static lv_color_t bandColor(BatteryHealthBand band) {
  switch (band) {
    case BatteryHealthBand::GOOD: return lv_color_hex(0x1B8A3A);
    case BatteryHealthBand::NEAR_LOW: return lv_color_hex(0xD9A500);
    case BatteryHealthBand::LOW_BATTERY: return lv_color_hex(0xD52B2B);
    case BatteryHealthBand::UNKNOWN: return lv_color_hex(0x2F80C9);
    case BatteryHealthBand::OFF_AIR: return lv_color_hex(0x343A40);
  }
  return lv_color_hex(0x343A40);
}

static const char *registryStatusName(const HealthRegistryEntry *entry) {
  if (!entry) return "unregistered";
  return entry->status == HealthRegistryStatus::COMMISSION_FAILED
             ? "commission failed"
             : "commissioned";
}

static char classLetter(uint8_t cls) {
  switch (cls) {
    case 1: return 'D';
    case 2: return 'P';
    case 3: return 'U';
    case 4: return 'C';
    default: return '?';
  }
}

static const char *programName(uint8_t program) {
  switch (program) {
    case 0: return "idle";
    case 1: return "ca";
    case 2: return "bridge";
    case 3: return "direct";
    case 4: return "dark";
    default: return "?";
  }
}

static int tileIndexForObject(lv_obj_t *object) {
  for (size_t i = 0; i < gUiTileCount; ++i) {
    if (gTileObjects[i] == object) return (int)i;
  }
  return -1;
}

static int focusedTileIndex() {
  lv_group_t *group = lvglGroup();
  return group ? tileIndexForObject(lv_group_get_focused(group)) : -1;
}

static void updateCaption(int index) {
  if (!gCaption) return;
  if (index < 0 || (size_t)index >= gTileCount) {
    lv_label_set_text(gCaption, "Select a square for fixture details");
    return;
  }
  const HealthTile &tile = gTiles[index];
  char identity[24];
  if (tile.registry)
    snprintf(identity, sizeof(identity), "%s [%02X%02X%02X]",
             tile.registry->callsign, tile.id[0], tile.id[1], tile.id[2]);
  else
    snprintf(identity, sizeof(identity), "%02X%02X%02X", tile.id[0],
             tile.id[1], tile.id[2]);
  if (tile.band == BatteryHealthBand::OFF_AIR) {
    if (tile.ageMs == UINT32_MAX) {
      lv_label_set_text_fmt(gCaption, "%s  OFF AIR  not seen", identity);
    } else {
      lv_label_set_text_fmt(gCaption, "%s  OFF AIR  last %lus", identity,
                            (unsigned long)(tile.ageMs / 1000));
    }
  } else if (tile.band == BatteryHealthBand::UNKNOWN) {
    lv_label_set_text_fmt(gCaption, "%s  live  no valid VBAT", identity);
  } else {
    lv_label_set_text_fmt(gCaption, "%s  %d.%03d V  %s", identity,
                          tile.battMv / 1000, tile.battMv % 1000,
                          bandName(tile.band));
  }
}

static void tileFocused(lv_event_t *event) {
  updateCaption(tileIndexForObject((lv_obj_t *)lv_event_get_target(event)));
}

static void tileClicked(lv_event_t *event) {
  int index = tileIndexForObject((lv_obj_t *)lv_event_get_target(event));
  if (index >= 0 && (size_t)index < gTileCount)
    openDetail(gTiles[index].id);
}

static void stopHealthTimer() {
  if (gTimer) {
    lv_timer_delete(gTimer);
    gTimer = nullptr;
  }
}

static void backFromHealth(lv_event_t *) {
  stopHealthTimer();
  gScreen = nullptr;
  gHeader = nullptr;
  gCaption = nullptr;
  gBack = nullptr;
  gUiTileCount = 0;
  uiGoHome();
}

static bool healthVertical(int direction) {
  int current = focusedTileIndex();
  if (current < 0) return false;
  int target = current + direction * gColumns;
  if (target < 0 || target >= (int)gUiTileCount) return true;
  lv_group_focus_obj(gTileObjects[target]);
  return true;
}

static const UiNavHooks kHealthHooks = {healthVertical, nullptr};

struct GridLayout {
  uint8_t columns;
  uint8_t tile;
  uint8_t gap;
};

static GridLayout layoutFor(size_t count) {
  if (count <= 144) return {16, 17, 2};
  return {20, 14, 2};  // up to the complete 192-entry census in ten rows
}

static bool tileIdentityChanged(size_t count) {
  if (count != gUiTileCount) return true;
  for (size_t i = 0; i < count; ++i) {
    const uint8_t *oldId = (const uint8_t *)lv_obj_get_user_data(gTileObjects[i]);
    if (!oldId || memcmp(oldId, gTiles[i].id, 3) != 0) return true;
  }
  return false;
}

static size_t firstAttentionTile() {
  const BatteryHealthBand priority[] = {
      BatteryHealthBand::LOW_BATTERY, BatteryHealthBand::NEAR_LOW,
      BatteryHealthBand::UNKNOWN, BatteryHealthBand::OFF_AIR,
      BatteryHealthBand::GOOD};
  for (BatteryHealthBand band : priority) {
    for (size_t i = 0; i < gTileCount; ++i) {
      if (gTiles[i].band == band) return i;
    }
  }
  return 0;
}

static void rebuildTileObjects() {
  lv_group_remove_all_objs(lvglGroup());
  for (size_t i = 0; i < gUiTileCount; ++i) {
    if (gTileObjects[i]) lv_obj_delete(gTileObjects[i]);
    gTileObjects[i] = nullptr;
  }

  GridLayout layout = layoutFor(gTileCount);
  gColumns = layout.columns;
  uint16_t width = layout.columns * layout.tile +
                   (layout.columns - 1) * layout.gap;
  int16_t x0 = (320 - width) / 2;
  const int16_t y0 = 28;

  for (size_t i = 0; i < gTileCount; ++i) {
    lv_obj_t *tile = lv_button_create(gScreen);
    gTileObjects[i] = tile;
    lv_obj_set_size(tile, layout.tile, layout.tile);
    lv_obj_set_pos(tile,
                   x0 + (i % layout.columns) * (layout.tile + layout.gap),
                   y0 + (i / layout.columns) * (layout.tile + layout.gap));
    lv_obj_set_style_radius(tile, 1, 0);
    lv_obj_set_style_pad_all(tile, 0, 0);
    lv_obj_set_style_shadow_width(tile, 0, 0);
    lv_obj_set_style_border_width(tile, 0, 0);
    lv_obj_set_style_border_color(tile, lv_color_white(), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(tile, 2, LV_STATE_FOCUSED);
    lv_obj_set_user_data(tile, gTiles[i].id);
    lv_obj_add_event_cb(tile, tileFocused, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(tile, tileClicked, LV_EVENT_CLICKED, nullptr);
    lv_group_add_obj(lvglGroup(), tile);
  }
  lv_group_add_obj(lvglGroup(), gBack);
  gUiTileCount = gTileCount;
  if (gTileCount) lv_group_focus_obj(gTileObjects[firstAttentionTile()]);
}

static void styleTiles() {
  for (size_t i = 0; i < gUiTileCount; ++i) {
    lv_obj_t *object = gTileObjects[i];
    lv_color_t color = bandColor(gTiles[i].band);
    lv_obj_set_style_bg_color(object, color, 0);
    lv_obj_set_style_bg_color(object, color, LV_STATE_FOCUSED);
    if (gTiles[i].band == BatteryHealthBand::LOW_BATTERY) {
      lv_obj_set_style_border_color(object, lv_color_hex(0xFFB0B0), 0);
      lv_obj_set_style_border_width(object, 1, 0);
    } else if (!gTiles[i].registry) {
      lv_obj_set_style_border_color(object, lv_color_hex(0x7FDBFF), 0);
      lv_obj_set_style_border_width(object, 1, 0);
    } else {
      lv_obj_set_style_border_width(object, 0, 0);
    }
  }
}

static void refreshHealth(lv_timer_t *) {
  if (!gScreen || lv_screen_active() != gScreen) return;
  uint32_t now = millis();
  size_t censusCount =
      censusSnapshotSafe(gCensusRows, CENSUS_MAX_TRACKED, now);
  for (size_t i = 0; i < censusCount; ++i) {
    memcpy(gObservations[i].id, gCensusRows[i].id, 3);
    gObservations[i].ageMs = gCensusRows[i].ageMs;
    gObservations[i].battMv = gCensusRows[i].battMv;
  }
  gTileCount = healthBuildTiles(
      kHealthRegistry, kHealthRegistryCount, gObservations, censusCount,
      censusFreshMsSafe(), gTiles, CENSUS_MAX_TRACKED);

  if (tileIdentityChanged(gTileCount)) rebuildTileObjects();
  styleTiles();

  HealthSummary summary = healthSummarize(gTiles, gTileCount);
  lv_label_set_text_fmt(gHeader, "Health G%u Y%u R%u ?%u -%u",
                        summary.good, summary.nearLow, summary.low,
                        summary.unknown, summary.offAir);
  updateCaption(focusedTileIndex());
}

static void backFromDetail(lv_event_t *) { appHealthOpen(); }

static void homeFromDetail(lv_event_t *) { uiGoHome(); }

static void openDetail(const uint8_t id[3]) {
  stopHealthTimer();
  lvglSetNavHooks(nullptr);
  const HealthRegistryEntry *registry =
      healthRegistryFind(kHealthRegistry, kHealthRegistryCount, id);
  PeerStat peer;
  bool hasPeer = censusPeerSafe(id, &peer);

  lv_obj_t *screen = lv_obj_create(nullptr);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(screen);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  if (registry)
    lv_label_set_text_fmt(title, "%s  %02X%02X%02X", registry->callsign,
                          id[0], id[1], id[2]);
  else
    lv_label_set_text_fmt(title, "%02X%02X%02X", id[0], id[1], id[2]);
  lv_obj_set_pos(title, 8, 5);

  lv_obj_t *body = lv_label_create(screen);
  lv_obj_set_style_text_font(body, &lv_font_montserrat_14, 0);
  lv_obj_set_width(body, 304);
  lv_obj_set_pos(body, 8, 38);

  const char *role = registry && registry->role[0] ? registry->role : "unassigned";
  uint16_t capacity = registry ? registry->capacityMah : 0;
  if (!hasPeer) {
    lv_label_set_text_fmt(
        body,
        "OFF AIR - not observed since bridge boot\n"
        "registry: %s\n"
        "role: %s\n"
        "capacity: %s",
        registryStatusName(registry), role, capacity ? "declared below" : "unknown");
    if (capacity)
      lv_label_set_text_fmt(body,
                            "OFF AIR - not observed since bridge boot\n"
                            "registry: %s\nrole: %s\ncapacity: %u mAh",
                            registryStatusName(registry), role, capacity);
  } else {
    uint32_t ageMs = millis() - peer.lastHeardMs;
    BatteryHealthBand band =
        batteryHealthBand(ageMs < censusFreshMsSafe(), peer.battMv);
    uint32_t total = peer.recv + peer.gaps;
    uint16_t pdr = total ? (uint16_t)((uint64_t)peer.recv * 1000 / total) : 0;
    int soc = peer.soc == 255 ? -1 : peer.soc;
    lv_label_set_text_fmt(
        body,
        "%s  battery %d.%03d V  current %d mA\n"
        "age %lus  RSSI %d/%d  PDR %u.%u%%\n"
        "supply %d.%03d V  %d mA  good %u\n"
        "SOC %d%% advisory  class %c  tier %u\n"
        "program %s  life %u  sensors 0x%02X%s\n"
        "registry %s  %u mAh  %s\n"
        "fw %s",
        bandName(band), peer.battMv / 1000, peer.battMv % 1000, peer.battMa,
        (unsigned long)(ageMs / 1000), peer.rssi, peer.rssiEwma, pdr / 10,
        pdr % 10, peer.supplyMv / 1000, peer.supplyMv % 1000, peer.supplyMa,
        peer.supplyGood, soc,
        classLetter(peer.classLatched),
        peer.hasFixtureState ? peer.powerTier : 0,
        programName(peer.hasFixtureState ? peer.activeProgram : 0),
        peer.hasFixtureState ? peer.lifeState : 0,
        peer.hasIdentityRecovery ? peer.sensorBits : 0,
        peer.hasIdentityRecovery && peer.classMismatch ? " MISMATCH" : "",
        registryStatusName(registry), capacity, role,
        peer.hasFw ? peer.fwRev : "(unknown)");
  }

  lv_obj_t *back = lv_button_create(screen);
  lv_obj_set_size(back, 116, 30);
  lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 8, -8);
  lv_obj_t *backLabel = lv_label_create(back);
  lv_label_set_text(backLabel, LV_SYMBOL_LEFT " health");
  lv_obj_center(backLabel);
  lv_obj_add_event_cb(back, backFromDetail, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *home = lv_button_create(screen);
  lv_obj_set_size(home, 92, 30);
  lv_obj_align(home, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
  lv_obj_t *homeLabel = lv_label_create(home);
  lv_label_set_text(homeLabel, "home");
  lv_obj_center(homeLabel);
  lv_obj_add_event_cb(home, homeFromDetail, LV_EVENT_CLICKED, nullptr);

  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), back);
  lv_group_add_obj(lvglGroup(), home);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(screen);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);
}

void appHealthOpen() {
  stopHealthTimer();
  gScreen = lv_obj_create(nullptr);
  lv_obj_clear_flag(gScreen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(gScreen, lv_color_hex(0x101418), 0);

  gHeader = lv_label_create(gScreen);
  lv_obj_set_style_text_font(gHeader, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gHeader, 5, 5);
  lv_label_set_text(gHeader, "Health");

  gBack = lv_button_create(gScreen);
  lv_obj_set_size(gBack, 46, 23);
  lv_obj_set_pos(gBack, 270, 2);
  lv_obj_t *backLabel = lv_label_create(gBack);
  lv_label_set_text(backLabel, LV_SYMBOL_LEFT);
  lv_obj_center(backLabel);
  lv_obj_add_event_cb(gBack, backFromHealth, LV_EVENT_CLICKED, nullptr);

  gCaption = lv_label_create(gScreen);
  lv_obj_set_style_text_font(gCaption, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gCaption, 5, 199);
  lv_label_set_text(gCaption, "Loading registry...");

  lv_obj_t *legend = lv_label_create(gScreen);
  lv_obj_set_style_text_font(legend, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(legend, lv_color_hex(0xB8C0C8), 0);
  lv_obj_set_pos(legend, 5, 219);
  lv_label_set_text(legend, "G>3.20  Y>3.10  R<=3.10  gray=off");

  gUiTileCount = 0;
  memset(gTileObjects, 0, sizeof(gTileObjects));
  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), gBack);
  lvglSetNavHooks(&kHealthHooks);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(gScreen);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);

  refreshHealth(nullptr);
  gTimer = lv_timer_create(refreshHealth, 2000, nullptr);
}
