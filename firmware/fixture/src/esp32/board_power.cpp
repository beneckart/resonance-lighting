#include "board_power.h"

#include <Arduino.h>
#include <Wire.h>
#include "driver/rtc_io.h"
#include "esp_sleep.h"

#include <PowerFeather.h>
// Resolved via build.sh's -I<repo>/firmware (the sketch tree is copied into
// the build dir, so a relative ../../../ escape cannot reach the shared header).
#include "powerfeather_solar_guard.h"

#include "../core/bq25628e_precharge.h"
#include "../core/deep_recovery.h"
#include "boot_park.h"
#include "identity.h"
#include "loads.h"
#include "nvs_store.h"
#include "solenoid.h"

using namespace PowerFeather;

#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 so the SDK uses the V2 MAX17260 fuel gauge (see build.sh)."
#endif

// Chemistry stays build-time (a wrong runtime chemistry against a connected
// cell is a charger-termination hazard); production default is LFP.
#ifndef RES_PF_BATTERY_TYPE
#define RES_PF_BATTERY_TYPE Mainboard::BatteryType::Generic_LFP
#endif

static const char *kTag = "fixture";
static const uint32_t kTransportWakeMagic = 0x54525054UL; // "TRPT"
RTC_DATA_ATTR static uint32_t gTransportWakeMagic = 0;

static bool gPfReady = false;
static bool gTransportWakeDark = false;
static bool gChargingEnabled = false;
static float gMaintainV = 4.6f;
static float gCbV = 0.0f, gCbMa = 0.0f, gCbMaRaw = 0.0f;
static int gCbSoc = -1;
static float gCsV = 0.0f, gCsMa = 0.0f;
static bool gCsGood = false;
static bool gPrechargeWriteOk = false;
static bool gDeepRecoveryChargeActive = false;
static bool gLowVbatRecoveryChargeActive = false;
static uint8_t gLowVbatRecoveryState = LOW_VBAT_RECOVERY_NONE;
static uint16_t gLowVbatRecoveryDetectMv = 0xFFFF;
static BqSnapshot gBq;

// ADR 0047 battery-corroboration state: a plausible voltage alone is not
// proof of a cell (a floating BAT node held up by the powered charger can
// read 2.5-3.05 V). Evidence sources, any one sufficient: recent >=30 mA
// charge/discharge current, a passed SLUAB31A presence test, a recovery-lane
// BQ detection, or battery-only operation.
static uint32_t gCurrentEvidenceMs = 0;    // 0 = never observed
static uint32_t gPresenceRealUntilMs = 0;  // BQ test said REAL, fresh window
static uint32_t gPresenceEmptyUntilMs = 0; // BQ test said EMPTY, veto window
static bool gRecoveryEverDetected = false;
static bool gPresenceCheckWanted = false;
static uint32_t gLastPresenceCheckMs = 0;
#define RES_BATT_CURRENT_EVIDENCE_MA 30.0f
#define RES_BATT_EVIDENCE_FRESH_MS 60000UL
#define RES_BATT_PRESENCE_REAL_MS 600000UL
#define RES_BATT_PRESENCE_EMPTY_MS 60000UL

bool pfIsReady() { return gPfReady; }
bool chargingEnabled() { return gChargingEnabled; }
static bool presenceEmptyFresh() {
  if (!gPresenceEmptyUntilMs) return false;
  if ((int32_t)(gPresenceEmptyUntilMs - millis()) > 0) return true;
  gPresenceEmptyUntilMs = 0; // expired: zero it so it can never go stale-fresh
  return false;
}

