#pragma once
#include "../core/boot_guard.h"
#include "../core/power_policy.h"
#include "../core/protect_retry.h"
#ifndef RES_PROTECT_RECOVERY_TARGET
#define RES_PROTECT_RECOVERY_TARGET 0UL
#endif
// ADR 0085 fleet promotion (Ben, 2026-10-17): every fixture in the image runs
// the bounded staged PROTECT recovery, not just one exact target.
#ifndef RES_PROTECT_RECOVERY_ALL
#define RES_PROTECT_RECOVERY_ALL 0
#endif
bool protectRecoveryEnabled();
void protectRecoveryBoot(BootDecision &decision, uint8_t storedStage);
void protectRecoveryInit();
void protectRecoveryNoteSensorRail(bool ok);
bool protectRecoveryStartupTimedOut();
bool protectRecoveryTick(const PowerSample &sample, PowerBudget &budget);
bool protectRecoveryOwnsOutputs();
bool protectRecoveryRender();
void protectRecoveryBeforeSleep(uint32_t seconds);
void protectRecoveryTest(uint16_t command); // exact-target, framed USB only
const ProtectRetryState &protectRecoveryState();
const ProtectRetryRecord &protectRecoveryRecord();
