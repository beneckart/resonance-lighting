#include "action_audit.h"

#include <string.h>

static constexpr uint32_t kActionAuditMagic = 0x41435431UL; // "ACT1"
static constexpr uint8_t kActionAuditVersion = 1;

static uint32_t checksum(const ActionAuditLog &log) {
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&log);
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < offsetof(ActionAuditLog, checksum); ++i) {
    hash ^= bytes[i];
    hash *= 16777619UL;
  }
  return hash;
}

void actionAuditInit(ActionAuditLog &log) {
  memset(&log, 0, sizeof(log));
  log.magic = kActionAuditMagic;
  log.version = kActionAuditVersion;
  log.checksum = checksum(log);
}

bool actionAuditValid(const ActionAuditLog &log) {
  return log.magic == kActionAuditMagic &&
         log.version == kActionAuditVersion &&
         log.count <= ACTION_AUDIT_CAPACITY &&
         log.next < ACTION_AUDIT_CAPACITY && log.checksum == checksum(log);
}

bool actionAuditAppend(ActionAuditLog &log, uint8_t action, uint32_t value,
                       uint32_t utcS, uint32_t bridgeUptimeMs,
                       uint32_t meshSeq, const uint8_t targetId[3],
                       uint8_t flags) {
  if (!actionAuditValid(log) || action <= ACTION_AUDIT_NONE ||
      action > ACTION_AUDIT_FORCE_AUTO)
    return false;
  ActionAuditRecord record = {};
  record.action = action;
  record.flags = flags;
  record.value = value;
  record.utc_s = utcS;
  record.bridge_uptime_ms = bridgeUptimeMs;
  record.mesh_seq = meshSeq;
  if (targetId) memcpy(record.target_id, targetId, sizeof(record.target_id));
  log.records[log.next] = record;
  log.next = (uint8_t)((log.next + 1) % ACTION_AUDIT_CAPACITY);
  if (log.count < ACTION_AUDIT_CAPACITY) ++log.count;
  log.checksum = checksum(log);
  return true;
}

bool actionAuditNewestOffset(const ActionAuditLog &log, size_t offset,
                             ActionAuditRecord &out) {
  if (!actionAuditValid(log) || offset >= log.count) return false;
  size_t index =
      (log.next + ACTION_AUDIT_CAPACITY - 1 - offset) % ACTION_AUDIT_CAPACITY;
  out = log.records[index];
  return true;
}

bool actionAuditNewest(const ActionAuditLog &log, ActionAuditRecord &out) {
  return actionAuditNewestOffset(log, 0, out);
}

const char *actionAuditName(uint8_t action) {
  switch (action) {
  case ACTION_AUDIT_SLEEP_ALL: return "sleep-all";
  case ACTION_AUDIT_DARK_ALL: return "dark-all";
  case ACTION_AUDIT_RELEASE_ALL: return "release-all";
  case ACTION_AUDIT_FORCE_DAY: return "force-day";
  case ACTION_AUDIT_FORCE_NIGHT: return "force-night";
  case ACTION_AUDIT_FORCE_AUTO: return "force-auto";
  default: return "none";
  }
}
