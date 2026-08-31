#include <stdio.h>

#include "../src/core/strike_policy.h"

static int fails = 0;
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      printf("FAIL %d: %s\n", __LINE__, #x);                                  \
      ++fails;                                                                 \
    }                                                                          \
  } while (0)

int main() {
  CHECK(strikePolicyMayAttempt(StrikeOrigin::OPERATOR_CONTROL, true));
  CHECK(strikePolicyMayAttempt(StrikeOrigin::OPERATOR_CONTROL, false));
  CHECK(strikePolicyMayAttempt(StrikeOrigin::FIELD_RITUAL_BEST_EFFORT, false));
  CHECK(strikePolicyMayAttempt(StrikeOrigin::AUTONOMOUS_PROGRAM, true));
  CHECK(!strikePolicyMayAttempt(StrikeOrigin::AUTONOMOUS_PROGRAM, false));

  printf("strike_policy %s\n", fails ? "FAIL" : "ok");
  return fails ? 1 : 0;
}
