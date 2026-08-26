#pragma once

#include <stdint.h>

// Why the fixture deliberately entered timer deep sleep. These values ride in
// the append-only heartbeat tail, so append new reasons; never renumber them.
enum SleepCause : uint8_t {
  SLEEP_CAUSE_NONE = 0,
  SLEEP_CAUSE_POWER_PROTECT = 1,
  SLEEP_CAUSE_DAY_CHARGE = 2,
  SLEEP_CAUSE_RADIO_ALL = 3,
  SLEEP_CAUSE_RADIO_TARGET = 4,
  SLEEP_CAUSE_TRANSPORT = 5,
  SLEEP_CAUSE_SERIAL = 6,
};

enum SleepAuditFlags : uint8_t {
  SLEEP_AUDIT_REMOTE_SOURCE = 0x01,
};

// One self-validating record is used both in RTC slow memory and NVS. RTC
// preserves the immediately preceding sleep across a timer wake without flash
// wear. NVS is reserved for rare operator commands and first PROTECT entry.
struct __attribute__((packed)) SleepAuditRecord {
  uint32_t magic;
  uint8_t version;
  uint8_t cause;
  uint8_t flags;
  uint8_t profile;
  uint8_t life_state;
  uint8_t power_tier;
  uint8_t source_id[3];
  int16_t batt_mv;
  uint32_t duration_s;
  uint32_t source_seq;
  uint32_t source_uptime_ms;
  uint32_t fixture_uptime_ms;
  uint8_t checksum;
};

static_assert(sizeof(SleepAuditRecord) == 32,
              "sleep audit NVS/RTC record layout drifted");

SleepAuditRecord sleepAuditMake(uint8_t cause, uint32_t durationS,
                                int16_t battMv, uint8_t profile,
                                uint8_t lifeState, uint8_t powerTier,
                                uint32_t fixtureUptimeMs,
                                const uint8_t sourceId[3] = nullptr,
                                uint32_t sourceSeq = 0,
                                uint32_t sourceUptimeMs = 0);
bool sleepAuditValid(const SleepAuditRecord &record);
bool sleepCauseIsOperator(uint8_t cause);
const char *sleepCauseName(uint8_t cause);

