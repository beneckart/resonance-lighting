#include "show_schedule.h"

#include <math.h>

static double wrap360(double v) {
  v = fmod(v, 360.0);
  return v < 0.0 ? v + 360.0 : v;
}

struct SolarPosition {
  double elevationDeg;
  double hourAngleDeg;
};

static SolarPosition solarPositionAt(uint32_t utcS) {
  // Compact solar-position calculation referenced to J2000. Its sub-minute
  // accuracy is far tighter than the build-week scheduling requirement.
  const double latitudeDeg = 40.7864;
  const double longitudeDeg = -119.2065; // east-positive convention
  const double rad = 3.14159265358979323846 / 180.0;
  double jd = 2440587.5 + (double)utcS / 86400.0;
  double n = jd - 2451545.0;
  double meanLong = wrap360(280.460 + 0.9856474 * n);
  double meanAnom = wrap360(357.528 + 0.9856003 * n);
  double ecliptic = wrap360(meanLong + 1.915 * sin(meanAnom * rad) +
                            0.020 * sin(2.0 * meanAnom * rad));
  double obliquity = 23.439 - 0.0000004 * n;
  double decl = asin(sin(obliquity * rad) * sin(ecliptic * rad));
  double rightAsc = atan2(cos(obliquity * rad) * sin(ecliptic * rad),
                          cos(ecliptic * rad)) / rad;
  rightAsc = wrap360(rightAsc);
  double sidereal = wrap360(280.46061837 + 360.98564736629 * n + longitudeDeg);
  double hourAngle = sidereal - rightAsc;
  if (hourAngle > 180.0) hourAngle -= 360.0;
  if (hourAngle < -180.0) hourAngle += 360.0;
  double lat = latitudeDeg * rad;
  double elevation = asin(sin(lat) * sin(decl) +
                          cos(lat) * cos(decl) * cos(hourAngle * rad)) / rad;
  return {elevation, hourAngle};
}

ShowScheduleResult showScheduleAt(uint32_t utcS) {
  SolarPosition now = solarPositionAt(utcS);
  SolarPosition oneHourLater = solarPositionAt(utcS + 3600UL);
  ShowScheduleResult result;
  result.solarElevationDeg = (float)now.elevationDeg;
  // Advancing the entire solar calculation would also end the show one hour
  // before dawn. Only the evening/rising hour-angle side looks ahead.
  bool civilNight = now.elevationDeg <= -6.0;
  bool withinPreDuskHour = now.hourAngleDeg > 0.0 &&
                           oneHourLater.elevationDeg <= -6.0;
  result.night = civilNight || withinPreDuskHour;
  return result;
}