bool batteryPresent() {
  // A fresh EMPTY verdict vetoes the plausible-voltage window; real current
  // (a cell installed mid-session) clears it early and it expires on its own,
  // so an installed cell is never locked out.
  if (presenceEmptyFresh()) return false;
  return (gCbV > 2.5f && gCbV < 4.4f) || gLowVbatRecoveryChargeActive;
}
bool lowVbatRecoveryActive() { return gLowVbatRecoveryChargeActive; }
void batteryRequestPresenceCheck() { gPresenceCheckWanted = true; }
bool batteryCorroborated() {
  uint32_t now = millis();
  if (gCurrentEvidenceMs && now - gCurrentEvidenceMs < RES_BATT_EVIDENCE_FRESH_MS)
    return true;
  if (gPresenceRealUntilMs) {
    if ((int32_t)(gPresenceRealUntilMs - now) > 0) return true;
    gPresenceRealUntilMs = 0; // expired: zero it (stale-fresh after ~25 d wrap)
  }
  if (gRecoveryEverDetected) return true;
  // Audit fix: there is deliberately NO battery-only voltage clause. True
  // battery-only operation always shows >=80 mA discharge and corroborates
  // via current evidence within a second; a voltage-only clause certified a
  // floating node whenever the supply-good flag flickered at dusk/dawn.
  return false;
}
float maintainVolts() { return gMaintainV; }
float batteryVolts() { return gCbV; }
float batteryMa() { return gCbMa; }
float batteryMaRaw() { return gCbMaRaw; }
int batterySocPct() { return gCbSoc; }
float supplyVolts() { return gCsV; }
float supplyMa() { return gCsMa; }
bool supplyGood() { return gCsGood; }
bool prechargeConfigured() {
  return gPrechargeWriteOk && gBq.reg10 != 0xFFFF &&
         gBq.precharge_ma == RES_PF_PRECHARGE_MA;
}
uint16_t prechargeTargetMa() { return RES_PF_PRECHARGE_MA; }
bool deepRecoveryBuild() { return RES_DEEP_RECOVERY_TARGET != 0UL; }
bool deepRecoveryTargetMatches() {
  uint32_t id = ((uint32_t)gMyId[0] << 16) |
                ((uint32_t)gMyId[1] << 8) | gMyId[2];
  return deepRecoveryBuild() && id == (uint32_t)RES_DEEP_RECOVERY_TARGET;
}
bool deepRecoveryChargeActive() { return gDeepRecoveryChargeActive; }
uint8_t lowVbatRecoveryState() { return gLowVbatRecoveryState; }
uint16_t lowVbatRecoveryDetectMv() { return gLowVbatRecoveryDetectMv; }
const BqSnapshot &bqSnapshot() { return gBq; }

const char *batteryTypeName() {
  switch (RES_PF_BATTERY_TYPE) {
  case Mainboard::BatteryType::Generic_3V7: return "Generic_3V7";
  case Mainboard::BatteryType::Generic_LFP: return "Generic_LFP";
  default: return "other";
  }
}

static uint16_t bqMvOrUnknown(bool ok, float volts) {
  if (!ok || volts < 0.0f) return 0xFFFF;
  return (uint16_t)min(65535UL, (unsigned long)(volts * 1000.0f + 0.5f));
}
static uint16_t bqMaOrUnknown(bool ok, float ma) {
  if (!ok || ma < 0.0f) return 0xFFFF;
  return (uint16_t)min(65535UL, (unsigned long)(ma + 0.5f));
}

static bool configurePrecharge() {
  const uint16_t targetMa = RES_PF_PRECHARGE_MA;
  uint16_t before = 0;
  if (!pfSolarGuardRead16(0x10, before)) {
    Serial.println("fixture precharge: read REG0x10 failed");
    gPrechargeWriteOk = false;
    return false;
  }
  uint16_t wanted = bq25628ePrechargeReg10(before, targetMa);
  bool wrote = (wanted == before) || pfSolarGuardWrite16(0x10, wanted);
  uint16_t after = 0;
  bool readback = pfSolarGuardRead16(0x10, after);
  uint16_t actualMa = readback ? bq25628ePrechargeMa(after) : 0;
  gPrechargeWriteOk = wrote && readback && actualMa == targetMa;
  Serial.printf("fixture precharge: REG0x10 0x%04X -> 0x%04X target=%umA readback=%umA %s\n",
                before, readback ? after : 0xFFFF, (unsigned)targetMa,
                (unsigned)actualMa, gPrechargeWriteOk ? "OK" : "ERR");
  return gPrechargeWriteOk;
}

