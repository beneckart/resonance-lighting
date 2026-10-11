#include "test_util.h"
#include <initializer_list>
#include "../src/core/packet.h"
#include "../src/core/sleep_audit.h"

int main() {
  const uint8_t me[3] = {0x12, 0x34, 0x56};
  NbStorageSleep r = {};
  r.h.ver = NB_PROTO_VER;
  r.h.type = NB_STORAGE_SLEEP;
  r.h.src_id[0] = 1;
  memcpy(r.target_id, me, 3);
  r.mode = NB_STORAGE_USB_WAKE;
  r.confirm = NB_STORAGE_CONFIRM;
  CHECK(nbStorageRequestValid(r, me));
  CHECK_EQ(sizeof(r), 21u);
  CHECK_EQ(offsetof(NbStorageSleep, confirm), 17u);
  CHECK_EQ(sizeof(NbStorageReceipt), 22u);
  r.mode = NB_STORAGE_RESET_WAKE;
  CHECK(nbStorageRequestValid(r, me));
  r.mode = 0;
  CHECK(!nbStorageRequestValid(r, me));
  r.mode = NB_STORAGE_USB_WAKE;
  r.confirm = 0;
  CHECK(!nbStorageRequestValid(r, me));
  r.confirm = NB_STORAGE_CONFIRM;
  memset(r.target_id, 0, 3);
  CHECK(!nbStorageRequestValid(r, me));
  memcpy(r.target_id, me, 3);
  r.target_id[2]++;
  CHECK(!nbStorageRequestValid(r, me));
  memcpy(r.target_id, me, 3);
  r.h.src_id[0] = 0;
  CHECK(!nbStorageRequestValid(r, me));
  for (uint8_t cause : {SLEEP_CAUSE_STORAGE_RESET, SLEEP_CAUSE_STORAGE_USB}) {
    auto record = sleepAuditMake(cause, 0, 3200, 1, 1, 3, 100, me, 42, 50);
    CHECK(sleepAuditValid(record));
    CHECK(sleepCauseIsOperator(cause));
    record.duration_s = 1;
    CHECK(!sleepAuditValid(record));
  }
  CHECK(!sleepAuditValid(sleepAuditMake(SLEEP_CAUSE_TRANSPORT, 0, 3200,
                                       1, 1, 3, 100, me, 42, 50)));
  return testReport("test_storage_request");
}
