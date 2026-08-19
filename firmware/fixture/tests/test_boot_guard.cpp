// The Phase-4 deterministic-transition matrix, verbatim
// (docs/tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md): no reset sequence may
// produce an unlimited load-reapply loop. Extended by ADR 0051: escalation
// additionally requires the durable load-armed marker, so power-ordering
// resets (panel-first, battery-last, bench USB) with all loads off cost no
// ladder progress.
#include "test_util.h"

#include "../src/core/boot_guard.h"

int main() {
  // POR while FULL with the load armed -> consume the only retry BEFORE
  // rail-on, run DIM.
  {
    BootDecision d = bootGuardDecide(true, STAGE_FULL, true, true);
    CHECK_EQ(d.stage, (uint8_t)STAGE_DIM);
    CHECK_EQ(d.persistStage, (uint8_t)STAGE_DIM); // durable before any rail-on
    CHECK(d.retryConsumed);
    CHECK(!d.park);
    CHECK(d.interrupted);
  }
  // POR while DIM, load armed -> rail off, durable PROTECT.
  {
    BootDecision d = bootGuardDecide(true, STAGE_DIM, true, true);
    CHECK_EQ(d.stage, (uint8_t)STAGE_PROTECT);
    CHECK_EQ(d.persistStage, (uint8_t)STAGE_PROTECT);
    CHECK(d.park);
    CHECK(!d.retryConsumed);
  }
  // POR while LEDS_OFF, load armed (the solenoid or a just-disarmed rail
  // collapsed the cell) -> PROTECT.
  {
    BootDecision d = bootGuardDecide(true, STAGE_LEDS_OFF, true, true);
    CHECK_EQ(d.stage, (uint8_t)STAGE_PROTECT);
    CHECK(d.park);
  }
  // ADR 0051: the same unexpected resets with all loads DISARMED are
  // power-ordering events, not collapses -- preserve the stage, write
  // nothing, burn no retry. The voltage ladder re-derives within one tick.
  {
    BootDecision full = bootGuardDecide(true, STAGE_FULL, true, false);
    CHECK_EQ(full.stage, (uint8_t)STAGE_FULL);
    CHECK_EQ(full.persistStage, (uint8_t)BOOT_GUARD_NO_WRITE);
    CHECK(!full.retryConsumed);
    CHECK(!full.park);
    CHECK(full.interrupted); // still telemetry-visible
    BootDecision dim = bootGuardDecide(true, STAGE_DIM, true, false);
    CHECK_EQ(dim.stage, (uint8_t)STAGE_DIM);
    CHECK_EQ(dim.persistStage, (uint8_t)BOOT_GUARD_NO_WRITE);
    CHECK(!dim.park);
    BootDecision off = bootGuardDecide(true, STAGE_LEDS_OFF, true, false);
    CHECK_EQ(off.stage, (uint8_t)STAGE_LEDS_OFF);
    CHECK_EQ(off.persistStage, (uint8_t)BOOT_GUARD_NO_WRITE);
    CHECK(!off.park);
  }
  // Any reset while PROTECT -> stay protected (durable charge-release latch),
  // armed or not: the latch outranks the marker.
  {
    BootDecision unexpected = bootGuardDecide(true, STAGE_PROTECT, true, true);
    CHECK_EQ(unexpected.stage, (uint8_t)STAGE_PROTECT);
    CHECK(unexpected.park);
    BootDecision disarmed = bootGuardDecide(true, STAGE_PROTECT, true, false);
    CHECK_EQ(disarmed.stage, (uint8_t)STAGE_PROTECT);
    CHECK(disarmed.park);
    BootDecision sw = bootGuardDecide(true, STAGE_PROTECT, false, false);
    CHECK_EQ(sw.stage, (uint8_t)STAGE_PROTECT);
    CHECK(sw.park);
    CHECK(!sw.interrupted);
    CHECK_EQ(sw.persistStage, (uint8_t)BOOT_GUARD_NO_WRITE); // already durable
  }
  // NVS unreadable + unexpected reset -> fail safe, parked, regardless of the
  // marker (an unreadable NVS cannot vouch for the marker either).
  {
    BootDecision d = bootGuardDecide(false, STAGE_IDLE, true, false);
    CHECK_EQ(d.stage, (uint8_t)STAGE_PROTECT);
    CHECK(d.park);
    BootDecision armed = bootGuardDecide(false, STAGE_IDLE, true, true);
    CHECK_EQ(armed.stage, (uint8_t)STAGE_PROTECT);
    CHECK(armed.park);
  }
  // NVS unreadable + clean reset -> idle (fresh board, first boot).
  {
    BootDecision d = bootGuardDecide(false, STAGE_IDLE, false, false);
    CHECK_EQ(d.stage, (uint8_t)STAGE_IDLE);
    CHECK(!d.park);
  }
  // Deliberate software reset / OTA from DIM -> dim preserved, retry NOT
  // burned, regardless of the marker (armed = mid-show OTA reboot).
  {
    BootDecision d = bootGuardDecide(true, STAGE_DIM, false, true);
    CHECK_EQ(d.stage, (uint8_t)STAGE_DIM);
    CHECK(!d.retryConsumed);
    CHECK(!d.park);
    CHECK_EQ(d.persistStage, (uint8_t)BOOT_GUARD_NO_WRITE);
  }
  // Software reset from FULL -> full preserved.
  {
    BootDecision d = bootGuardDecide(true, STAGE_FULL, false, false);
    CHECK_EQ(d.stage, (uint8_t)STAGE_FULL);
    CHECK(!d.retryConsumed);
  }
  // Idle marker + unexpected reset -> plain idle boot (no live load to guard),
  // armed or not.
  {
    BootDecision d = bootGuardDecide(true, STAGE_IDLE, true, true);
    CHECK_EQ(d.stage, (uint8_t)STAGE_IDLE);
    CHECK(!d.park);
    CHECK(!d.interrupted);
  }
  // Out-of-range stored stage clamps to PROTECT (fail-safe on corruption).
  {
    BootDecision d = bootGuardDecide(true, 200, false, false);
    CHECK_EQ(d.stage, (uint8_t)STAGE_PROTECT);
    CHECK(d.park);
  }
  // Reset classification pins the esp_reset_reason_t numeric contract.
  CHECK(bootGuardUnexpectedResetClass(1));  // POWERON
  CHECK(!bootGuardUnexpectedResetClass(3)); // SW
  CHECK(bootGuardUnexpectedResetClass(4));  // PANIC
  CHECK(bootGuardUnexpectedResetClass(6));  // TASK_WDT
  CHECK(!bootGuardUnexpectedResetClass(8)); // DEEPSLEEP
  CHECK(bootGuardUnexpectedResetClass(9));  // BROWNOUT

  return testReport("test_boot_guard");
}
