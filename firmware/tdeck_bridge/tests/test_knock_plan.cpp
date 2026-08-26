#include <stdio.h>
#include <string.h>

#include "core/knock_plan.h"

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                    \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static void setRow(CensusView &row, unsigned id, uint32_t ageMs) {
  memset(&row, 0, sizeof(row));
  row.id[0] = (uint8_t)(id >> 16);
  row.id[1] = (uint8_t)(id >> 8);
  row.id[2] = (uint8_t)id;
  row.ageMs = ageMs;
}

int main() {
  CensusView rows[132];
  for (size_t i = 0; i < 130; ++i)
    setRow(rows[i], (unsigned)(0xF40000 + (129 - i)), 1000);
  setRow(rows[130], 0xF4FFFF, 5000);  // boundary is stale
  setRow(rows[131], 0x000000, 0);     // broadcast ID is never a fixture

  uint8_t ids[192][3] = {};
  size_t n = knockPlanFresh(rows, 132, 5000, ids, 192);
  CHECK(n == 130);
  CHECK(ids[0][0] == 0xF4 && ids[0][1] == 0x00 && ids[0][2] == 0x00);
  CHECK(ids[129][0] == 0xF4 && ids[129][1] == 0x00 && ids[129][2] == 129);
  for (size_t i = 1; i < n; ++i) CHECK(memcmp(ids[i - 1], ids[i], 3) < 0);

  uint8_t capped[32][3] = {};
  CHECK(knockPlanFresh(rows, 132, 5000, capped, 32) == 32);
  CHECK(targetPlanFresh(rows, 132, 5000, capped, 32) == 32);

  printf("knock_plan ok\n");
  return 0;
}
