#include <cassert>
#include <cstdio>
#include <cstring>

#include "core/action_audit.h"

int main() {
  ActionAuditLog log;
  actionAuditInit(log);
  assert(actionAuditValid(log));
  assert(log.count == 0);

  const uint8_t all[3] = {0, 0, 0};
  assert(actionAuditAppend(log, ACTION_AUDIT_SLEEP_ALL, 3600, 1787640000,
                           123000, 42, all, ACTION_AUDIT_UTC_VALID));
  ActionAuditRecord record;
  assert(actionAuditNewest(log, record));
  assert(record.action == ACTION_AUDIT_SLEEP_ALL);
  assert(record.value == 3600 && record.mesh_seq == 42);
  assert(record.flags == ACTION_AUDIT_UTC_VALID);
  assert(std::strcmp(actionAuditName(record.action), "sleep-all") == 0);

  const uint8_t target[3] = {0x9F, 0x26, 0x64};
  assert(actionAuditAppend(log, ACTION_AUDIT_RESET_TARGET, 1, 0, 124000, 43,
                           target, 0));
  assert(actionAuditNewest(log, record));
  assert(record.action == ACTION_AUDIT_RESET_TARGET);
  assert(record.value == 1 && record.mesh_seq == 43);
  assert(std::memcmp(record.target_id, target, sizeof(target)) == 0);
  assert(std::strcmp(actionAuditName(record.action), "reset-target") == 0);

  // The October merge must retain the deployed storage branch's action IDs.
  // Its field-tuning ID collided with the old laptop-only reset ID (7).
  static_assert(ACTION_AUDIT_FIELD_TUNING == 7, "field audit ID changed");
  static_assert(ACTION_AUDIT_STORAGE_RESET == 8, "storage reset ID changed");
  static_assert(ACTION_AUDIT_STORAGE_USB == 9, "storage USB ID changed");
  static_assert(ACTION_AUDIT_TRANSPORT == 10, "transport ID changed");
  static_assert(ACTION_AUDIT_RESET_TARGET == 11, "reset ID must be appended");
  const char *mergedNames[] = {
      "field-tuning", "storage-reset", "storage-usb", "transport", "reset-target"};
  for (uint8_t action = 7; action <= 11; ++action) {
    assert(actionAuditAppend(log, action, 1, 0, 125000, 44, target, 0));
    assert(actionAuditNewest(log, record));
    assert(record.action == action);
    assert(std::strcmp(actionAuditName(record.action), mergedNames[action - 7]) == 0);
  }
  assert(!actionAuditAppend(log, 12, 1, 0, 125000, 44, target, 0));

  // Additional writes overwrite the oldest entries in the four-entry ring.
  for (uint8_t i = 0; i < 4; ++i)
    assert(actionAuditAppend(log, ACTION_AUDIT_FORCE_DAY + (i % 3), i,
                             0, 200000 + i, 50 + i, all, 0));
  assert(log.count == ACTION_AUDIT_CAPACITY);
  assert(actionAuditNewestOffset(log, 0, record));
  assert(record.mesh_seq == 53);
  assert(actionAuditNewestOffset(log, 3, record));
  assert(record.mesh_seq == 50);
  assert(!actionAuditNewestOffset(log, 4, record));

  ActionAuditLog corrupt = log;
  corrupt.records[0].value++;
  assert(!actionAuditValid(corrupt));
  assert(!actionAuditAppend(corrupt, ACTION_AUDIT_DARK_ALL, 600, 0, 1, 1,
                            all, 0));

  std::puts("action audit ok");
  return 0;
}
