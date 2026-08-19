#include "boot_guard.h"

// Mirrors esp_reset_reason_t values (stable in esp-idf; pinned by native test
// against the numeric values so the core stays Arduino-free).
// 1=POWERON 3=SW 4=PANIC 5=INT_WDT 6=TASK_WDT 7=WDT 8=DEEPSLEEP 9=BROWNOUT
bool bootGuardUnexpectedResetClass(int r) {
  switch (r) {
  case 1:  // ESP_RST_POWERON
  case 4:  // ESP_RST_PANIC
  case 5:  // ESP_RST_INT_WDT
  case 6:  // ESP_RST_TASK_WDT
  case 7:  // ESP_RST_WDT
  case 9:  // ESP_RST_BROWNOUT
    return true;
  default:
    return false;
  }
}

BootDecision bootGuardDecide(bool nvsOk, uint8_t storedStage, bool unexpectedReset,
                             bool loadArmed) {
  BootDecision d = {};
  d.persistStage = BOOT_GUARD_NO_WRITE;

  if (!nvsOk) {
    // NVS unreadable: on an unexpected reset we cannot rule out a collapse
    // loop -- fail safe, rail off. (Persisting is pointless: writes will fail
    // too, and callers treat a failed persist as PROTECT anyway.)
    if (unexpectedReset) {
      d.stage = STAGE_PROTECT;
      d.park = true;
      d.interrupted = true;
    } else {
      d.stage = STAGE_IDLE;
    }
    return d;
  }

  if (storedStage > STAGE_PROTECT) storedStage = STAGE_PROTECT;

  if (storedStage == STAGE_PROTECT) {
    // PROTECT is a durable charge-release latch, authoritative on EVERY reset
    // type (RTC state does not survive OTA/software resets, NVS does).
    d.stage = STAGE_PROTECT;
    d.park = true;
    d.interrupted = unexpectedReset;
    return d;
  }

  // ADR 0051: escalation costs ladder progress only when a real load (LED
  // rail or solenoid gate) was actually armed when the reset hit. Panel-first,
  // battery-last, and bench USB power-ordering resets arrive with loads off
  // and must not walk FULL -> DIM -> PROTECT (the 2026-08 "31 poweron resets
  // in 19 minutes" class of false latch). A collapse loop under load still
  // escalates exactly as before because the armed marker persists through it.
  if (unexpectedReset && storedStage != STAGE_IDLE && loadArmed) {
    d.interrupted = true;
    if (storedStage == STAGE_FULL) {
      // Consume the only retry BEFORE any rail can turn on: a reset during the
      // dim attempt then boots with DIM already persisted and parks below.
      d.stage = STAGE_DIM;
      d.persistStage = STAGE_DIM;
      d.retryConsumed = true;
    } else {
      // A reset from DIM or LEDS_OFF with the marker armed means even the
      // reduced load collapsed the cell: durable PROTECT (Phase-4 "POR while
      // DIM"). The loads-OFF collapse class (bare radio, sensor bring-up)
      // reaches this same escalation through the glue consecutive
      // unexpected-reset streak (ADR 0028 rule 4), which passes
      // loadArmed=true at the threshold.
      d.stage = STAGE_PROTECT;
      d.persistStage = STAGE_PROTECT;
      d.park = true;
    }
    return d;
  }

  // Expected reset (software/OTA/deepsleep), idle marker, or an unexpected
  // reset with all loads disarmed (power-ordering, not collapse): preserve the
  // stored tier; the voltage ladder re-derives the truth within one tick.
  d.stage = storedStage;
  d.interrupted = unexpectedReset && storedStage != STAGE_IDLE;
  return d;
}
