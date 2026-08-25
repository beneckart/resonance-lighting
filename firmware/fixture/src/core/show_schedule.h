// Site/date astronomical schedule. Time arrives separately through the sparse
// UTC consensus; this module has no timezone, RTC, GPS, or Arduino dependency.
#pragma once

#include <stdint.h>

struct ShowScheduleResult {
  bool night;
  float solarElevationDeg;
};

// Burning Man / Black Rock City 2026 installation site. Civil twilight
// (-6 degrees solar elevation) is the default dusk/dawn boundary.
ShowScheduleResult showScheduleAt(uint32_t utcS);

