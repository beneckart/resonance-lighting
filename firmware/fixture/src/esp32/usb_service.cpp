#include "usb_service.h"
#include <Arduino.h>
#include "driver/rtc_io.h"
#include "board_power.h"
#include "boot_guard_io.h"
#include "boot_park.h"
#include "identity.h"
#include "loads.h"
#include "maintenance.h"
#include "nvs_store.h"
#include "ota_verify.h"
#include "power_glue.h"
#include "telemetry.h"

static bool gActive = false;
static bool gReady = false;
static uint32_t gStartedMs = 0;
bool usbServiceActive() { return gActive; }
bool usbServiceReady() { return gReady; }

void usbServicePrepare() {
  if (gActive) return; // repeated requests cannot renew the deadline
  readBatteryNow();
  if (otaVerifyPending() || maintMode() != MODE_COMMS || !pfIsReady() ||
      !supplyGood() || supplyVolts() < 4.5f) {
    Serial.println("usb-service: REFUSED power/mode/verify");
    return;
  }
  gActive = true; // fence every LED/solenoid arm BEFORE shutting down
  gStartedMs = millis();
  bool radioOff = commsPauseRadio();
  allLoadsOff("USB service");
  bool railsOff = rtc_gpio_get_level(GPIO_NUM_4) == 0 &&
                  digitalRead(RES_SOLENOID_PIN) == LOW;
  bool armed = true;
  uint8_t stored = STAGE_PROTECT;
  bool durable = railsOff && nvsReadLoadArmed(armed) && !armed &&
                 nvsReadStage(stored) && stored == bootGuardStage();
  // Only a deliberately quiesced, acknowledged service operation resets the
  // reset streak. No tier write: an existing PROTECT remains authoritative.
  gReady = radioOff && durable && nvsClearBootCount();
  Serial.printf("usb-service: %s target=%s window_ms=30000 stage=%u\n",
                gReady ? "READY" : "REFUSED", gShortId.c_str(), stored);
  Serial.flush();
}

void usbServiceCancel() {
  if (!gActive) return;
  Serial.println("usb-service: clean reboot");
  Serial.flush();
  ESP.restart(); // expected reset; never resume old queued output commands
}

void usbServiceTick() {
  if (gActive && millis() - gStartedMs >= 30000UL) usbServiceCancel();
}

void powerRecoveryTest(uint16_t tier) {
#if defined(RES_POWER_RECOVERY_TEST_TARGET)
  uint32_t id = ((uint32_t)gMyId[0] << 16) | ((uint32_t)gMyId[1] << 8) | gMyId[2];
  readBatteryNow();
  if (id == RES_POWER_RECOVERY_TEST_TARGET && !otaVerifyPending() &&
      maintMode() == MODE_COMMS && batteryPresent() && batteryVolts() >= 3.3f &&
      powerBudget().tier == LedTier::FULL && bootGuardStage() == STAGE_FULL &&
      (tier == 1 || tier == 2)) {
    allLoadsOff("power recovery test");
    if (bootGuardSetStage(tier == 1 ? STAGE_DIM : STAGE_LEDS_OFF)) {
      Serial.printf("power-test: tier=%u sleeping 3s\n", tier);
      Serial.flush();
      enterTimedDeepSleep(3, SLEEP_CAUSE_SERIAL);
    }
  }
#else
  (void)tier;
#endif
  Serial.println("power-test: REFUSED");
}
