#include "decompression.h"
#include <Arduino.h>
#include "esp_timer.h"
#include "../core/decompression_session.h"
#include "board_power.h"
#include "boot_guard_io.h"
#include "maintenance.h"
#include "ota_verify.h"
#include "power_glue.h"
#include "solenoid.h"

static DecompressionSession gSession;
static bool gWakePending = false;
static bool gWasActive = false;

bool decompressionActive() {
  return decompressionBuild() && gSession.active(esp_timer_get_time());
}
uint32_t decompressionSecondsLeft() {
  return decompressionBuild() ? gSession.remainingSeconds(esp_timer_get_time()) : 0;
}
bool decompressionSetSession(uint16_t minutes) {
  if (!decompressionBuild() || minutes > 120) return false;
  if (!minutes) solenoidStop("decompression session ended");
  return gSession.set(esp_timer_get_time(), minutes);
}
bool decompressionPlaySignature() {
  if (!decompressionBuild() || maintMode() != MODE_COMMS || otaVerifyPending() ||
      bootGuardParked() || !pfIsReady() || !batteryPresent() ||
      (uint8_t)powerBudget().tier > (uint8_t)LedTier::DIM)
    return false;
  if (!solenoidSignatureStart()) return false;
  // Explicit first trigger opens one hour; later hits do not extend it.
  if (!decompressionActive()) decompressionSetSession(60);
  return true;
}
void decompressionButtonWake() { gWakePending = true; }
void decompressionTick() {
  if (!decompressionBuild()) return;
  bool active = decompressionActive();
  if ((gWasActive && !active) || !pfIsReady() || !batteryPresent() ||
      (uint8_t)powerBudget().tier > (uint8_t)LedTier::DIM)
    solenoidStop("decompression expiry/power veto");
  gWasActive = active;
  if (gWakePending && millis() >= 5000) {
    gWakePending = false;
    (void)decompressionPlaySignature();
  }
}
