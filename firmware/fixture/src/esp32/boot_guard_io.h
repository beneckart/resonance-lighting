// NVS/reset-reason glue around core/boot_guard. bootGuardPreInit() runs as
// step 2 of setup(), BEFORE Serial: the consumed-retry persist must land
// before anything can energize a rail.
#pragma once

#include <stdint.h>

#include "../core/sleep_audit.h"

void bootGuardPreInit();
void bootGuardReport();          // one serial line once Serial is up
uint8_t bootGuardStage();        // stage this boot runs with
bool bootGuardParked();          // hard-park: LED rail must stay off
bool bootGuardRetryConsumed();
bool bootGuardInterrupted();
// Fresh provenance only when this boot itself escalated into PROTECT. A boot
// that merely inherited stored PROTECT returns false; its durable audit record
// remains authoritative.
bool bootGuardProtectContext(ProtectAuditContext &out);

// Stage transitions during operation (power policy from P3): persists FIRST,
// then updates RAM; a failed persist reports false and the caller parks.
bool bootGuardSetStage(uint8_t stage);

// ADR 0051 load-armed marker: arm BEFORE energizing the LED rail or solenoid
// gate (false = the durable write failed; the caller must refuse the load).
// Disarm from allLoadsOff and the debounced rail-quiet path; write-on-change.
bool bootGuardLoadArm();
void bootGuardLoadDisarm(const char *why);
bool bootGuardLoadArmed();
