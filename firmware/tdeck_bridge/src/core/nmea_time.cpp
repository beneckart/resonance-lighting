#include "nmea_time.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  c = (char)toupper((unsigned char)c);
  return c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

static int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = (unsigned)(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? (unsigned)-3 : 9)) + 2) / 5 +
                       day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

static bool digits(const char *s, size_t n) {
  for (size_t i = 0; i < n; ++i)
    if (!isdigit((unsigned char)s[i])) return false;
  return true;
}

static int daysInMonth(int year, int month) {
  static const uint8_t kDays[] = {31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31};
  if (month == 2 && (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0))
    return 29;
  return month >= 1 && month <= 12 ? kDays[month - 1] : 0;
}

bool nmeaParseRmcUtc(const char *line, NmeaUtcFix &out) {
  out = NmeaUtcFix{};
  if (!line || line[0] != '$') return false;
  const char *star = strchr(line, '*');
  if (!star || star - line < 8 || !star[1] || !star[2]) return false;
  int hi = hexNibble(star[1]), lo = hexNibble(star[2]);
  if (hi < 0 || lo < 0) return false;
  uint8_t sum = 0;
  for (const char *p = line + 1; p < star; ++p) sum ^= (uint8_t)*p;
  if (sum != (uint8_t)((hi << 4) | lo)) return false;

  char copy[128];
  size_t payload = (size_t)(star - (line + 1));
  if (payload >= sizeof(copy)) return false;
  memcpy(copy, line + 1, payload);
  copy[payload] = 0;

  char *fields[12] = {};
  size_t count = 0;
  fields[count++] = copy;
  for (char *p = copy; *p && count < 12; ++p) {
    if (*p == ',') {
      *p = 0;
      fields[count++] = p + 1;
    }
  }
  size_t nameLen = strlen(fields[0]);
  if (count < 10 || nameLen < 3 || strcmp(fields[0] + nameLen - 3, "RMC") != 0)
    return false;
  const char *time = fields[1];
  const char *status = fields[2];
  const char *date = fields[9];
  if (status[0] != 'A' || !digits(time, 6) || !digits(date, 6)) return false;

  int hour = (time[0] - '0') * 10 + time[1] - '0';
  int minute = (time[2] - '0') * 10 + time[3] - '0';
  int second = (time[4] - '0') * 10 + time[5] - '0';
  unsigned day = (unsigned)((date[0] - '0') * 10 + date[1] - '0');
  unsigned month = (unsigned)((date[2] - '0') * 10 + date[3] - '0');
  int yy = (date[4] - '0') * 10 + date[5] - '0';
  int year = yy < 80 ? 2000 + yy : 1900 + yy;
  if (hour > 23 || minute > 59 || second > 59 || day < 1 ||
      day > (unsigned)daysInMonth(year, (int)month) || month < 1 ||
      month > 12 || year < 2025 || year >= 2036)
    return false;

  uint16_t subMs = 0;
  if (time[6] == '.') {
    int scale = 100;
    for (int i = 7; time[i] && scale > 0; ++i, scale /= 10) {
      if (!isdigit((unsigned char)time[i])) return false;
      subMs += (uint16_t)((time[i] - '0') * scale);
    }
  } else if (time[6] != 0) {
    return false;
  }

  int64_t days = daysFromCivil(year, month, day);
  int64_t utc = days * 86400 + hour * 3600 + minute * 60 + second;
  if (utc < 0 || utc > UINT32_MAX) return false;
  out.valid = true;
  out.utcS = (uint32_t)utc;
  out.subMs = subMs;
  return true;
}
