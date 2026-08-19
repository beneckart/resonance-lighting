#include "nvs_store.h"

#include <Arduino.h>
#include <Preferences.h>

#include "../core/radio_config.h"
#include "../core/solenoid_config.h"
#ifndef RES_PROFILE_DEFAULT
#define RES_PROFILE_DEFAULT PROFILE_DEV // M1 bringup posture; promote to field later
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

static void migrateChannelPolicy(Preferences &pf) {
  uint32_t version = pf.getUInt("channel_policy", 0);
  uint8_t prior = pf.getUChar("channel", 0);
  uint8_t resolved = resolveRadioChannel(prior, version);
  if (radioChannelNeedsPersist(prior, version)) {
    pf.putUChar("channel", resolved);
    pf.putUInt("channel_policy", RES_CHANNEL_POLICY_VERSION);
    Serial.printf("nvs: channel policy v%u %u -> %u\n",
                  (unsigned)RES_CHANNEL_POLICY_VERSION, prior, resolved);
  }
}

static uint8_t migrateSolenoidPolicy(Preferences &pf) {
  uint32_t version = pf.getUInt("sol_policy", 0);
  uint8_t prior = pf.getUChar("sol_en", 0);
  uint8_t resolved = resolveSolenoidEnabled(prior, version);
  if (solenoidPolicyNeedsPersist(prior, version)) {
    bool valueOk = pf.putUChar("sol_en", resolved) == sizeof(uint8_t);
    bool versionOk = valueOk &&
                     pf.putUInt("sol_policy", RES_SOLENOID_POLICY_VERSION) ==
                         sizeof(uint32_t);
    Serial.printf("nvs: solenoid policy v%u %u -> %u%s\n",
                  (unsigned)RES_SOLENOID_POLICY_VERSION, prior, resolved,
                  versionOk ? "" : " (persist FAILED; retry next boot)");
  }
  return resolved;
}

void nvsLoadConfig() {
  if (gLoaded) return;
  gLoaded = true;
  Preferences pf;
  uint8_t solenoidEnabled = RES_SOLENOID_DEFAULT_ENABLED;
  if (!pf.begin(kNs, false)) {
    Serial.println("nvs: OPEN FAILED -> compiled defaults");
  } else {
    migrateFromNetbench(pf);
    migrateChargePolicy(pf);
    migrateChannelPolicy(pf);
    solenoidEnabled = migrateSolenoidPolicy(pf);
  }
  gCfg.capMah = checkedU16(pf, "cap_mah", 6000, RES_CAPACITY_MIN_MAH, RES_CAPACITY_MAX_MAH);
  gCfg.chargeMa = checkedU16(pf, "chg_ma", RES_CHARGE_DEFAULT_MA,
                             RES_CHARGE_MIN_MA, RES_CHARGE_MAX_MA);
  gCfg.classOvr = pf.getUChar("class_ovr", FIXTURE_UNKNOWN);
  gCfg.classLast = pf.getUChar("class_last", FIXTURE_UNKNOWN);
  gCfg.profile = pf.getUChar("profile", (uint8_t)RES_PROFILE_DEFAULT);
  gCfg.battTier = pf.getUChar("batt_tier", 0);
#if defined(RES_SOLENOID_FORCE_ENABLED)
  // Targeted bring-up image only: ignore any stale/missing NVS arm bit. The
  // ordinary production build continues to honor the migrated runtime value.
  gCfg.solEn = 1;
#else
  gCfg.solEn = solenoidEnabled;
#endif
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
                gCfg.profile == PROFILE_DEV ? "commission" : "field", gCfg.solEn,
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
  Preferences pf;
  if (!pf.begin(kNs, false)) return false;
  en = en ? 1 : 0;
  bool ok = pf.putUChar("sol_en", en) == sizeof(uint8_t) &&
            pf.putUInt("sol_policy", RES_SOLENOID_POLICY_VERSION) ==
                sizeof(uint32_t);
  pf.end();
  if (!ok) return false;
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
  Preferences pf;
  if (!pf.begin(kNs, false)) return false;
  bool ok = pf.putUChar("channel", channel) == sizeof(uint8_t) &&
            pf.putUInt("channel_policy", RES_CHANNEL_POLICY_VERSION) ==
                sizeof(uint32_t);
  pf.end();
  if (!ok) return false;
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

bool nvsReadLoadArmed(bool &armed) {
  Preferences pf;
  if (!pf.begin(kNs, false)) return false;
  // Not-yet-written is a valid "never armed", distinct from "NVS unreadable".
  armed = pf.isKey("load_arm") ? pf.getUChar("load_arm", 0) != 0 : false;
  pf.end();
  return true;
}

bool nvsWriteLoadArmed(bool armed) { return putU8("load_arm", armed ? 1 : 0); }

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
