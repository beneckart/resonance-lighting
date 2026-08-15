// The Phase-4 deterministic-transition matrix, verbatim
// (docs/tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md): no reset sequence may
// produce an unlimited load-reapply loop.
#include "test_util.h"

#include "../src/core/boot_guard.h"

int main() {
  // POR while FULL -> consume the only retry BEFORE rail-on, run DIM.
  {
    BootDecision d = bootGuardDecide(true, STAGE_FULL, true);
    CHECK_EQ(d.stage, (uint8_t)STAGE_DIM);
    CHECK_EQ(d.persistStage, (uint8_t)STAGE_DIM); // durable before any rail-on
    CHECK(d.retryConsumed);
    CHECK(!d.park);
    CHECK(d.interrupted);
  }
  // POR while DIM -> rail off, durable PROTECT.
  {
    BootDecision d = bootGuardDecide(true, STAGE_DIM, true);
    CHECK_EQ(d.stage, (uint8_t)STAGE_PROTECT);
    CHECK_EQ(d.persistStage, (uint8_t)STAGE_PROTECT);
    CHECK(d.park);
    CHECK(!d.retryConsumed);
  }
  // POR while LEDS_OFF (even the radio-only load collapsed the cell) -> PROTECT.
  {
    BootDecision d = bootGuardDecide(true, STAGE_LEDS_OFF, true);
    CHECK_EQ(d.stage, (uint8_t)STAGE_PROTECT);
    CHECK(d.park);
  }
  // Any reset while PROTECT -> stay protected (durable charge-release latch).
  {
    BootDecision unexpected = bootGuardDecide(true, STAGE_PROTECT, true);
    CHECK_EQ(unexpected.stage, (uint8_t)STAGE_PROTECT);
    CHECK(unexpected.park);
    BootDecision sw = bootGuardDecide(true, STAGE_PROTECT, false);
    CHECK_EQ(sw.stage, (uint8_t)STAGE_PROTECT);
    CHECK(sw.park);
    CHECK(!sw.interrupted);
    CHECK_EQ(sw.persistStage, (uint8_t)BOOT_GUARD_NO_WRITE); // already durable
  }
  // NVS unreadable + unexpected reset -> fail safe, parked.
  {
    BootDecision d = bootGuardDecide(false, STAGE_IDLE, true);
    CHECK_EQ(d.stage, (uint8_t)STAGE_PROTECT);
    CHECK(d.park);
  }
  // NVS unreadable + clean reset -> idle (fresh board, first boot).
  {
    BootDecision d = bootGuardDecide(false, STAGE_IDLE, false);
    CHECK_EQ(d.stage, (uint8_t)STAGE_IDLE);
    CHECK(!d.park);
  }
  // Deliberate software reset / OTA from DIM -> dim preserved, retry NOT burned.
  {
    BootDecision d = bootGuardDecide(true, STAGE_DIM, false);
    CHECK_EQ(d.stage, (uint8_t)STAGE_DIM);
    CHECK(!d.retryConsumed);
    CHECK(!d.park);
    CHECK_EQ(d.persistStage, (uint8_t)BOOT_GUARD_NO_WRITE);
  }
  // Software reset from FULL -> full preserved.
  {
    BootDecision d = bootGuardDecide(true, STAGE_FULL, false);
    CHECK_EQ(d.stage, (uint8_t)STAGE_FULL);
    CHECK(!d.retryConsumed);
  }
  // Idle marker + unexpected reset -> plain idle boot (no live load to guard).
  {
    BootDecision d = bootGuardDecide(true, STAGE_IDLE, true);
    CHECK_EQ(d.stage, (uint8_t)STAGE_IDLE);
    CHECK(!d.park);
    CHECK(!d.interrupted);
  }
  // Out-of-range stored stage clamps to PROTECT (fail-safe on corruption).
  {
    BootDecision d = bootGuardDecide(true, 200, false);
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
