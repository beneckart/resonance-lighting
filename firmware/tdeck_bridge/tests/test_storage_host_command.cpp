#include "core/storage_host_command.h"
#include <assert.h>
#include <stdio.h>
#include <string>

int main() {
  StorageHostRequest r = {};
  assert(storageHostRequest("1234ABCD", "9F26C4,F2BE60", "CONFIRM-USB-WAKE", r));
  assert(r.jobId == 0x1234ABCD && r.count == 2 && r.targets[0][2] == 0xC4);
  const char *badIds[] = {"", "000000", "9F26C", "9F26C44", "9F26C4,",
      ",9F26C4", "9F26C4;F2BE60", "9F26C4,9f26c4", "GG26C4", "F40344"};
  for (const char *ids : badIds) {
    assert(!storageHostRequest("1234ABCD", ids, "CONFIRM-USB-WAKE", r));
    assert(r.jobId == 0 && r.count == 0);
  }
  assert(!storageHostRequest("00000000", "9F26C4", "CONFIRM-USB-WAKE", r));
  assert(!storageHostRequest("1234ABCDE", "9F26C4", "CONFIRM-USB-WAKE", r));
  assert(!storageHostRequest("1234ABCZ", "9F26C4", "CONFIRM-USB-WAKE", r));
  assert(!storageHostRequest("1234ABCD", "9F26C4", "CONFIRM-RESET", r));
  assert(!storageHostRequest("1234ABCD", "9F26C4", "CONFIRM-USB-WAKE extra", r));
  std::string ids;
  for (unsigned i = 1; i <= 20; ++i) {
    char id[8]; snprintf(id, sizeof(id), "%s%06X", i == 1 ? "" : ",", i);
    ids += id;
  }
  assert(storageHostRequest("abcdef12", ids.c_str(), "CONFIRM-USB-WAKE", r));
  assert(r.count == 20);
  ids += ",000015";
  assert(!storageHostRequest("1234ABCD", ids.c_str(), "CONFIRM-USB-WAKE", r));
  puts("storage host request boundaries passed");
}
