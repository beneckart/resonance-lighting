#include <lvgl.h>

#include <Arduino.h>
#include <string.h>

#include "fixture/src/core/packet.h"
#include "../core/fleet_registry_generated.h"
#include "../core/health_model.h"
#include "../net/census_svc.h"
#include "app_health.h"
#include "lvgl_glue.h"
#include "ui_theme.h"
#include "ui_task.h"

static lv_obj_t *gScreen = nullptr;
static lv_obj_t *gHeader = nullptr;
static lv_obj_t *gCaption = nullptr;
static lv_obj_t *gBack = nullptr;
static lv_obj_t *gModeButton = nullptr;
static lv_obj_t *gLegend = nullptr;
static lv_timer_t *gTimer = nullptr;

enum class HealthColorMode : uint8_t { VBAT = 0, CHARGE = 1 };
static HealthColorMode gColorMode = HealthColorMode::VBAT;

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

static lv_color_t chargeColor(ChargeStatus status) {
  switch (status) {
    case ChargeStatus::CHARGING_CC: return lv_color_hex(0x1B8A3A);
    case ChargeStatus::CHARGING_CV: return lv_color_hex(0x168AAD);
    case ChargeStatus::TOP_OFF: return lv_color_hex(0x805AD5);
    case ChargeStatus::NOT_CHARGING: return lv_color_hex(0xD98E00);
    case ChargeStatus::CHARGE_DISABLED: return lv_color_hex(0x8A651B);
    case ChargeStatus::FAULT: return lv_color_hex(0xD52B2B);
    case ChargeStatus::UNKNOWN: return lv_color_hex(0x2F80C9);
    case ChargeStatus::OFF_AIR: return lv_color_hex(0x343A40);
  }
  return lv_color_hex(0x343A40);
}

static const char *registryStatusName(const HealthRegistryEntry *entry) {
  if (!entry) return "unregistered";
  return entry->status == HealthRegistryStatus::COMMISSION_FAILED
             ? "commission failed"
             : "commissioned";
}

static const char *className(uint8_t cls) {
  switch (cls) {
    case 1: return "downlight";
    case 2: return "perimeter";
    case 3: return "uplight";
    case 4: return "chandelier";
    default: return "unknown";
  }
}

static const char *programName(uint8_t program) {
  switch (program) {
    case 0: return "idle";
    case 1: return "ca";
    case 2: return "bridge";
    case 3: return "direct";
    case 4: return "dark";
    case 5: return "contagion";
    default: return "?";
  }
}

static const char *lifeName(bool known, uint8_t life) {
  if (!known) return "UNKNOWN";
  switch (life) {
    case 0: return "BOOT";
    case 1: return "DAY_CHARGE";
    case 2: return "DAY_ACTIVE";
    case 3: return "NIGHT_SHOW";
    case 4: return "COMMISSION";
    default: return "UNKNOWN";
  }
}

static const char *tierName(bool known, uint8_t tier) {
  if (!known) return "UNKNOWN";
  switch (tier) {
    case 0: return "FULL";
    case 1: return "DIM";
    case 2: return "LEDS_OFF";
    case 3: return "PROTECT";
    default: return "UNKNOWN";
  }
}

