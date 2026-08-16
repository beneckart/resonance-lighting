#include "power_glue.h"

#include <Arduino.h>
#include "esp_system.h"

#include "../core/power_integrator.h"
#include "board_power.h"
#include "boot_guard_io.h"
#include "led_driver.h"
#include "loads.h"
#include "net_peer.h"
#include "nvs_store.h"
#include "ota_verify.h"
#include "telemetry.h"

// Cold-boot grace: serve telemetry/maintenance before the first PROTECT sleep
// can fire (donor cold-listen pattern), and give a deep-sleep wake a short
// command window.
#define RES_PROTECT_BOOT_GRACE_MS 30000
#define RES_PROTECT_WAKE_GRACE_MS 8000
#define RES_COMMISSION_PROTECT_SLEEP_S 60

static PowerState gState;
static PowerBudget gBudget;
static PowerConfig gConfig;
static PowerIntegrator gIntegrator;
static uint32_t gGraceUntilMs = 0;

const PowerBudget &powerBudget() { return gBudget; }

static LedTier stageToTier(uint8_t stage) {
  switch (stage) {
  case STAGE_DIM: return LedTier::DIM;
  case STAGE_LEDS_OFF: return LedTier::OFF;
  case STAGE_PROTECT: return LedTier::PROTECT;
  default: return LedTier::FULL; // idle/full: ladder re-derives from voltage
  }
}

void powerGlueInit() {
  gConfig = powerConfigDefaults();
  // Per-unit NVS threshold overrides (two-tier fleet: 33140 thresholds are
  // pending qualification, so they arrive as runtime data, not builds).
  if (gCfg.dimMv) gConfig.dim_mv = gCfg.dimMv;
  if (gCfg.offMv) gConfig.off_mv = gCfg.offMv;
  if (gCfg.slpMv) gConfig.protect_mv = gCfg.slpMv;
  powerStateInit(gState, stageToTier(bootGuardStage()));
  // Publish an initial budget so consumers never see zeros (an invalid sample
  // freezes the ladder but still fills the budget for the current tier).
  PowerSample s = {};
  gBudget = powerPolicyTick(gState, s, gConfig);
  integratorReset(gIntegrator);
  gGraceUntilMs = millis() + (esp_reset_reason() == ESP_RST_DEEPSLEEP
                                  ? RES_PROTECT_WAKE_GRACE_MS
                                  : RES_PROTECT_BOOT_GRACE_MS);
  gTelemetryPowerTier = (uint8_t)gBudget.tier;
  gNetPowerTier = (uint8_t)gBudget.tier;
}

void powerGlueTick() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < 1000) return;
  last = now;

  PowerSample s = {};
  s.now_ms = now;
  // An empty BAT input can briefly return a nonzero sub-cell voltage. Use the
  // same plausible-cell window as the deferred charging guard; otherwise a
  // bare USB commission can falsely persist immediate low-voltage PROTECT.
  s.batt_valid = pfIsReady() && batteryPresent();
  s.batt_v = batteryVolts();
  s.batt_ma = batteryMa();
  s.supply_valid = pfIsReady();
  s.supply_v = supplyVolts();
  s.supply_ma = supplyMa();
  s.supply_good = supplyGood();
  const BqSnapshot &bq = bqSnapshot();
  s.charger_fault = (bq.fault0 != 0x00 && bq.fault0 != 0xFF);

  PowerBudget b = powerPolicyTick(gState, s, gConfig);

  // Commissioning keeps a parked fixture serviceable whenever a verified
  // external source is present. This does not energize a load or clear the
  // durable latch. If genuinely battery-only and critical, it still sleeps,
  // but retries every minute instead of disappearing for fifteen.
  if (gCfg.profile == PROFILE_DEV && b.tier == LedTier::PROTECT) {
    b.sleep_s = RES_COMMISSION_PROTECT_SLEEP_S;
    if (s.supply_valid && s.supply_good) b.must_sleep = false;
  }

  if (s.batt_valid)
    integratorTick(gIntegrator, now, s.batt_ma, (uint16_t)(s.batt_v * 1000.0f));

  if (b.tier_changed) {
    // Persist the stage BEFORE any load change becomes visible: a reset
    // mid-transition must boot into the safer stage, never the brighter one.
    if (!bootGuardSetStage(powerTierToStage(b.tier)) && b.tier < LedTier::PROTECT) {
      Serial.println("power: stage persist FAILED -> parking");
      b.tier = LedTier::PROTECT;
      b.brightness_cap = 0;
      b.must_sleep = true;
      powerStateInit(gState, LedTier::PROTECT);
    }
    Serial.printf("power: tier -> %u (bv=%.3f ma=%.0f)%s\n", (unsigned)b.tier,
                  s.batt_v, s.batt_ma, b.protect_released ? " [protect released]" : "");

    if (b.protect_released) {
      // A parked boot deliberately skipped the sensor-domain cold start,
      // class probe, sensor init, and LED profile. The qualified 60-second
      // charge release above has now durably replaced PROTECT with LEDS_OFF;
      // reboot cleanly instead of merely clearing the in-RAM park flag and
      // trying to run partially initialized hardware. This makes recovery
      // automatic while preserving the rule that no reset alone clears the
      // durable PROTECT latch.
      Serial.println("power: PROTECT release persisted -> clean reboot");
      Serial.flush();
      delay(100);
      ESP.restart();
      return;
    }

    if (b.brightness_cap == 0 && ledRailIsOn()) {
      gSmokeRender = false;
      ledRailOff();
    }
  }

  gBudget = b;
  gTelemetryPowerTier = (uint8_t)b.tier;
  gNetPowerTier = (uint8_t)b.tier;

  // Never deep-sleep with an unverified image: the pending-verify window must
  // resolve to valid-or-rollback first.
  if (b.must_sleep && !otaVerifyPending() && (int32_t)(now - gGraceUntilMs) >= 0) {
    netPeerSendHeartbeat(true); // last full status before parking
    delay(50);
    enterTimedDeepSleep(b.sleep_s, "protect");
  }
}
