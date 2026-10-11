#include "core/storage_calendar.h"
#include <cassert>
#include <cstdio>
int main() {
  uint32_t utc = 0;
  StorageDate local = {2026, 9, 19, 9, 0, false};
  assert(storagePacificToUtc(local, utc));
  assert(utc == 1789833600UL);
  auto d = storagePacificAt(utc);
  assert(d.year == 2026 && d.month == 9 && d.day == 19 && d.hour == 9 && d.daylight);
  local = {2026, 3, 8, 2, 30, false};
  assert(!storagePacificToUtc(local, utc)); // spring gap
  local = {2026, 11, 1, 1, 30, false};
  assert(!storagePacificToUtc(local, utc)); // fall repeat
  local = {2026, 11, 1, 3, 0, false};
  assert(storagePacificToUtc(local, utc));
  assert(!storagePacificAt(utc).daylight);
  local = {2026, 2, 29, 9, 0, false};
  assert(!storagePacificToUtc(local, utc));
  local = {2028, 2, 29, 9, 0, false};
  assert(storagePacificToUtc(local, utc));
  local = {2026, 12, 31, 23, 30, false};
  assert(storagePacificToUtc(local, utc));
  d = storagePacificDay(utc, 1);
  assert(d.year == 2027 && d.month == 1 && d.day == 1);
  puts("storage calendar ok");
}
