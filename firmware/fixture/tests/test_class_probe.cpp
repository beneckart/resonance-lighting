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
    ProbeBits b = {false, false, false, true};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(!d.mismatch);
    CHECK_EQ(d.persistLast, (uint8_t)FIXTURE_UPLIGHT);
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
  // No sensors is a chandelier signature, so a different override is flagged.
  {
    ProbeBits b = {false, false, false, false};
    ClassDecision d = classDecide(b, FIXTURE_UPLIGHT, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(d.mismatch);
    CHECK_EQ(d.persistLast, 0);
  }
  // Sensor death: an MSA-only result is ambiguous when class_last remembers a
  // ToF-bearing class. Keep the remembered class and do not overwrite it.
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
  {
    ProbeBits b = {false, false, false, false}; // MSA died
    ClassDecision d = classDecide(b, 0, FIXTURE_UPLIGHT);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(d.mismatch);
    CHECK_EQ(d.persistLast, 0);
  }
  // BMP581 is environmental and never determines fixture class. A lone BMP is
  // anomalous: use the safe chandelier profile without persisting it.
  {
    ProbeBits b = {false, false, true, false};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_CHANDELIER);
    CHECK(d.mismatch);
    CHECK_EQ(d.persistLast, 0);
  }
  // BMP581 does not perturb a valid MSA-only uplight signature.
  {
    ProbeBits b = {false, false, true, true};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(!d.mismatch);
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