static void readChargerStatus() {
  gBq.vindpm_mv = gBq.ichg_ma = gBq.vreg_mv = gBq.precharge_ma = 0xFFFF;
  gBq.reg10 = 0xFFFF;
  gBq.reg16 = gBq.reg18 = gBq.stat0 = gBq.stat1 = 0xFF;
  gBq.fault0 = gBq.flag0 = gBq.flag1 = gBq.fault_flag0 = gBq.part = 0xFF;
  if (!gPfReady) return;

  float v = 0.0f;
  gBq.vindpm_mv = bqMvOrUnknown(Board.getCharger().getVINDPM(v), v);
  gBq.ichg_ma = bqMaOrUnknown(Board.getCharger().getChargeCurrentLimit(v), v);
  gBq.vreg_mv = bqMvOrUnknown(Board.getCharger().getChargeVoltageLimit(v), v);

  uint16_t w = 0;
  if (pfSolarGuardRead16(0x10, w)) {
    gBq.reg10 = w;
    gBq.precharge_ma = bq25628ePrechargeMa(w);
  }
  uint8_t b = 0;
  if (pfSolarGuardRead8(PF_SOLAR_GUARD_REG_CHG_CTRL0, b)) gBq.reg16 = b;
  if (pfSolarGuardRead8(0x18, b)) gBq.reg18 = b;
  if (pfSolarGuardRead8(0x1D, b)) gBq.stat0 = b;
  if (pfSolarGuardRead8(0x1E, b)) gBq.stat1 = b;
  if (pfSolarGuardRead8(0x1F, b)) gBq.fault0 = b;
  if (pfSolarGuardRead8(0x20, b)) gBq.flag0 = b;
  if (pfSolarGuardRead8(0x21, b)) gBq.flag1 = b;
  if (pfSolarGuardRead8(0x22, b)) gBq.fault_flag0 = b;
  if (pfSolarGuardRead8(0x38, b)) gBq.part = b;
}

// TI SLUAB31A battery-removal sequence for BQ2562x: charging off, apply the
// charger's ~30 mA BAT discharge for 5 ms, then one-shot its VBAT ADC. A real
// attached cell remains above VBAT_UVLO while an empty BAT node collapses.
// All touched ADC/discharge state is restored before returning.
static bool bqBatteryPresenceTest(uint16_t &batteryMv) {
  batteryMv = 0xFFFF;
  uint8_t adcBefore = 0;
  if (!pfSolarGuardRead8(0x26, adcBefore)) return false;

  Board.enableBatteryCharging(false);
  gChargingEnabled = false;
  bool ok = pfSolarGuardUpdate8(PF_SOLAR_GUARD_REG_CHG_CTRL0, 1u << 5, false);
  ok = pfSolarGuardUpdate8(PF_SOLAR_GUARD_REG_CHG_CTRL0, 1u << 6, true) && ok;
  if (ok) delay(5); // node bleeds / cell settles under the 30 mA sink
  // Audit fix (SLUAB31A): measure WHILE the discharge is asserted -- a
  // floating node refloats within milliseconds of release, and the old
  // sequence (release, then fixed 50 ms wait) could also return the STALE
  // pre-test conversion: the one-shot 10-bit full sequence needs ~80 ms
  // (SDK waits 100 ms and polls done). Trigger the one-shot, poll ADC_EN
  // self-clear as an early exit, and in the worst case wait the full 150 ms
  // before reading -- past every documented conversion time either way.
  uint8_t oneShot = (uint8_t)(adcBefore | (1u << 7) | (1u << 6));
  ok = ok && pfSolarGuardWrite8(0x26, oneShot);
  if (ok) {
    for (int i = 0; i < 30; i++) { // <= 150 ms
      delay(5);
      uint8_t ctl = 0;
      if (!pfSolarGuardRead8(0x26, ctl)) break;
      if (!(ctl & (1u << 7))) break; // one-shot completed
    }
  }
  uint16_t raw = 0;
  bool readOk = ok && pfSolarGuardRead16(0x30, raw);
  // Release the discharge and restore ADC config regardless of outcome.
  bool releaseOk = pfSolarGuardUpdate8(PF_SOLAR_GUARD_REG_CHG_CTRL0, 1u << 6, false);
  bool restoreOk = pfSolarGuardWrite8(0x26, adcBefore);
  if (!ok || !readOk || !releaseOk || !restoreOk) return false;
  batteryMv = (uint16_t)(((uint32_t)(raw >> 1) * 199u + 50u) / 100u);
  return true;
}

