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

  // The fifth write overwrites the oldest of a four-entry ring.
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
