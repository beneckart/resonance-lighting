#include "rtc_calendar.h"

static const uint32_t kMinUtc = 1735689600UL; // 2025-01-01 00:00:00 UTC
static const uint32_t kMaxUtc = 2082758400UL; // 2036-01-01 00:00:00 UTC

static int daysInMonth(int year, int month) {
  static const uint8_t kDays[] = {31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31};
  if (month == 2 && (year % 4 == 0) &&
      (year % 100 != 0 || year % 400 == 0))
    return 29;
  return month >= 1 && month <= 12 ? kDays[month - 1] : 0;
}

static int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = (unsigned)(year - era * 400);
  const unsigned doy =
      (153 * (month + (month > 2 ? (unsigned)-3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

bool rtcUtcFromCalendar(const RtcCalendar &calendar, uint32_t &utcS) {
  if (calendar.year < 2025 || calendar.year >= 2036 || calendar.month < 1 ||
      calendar.month > 12 || calendar.day < 1 ||
      calendar.day > daysInMonth(calendar.year, calendar.month) ||
      calendar.hour > 23 || calendar.minute > 59 || calendar.second > 59)
    return false;
  int64_t utc =
      daysFromCivil(calendar.year, calendar.month, calendar.day) * 86400LL +
      calendar.hour * 3600 + calendar.minute * 60 + calendar.second;
  if (utc < kMinUtc || utc >= kMaxUtc) return false;
  utcS = (uint32_t)utc;
  return true;
}

bool rtcCalendarFromUtc(uint32_t utcS, RtcCalendar &out) {
  if (utcS < kMinUtc || utcS >= kMaxUtc) return false;

  int64_t days = utcS / 86400UL;
  uint32_t daySeconds = utcS % 86400UL;
  int64_t z = days + 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = (unsigned)(z - era * 146097);
  const unsigned yoe =
      (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int year = (int)yoe + (int)era * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  const unsigned day = doy - (153 * mp + 2) / 5 + 1;
  const int month = (int)mp + (mp < 10 ? 3 : -9);
  year += month <= 2;

  out.year = (uint16_t)year;
  out.month = (uint8_t)month;
  out.day = (uint8_t)day;
  out.hour = (uint8_t)(daySeconds / 3600UL);
  out.minute = (uint8_t)((daySeconds % 3600UL) / 60UL);
  out.second = (uint8_t)(daySeconds % 60UL);
  out.weekday = (uint8_t)(((days + 4) % 7) + 1); // 1970-01-01 was Thursday
  return true;
}
