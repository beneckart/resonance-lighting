#include "sleep_audit.h"

#include <stddef.h>
#include <string.h>

static constexpr uint32_t kSleepAuditMagic = 0x534C5031UL; // "SLP1"
static constexpr uint8_t kSleepAuditVersion = 1;

static uint8_t checksum(const SleepAuditRecord &record) {
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&record);
  uint8_t value = 0xA7;
  for (size_t i = 0; i < offsetof(SleepAuditRecord, checksum); ++i)
    value = (uint8_t)((value << 5) | (value >> 3)) ^ bytes[i];
  return value;
}

SleepAuditRecord sleepAuditMake(uint8_t cause, uint32_t durationS,
                                int16_t battMv, uint8_t profile,
                                uint8_t lifeState, uint8_t powerTier,
                                uint32_t fixtureUptimeMs,
                                const uint8_t sourceId[3],
                                uint32_t sourceSeq,
                                uint32_t sourceUptimeMs) {
  SleepAuditRecord record = {};
  record.magic = kSleepAuditMagic;
  record.version = kSleepAuditVersion;
  record.cause = cause;
  record.profile = profile;
  record.life_state = lifeState;
  record.power_tier = powerTier;
  record.batt_mv = battMv;
  record.duration_s = durationS;
  record.fixture_uptime_ms = fixtureUptimeMs;
  if (sourceId) {
    record.flags |= SLEEP_AUDIT_REMOTE_SOURCE;
    memcpy(record.source_id, sourceId, sizeof(record.source_id));
    record.source_seq = sourceSeq;
    record.source_uptime_ms = sourceUptimeMs;
  }
  record.checksum = checksum(record);
  return record;
}

bool sleepAuditValid(const SleepAuditRecord &record) {
  return record.magic == kSleepAuditMagic &&
         record.version == kSleepAuditVersion &&
         record.cause > SLEEP_CAUSE_NONE && record.cause <= SLEEP_CAUSE_SERIAL &&
         record.duration_s > 0 && record.checksum == checksum(record);
}

bool sleepCauseIsOperator(uint8_t cause) {
  return cause == SLEEP_CAUSE_RADIO_ALL ||
         cause == SLEEP_CAUSE_RADIO_TARGET ||
         cause == SLEEP_CAUSE_TRANSPORT || cause == SLEEP_CAUSE_SERIAL;
}

const char *sleepCauseName(uint8_t cause) {
  switch (cause) {
  case SLEEP_CAUSE_POWER_PROTECT: return "protect";
  case SLEEP_CAUSE_DAY_CHARGE: return "day-charge";
  case SLEEP_CAUSE_RADIO_ALL: return "radio-all";
  case SLEEP_CAUSE_RADIO_TARGET: return "radio-target";
  case SLEEP_CAUSE_TRANSPORT: return "transport";
  case SLEEP_CAUSE_SERIAL: return "serial";
  default: return "none";
  }
}

