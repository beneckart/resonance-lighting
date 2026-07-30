#include "boot_guard_io.h"

#include <Arduino.h>
#include "esp_system.h"

#include "../core/boot_guard.h"
#include "identity.h"
#include "nvs_store.h"
#include "telemetry.h"

static BootDecision gDecision;
static bool gNvsOk = false;
static uint8_t gStoredStage = STAGE_IDLE;

void bootGuardPreInit() {
  gNvsOk = nvsReadStage(gStoredStage);
  bool unexpected = bootGuardUnexpectedResetClass((int)esp_reset_reason());
  gDecision = bootGuardDecide(gNvsOk, gStoredStage, unexpected);
  if (gDecision.persistStage != BOOT_GUARD_NO_WRITE) {
    if (!nvsWriteStage(gDecision.persistStage)) {
      // Retry-consumption could not be made durable: fail safe to PROTECT
      // (donor semantics -- an unrecordable retry must not be spent).
      gDecision.stage = STAGE_PROTECT;
      gDecision.park = true;
      gDecision.retryConsumed = false;
    }
  }
  gTelemetryGuardStage = gDecision.stage;
  gTelemetryGuardInterrupted = gDecision.interrupted;
}

void bootGuardReport() {
  Serial.printf("boot-guard: stage=%u stored=%u park=%d retry=%d interrupted=%d nvs=%d reset=%s\n",
                gDecision.stage, gStoredStage, gDecision.park ? 1 : 0,
                gDecision.retryConsumed ? 1 : 0, gDecision.interrupted ? 1 : 0,
                gNvsOk ? 1 : 0, resetReasonName(esp_reset_reason()));
}

uint8_t bootGuardStage() { return gDecision.stage; }
bool bootGuardParked() { return gDecision.park; }
bool bootGuardRetryConsumed() { return gDecision.retryConsumed; }
bool bootGuardInterrupted() { return gDecision.interrupted; }

bool bootGuardSetStage(uint8_t stage) {
  if (stage > STAGE_PROTECT) stage = STAGE_PROTECT;
  if (gNvsOk && gDecision.stage == stage) return true;
  if (!nvsWriteStage(stage)) return false;
  gDecision.stage = stage;
  gTelemetryGuardStage = stage;
  return true;
}
