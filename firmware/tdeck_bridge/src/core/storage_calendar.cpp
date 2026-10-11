#include "storage_calendar.h"

static int monthDays(int y, int m) {
  const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (m < 1 || m > 12) return 0;
  return days[m-1] + (m == 2 && y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
}
static int64_t civilSeconds(const StorageDate &d) {
  int64_t days = 0;
  for (int y = 1970; y < d.year; ++y) days += 365 + (monthDays(y, 2) == 29);
  for (int m = 1; m < d.month; ++m) days += monthDays(d.year, m);
  return (days + d.day - 1) * 86400 + d.hour * 3600 + d.minute * 60;
}
static StorageDate civilAt(int64_t seconds) {
  StorageDate d = {1970, 1, 1, 0, 0, false};
  int64_t days = seconds / 86400;
  d.hour = (int)(seconds % 86400 / 3600);
  d.minute = (int)(seconds % 3600 / 60);
  while (days >= 365 + (monthDays(d.year, 2) == 29))
    days -= 365 + (monthDays(d.year++, 2) == 29);
  while (days >= monthDays(d.year, d.month)) days -= monthDays(d.year, d.month++);
  d.day = (int)days + 1;
  return d;
}
static int sunday(int year, int month, int occurrence) {
  StorageDate first = {year, month, 1, 0, 0, false};
  int weekday = (int)((civilSeconds(first) / 86400 + 4) % 7);
  return 1 + (7 - weekday) % 7 + 7 * (occurrence - 1);
}
StorageDate storagePacificAt(uint32_t utcS) {
  StorageDate utc = civilAt(utcS);
  StorageDate spring = {utc.year, 3, sunday(utc.year, 3, 2), 10, 0, false};
  StorageDate fall = {utc.year, 11, sunday(utc.year, 11, 1), 9, 0, false};
  bool daylight = utcS >= civilSeconds(spring) && utcS < civilSeconds(fall);
  StorageDate local = civilAt((int64_t)utcS - (daylight ? 7 : 8) * 3600);
  local.daylight = daylight;
  return local;
}
StorageDate storagePacificDay(uint32_t nowUtcS, unsigned daysAhead) {
  StorageDate d = storagePacificAt(nowUtcS);
  d.hour = 12;
  d.minute = 0;
  return civilAt(civilSeconds(d) + (int64_t)daysAhead * 86400);
}
bool storagePacificToUtc(const StorageDate &local, uint32_t &utcS) {
  if (local.year < 2025 || local.year > 2035 || local.month < 1 || local.month > 12 ||
      local.day < 1 || local.day > monthDays(local.year, local.month) ||
      local.hour < 0 || local.hour > 23 || local.minute < 0 || local.minute > 59)
    return false;
  int matches = 0;
  for (int offset = 7; offset <= 8; ++offset) {
    uint32_t candidate = (uint32_t)(civilSeconds(local) + offset * 3600);
    StorageDate d = storagePacificAt(candidate);
    if (d.year == local.year && d.month == local.month && d.day == local.day &&
        d.hour == local.hour && d.minute == local.minute) {
      utcS = candidate;
      ++matches;
    }
  }
  return matches == 1;
}