void boardPowerInit() {
  // RTC slow memory survives deep sleep (and software/watchdog resets) but is
  // initialized on a true power cycle. Once armed, stay dark across any
  // incidental restart until an explicit bridge program command releases it.
  gTransportWakeDark = gTransportWakeMagic == kTransportWakeMagic;
  if (gTransportWakeDark)
    Serial.println("transport wake: radio live, LED output latched dark");
  Serial.println("PowerFeather SDK init:");
  nvsLoadConfig();
  Result r = Result::Failure;
  for (int a = 1; a <= 4; a++) {
    r = Board.init(gCfg.capMah, RES_PF_BATTERY_TYPE);
    // The SDK intentionally enables the header rail on cold init; production
    // owns the LED rail, so park it after EVERY attempt before anything else
    // can overlap an old/unknown pixel frame.
    bootParkRailLow();
    if (r == Result::Ok) break;
    Serial.printf("  Board.init attempt %d -> %d, retrying\n", a, (int)r);
    delay(250);
  }
  gPfReady = (r == Result::Ok);
  Serial.printf("  Board.init(cap=%u, %s) -> %s\n", (unsigned)gCfg.capMah,
                batteryTypeName(), gPfReady ? "Ok" : "ERR");
  if (!gPfReady) return;
  gMaintainV = gCfg.maintV10 / 10.0f;
  Board.setSupplyMaintainVoltage(gMaintainV);
  Board.setBatteryChargingMaxCurrent((float)gCfg.chargeMa);
  // Charging a missing battery can brownout-loop the board on USB. Keep
  // charging off until the warmed gauge reports a plausible cell (the deferred
  // guard below); one image stays safe for bare-board commissioning AND the
  // same board with a production cell installed.
  Board.enableBatteryCharging(false);
  gChargingEnabled = false;
  pfSolarGuardInit(kTag, gMaintainV, false);
  configurePrecharge();
}

static void readBatteryCell() {
  float v;
  if (Board.getBatteryVoltage(v) == Result::Ok) gCbV = v;
  if (Board.getBatteryCurrent(v) == Result::Ok) {
    gCbMaRaw = v;
    gCbMa = v / RES_GAUGE_CURRENT_DIVISOR;
    // Real charge/discharge current is battery corroboration (ADR 0047): a
    // floating BAT node cannot source or sink tens of milliamps.
    if (fabsf(gCbMa) >= RES_BATT_CURRENT_EVIDENCE_MA) {
      uint32_t now = millis();
      gCurrentEvidenceMs = now ? now : 1;
      gPresenceEmptyUntilMs = 0; // a cell was installed mid-session
    }
  }
  uint8_t s;
  if (Board.getBatteryCharge(s) == Result::Ok) gCbSoc = s;
}

