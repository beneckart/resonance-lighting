#include "ota_verify.h"

#include <Arduino.h>
#include <Preferences.h>
#include "esp_ota_ops.h"

#include "board_power.h"
#include "espnow_link.h"
#include "watchdog.h"

// C linkage is the whole trick (see header). Do not "clean this up" into C++.
extern "C" bool verifyRollbackLater() { return true; }
extern "C" bool verifyOta() { return true; } // fallback if the deferred path is disabled

#define RES_OTA_VERIFY_AT_MS 20000

static bool gChecked = false;
static bool gPendingCache = true;

bool otaVerifyPending() {
  if (gChecked) return false;
  if (!gPendingCache) return false;
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t st = ESP_OTA_IMG_UNDEFINED;
  if (esp_ota_get_state_partition(running, &st) != ESP_OK) {
    gPendingCache = false;
    return false;
  }
  gPendingCache = (st == ESP_OTA_IMG_PENDING_VERIFY);
  return gPendingCache;
}

static bool selfTest() {
#ifdef RES_OTA_FAIL_SELFTEST
  Serial.println("ota-verify: RES_OTA_FAIL_SELFTEST forcing failure (rollback drill)");
  return false;
#endif
  // 1. Power chip alive: SDK init succeeded and the BQ part id byte read back.
  if (!pfIsReady()) return false;
  if (bqSnapshot().part == 0xFF) return false;
  // 2. Gauge sanity: voltage in a physical range. A bare board on USB reads
  //    ~0 V -- that is a PASS (bringup flashes batteryless boards).
  if (batteryVolts() > 4.4f) return false;
  // 3. Radio: ESP-NOW came up and at least one send completed (the boot
  //    announce guarantees attempts; broadcast completion needs no receiver).
  if (!espNowUp() || espNowSendOk() == 0) return false;
  // 4. NVS writable.
  {
    Preferences pf;
    if (!pf.begin("resfx", false)) return false;
    uint32_t probe = millis() | 1;
    bool ok = pf.putUInt("otaprobe", probe) == sizeof(uint32_t) &&
              pf.getUInt("otaprobe", 0) == probe;
    pf.end();
    if (!ok) return false;
  }
  // 5. Watchdog armed (a hang after this point still reverts the image).
  if (!watchdogArmed()) return false;
  return true;
}

void otaVerifyTick() {
  if (gChecked || millis() < RES_OTA_VERIFY_AT_MS) return;
  if (!otaVerifyPending()) {
    gChecked = true;
    return;
  }
  gChecked = true;
  gPendingCache = false;
  if (selfTest()) {
    esp_ota_mark_app_valid_cancel_rollback();
    Serial.println("ota-verify: VALID (rollback cancelled)");
  } else {
    Serial.println("ota-verify: SELF-TEST FAILED -> rolling back to last-good");
    Serial.flush();
    delay(100);
    esp_ota_mark_app_invalid_rollback_and_reboot();
  }
}
