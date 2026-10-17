#include "protect_recovery.h"
#include <Arduino.h>
#include <math.h>
#include "esp_mac.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "board_power.h"
#include "boot_guard_io.h"
#include "led_driver.h"
#include "loads.h"
#include "nvs_store.h"
#include "ota_verify.h"
#include "power_glue.h"
#include "sensors/sensors.h"
#include "telemetry.h"

static ProtectRetryRecord gRecord;
static ProtectRetryState gState;
static bool gStorageOk = false, gTrial = false, gInitialized = false;
static uint32_t gStableSince = 0;
static bool gSensorRailOk = false;
static bool gRenderFailed = false;
static bool gWasProtected = false;
static uint16_t gTrialMinMv = UINT16_MAX;
static int16_t gTrialMinMa = INT16_MAX;
static bool gTrialExternalSeen = false;
static uint8_t gReportedPhase = 255;
struct RetrySleepCredit {
  uint32_t magic, generation, remainingMs, sleepS;
};
RTC_DATA_ATTR static RetrySleepCredit gSleepCredit;
static constexpr uint32_t kCreditMagic = 0x50525231;

bool protectRecoveryEnabled() {
  if (RES_PROTECT_RECOVERY_ALL) return true;
  if (RES_PROTECT_RECOVERY_TARGET == 0UL) return false;
  uint8_t mac[6];
  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) return false;
  return (((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | mac[5]) ==
         (uint32_t)RES_PROTECT_RECOVERY_TARGET;
}
const ProtectRetryState &protectRecoveryState() { return gState; }
const ProtectRetryRecord &protectRecoveryRecord() { return gRecord; }
void protectRecoveryNoteSensorRail(bool ok) { gSensorRailOk = ok; }
bool protectRecoveryStartupTimedOut() {
  return gTrial && !gInitialized && millis() >= 60000UL;
}

void protectRecoveryBoot(BootDecision &d, uint8_t storedStage) {
  if (!protectRecoveryEnabled()) return;
  gStorageOk = nvsReadProtectRetry(gRecord);
  if (!gStorageOk) {
    d.stage = STAGE_PROTECT; d.park = true; d.persistStage = STAGE_PROTECT;
    return;
  }
  uint8_t pending = gRecord.pending;
  if (!pending) return;
  gTrial = protectRetryBoot(gRecord, esp_reset_reason() == ESP_RST_SW,
                           storedStage == STAGE_PROTECT);
  if (!nvsWriteProtectRetry(gRecord)) { gStorageOk = false; gTrial = false; }
  // A running/aborted trial, including a deliberate reset, always parks. Only
  // the once-consumed software reboot ticket may initialize the sensor rail.
  d.stage = gTrial ? STAGE_LEDS_OFF : STAGE_PROTECT;
  d.park = !gTrial;
  d.persistStage = gTrial ? BOOT_GUARD_NO_WRITE : STAGE_PROTECT;
  d.retryConsumed = gTrial;
}

void protectRecoveryInit() {
  if (!protectRecoveryEnabled()) return;
  uint32_t cooldown = protectRetryCooldownS(gRecord.attempts);
  if (!gTrial && esp_reset_reason() == ESP_RST_DEEPSLEEP &&
      esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER &&
      gSleepCredit.magic == kCreditMagic &&
      gSleepCredit.generation == gRecord.generation &&
      gSleepCredit.remainingMs <= cooldown * 1000UL) {
    uint32_t elapsed = min(gSleepCredit.sleepS, cooldown) * 1000UL;
    cooldown = (gSleepCredit.remainingMs > elapsed)
        ? (gSleepCredit.remainingMs - elapsed + 999UL) / 1000UL : 0;
  }
  gSleepCredit.magic = 0; // consume once; cold/USB/button resets earn no time
  protectRetryInit(gState, millis(), gTrial, cooldown);
  gInitialized = true;
  gWasProtected = protectRecoveryOwnsOutputs();
  Serial.printf("protect-recovery: boot trial=%d attempts=%u cooldown=%lus storage=%d\n",
                gTrial, gRecord.attempts, (unsigned long)cooldown, gStorageOk);
}

bool protectRecoveryOwnsOutputs() {
  return protectRecoveryEnabled() && (gTrial || bootGuardStage() == STAGE_PROTECT);
}

static void failTrial(uint8_t reason) {
  allLoadsOff("recovery trial failed");
  railEnableVSQT(false);
  gState.reason = reason;
  bool parked = bootGuardSetStage(STAGE_PROTECT);
  bool saved = false;
  if (parked) {
    gRecord.last_reason = reason;
    gRecord.pending = 0; protectRetrySeal(gRecord);
    saved = nvsWriteProtectRetry(gRecord);
  }
  Serial.printf("protect-recovery: FAIL reason=%u attempts=%u journal=%d park=%d\n",
                reason, gRecord.attempts, saved, parked);
  Serial.flush();
  // The original fc_stage was never cleared during the trial. Even failed
  // writes above cannot leave a brighter boot stage behind.
  ESP.restart();
}

bool protectRecoveryTick(const PowerSample &s, PowerBudget &b) {
  if (!protectRecoveryEnabled() || !gInitialized) return false;
  if (!protectRecoveryOwnsOutputs()) {
    gWasProtected = false;
    // Do not forget recurrent collapse history after a mere reboot or dark
    // idle. Clear it only after two continuous minutes of healthy lit service.
    if (gRecord.attempts && s.batt_valid && batteryCurrentValid() && s.batt_corroborated &&
        s.charger_valid && !s.charger_fault && s.batt_v >= 3.15f && ledRailIsOn()) {
      if (!gStableSince) gStableSince = s.now_ms;
      if (s.now_ms - gStableSince >= 120000UL) {
        auto clear = gRecord; clear.attempts = 0;
        protectRetrySeal(clear);
        if (nvsWriteProtectRetry(clear)) gRecord = clear;
      }
    } else gStableSince = 0;
    return false;
  }
  if (!gWasProtected) {
    allLoadsOff("recovery park");
    railEnableVSQT(false);
    protectRetryInit(gState, s.now_ms, false, protectRetryCooldownS(gRecord.attempts));
    gWasProtected = true;
  }
  b = {}; b.tier = LedTier::PROTECT; b.sleep_s = 900;
  if (!gStorageOk) {
    gState.reason = PR_STORAGE;
    if (s.now_ms >= 15000UL && !otaVerifyPending())
      enterTimedDeepSleep(900, SLEEP_CAUSE_POWER_PROTECT);
    return true; // no retry can be spent without a readable/writable journal
  }
  // A full battery on USB can have near-zero net current. Preserve the
  // independent, rate-limited BQ presence test instead of requiring it to
  // accept charging current merely to prove that it exists.
  if (!s.batt_corroborated && s.batt_valid && s.supply_valid && s.supply_good &&
      s.charger_valid && !s.charger_fault)
    batteryRequestPresenceCheck();
  bool saneVoltage = isfinite(s.batt_v) && s.batt_v > 0 && s.batt_v < 4.4f;
  ProtectRetryInput input = {s.now_ms, s.batt_valid && batteryCurrentValid() && saneVoltage, s.batt_corroborated,
      saneVoltage ? (uint16_t)(s.batt_v * 1000.0f + 0.5f) : (uint16_t)0,
      s.charger_valid, s.charger_fault, gSensorRailOk, ledRailIsOn() && !gRenderFailed};
  if (gTrial) {
    if (!s.supply_valid || s.supply_good) gTrialExternalSeen = true;
    if (input.battery_valid) {
      if (input.mv < gTrialMinMv) gTrialMinMv = input.mv;
      int16_t ma = (int16_t)constrain(s.batt_ma, -32768.0f, 32767.0f);
      if (ma < gTrialMinMa) gTrialMinMa = ma;
    }
  }
  uint16_t entryMv = max((uint16_t)3250, (uint16_t)(gCfg.offMv ? gCfg.offMv + 150 : 3250));
  uint16_t trialMv = max((uint16_t)3150, (uint16_t)(gCfg.offMv ? gCfg.offMv + 50 : 3150));
  auto action = protectRetryTick(gState, input, entryMv, trialMv);
  if (gReportedPhase != gState.phase) {
    gReportedPhase = gState.phase;
    Serial.printf("protect-recovery: phase=%u attempts=%u bv=%u\n",
                  gState.phase, gRecord.attempts, input.mv);
  }
  if (gRenderFailed && gTrial) { gState.reason = PR_RAIL_FAILURE; action = PR_FAIL; }
  if (action == PR_BEGIN) {
    allLoadsOff("recovery begin");
    auto next = gRecord; protectRetrySpend(next);
    // Stage stays PROTECT. The journal grants ONE sensor-only boot, never an
    // unconditional clear or a reusable retry after a power loss.
    if (!bootGuardSetStage(STAGE_PROTECT) || !nvsWriteProtectRetry(next)) {
      gStorageOk = false; gState.reason = PR_STORAGE; return true;
    }
    gRecord = next;
    Serial.printf("protect-recovery: attempt=%u ticket persisted; reboot to sensors\n", gRecord.attempts);
    Serial.flush(); ESP.restart(); return true;
  }
  if (action == PR_FAIL) { failTrial(gState.reason); return true; }
  if (action == PR_PASS) {
    allLoadsOff("recovery complete");
    // Write the stage first; a reset before clearing pending conservatively
    // re-parks at next boot. Never clear the trial ticket before stage success.
    if (!bootGuardSetStage(STAGE_DIM)) { failTrial(PR_STORAGE); return true; }
    auto next = gRecord; next.pending = 0; next.last_reason = PR_OK;
    next.success_generation = next.generation;
    next.success_min_mv = gTrialMinMv; next.success_min_ma = gTrialMinMa;
    next.success_external_seen = gTrialExternalSeen ? 1 : 0;
    protectRetrySeal(next);
    if (!nvsWriteProtectRetry(next)) { failTrial(PR_STORAGE); return true; }
    gRecord = next; gTrial = false;
    powerGlueAdoptRecoveredTier();
    Serial.println("protect-recovery: PASS sensors + low LED + DIM LED; released to DIM");
    return false;
  }
  if (action == PR_SLEEP && !gTrial && s.now_ms >= 15000UL && !otaVerifyPending()) {
    uint32_t seconds = gState.remaining_ms ?
        min(900UL, (gState.remaining_ms + 999UL) / 1000UL) : 900UL;
    Serial.printf("protect-recovery: park reason=%u retry-in=%lums sleep=%lus\n",
                  gState.reason, (unsigned long)gState.remaining_ms, (unsigned long)seconds);
    enterTimedDeepSleep(seconds, SLEEP_CAUSE_POWER_PROTECT);
  }
  return true;
}

bool protectRecoveryRender() {
  if (!protectRecoveryOwnsOutputs()) return false;
  uint8_t cap = gTrial ? protectRetryBrightness(gState.phase) : 0;
  if (!cap || gBenchRailForcedOff) {
    if (ledRailIsOn()) ledRailOff();
    return true;
  }
  FrameBuffer f = {}; f.count = ledPixelCount();
  // One white pixel keeps both stages below the dense-HEX frame limiter.
  // Filling the whole perimeter would clamp cap 32 and cap 128 to the same
  // total load, defeating the staged test (ADR 0085).
  if (f.count) f.px[0][0] = f.px[0][1] = f.px[0][2] = 255;
  if (!ledRailIsOn() && !ledRailOnWithRamp(f, cap)) { gRenderFailed = true; return true; }
  static uint32_t last = 0;
  if (millis() - last >= 100) { last = millis(); ledRender(f, cap); }
  return true;
}

void protectRecoveryBeforeSleep(uint32_t seconds) {
  gSleepCredit.magic = 0;
  if (!protectRecoveryEnabled() || !gInitialized || gTrial || !gWasProtected ||
      gState.phase != PR_PARKED ||
      bootGuardStage() != STAGE_PROTECT || !gStorageOk) return;
  gSleepCredit = {kCreditMagic, gRecord.generation, gState.remaining_ms, seconds};
}

void protectRecoveryTest(uint16_t command) {
  if (!protectRecoveryEnabled() || !gStorageOk || otaVerifyPending()) return;
  if (command == 2 && gTrial) {
    Serial.println("protect-recovery: TEST reset during trial");
    Serial.flush(); ESP.restart(); return;
  }
  if ((command != 1 && command != 3) || gTrial) return;
  // Test entry deliberately preserves the historical burn-week audit and
  // retry count. It cannot reset backoff or grant a recovery ticket.
  allLoadsOff("explicit recovery test");
  if (!bootGuardSetStage(STAGE_PROTECT)) return;
  Serial.println("protect-recovery: TEST latch injected; historical audit retained");
  Serial.flush();
  if (command == 3) enterTimedDeepSleep(20, SLEEP_CAUSE_SERIAL);
  else ESP.restart();
}
