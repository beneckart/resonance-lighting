#include "nvs_store.h"

#include <Arduino.h>
#include <Preferences.h>

#ifndef RES_CHANNEL
#define RES_CHANNEL 6
#endif
#ifndef RES_PROFILE_DEFAULT
#define RES_PROFILE_DEFAULT PROFILE_DEV // M1 bringup posture; ship prod later
#endif
#ifndef RES_NIGHT_MAX_MIN_DEFAULT
#define RES_NIGHT_MAX_MIN_DEFAULT 630 // 10.5 h; BRC dusk-to-dawn is 9h53m-10h15m
#endif

FixtureConfig gCfg;

static const char *kNs = "resfx";
static bool gLoaded = false;

static uint16_t checkedU16(Preferences &pf, const char *key, uint16_t fallback,
                           uint16_t minVal, uint16_t maxVal) {
  uint32_t raw = pf.getUInt(key, fallback);
  if (raw < minVal || raw > maxVal) return fallback;
  return (uint16_t)raw;
}

static void migrateFromNetbench(Preferences &pf) {
  if (pf.getBool("migrated", false)) return;
  Preferences old;
  if (old.begin("netbench", true)) {
    uint32_t cap = old.getUInt("cap_mah", 0);
    uint32_t chg = old.getUInt("chg_ma", 0);
    uint8_t stage = old.getUChar("fc_led_stage", 0);
    old.end();
    bool any = false;
    if (cap >= RES_CAPACITY_MIN_MAH && cap <= RES_CAPACITY_MAX_MAH) {
      pf.putUInt("cap_mah", cap);
      any = true;
    }
    if (chg >= RES_CHARGE_MIN_MA && chg <= RES_CHARGE_MAX_MA) {
      pf.putUInt("chg_ma", chg);
      any = true;
    }
    // Only a parked stage carries over: production must not un-park a unit the
    // bench firmware latched into PROTECT (the old FIELD_SESSION_PROTECT is the
    // max legacy value, mapped onto our STAGE_PROTECT).
    if (stage >= 4 /* legacy FIELD_SESSION_PROTECT */) {
      pf.putUChar("fc_stage", STAGE_PROTECT);
      any = true;
    }
    if (any)
      Serial.printf("nvs: migrated netbench config (cap=%lu chg=%lu stage=%u)\n",
                    (unsigned long)cap, (unsigned long)chg, stage);
  }
  pf.putBool("migrated", true);
}

static void migrateChargePolicy(Preferences &pf) {
  uint32_t version = pf.getUInt("chg_policy", 0);
  if (version >= RES_CHARGE_POLICY_VERSION) return;

  // ADR 0033 raises known historical defaults to the PowerFeather V2 maximum.
  // Preserve any nonstandard value: it may be an intentional small-cell limit.
  uint32_t priorMa = pf.getUInt("chg_ma", 0);
  bool legacyDefault = priorMa == 0 || priorMa == 500 ||
                       priorMa == 1000 || priorMa == 1500;
  if (legacyDefault) pf.putUInt("chg_ma", RES_CHARGE_DEFAULT_MA);
  pf.putUInt("chg_policy", RES_CHARGE_POLICY_VERSION);
  Serial.printf("nvs: charge policy v%u %s %lu mA%s\n",
                (unsigned)RES_CHARGE_POLICY_VERSION,
                legacyDefault ? "migrated" : "preserved",
                (unsigned long)(legacyDefault ? RES_CHARGE_DEFAULT_MA : priorMa),
                legacyDefault ? "" : " (explicit override)");
}

