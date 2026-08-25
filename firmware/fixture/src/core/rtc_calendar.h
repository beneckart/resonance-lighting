#pragma once

#include <stdint.h>

struct RtcCalendar {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t weekday; // DS3231 convention used here: Sunday=1 through Saturday=7
};

// The deployed schedule accepts UTC from 2025-01-01 through 2035-12-31.
bool rtcCalendarFromUtc(uint32_t utcS, RtcCalendar &out);
bool rtcUtcFromCalendar(const RtcCalendar &calendar, uint32_t &utcS);
