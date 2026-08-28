#include "boot_guard_io.h"

#include <Arduino.h>
#include "esp_system.h"

#include "../core/boot_guard.h"
#include "identity.h"
#include "nvs_store.h"
#include "telemetry.h"

#define RES_BOOT_LOOP_ESCALATE_STREAK 3

static BootDecision gDecision;
static bool gNvsOk = false;
static uint8_t gStoredStage = STAGE_IDLE;
static bool gLoadArmed = false; // RAM mirror of the ADR 0051 durable marker
static bool gHasProtectContext = false;
static ProtectAuditContext gProtectContext;

static void setProtectContext(uint8_t origin, uint8_t predecessor,
                              uint8_t resetReason, bool loadArmed,
                              uint32_t streak) {
  gProtectContext = ProtectAuditContext{};
  gProtectContext.origin = origin;
  gProtectContext.predecessor_stage = predecessor;
  gProtectContext.reset_reason = resetReason;
  gProtectContext.load_armed = loadArmed ? 1 : 0;
  gProtectContext.reset_streak = streak;
  gHasProtectContext = true;
}

void bootGuardPreInit() {
  gHasProtectContext = false;
  gProtectContext = ProtectAuditContext{};
  gNvsOk = nvsReadStage(gStoredStage);
  uint8_t resetReason = (uint8_t)esp_reset_reason();
  bool unexpected = bootGuardUnexpectedResetClass((int)resetReason);
  // ADR 0051: read the load-armed marker from the PREVIOUS run. Unreadable
  // marker -> assume armed (conservative: escalation stays possible).
  bool prevArmed = true;
  if (!nvsReadLoadArmed(prevArmed)) prevArmed = true;
  // Audit fix (ADR 0028 rule 4, finally wired): a collapse loop with loads
  // OFF (bare radio, VSQT sensor bring-up) has no marker to arm, but it must
  // not grind the cell down unbounded. Count consecutive unexpected resets;
  // an expected reset or 60 s of healthy uptime (power glue) clears the
  // streak; at the threshold, escalate exactly as if the load were armed.
  uint32_t streak = 0;
  if (unexpected) streak = nvsBumpBootCount();
  else nvsClearBootCount();
  bool escalate = prevArmed || streak >= RES_BOOT_LOOP_ESCALATE_STREAK;
  gDecision = bootGuardDecide(gNvsOk, gStoredStage, unexpected, escalate);
  if (gDecision.stage == STAGE_PROTECT && gStoredStage != STAGE_PROTECT) {
    if (!gNvsOk) {
      setProtectContext(PROTECT_ORIGIN_NVS_FAILSAFE, gStoredStage, resetReason,
                        prevArmed, streak);
    } else if (unexpected && escalate) {
      setProtectContext(prevArmed ? PROTECT_ORIGIN_RESET_LOAD_ARMED
                                  : PROTECT_ORIGIN_RESET_STREAK,
                        gStoredStage, resetReason, prevArmed, streak);
    }
  }
  if (gDecision.persistStage != BOOT_GUARD_NO_WRITE) {
    if (!nvsWriteStage(gDecision.persistStage)) {
      // Retry-consumption could not be made durable: fail safe to PROTECT
      // (donor semantics -- an unrecordable retry must not be spent).
      gDecision.stage = STAGE_PROTECT;
      gDecision.park = true;
      gDecision.retryConsumed = false;
      setProtectContext(PROTECT_ORIGIN_STAGE_PERSIST_FAILURE, gStoredStage,
                        resetReason, prevArmed, streak);
    }
  }
  // This run starts with every load off (bootParkRailLow before anything else
  // can energize): clear the marker so a reset before the first rail-on reads
  // as power-ordering, not collapse. Write-on-change keeps NVS wear nil.
  if (prevArmed) (void)nvsWriteLoadArmed(false);
  gLoadArmed = false;
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
bool bootGuardProtectContext(ProtectAuditContext &out) {
  out = gProtectContext;
  return gHasProtectContext;
}

// ADR 0051: durable load-armed marker. Arm BEFORE energizing (a failed write
// refuses the load, mirroring the stage-persist doctrine); disarm is called
// from allLoadsOff and from the debounced rail-quiet path in power glue.
bool bootGuardLoadArm() {
  if (gLoadArmed) return true;
  if (!nvsWriteLoadArmed(true)) return false;
  gLoadArmed = true;
  return true;
}

void bootGuardLoadDisarm(const char *why) {
  if (!gLoadArmed) return;
  if (!nvsWriteLoadArmed(false)) return; // stays armed: conservative
  gLoadArmed = false;
  (void)why;
}

bool bootGuardLoadArmed() { return gLoadArmed; }

bool bootGuardSetStage(uint8_t stage) {
  if (stage > STAGE_PROTECT) stage = STAGE_PROTECT;
  if (gNvsOk && gDecision.stage == stage) return true;
  if (!nvsWriteStage(stage)) return false;
  gDecision.stage = stage;
  gTelemetryGuardStage = stage;
  return true;
}
