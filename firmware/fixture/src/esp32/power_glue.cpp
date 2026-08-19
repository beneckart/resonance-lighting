#include "power_glue.h"

#include <Arduino.h>
#include "esp_system.h"

#include "../core/power_integrator.h"
#include "board_power.h"
#include "boot_guard_io.h"
#include "led_driver.h"
#include "loads.h"
#include "solenoid.h"
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
static bool gProtectPersistDeferred = false; // ADR 0047 RAM-only PROTECT
static uint32_t gLoadsQuietSinceMs = 0;      // rail+solenoid quiet streak

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
  if (!powerConfigSanitize(gConfig)) {
    Serial.println("power: NVS threshold overrides invert the ladder -> defaults");
  }
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
  // ADR 0047: an active low-VBAT recovery lane also freezes the ladder --
  // batteryPresent() goes true at ~2.3 V for the charger's benefit, but a
  // rescue in progress must not walk the tier ladder or persist PROTECT
  // (the freeze keeps the fixture awake on verified external power, which
  // the recovery gate guarantees is present).
  s.batt_valid = pfIsReady() && batteryPresent() && !lowVbatRecoveryActive();
  s.batt_corroborated = batteryCorroborated();
  s.batt_v = batteryVolts();
  s.batt_ma = batteryMa();
  s.supply_valid = pfIsReady();
  s.supply_v = supplyVolts();
  s.supply_ma = supplyMa();
  s.supply_good = supplyGood();
  const BqSnapshot &bq = bqSnapshot();
  s.charger_fault = (bq.fault0 != 0x00 && bq.fault0 != 0xFF);

  PowerBudget b = powerPolicyTick(gState, s, gConfig);

  if (s.batt_valid)
    integratorTick(gIntegrator, now, s.batt_ma, (uint16_t)(s.batt_v * 1000.0f));

  if (b.tier_changed) {
    // Persist the stage BEFORE any load change becomes visible: a reset
    // mid-transition must boot into the safer stage, never the brighter one.
    // ADR 0047 exception: an uncorroborated PROTECT stays RAM-only (posture
    // applies -- rails off, park -- but the durable latch waits for battery
    // corroboration; a rate-limited BQ presence check is requested below).
    if (b.tier == LedTier::PROTECT && b.defer_protect_persist) {
      gProtectPersistDeferred = true;
      batteryRequestPresenceCheck();
      Serial.println("power: PROTECT held RAM-only (battery uncorroborated)");
    } else if (!bootGuardSetStage(powerTierToStage(b.tier)) &&
               b.tier < LedTier::PROTECT) {
      Serial.println("power: stage persist FAILED -> parking");
      b.tier = LedTier::PROTECT;
      b.brightness_cap = 0;
      b.must_sleep = true;
      b.protect_released = false; // release did NOT persist: no clean reboot
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

  // Resolve a deferred PROTECT persist: corroboration arriving while the tier
  // still holds writes the durable latch; leaving PROTECT (release path, which
  // requires charge current and therefore corroboration) abandons it.
  if (gProtectPersistDeferred) {
    if (b.tier != LedTier::PROTECT) {
      gProtectPersistDeferred = false;
    } else if (!b.defer_protect_persist && s.batt_valid && s.batt_corroborated) {
      // POSITIVE corroboration only (audit fix): the absence of the defer
      // flag alone also occurs on freeze ticks (EMPTY veto, recovery lane,
      // gauge dropout) and must never be misread as corroboration.
      gProtectPersistDeferred = false;
      if (bootGuardSetStage(powerTierToStage(LedTier::PROTECT)))
        Serial.println("power: PROTECT persisted after corroboration");
      else
        Serial.println("power: deferred PROTECT persist FAILED (tier already parked)");
    } else {
      batteryRequestPresenceCheck(); // nudge the rate-limited check
    }
  }

  // Commissioning keeps a parked fixture serviceable whenever a verified
  // external source is present. This does not energize a load or clear the
  // durable latch. If genuinely battery-only and critical, it still sleeps,
  // but retries every minute instead of disappearing for fifteen. Applied
  // AFTER every tier mutation (audit fix: the stage-persist-failure fallback
  // above can park a commission unit, and the override must cover that too).
  if (gCfg.profile == PROFILE_DEV && b.tier == LedTier::PROTECT) {
    b.sleep_s = RES_COMMISSION_PROTECT_SLEEP_S;
    if (s.supply_valid && s.supply_good) b.must_sleep = false;
  }

  // ADR 0028 rule 4: one healthy minute of uptime clears the consecutive
  // unexpected-reset streak that backs disarmed-loop escalation (audit fix;
  // read-before-write keeps this wear-free on ordinary wakes).
  static bool gStreakCleared = false;
  if (!gStreakCleared && now >= 60000UL) {
    gStreakCleared = true;
    nvsClearBootCount();
  }

  // ADR 0047 debounced disarm: clear the load-armed marker once both loads
  // have been provably quiet for a minute (collapses lease-flap NVS churn to
  // at most one write pair per quiet period; strike series stay armed).
  if (ledRailIsOn() || !solenoidQuietFor(60000UL)) {
    gLoadsQuietSinceMs = 0;
  } else {
    if (!gLoadsQuietSinceMs) gLoadsQuietSinceMs = now ? now : 1;
    if (now - gLoadsQuietSinceMs >= 60000UL)
      bootGuardLoadDisarm("loads quiet 60s");
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
