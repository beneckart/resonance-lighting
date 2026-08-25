#include "knock_plan.h"

#include <string.h>

static bool realId(const uint8_t id[3]) {
  return id[0] != 0 || id[1] != 0 || id[2] != 0;
}

size_t knockPlanFresh(const CensusView *rows, size_t rowCount,
                      uint32_t freshMs, uint8_t out[][3], size_t outCap) {
  if (!rows || !out || outCap == 0) return 0;
  size_t n = 0;
  for (size_t i = 0; i < rowCount && n < outCap; ++i) {
    if (rows[i].ageMs >= freshMs || !realId(rows[i].id)) continue;
    memcpy(out[n++], rows[i].id, 3);
  }

  // Fleet scale is <=192, so a simple in-place insertion sort is bounded and
  // avoids dynamic allocation in both the UI and native tests.
  for (size_t i = 1; i < n; ++i) {
    uint8_t id[3];
    memcpy(id, out[i], 3);
    size_t j = i;
    while (j > 0 && memcmp(out[j - 1], id, 3) > 0) {
      memcpy(out[j], out[j - 1], 3);
      --j;
    }
    memcpy(out[j], id, 3);
  }
  return n;
}

