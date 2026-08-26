#include "test_util.h"

#include <cstring>
#include "../src/core/sleep_audit.h"

int main() {
  const uint8_t source[3] = {0x9F, 0x0E, 0x7C};
  SleepAuditRecord record = sleepAuditMake(
      SLEEP_CAUSE_RADIO_ALL, 3600, 3175, 1, 3, 0, 123456, source, 42, 98765);
  CHECK(sleepAuditValid(record));
  CHECK_EQ(record.flags, (uint8_t)SLEEP_AUDIT_REMOTE_SOURCE);
  CHECK_EQ(record.duration_s, 3600u);
  CHECK_EQ(record.source_seq, 42u);
  CHECK(std::memcmp(record.source_id, source, 3) == 0);
  CHECK(sleepCauseIsOperator(record.cause));
  CHECK(!sleepCauseIsOperator(SLEEP_CAUSE_POWER_PROTECT));
  CHECK(std::strcmp(sleepCauseName(SLEEP_CAUSE_DAY_CHARGE), "day-charge") == 0);

  SleepAuditRecord corrupt = record;
  corrupt.duration_s++;
  CHECK(!sleepAuditValid(corrupt));
  corrupt = record;
  corrupt.cause = 99;
  CHECK(!sleepAuditValid(corrupt));

  SleepAuditRecord local = sleepAuditMake(
      SLEEP_CAUSE_POWER_PROTECT, 900, 3040, 1, 3, 3, 30000);
  CHECK(sleepAuditValid(local));
  CHECK_EQ(local.flags, 0u);
  CHECK_EQ(local.source_seq, 0u);

  return testReport("test_sleep_audit");
}
