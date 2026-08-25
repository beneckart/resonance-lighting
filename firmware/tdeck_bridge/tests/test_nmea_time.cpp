#include "../src/core/nmea_time.h"

#include <cstdio>

static int fails = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL line %d: %s\n", __LINE__, #x); ++fails; } } while (0)
#define CHECK_EQ(a,b) CHECK((a) == (b))

int main() {
  NmeaUtcFix fix = {};
  // Officially shaped RMC sample with checksum recomputed for this date.
  CHECK(nmeaParseRmcUtc("$GNRMC,123519.250,A,4807.038,N,01131.000,E,0.0,0.0,300826,,,A*77", fix));
  CHECK(fix.valid);
  CHECK_EQ(fix.utcS, 1788093319UL);
  CHECK_EQ(fix.subMs, 250u);
  CHECK(!nmeaParseRmcUtc("$GNRMC,123519.250,V,4807.038,N,01131.000,E,0.0,0.0,300826,,,A*60", fix));
  CHECK(!nmeaParseRmcUtc("$GNRMC,123519.250,A,4807.038,N,01131.000,E,0.0,0.0,300826,,,A*00", fix));
  CHECK(!nmeaParseRmcUtc("$GNRMC,123519.250,A,4807.038,N,01131.000,E,0.0,0.0,310226,,,A*7C", fix));
  std::printf(fails ? "FAIL test_nmea_time (%d)\n" : "PASS test_nmea_time\n", fails);
  return fails ? 1 : 0;
}
