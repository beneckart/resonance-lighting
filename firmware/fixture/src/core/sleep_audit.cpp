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

bool sleepAuditSetProtectContext(SleepAuditRecord &record,
                                 const ProtectAuditContext &context) {
  if (!sleepAuditValid(record) ||
      record.cause != SLEEP_CAUSE_POWER_PROTECT ||
      context.origin <= PROTECT_ORIGIN_UNKNOWN ||
      context.origin > PROTECT_ORIGIN_LEGACY)
    return false;
  // PROTECT is a local power-policy/boot-guard event, never a remote source.
  // Its source fields were zero before ADR 0068 and are safe to reuse.
  record.flags &= (uint8_t)~SLEEP_AUDIT_REMOTE_SOURCE;
  record.flags |= SLEEP_AUDIT_PROTECT_CONTEXT;
  record.source_id[0] = context.origin;
  record.source_id[1] = context.predecessor_stage;
  record.source_id[2] = context.reset_reason;
  record.source_seq = context.reset_streak;
  record.source_uptime_ms = context.load_armed ? 1U : 0U;
  record.checksum = checksum(record);
  return true;
}

bool sleepAuditGetProtectContext(const SleepAuditRecord &record,
                                 ProtectAuditContext &context) {
  context = ProtectAuditContext{};
  if (!sleepAuditValid(record) ||
      record.cause != SLEEP_CAUSE_POWER_PROTECT ||
      !(record.flags & SLEEP_AUDIT_PROTECT_CONTEXT) ||
      record.source_id[0] <= PROTECT_ORIGIN_UNKNOWN ||
      record.source_id[0] > PROTECT_ORIGIN_LEGACY)
    return false;
  context.origin = record.source_id[0];
  context.predecessor_stage = record.source_id[1];
  context.reset_reason = record.source_id[2];
  context.reset_streak = record.source_seq;
  context.load_armed = record.source_uptime_ms ? 1 : 0;
  return true;
}

const char *protectOriginName(uint8_t origin) {
  switch (origin) {
  case PROTECT_ORIGIN_LOW_VBAT: return "low-vbat";
  case PROTECT_ORIGIN_RESET_LOAD_ARMED: return "reset-load-armed";
  case PROTECT_ORIGIN_RESET_STREAK: return "reset-streak";
  case PROTECT_ORIGIN_NVS_FAILSAFE: return "nvs-failsafe";
  case PROTECT_ORIGIN_STAGE_PERSIST_FAILURE: return "stage-persist-failure";
  case PROTECT_ORIGIN_LEGACY: return "legacy-unknown";
  default: return "unknown";
  }
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
