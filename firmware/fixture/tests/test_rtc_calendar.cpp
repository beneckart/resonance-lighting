#include "test_util.h"

#include "../src/core/rtc_calendar.h"

static void roundTrip(const RtcCalendar &expected) {
  uint32_t utc = 0;
  CHECK(rtcUtcFromCalendar(expected, utc));
  RtcCalendar actual = {};
  CHECK(rtcCalendarFromUtc(utc, actual));
  CHECK_EQ(actual.year, expected.year);
  CHECK_EQ(actual.month, expected.month);
  CHECK_EQ(actual.day, expected.day);
  CHECK_EQ(actual.hour, expected.hour);
  CHECK_EQ(actual.minute, expected.minute);
  CHECK_EQ(actual.second, expected.second);
}

int main() {
  RtcCalendar start = {2025, 1, 1, 0, 0, 0, 0};
  uint32_t utc = 0;
  CHECK(rtcUtcFromCalendar(start, utc));
  CHECK_EQ(utc, 1735689600UL);
  RtcCalendar decoded = {};
  CHECK(rtcCalendarFromUtc(utc, decoded));
  CHECK_EQ(decoded.weekday, 4u); // Wednesday with Sunday=1

  roundTrip({2026, 8, 25, 4, 20, 20, 0});
  roundTrip({2028, 2, 29, 23, 59, 59, 0});
  roundTrip({2035, 12, 31, 23, 59, 59, 0});

  CHECK(!rtcUtcFromCalendar({2026, 2, 29, 0, 0, 0, 0}, utc));
  CHECK(!rtcUtcFromCalendar({2026, 13, 1, 0, 0, 0, 0}, utc));
  CHECK(!rtcCalendarFromUtc(1735689599UL, decoded));
  CHECK(!rtcCalendarFromUtc(2082758400UL, decoded));

  return testReport("test_rtc_calendar");
}
