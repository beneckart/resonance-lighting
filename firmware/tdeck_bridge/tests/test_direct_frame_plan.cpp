#include <stdio.h>
#include <string.h>

#include "core/direct_frame_plan.h"

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                    \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static void setRow(CensusView &row, unsigned id, uint32_t ageMs,
                   uint8_t fixtureClass) {
  memset(&row, 0, sizeof(row));
  row.id[0] = (uint8_t)(id >> 16);
  row.id[1] = (uint8_t)(id >> 8);
  row.id[2] = (uint8_t)id;
  row.ageMs = ageMs;
  row.fixtureClass = fixtureClass;
}

int main() {
  CensusView rows[132];
  for (size_t i = 0; i < 130; ++i)
    setRow(rows[i], (unsigned)(0xF40000 + (129 - i)), 1000,
           (uint8_t)(1 + (i % 4)));
  setRow(rows[130], 0xF4FFFF, 5000, 1);  // stale at the boundary
  setRow(rows[131], 0x000000, 0, 1);     // never address broadcast as a node

  DirectPlanEntry plan[192] = {};
  size_t n = directFramePlan(rows, 132, 5000, 0, 255, 128, 64, 32, 128,
                             true, plan, 192);
  CHECK(n == 130);
  CHECK(plan[0].id[0] == 0xF4 && plan[0].id[1] == 0 && plan[0].id[2] == 0);
  CHECK(plan[0].r == 128);
  CHECK(plan[0].g == 64);
  CHECK(plan[0].b == 32);
  CHECK(plan[0].w == 16);
  CHECK(directFrameChunkCount(n) == 8);
  for (size_t i = 0; i < 7; ++i) CHECK(directFrameChunkSize(n, i) == 18);
  CHECK(directFrameChunkSize(n, 7) == 4);
  CHECK(directFrameChunkSize(n, 8) == 0);

  size_t classOne = directFramePlan(rows, 132, 5000, 1, 10, 20, 30, 40,
                                    255, true, plan, 192);
  CHECK(classOne == 33);
  for (size_t i = 0; i < classOne; ++i) {
    CHECK(plan[i].r == 10 && plan[i].g == 20 && plan[i].b == 30 &&
          plan[i].w == 40);
  }

  size_t dark = directFramePlan(rows, 132, 5000, 0, 255, 255, 255, 255,
                                255, false, plan, 192);
  CHECK(dark == 130);
  for (size_t i = 0; i < dark; ++i)
    CHECK((plan[i].r | plan[i].g | plan[i].b | plan[i].w) == 0);

  CensusView whiteRows[4];
  setRow(whiteRows[0], 0x010001, 10, 1); // downlight: dedicated W
  setRow(whiteRows[1], 0x010002, 10, 2); // perimeter: RGB white
  setRow(whiteRows[2], 0x010003, 10, 3); // uplight: RGB white
  setRow(whiteRows[3], 0x010004, 10, 4); // chandelier: RGB white
  size_t white = directFramePlan(whiteRows, 4, 5000, 0, 0, 0, 0, 255,
                                 128, true, plan, 192);
  CHECK(white == 4);
  CHECK(plan[0].r == 0 && plan[0].g == 0 && plan[0].b == 0 &&
        plan[0].w == 128);
  for (size_t i = 1; i < white; ++i)
    CHECK(plan[i].r == 128 && plan[i].g == 128 && plan[i].b == 128 &&
          plan[i].w == 0);

  CHECK(directFrameBlinkVisible(0));
  CHECK(directFrameBlinkVisible(499));
  CHECK(!directFrameBlinkVisible(500));
  CHECK(!directFrameBlinkVisible(999));
  CHECK(directFrameBlinkVisible(1000));

  printf("direct_frame_plan ok\n");
  return 0;
}