void nvsLoadConfig() {
  if (gLoaded) return;
  gLoaded = true;
  Preferences pf;
  if (!pf.begin(kNs, false)) {
    Serial.println("nvs: OPEN FAILED -> compiled defaults");
  } else {
    migrateFromNetbench(pf);
    migrateChargePolicy(pf);
  }
  gCfg.capMah = checkedU16(pf, "cap_mah", 6000, RES_CAPACITY_MIN_MAH, RES_CAPACITY_MAX_MAH);
  gCfg.chargeMa = checkedU16(pf, "chg_ma", RES_CHARGE_DEFAULT_MA,
                             RES_CHARGE_MIN_MA, RES_CHARGE_MAX_MA);
  gCfg.classOvr = pf.getUChar("class_ovr", FIXTURE_UNKNOWN);
  gCfg.classLast = pf.getUChar("class_last", FIXTURE_UNKNOWN);
  gCfg.profile = pf.getUChar("profile", (uint8_t)RES_PROFILE_DEFAULT);
  gCfg.battTier = pf.getUChar("batt_tier", 0);
  gCfg.solEn = pf.getUChar("sol_en", 0);
  gCfg.maintV10 = pf.getUChar("maint_v10", 46);
  if (gCfg.maintV10 < RES_MAINTAIN_MIN_V10 || gCfg.maintV10 > RES_MAINTAIN_MAX_V10)
    gCfg.maintV10 = 46;
  gCfg.channel = pf.getUChar("channel", RES_CHANNEL);
  if (gCfg.channel < 1 || gCfg.channel > 13) gCfg.channel = RES_CHANNEL;
  gCfg.nightMaxMin = checkedU16(pf, "night_max", RES_NIGHT_MAX_MIN_DEFAULT, 60, 1440);
  gCfg.dimMv = checkedU16(pf, "dim_mv", 0, 0, 4000);
  gCfg.offMv = checkedU16(pf, "off_mv", 0, 0, 4000);
  gCfg.slpMv = checkedU16(pf, "slp_mv", 0, 0, 4000);
  pf.end();
  Serial.printf("  config: cap=%u mAh charge=%u mA class_ovr=%u profile=%s "
                "sol_en=%u maintain=%.1fV ch=%u night_max=%umin\n",
                gCfg.capMah, gCfg.chargeMa, gCfg.classOvr,
                gCfg.profile == PROFILE_DEV ? "dev" : "prod", gCfg.solEn,
                gCfg.maintV10 / 10.0f, gCfg.channel, gCfg.nightMaxMin);
}

static bool putU32(const char *key, uint32_t v) {
  Preferences pf;
  if (!pf.begin(kNs, false)) return false;
  bool ok = pf.putUInt(key, v) == sizeof(uint32_t);
  pf.end();
  return ok;
}
static bool putU8(const char *key, uint8_t v) {
  Preferences pf;
  if (!pf.begin(kNs, false)) return false;
  bool ok = pf.putUChar(key, v) == sizeof(uint8_t);
  pf.end();
  return ok;
}

bool nvsPersistCapacity(uint16_t mah) {
  if (!putU32("cap_mah", mah)) return false;
  gCfg.capMah = mah;
  return true;
}
bool nvsPersistChargeMa(uint16_t ma) {
  if (!putU32("chg_ma", ma)) return false;
  gCfg.chargeMa = ma;
  return true;
}
bool nvsPersistProfile(uint8_t profile) {
  if (!putU8("profile", profile)) return false;
  gCfg.profile = profile;
  return true;
}
bool nvsPersistClassOvr(uint8_t cls) {
  if (!putU8("class_ovr", cls)) return false;
  gCfg.classOvr = cls;
  return true;
}
bool nvsPersistClassLast(uint8_t cls) {
  if (!putU8("class_last", cls)) return false;
  gCfg.classLast = cls;
  return true;
}
bool nvsPersistSolEn(uint8_t en) {
  if (!putU8("sol_en", en)) return false;
  gCfg.solEn = en;
  return true;
}
bool nvsPersistMaintV10(uint8_t v10) {
  if (!putU8("maint_v10", v10)) return false;
  gCfg.maintV10 = v10;
  return true;
}
bool nvsPersistChannel(uint8_t channel) {
  if (channel < 1 || channel > 13) return false;
  if (!putU8("channel", channel)) return false;
  gCfg.channel = channel;
  return true;
}

bool nvsReadStage(uint8_t &stage) {
  Preferences pf;
  if (!pf.begin(kNs, false)) return false;
  // Not-yet-written is a valid IDLE, distinct from "NVS unreadable".
  stage = pf.isKey("fc_stage") ? pf.getUChar("fc_stage", STAGE_IDLE) : STAGE_IDLE;
  pf.end();
  if (stage > STAGE_PROTECT) stage = STAGE_PROTECT;
  return true;
}

bool nvsWriteStage(uint8_t stage) {
  if (stage > STAGE_PROTECT) stage = STAGE_PROTECT;
  return putU8("fc_stage", stage);
}

uint32_t nvsBumpBootCount() {
  Preferences pf;
  if (!pf.begin(kNs, false)) return 0;
  uint32_t boots = pf.getUInt("boots", 0) + 1;
  pf.putUInt("boots", boots);
  pf.end();
  return boots;
}

void nvsClearBootCount() {
  putU32("boots", 0);
}