static ChargeStatus peerChargeStatus(const PeerStat &peer, bool fresh) {
  return chargeStatus(fresh, peer.hasBq, peer.bqReg16, peer.bqStat1,
                      peer.bqFault0);
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
    if (gColorMode == HealthColorMode::VBAT)
      lv_label_set_text_fmt(gCaption, "%s  live  no valid VBAT", identity);
    else
      lv_label_set_text_fmt(gCaption, "%s  %s  IBAT %s", identity,
                            chargeStatusName(tile.chargeStatus),
                            tile.battMaValid ? "valid" : "--");
  } else if (gColorMode == HealthColorMode::CHARGE) {
    if (tile.battMaValid)
      lv_label_set_text_fmt(gCaption, "%s  %s  %+dmA  in %d.%02dV", identity,
                            chargeStatusName(tile.chargeStatus), tile.battMa,
                            tile.supplyMv / 1000,
                            (tile.supplyMv % 1000) / 10);
    else
      lv_label_set_text_fmt(gCaption, "%s  %s  IBAT --  in %d.%02dV", identity,
                            chargeStatusName(tile.chargeStatus),
                            tile.supplyMv / 1000,
                            (tile.supplyMv % 1000) / 10);
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
  gModeButton = nullptr;
  gLegend = nullptr;
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
  if (gColorMode == HealthColorMode::CHARGE) {
    const ChargeStatus priority[] = {
        ChargeStatus::FAULT, ChargeStatus::UNKNOWN,
        ChargeStatus::CHARGE_DISABLED,
        ChargeStatus::NOT_CHARGING, ChargeStatus::OFF_AIR,
        ChargeStatus::CHARGING_CC, ChargeStatus::CHARGING_CV,
        ChargeStatus::TOP_OFF};
    for (ChargeStatus status : priority) {
      for (size_t i = 0; i < gTileCount; ++i) {
        if (gTiles[i].chargeStatus == status) return i;
      }
    }
    return 0;
  }
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
    lv_obj_set_style_border_color(
        tile, uiDayMode() ? lv_color_black() : lv_color_white(),
        LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(tile, 2, LV_STATE_FOCUSED);
    lv_obj_set_user_data(tile, gTiles[i].id);
    lv_obj_add_event_cb(tile, tileFocused, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(tile, tileClicked, LV_EVENT_CLICKED, nullptr);
    lv_group_add_obj(lvglGroup(), tile);
  }
  if (gModeButton) lv_group_add_obj(lvglGroup(), gModeButton);
  lv_group_add_obj(lvglGroup(), gBack);
  gUiTileCount = gTileCount;
  if (gTileCount) lv_group_focus_obj(gTileObjects[firstAttentionTile()]);
}

static void styleTiles() {
  for (size_t i = 0; i < gUiTileCount; ++i) {
    lv_obj_t *object = gTileObjects[i];
    lv_color_t color = gColorMode == HealthColorMode::VBAT
                           ? bandColor(gTiles[i].band)
                           : chargeColor(gTiles[i].chargeStatus);
    lv_obj_set_style_bg_color(object, color, 0);
    lv_obj_set_style_bg_color(object, color, LV_STATE_FOCUSED);
    if ((gColorMode == HealthColorMode::VBAT &&
         gTiles[i].band == BatteryHealthBand::LOW_BATTERY) ||
        (gColorMode == HealthColorMode::CHARGE &&
         gTiles[i].chargeStatus == ChargeStatus::FAULT)) {
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

static void updateModeChrome() {
  if (gModeButton) {
    lv_obj_t *label = lv_obj_get_child(gModeButton, 0);
    if (label)
      lv_label_set_text(label, gColorMode == HealthColorMode::VBAT ? "VBAT"
                                                                   : "CHG");
  }
  if (gLegend) {
    lv_label_set_text(
        gLegend,
        gColorMode == HealthColorMode::VBAT
            ? "G>3.20  Y>3.10  R<=3.10  gray=off"
            : "CC=green CV=cyan TOP=purple N=amber !=fault");
  }
}

static void modeCb(lv_event_t *) {
  gColorMode = gColorMode == HealthColorMode::VBAT
                   ? HealthColorMode::CHARGE
                   : HealthColorMode::VBAT;
  updateModeChrome();
  styleTiles();
  updateCaption(focusedTileIndex());
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
    gObservations[i].battMa = gCensusRows[i].battMa;
    gObservations[i].battMaValid = gCensusRows[i].battMaValid;
    gObservations[i].supplyMv = gCensusRows[i].supplyMv;
    gObservations[i].supplyMa = gCensusRows[i].supplyMa;
    gObservations[i].supplyGood = gCensusRows[i].supplyGood;
    gObservations[i].hasBq = gCensusRows[i].hasBq;
    gObservations[i].bqReg16 = gCensusRows[i].bqReg16;
    gObservations[i].bqStat1 = gCensusRows[i].bqStat1;
    gObservations[i].bqFault0 = gCensusRows[i].bqFault0;
  }
  gTileCount = healthBuildTiles(
      kHealthRegistry, kHealthRegistryCount, gObservations, censusCount,
      censusFreshMsSafe(), gTiles, CENSUS_MAX_TRACKED);

  if (tileIdentityChanged(gTileCount)) rebuildTileObjects();
  styleTiles();

  if (gColorMode == HealthColorMode::VBAT) {
    HealthSummary summary = healthSummarize(gTiles, gTileCount);
    lv_label_set_text_fmt(gHeader, "G%u Y%u R%u ?%u -%u",
                          summary.good, summary.nearLow, summary.low,
                          summary.unknown, summary.offAir);
  } else {
    uint16_t cc = 0, cv = 0, top = 0, notCharging = 0;
    uint16_t fault = 0, unknown = 0, offAir = 0;
    for (size_t i = 0; i < gTileCount; ++i) {
      switch (gTiles[i].chargeStatus) {
        case ChargeStatus::CHARGING_CC: ++cc; break;
        case ChargeStatus::CHARGING_CV: ++cv; break;
        case ChargeStatus::TOP_OFF: ++top; break;
        case ChargeStatus::NOT_CHARGING:
        case ChargeStatus::CHARGE_DISABLED: ++notCharging; break;
        case ChargeStatus::FAULT: ++fault; break;
        case ChargeStatus::UNKNOWN: ++unknown; break;
        case ChargeStatus::OFF_AIR: ++offAir; break;
      }
    }
    lv_label_set_text_fmt(gHeader, "C%u V%u T%u N%u !%u ?%u -%u", cc,
                          cv, top, notCharging, fault, unknown, offAir);
  }
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
    bool fresh = ageMs < censusFreshMsSafe();
    BatteryHealthBand band =
        batteryHealthBand(fresh, peer.battMv);
    uint32_t total = peer.recv + peer.gaps;
    uint16_t pdr = total ? (uint16_t)((uint64_t)peer.recv * 1000 / total) : 0;
    char ibat[20];
    char soc[16];
    if (peer.hasPowerSampleFlags &&
        (peer.powerSampleFlags & NB_POWER_SAMPLE_IBAT_VALID))
      snprintf(ibat, sizeof(ibat), "%+d mA", peer.battMa);
    else
      snprintf(ibat, sizeof(ibat), "-- (unverified)");
    if (peer.soc == 255)
      snprintf(soc, sizeof(soc), "unknown");
    else
      snprintf(soc, sizeof(soc), "%u%%", peer.soc);
    ChargeStatus charge = peerChargeStatus(peer, fresh);
    lv_label_set_text_fmt(
        body,
        "%s  VBAT %d.%03dV  IBAT %s\n"
        "charge %s\n"
        "input %d.%03dV  %dmA  %s\n"
        "age %lus  signal %d/%d  PDR %u.%u%%\n"
        "SOC %s  class %s  tier %s\n"
        "program %s  life %s  sensors 0x%02X%s\n"
        "registry %s  %u mAh  %s\n"
        "fw %s",
        bandName(band), peer.battMv / 1000, peer.battMv % 1000, ibat,
        chargeStatusName(charge), peer.supplyMv / 1000,
        peer.supplyMv % 1000, peer.supplyMa,
        peer.supplyGood ? "GOOD" : "NOT GOOD",
        (unsigned long)(ageMs / 1000), peer.rssi, peer.rssiEwma, pdr / 10,
        pdr % 10, soc, className(peer.classLatched),
        tierName(peer.hasFixtureState, peer.powerTier),
        peer.hasFixtureState ? programName(peer.activeProgram) : "unknown",
        lifeName(peer.hasFixtureState, peer.lifeState),
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
  lv_obj_set_style_bg_color(gScreen, uiScreenColor(), 0);

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

  gModeButton = lv_button_create(gScreen);
  lv_obj_set_size(gModeButton, 64, 23);
  lv_obj_set_pos(gModeButton, 201, 2);
  lv_obj_t *modeLabel = lv_label_create(gModeButton);
  lv_obj_center(modeLabel);
  lv_obj_add_event_cb(gModeButton, modeCb, LV_EVENT_CLICKED, nullptr);

  gCaption = lv_label_create(gScreen);
  lv_obj_set_style_text_font(gCaption, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gCaption, 5, 199);
  lv_label_set_text(gCaption, "Loading registry...");

  gLegend = lv_label_create(gScreen);
  lv_obj_set_style_text_font(gLegend, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(gLegend, uiMutedTextColor(), 0);
  lv_obj_set_pos(gLegend, 5, 219);
  updateModeChrome();

  gUiTileCount = 0;
  memset(gTileObjects, 0, sizeof(gTileObjects));
  lv_group_remove_all_objs(lvglGroup());
  lv_group_add_obj(lvglGroup(), gModeButton);
  lv_group_add_obj(lvglGroup(), gBack);
  lvglSetNavHooks(&kHealthHooks);

  lv_obj_t *old = lv_screen_active();
  lv_screen_load(gScreen);
  if (old && old != uiLauncherScreen()) lv_obj_delete(old);

  refreshHealth(nullptr);
  gTimer = lv_timer_create(refreshHealth, 2000, nullptr);
}
