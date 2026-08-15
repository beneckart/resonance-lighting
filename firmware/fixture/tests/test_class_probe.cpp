#include "test_util.h"

#include "../src/core/class_probe.h"

int main() {
  // Clean decision table on first boot (no last, no override).
  {
    ProbeBits b = {true, false, false, true};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_DOWNLIGHT);
    CHECK(!d.mismatch);
    CHECK_EQ(d.persistLast, (uint8_t)FIXTURE_DOWNLIGHT);
  }
  {
    ProbeBits b = {false, true, false, true};
    CHECK_EQ(classDecide(b, 0, 0).cls, (uint8_t)FIXTURE_PERIMETER);
  }
  {
    ProbeBits b = {false, false, true, false};
    CHECK_EQ(classDecide(b, 0, 0).cls, (uint8_t)FIXTURE_UPLIGHT);
  }
  {
    ProbeBits b = {false, false, false, false};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_CHANDELIER);
    CHECK_EQ(d.persistLast, (uint8_t)FIXTURE_CHANDELIER); // true chandelier learns
  }
  // TMF + VL53 conflict -> downlight with mismatch flag.
  {
    ProbeBits b = {true, true, false, false};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_DOWNLIGHT);
    CHECK(d.mismatch);
  }
  // Override wins outright and does not teach class_last.
  {
    ProbeBits b = {true, false, false, false};
    ClassDecision d = classDecide(b, FIXTURE_PERIMETER, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_PERIMETER);
    CHECK(d.mismatch); // probe says downlight
    CHECK_EQ(d.persistLast, 0);
  }
  // Override matching a probe of "nothing" is not a mismatch (chandelier rigs).
  {
    ProbeBits b = {false, false, false, false};
    ClassDecision d = classDecide(b, FIXTURE_UPLIGHT, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(!d.mismatch);
  }
  // Sensor death: probe collapses to chandelier but class_last remembers a
  // sensored class -> keep the remembered class, flag, do NOT overwrite last.
  {
    ProbeBits b = {false, false, false, true}; // TMF died, MSA alive
    ClassDecision d = classDecide(b, 0, FIXTURE_DOWNLIGHT);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_DOWNLIGHT);
    CHECK(d.mismatch);
    CHECK_EQ(d.persistLast, 0);
  }
  {
    ProbeBits b = {false, false, false, false}; // VL53 died
    ClassDecision d = classDecide(b, 0, FIXTURE_PERIMETER);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_PERIMETER);
    CHECK(d.mismatch);
  }
  // A changed sensor complement (repurposed hardware) is accepted and learned.
  {
    ProbeBits b = {false, true, false, false}; // was downlight, now has VL53
    ClassDecision d = classDecide(b, 0, FIXTURE_DOWNLIGHT);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_PERIMETER);
    CHECK_EQ(d.persistLast, (uint8_t)FIXTURE_PERIMETER);
  }
  // Stable class re-probe does not rewrite NVS.
  {
    ProbeBits b = {true, false, false, true};
    ClassDecision d = classDecide(b, 0, FIXTURE_DOWNLIGHT);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_DOWNLIGHT);
    CHECK_EQ(d.persistLast, 0);
  }

  return testReport("test_class_probe");
}
