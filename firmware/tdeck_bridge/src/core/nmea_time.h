// Tiny native-testable NMEA RMC UTC parser. The T-Deck HAL owns buffering;
// this module validates checksum/status/date and performs calendar conversion.
#pragma once

#include <stdint.h>

struct NmeaUtcFix {
  bool valid;
  uint32_t utcS;
  uint16_t subMs;
};

bool nmeaParseRmcUtc(const char *line, NmeaUtcFix &out);

