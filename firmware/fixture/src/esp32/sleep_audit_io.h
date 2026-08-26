#pragma once

#include <stdint.h>

#include "../core/packet.h"
#include "../core/sleep_audit.h"

// Load and validate RTC/NVS evidence. Call after nvsLoadConfig(), before the
// first heartbeat, so the boot announcement already carries provenance.
void sleepAuditInit();

// Capture the imminent sleep in RTC memory. Operator-caused sleeps are also
// persisted to NVS first; false means that required durable write failed and
// the caller must remain awake.
bool sleepAuditBeforeSleep(uint8_t cause, uint32_t durationS,
                           const NbHeader *source = nullptr);

// One durable checkpoint per transition into PROTECT, not per wake/sleep loop.
bool sleepAuditRecordProtectEntry(uint32_t durationS);

bool sleepAuditWakeRecord(SleepAuditRecord &out);
bool sleepAuditCommandRecord(SleepAuditRecord &out);
bool sleepAuditProtectRecord(SleepAuditRecord &out);
bool sleepAuditHasProtectRecord();