static void chargingGuardTick() {
  static bool done = false;
  static uint32_t lastRecoveryAttemptMs = 0;
  static uint32_t recoveryAboveSinceMs = 0;

  if (gLowVbatRecoveryChargeActive) {
    if (gBq.fault0 != 0x00 && gBq.fault0 != 0xFF) {
      Board.enableBatteryCharging(false);
      gChargingEnabled = false;
      gLowVbatRecoveryChargeActive = false;
      gLowVbatRecoveryState = LOW_VBAT_RECOVERY_REFUSED;
      Serial.printf("LOW-VBAT RECOVERY STOPPED fault=0x%02X\n", gBq.fault0);
      return;
    }
    // Exact-target test images deliberately stay at their immutable 100 mA
    // ceiling. Production images graduate after one stable minute above the
    // old presence boundary with 50 mV hysteresis.
    if (deepRecoveryBuild()) return;
    if (gCbV >= 2.55f) {
      if (!recoveryAboveSinceMs) recoveryAboveSinceMs = millis();
      if (millis() - recoveryAboveSinceMs >= 60000UL) {
        Board.setBatteryChargingMaxCurrent((float)gCfg.chargeMa);
        gLowVbatRecoveryChargeActive = false;
        gLowVbatRecoveryState = LOW_VBAT_RECOVERY_GRADUATED;
        Serial.printf("LOW-VBAT RECOVERY GRADUATED bv=%.3fV -> normal cap %umA\n",
                      gCbV, (unsigned)gCfg.chargeMa);
      }
    } else {
      recoveryAboveSinceMs = 0;
    }
    return;
  }

  if (done || !gPfReady || millis() < 6000) return;
  if (gCbV < 0.1f) {
    if (millis() > 60000) {
      done = true;
      Serial.println("no battery reading after 60 s -> charging stays OFF until reboot");
    }
    return;
  }
  if (deepRecoveryBuild()) {
    done = true;
    // This is deliberately narrower than batteryPresent(): one immutable test
    // image, one MAC, external power proven, a real low LFP voltage, clean BQ,
    // and the requested precharge register verified. A non-target that somehow
    // receives the image remains dark with charging disabled.
    DeepRecoverySample sample = {
        .build_enabled = deepRecoveryBuild(),
        .target_matches = deepRecoveryTargetMatches(),
        .supply_good = gCsGood,
        .precharge_configured = prechargeConfigured(),
        .battery_v = gCbV,
        .battery_ma = gCbMa,
        .supply_v = gCsV,
        .supply_ma = gCsMa,
        .charger_fault = gBq.fault0,
    };
    if (!deepRecoveryMayEnable(sample)) {
      gLowVbatRecoveryState = LOW_VBAT_RECOVERY_REFUSED;
      Serial.printf("DEEP RECOVERY REFUSED target=%d bv=%.3f input=%.3f/%.0f/%d "
                    "fault=0x%02X batt_ma=%.0f precharge=%d\n",
                    sample.target_matches ? 1 : 0, gCbV, gCsV, gCsMa, gCsGood ? 1 : 0,
                    gBq.fault0, gCbMa, prechargeConfigured() ? 1 : 0);
      return;
    }
    allLoadsOff("deep recovery");
    railEnableVSQT(false);
    Board.setBatteryChargingMaxCurrent((float)RES_DEEP_RECOVERY_MAX_CHARGE_MA);
    configurePrecharge();
    Board.enableBatteryCharging(true);
    gChargingEnabled = true;
    gDeepRecoveryChargeActive = true;
    gLowVbatRecoveryChargeActive = true;
    gRecoveryEverDetected = true; // ADR 0047: BQ-qualified = corroborated
    gLowVbatRecoveryState = LOW_VBAT_RECOVERY_ACTIVE;
    gLowVbatRecoveryDetectMv = (uint16_t)(gCbV * 1000.0f);
    pfSolarGuardInit(kTag, gMaintainV, true);
    Serial.printf("DEEP RECOVERY ACTIVE target=%06lX bv=%.3fV precharge=%umA "
                  "charge_ceiling=%umA\n",
                  (unsigned long)RES_DEEP_RECOVERY_TARGET, gCbV,
                  (unsigned)RES_PF_PRECHARGE_MA,
                  (unsigned)RES_DEEP_RECOVERY_MAX_CHARGE_MA);
  } else if (presenceEmptyFresh()) {
    // Audit fix: the SLUAB31A test just proved the BAT node is empty; the
    // plausible floating voltage must not enable charging (brownout-loop
    // hazard). One-shot consumed with charging off; a cell installed later
    // is picked up by the on-demand presence check, whose REAL verdict
    // enables charging itself.
    done = true;
    Serial.println("BAT node proven empty -> charging stays OFF");
  } else if (gCbV > 2.5f && gCbV < 4.4f) {
    done = true;
    Board.setBatteryChargingMaxCurrent((float)gCfg.chargeMa);
    configurePrecharge();
    Board.enableBatteryCharging(true);
    gChargingEnabled = true;
    pfSolarGuardInit(kTag, gMaintainV, true);
    Serial.printf("battery %.2fV present -> charging ON (%u mA, %s profile)\n",
                  gCbV, (unsigned)gCfg.chargeMa, batteryTypeName());
  } else if (RES_LOW_VBAT_RECOVERY && gCbV >= 2.20f && gCbV <= 2.50f) {
    if (lastRecoveryAttemptMs && millis() - lastRecoveryAttemptMs < 5000UL) return;
    lastRecoveryAttemptMs = millis();
    FleetRecoverySample sample = {
        .supply_good = gCsGood,
        .precharge_configured = prechargeConfigured(),
        .battery_v = gCbV,
        .battery_ma = gCbMa,
        .supply_v = gCsV,
        .supply_ma = gCsMa,
        .charger_fault = gBq.fault0,
    };
    if (!fleetRecoveryMayTest(sample)) {
      gLowVbatRecoveryState = LOW_VBAT_RECOVERY_WAITING;
      Serial.printf("low-VBAT recovery waiting bv=%.3f input=%.3f/%.0f/%d "
                    "fault=0x%02X batt_ma=%.0f precharge=%d\n",
                    gCbV, gCsV, gCsMa, gCsGood ? 1 : 0, gBq.fault0, gCbMa,
                    prechargeConfigured() ? 1 : 0);
      return;
    }
    uint16_t detectedMv = 0xFFFF;
    if (!bqBatteryPresenceTest(detectedMv)) {
      gLowVbatRecoveryState = LOW_VBAT_RECOVERY_IO_ERROR;
      Serial.println("low-VBAT recovery: BQ presence test I2C error; retrying");
      return;
    }
    gLowVbatRecoveryDetectMv = detectedMv;
    if (!fleetRecoveryBatteryDetected(detectedMv)) {
      done = true;
      gLowVbatRecoveryState = LOW_VBAT_RECOVERY_REFUSED;
      Serial.printf("LOW-VBAT RECOVERY REFUSED: BQ presence ADC=%umV\n",
                    (unsigned)detectedMv);
      return;
    }
    allLoadsOff("low-VBAT recovery");
    railEnableVSQT(false);
    Board.setBatteryChargingMaxCurrent((float)RES_LOW_VBAT_RECOVERY_MAX_CHARGE_MA);
    configurePrecharge();
    Board.enableBatteryCharging(true);
    gChargingEnabled = true;
    gLowVbatRecoveryChargeActive = true;
    gRecoveryEverDetected = true; // ADR 0047: presence test passed above
    gLowVbatRecoveryState = LOW_VBAT_RECOVERY_ACTIVE;
    pfSolarGuardInit(kTag, gMaintainV, true);
    done = true;
    Serial.printf("LOW-VBAT RECOVERY ACTIVE gauge=%.3fV BQ=%umV "
                  "charge_ceiling=%umA\n",
                  gCbV, (unsigned)detectedMv,
                  (unsigned)RES_LOW_VBAT_RECOVERY_MAX_CHARGE_MA);
  } else {
    done = true;
    gLowVbatRecoveryState = LOW_VBAT_RECOVERY_REFUSED;
    Serial.printf("battery %.2fV implausible -> charging stays OFF\n", gCbV);
  }
}

