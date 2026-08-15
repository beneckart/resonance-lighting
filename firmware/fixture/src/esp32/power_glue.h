// Wires core/power_policy into the running system: 1 Hz samples from
// board_power, stage persistence BEFORE load changes (the POR-loop rule),
// budget publication for render/behavior, PROTECT deep-sleep execution.
#pragma once

#include "../core/power_policy.h"

void powerGlueInit();  // seeds the tier from the boot-guard stage
void powerGlueTick();  // call from loop (COMMS mode); internally 1 Hz
const PowerBudget &powerBudget();
