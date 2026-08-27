#pragma once

#include <stddef.h>
#include <stdint.h>

// Durable operator actions that can materially alter fleet availability.
// Values are emitted in nb-master, so append new actions; never renumber.
enum ActionAuditKind : uint8_t {
  ACTION_AUDIT_NONE = 0,
  ACTION_AUDIT_SLEEP_ALL = 1,
  ACTION_AUDIT_DARK_ALL = 2,
  ACTION_AUDIT_RELEASE_ALL = 3,
  ACTION_AUDIT_FORCE_DAY = 4,
  ACTION_AUDIT_FORCE_NIGHT = 5,
  ACTION_AUDIT_FORCE_AUTO = 6,
};

enum ActionAuditFlags : uint8_t {
  ACTION_AUDIT_UTC_VALID = 0x01,
};

struct __attribute__((packed)) ActionAuditRecord {
  uint8_t action;
  uint8_t flags;
  uint16_t reserved;
  uint32_t value;
  uint32_t utc_s;
  uint32_t bridge_uptime_ms;
  uint32_t mesh_seq;
  uint8_t target_id[3];
  uint8_t reserved_tail;
};

static constexpr size_t ACTION_AUDIT_CAPACITY = 4;

struct __attribute__((packed)) ActionAuditLog {
  uint32_t magic;
  uint8_t version;
  uint8_t count;
  uint8_t next;
  uint8_t reserved;
  ActionAuditRecord records[ACTION_AUDIT_CAPACITY];
  uint32_t checksum;
};

static_assert(sizeof(ActionAuditRecord) == 24,
              "action audit record layout drifted");
static_assert(sizeof(ActionAuditLog) == 108,
              "action audit NVS blob layout drifted");

void actionAuditInit(ActionAuditLog &log);
bool actionAuditValid(const ActionAuditLog &log);
bool actionAuditAppend(ActionAuditLog &log, uint8_t action, uint32_t value,
                       uint32_t utcS, uint32_t bridgeUptimeMs,
                       uint32_t meshSeq, const uint8_t targetId[3],
                       uint8_t flags);
bool actionAuditNewest(const ActionAuditLog &log, ActionAuditRecord &out);
bool actionAuditNewestOffset(const ActionAuditLog &log, size_t offset,
                             ActionAuditRecord &out);
const char *actionAuditName(uint8_t action);
