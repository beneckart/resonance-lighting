// POR/reboot-loop boot guard: the pure decision behind net_bench's
// fieldSessionGuardPreInit, hardened per the Phase-4 deterministic-transition
// matrix (docs/tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md).
//
// Ground truth this encodes (POWERFEATHER_NOTES "A POR can erase low-voltage
// protection"): a heavy load can collapse the source before a dim decision
// confirms; the POR erases RAM/RTC latches; rebound voltage looks healthy; the
// load reapplies; repeat forever. NVS is the only memory that survives, so the
// stage ladder lives there and every unexpected reset costs ladder progress.
#pragma once

#include <stdint.h>
#include "fixture_context.h"

struct BootDecision {
  uint8_t stage;        // stage to run this boot (BootStage)
  uint8_t persistStage; // stage to write BEFORE any rail-on; 0xFF = no write
  bool park;            // hard-park: LED rail stays off until verified charge
  bool retryConsumed;   // this boot consumed the single reduced-load retry
  bool interrupted;     // unexpected reset arrived with a live stage marker
};

#define BOOT_GUARD_NO_WRITE 0xFF

// nvsOk=false means the stage could not be READ (unwritten counts as ok/IDLE).
// unexpectedReset covers poweron/panic/all-watchdogs/brownout.
BootDecision bootGuardDecide(bool nvsOk, uint8_t storedStage, bool unexpectedReset);

bool bootGuardUnexpectedResetClass(int espResetReason); // esp_reset_reason_t