void readBatteryNow() {
  if (!gPfReady) return;
  readBatteryCell();
  float v;
  if (Board.getSupplyVoltage(v) == Result::Ok) gCsV = v;
  if (Board.getSupplyCurrent(v) == Result::Ok) gCsMa = v;
  bool g;
  if (Board.checkSupplyGood(g) == Result::Ok) gCsGood = g;
  readChargerStatus();
  chargingGuardTick();
  // Refresh the enable bit/current/fault snapshot after the one-shot guard.
  if (gChargingEnabled) readChargerStatus();
  pfSolarGuardTick(kTag, gCsV, gCsMa, gCsGood, gMaintainV, gChargingEnabled);

  // ADR 0047 on-demand presence check: requested by the power policy when it
  // wants to persist PROTECT without corroboration. Only with proven external
  // power (the floating scenario requires it, and the ~55 ms test briefly
  // gates a 30 mA BAT discharge), never during a recovery lane, and at most
  // once per minute. The test clears the charging enable; restore it for a
  // REAL verdict -- never for EMPTY (charging a phantom node is the
  // brownout-loop hazard the deferred guard exists to prevent).
  if (gPresenceCheckWanted && gCsGood && !gLowVbatRecoveryChargeActive &&
      (!gLastPresenceCheckMs ||
       millis() - gLastPresenceCheckMs >= 60000UL)) {
    gPresenceCheckWanted = false;
    uint32_t now = millis();
    gLastPresenceCheckMs = now ? now : 1;
    bool wasCharging = gChargingEnabled;
    uint16_t mv = 0xFFFF;
    if (!bqBatteryPresenceTest(mv)) {
      gPresenceCheckWanted = true; // I2C hiccup: retry after the rate limit
      if (wasCharging) {
        Board.enableBatteryCharging(true);
        gChargingEnabled = true;
      }
      Serial.println("battery presence test I2C error; will retry");
    } else if (mv >= 2000 && mv < 4400) {
      // 2000, not 2200 (audit fix): the measurement now runs UNDER the 30 mA
      // discharge, and a real 2.2 V high-IR cell sags tens of mV while a
      // floating node collapses toward zero -- keep discrimination with
      // margin on the real-cell side.
      gPresenceRealUntilMs = millis() + RES_BATT_PRESENCE_REAL_MS;
      // REAL = a physical cell in plausible range: (re)enable charging
      // outright, not just when it was on before -- an earlier EMPTY-era
      // decision may have left it off for a cell installed mid-session.
      Board.setBatteryChargingMaxCurrent((float)gCfg.chargeMa);
      configurePrecharge();
      Board.enableBatteryCharging(true);
      gChargingEnabled = true;
      Serial.printf("battery presence VERIFIED: BQ ADC=%umV -> charging ON\n",
                    (unsigned)mv);
    } else {
      gPresenceEmptyUntilMs = millis() + RES_BATT_PRESENCE_EMPTY_MS;
      Board.enableBatteryCharging(false); // test left it off; make it explicit
      gChargingEnabled = false;
      Serial.printf("battery presence EMPTY: BQ ADC=%umV -> BAT treated absent\n",
                    (unsigned)mv);
    }
  }
}

