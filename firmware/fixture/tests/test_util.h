// Minimal native test harness: no framework, just CHECK + a counter.
#pragma once

#include <cstdio>
#include <cstdlib>

static int gChecks = 0;
static int gFails = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    gChecks++;                                                                 \
    if (!(cond)) {                                                             \
      gFails++;                                                                \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);              \
    }                                                                          \
  } while (0)

#define CHECK_EQ(a, b)                                                         \
  do {                                                                         \
    gChecks++;                                                                 \
    auto va = (a);                                                             \
    auto vb = (b);                                                             \
    if (!(va == vb)) {                                                         \
      gFails++;                                                                \
      std::printf("FAIL %s:%d: %s == %s (%lld != %lld)\n", __FILE__, __LINE__, \
                  #a, #b, (long long)va, (long long)vb);                       \
    }                                                                          \
  } while (0)

static int testReport(const char *name) {
  std::printf("%s: %d checks, %d failures\n", name, gChecks, gFails);
  return gFails ? 1 : 0;
}
