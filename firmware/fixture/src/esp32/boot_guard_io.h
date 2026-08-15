// NVS/reset-reason glue around core/boot_guard. bootGuardPreInit() runs as
// step 2 of setup(), BEFORE Serial: the consumed-retry persist must land
// before anything can energize a rail.
#pragma once

#include <stdint.h>

void bootGuardPreInit();
void bootGuardReport();          // one serial line once Serial is up
uint8_t bootGuardStage();        // stage this boot runs with
bool bootGuardParked();          // hard-park: LED rail must stay off
bool bootGuardRetryConsumed();
bool bootGuardInterrupted();

// Stage transitions during operation (power policy from P3): persists FIRST,
// then updates RAM; a failed persist reports false and the caller parks.
bool bootGuardSetStage(uint8_t stage);