void boardPowerTick() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < 1000) return;
  last = now;
  readBatteryNow();
}

bool railEnable3V3(bool on) {
  if (!gPfReady) return false;
  if (!on) {
    // Blank-before-cut is the LED driver's job; here we just drop and hold the pad.
    Board.enable3V3(false);
    bootParkRailLow();
    return false;
  }
  // Pre-init fail-safe holds EN_3V3 low in the RTC domain. Release the hold,
  // let the SDK take the mutex-guarded path, then re-init and VERIFY the
  // physical pad: Result::Ok has been observed with the pad still low.
  rtc_gpio_hold_dis(GPIO_NUM_4);
  if (Board.enable3V3(true) != Result::Ok) return false;
  if (rtc_gpio_hold_dis(GPIO_NUM_4) != ESP_OK ||
      rtc_gpio_init(GPIO_NUM_4) != ESP_OK ||
      rtc_gpio_set_direction(GPIO_NUM_4, RTC_GPIO_MODE_INPUT_OUTPUT) != ESP_OK ||
      rtc_gpio_set_level(GPIO_NUM_4, 1) != ESP_OK ||
      rtc_gpio_get_level(GPIO_NUM_4) != 1 ||
      rtc_gpio_hold_en(GPIO_NUM_4) != ESP_OK)
    return false;
  return true;
}

