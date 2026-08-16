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
#include "boot_park.h"
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

static bool gPfReady = false;
static bool gChargingEnabled = false;
static float gMaintainV = 4.6f;
static float gCbV = 0.0f, gCbMa = 0.0f, gCbMaRaw = 0.0f;
static int gCbSoc = -1;
static float gCsV = 0.0f, gCsMa = 0.0f;
static bool gCsGood = false;
static bool gPrechargeWriteOk = false;
static BqSnapshot gBq;

bool pfIsReady() { return gPfReady; }
bool chargingEnabled() { return gChargingEnabled; }
bool batteryPresent() { return gCbV > 2.5f && gCbV < 4.4f; }
float maintainVolts() { return gMaintainV; }
float batteryVolts() { return gCbV; }
float batteryMa() { return gCbMa; }
float batteryMaRaw() { return gCbMaRaw; }
int batterySocPct() { return gCbSoc; }
float supplyVolts() { return gCsV; }
float supplyMa() { return gCsMa; }
bool supplyGood() { return gCsGood; }
bool prechargeConfigured() {
  return gPrechargeWriteOk && gBq.reg10 != 0xFF &&
         gBq.precharge_ma == RES_PF_PRECHARGE_MA;
}
uint16_t prechargeTargetMa() { return RES_PF_PRECHARGE_MA; }
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
  uint8_t before = 0;
  if (!pfSolarGuardRead8(0x10, before)) {
    Serial.println("fixture precharge: read REG0x10 failed");
    gPrechargeWriteOk = false;
    return false;
  }
  uint8_t wanted = bq25628ePrechargeReg10(before, targetMa);
  bool wrote = (wanted == before) || pfSolarGuardWrite8(0x10, wanted);
  uint8_t after = 0;
  bool readback = pfSolarGuardRead8(0x10, after);
  uint16_t actualMa = readback ? bq25628ePrechargeMa(after) : 0;
  gPrechargeWriteOk = wrote && readback && actualMa == targetMa;
  Serial.printf("fixture precharge: REG0x10 0x%02X -> 0x%02X target=%umA readback=%umA %s\n",
                before, readback ? after : 0xFF, (unsigned)targetMa,
                (unsigned)actualMa, gPrechargeWriteOk ? "OK" : "ERR");
  return gPrechargeWriteOk;
}

static void readChargerStatus() {
  gBq.vindpm_mv = gBq.ichg_ma = gBq.vreg_mv = gBq.precharge_ma = 0xFFFF;
  gBq.reg10 = gBq.reg16 = gBq.reg18 = gBq.stat0 = gBq.stat1 = 0xFF;
  gBq.fault0 = gBq.flag0 = gBq.flag1 = gBq.fault_flag0 = gBq.part = 0xFF;
  if (!gPfReady) return;

  float v = 0.0f;
  gBq.vindpm_mv = bqMvOrUnknown(Board.getCharger().getVINDPM(v), v);
  gBq.ichg_ma = bqMaOrUnknown(Board.getCharger().getChargeCurrentLimit(v), v);
  gBq.vreg_mv = bqMvOrUnknown(Board.getCharger().getChargeVoltageLimit(v), v);

  uint8_t b = 0;
  if (pfSolarGuardRead8(0x10, b)) {
    gBq.reg10 = b;
    gBq.precharge_ma = bq25628ePrechargeMa(b);
  }
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

void boardPowerInit() {
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
  }
  uint8_t s;
  if (Board.getBatteryCharge(s) == Result::Ok) gCbSoc = s;
}

static void chargingGuardTick() {
  static bool done = false;
  if (done || !gPfReady || millis() < 6000) return;
  if (gCbV < 0.1f) {
    if (millis() > 60000) {
      done = true;
      Serial.println("no battery reading after 60 s -> charging stays OFF until reboot");
    }
    return;
  }
  done = true;
  if (batteryPresent()) {
    Board.setBatteryChargingMaxCurrent((float)gCfg.chargeMa);
    configurePrecharge();
    Board.enableBatteryCharging(true);
    gChargingEnabled = true;
    pfSolarGuardInit(kTag, gMaintainV, true);
    Serial.printf("battery %.2fV present -> charging ON (%u mA, %s profile)\n",
                  gCbV, (unsigned)gCfg.chargeMa, batteryTypeName());
  } else {
    Serial.printf("battery %.2fV implausible -> charging stays OFF\n", gCbV);
  }
}

void readBatteryNow() {
  if (!gPfReady) return;
  readBatteryCell();
  chargingGuardTick();
  float v;
  if (Board.getSupplyVoltage(v) == Result::Ok) gCsV = v;
  if (Board.getSupplyCurrent(v) == Result::Ok) gCsMa = v;
  bool g;
  if (Board.checkSupplyGood(g) == Result::Ok) gCsGood = g;
  readChargerStatus();
  pfSolarGuardTick(kTag, gCsV, gCsMa, gCsGood, gMaintainV, gChargingEnabled);
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

void enterTimedDeepSleep(uint16_t seconds, const char *why) {
  if (seconds == 0) seconds = 1;
  allLoadsOff("deep sleep");
  if (gPfReady) {
    railEnable3V3(false);
    railEnableVSQT(false);
  }
  Serial.printf("deep sleep (%s), timer wake %us\n", why, (unsigned)seconds);
  solenoidButtonPrepareSleep();
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
  esp_deep_sleep_start();
}