bool railEnableVSQT(bool on) {
  if (!gPfReady) return false;
  const int wanted = on ? 1 : 0;
  for (int a = 1; a <= 4; a++) {
    Result r = Board.enableVSQT(on);
    int observed = rtc_gpio_get_level(GPIO_NUM_14);
    if (r == Result::Ok && observed == wanted) return true;
    Serial.printf("VSQT %s attempt %d -> sdk=%d gpio14=%d\n",
                  on ? "ON" : "OFF", a, (int)r, observed);
    delay(10);
  }
  return false;
}

bool railCycleVSQT(uint16_t offMs, uint16_t settleMs) {
  bool offOk = railEnableVSQT(false);
  delay(offMs);
  bool onOk = railEnableVSQT(true);
  delay(settleMs);
  Serial.printf("VSQT boot/recovery cycle -> off=%s on=%s\n",
                offOk ? "verified" : "ERR", onOk ? "verified" : "ERR");
  return offOk && onOk;
}

bool applyCapacityAndReboot(uint16_t mah) {
  if (mah < RES_CAPACITY_MIN_MAH || mah > RES_CAPACITY_MAX_MAH) {
    Serial.printf("capacity %u rejected (range %u..%u mAh)\n", mah,
                  (unsigned)RES_CAPACITY_MIN_MAH, (unsigned)RES_CAPACITY_MAX_MAH);
    return false;
  }
  if (!nvsPersistCapacity(mah)) {
    Serial.println("capacity persist FAILED");
    return false;
  }
  Serial.printf("battery capacity stored -> %u mAh; rebooting to apply gauge model\n", mah);
  Serial.flush();
  delay(150);
  esp_restart();
  return true;
}

bool applyChargeMa(uint16_t ma) {
  if (ma < RES_CHARGE_MIN_MA || ma > RES_CHARGE_MAX_MA) {
    Serial.printf("charge current %u rejected (range %u..%u mA)\n", ma,
                  (unsigned)RES_CHARGE_MIN_MA, (unsigned)RES_CHARGE_MAX_MA);
    return false;
  }
  if (!nvsPersistChargeMa(ma)) {
    Serial.println("charge cap persist FAILED");
    return false;
  }
  if (gPfReady) Board.setBatteryChargingMaxCurrent((float)ma);
  Serial.printf("charge current cap stored/applied -> %u mA\n", ma);
  return true;
}

bool applyMaintainV10(uint8_t v10) {
  if (v10 < RES_MAINTAIN_MIN_V10 || v10 > RES_MAINTAIN_MAX_V10) {
    Serial.printf("maintain %u rejected (range %u..%u)\n", v10,
                  (unsigned)RES_MAINTAIN_MIN_V10, (unsigned)RES_MAINTAIN_MAX_V10);
    return false;
  }
  if (!nvsPersistMaintV10(v10)) return false;
  gMaintainV = v10 / 10.0f;
  if (gPfReady) Board.setSupplyMaintainVoltage(gMaintainV);
  Serial.printf("maintain/VINDPM -> %.1f V\n", gMaintainV);
  return true;
}

bool transportWakeDarkActive() { return gTransportWakeDark; }

void transportWakeDarkRelease() {
  if (!gTransportWakeDark && gTransportWakeMagic != kTransportWakeMagic) return;
  gTransportWakeDark = false;
  gTransportWakeMagic = 0;
  Serial.println("transport wake: dark latch released by bridge program command");
}

void enterTimedDeepSleep(uint32_t seconds, const char *why) {
  if (seconds == 0) seconds = 1;
  allLoadsOff("deep sleep");
  if (gPfReady) {
    railEnable3V3(false);
    railEnableVSQT(false);
  }
  Serial.printf("deep sleep (%s), timer wake %lus\n", why,
                (unsigned long)seconds);
  solenoidButtonPrepareSleep();
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
  esp_deep_sleep_start();
}

void enterTransportSleep(uint32_t seconds, const char *why) {
  if (seconds == 0 || seconds > 7UL * 24UL * 3600UL) {
    Serial.printf("transport sleep refused: %lus outside 1..604800s\n",
                  (unsigned long)seconds);
    return;
  }
  gTransportWakeMagic = kTransportWakeMagic;
  gTransportWakeDark = true;
  enterTimedDeepSleep(seconds, why);
}
