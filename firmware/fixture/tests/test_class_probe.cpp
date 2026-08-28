#include "test_util.h"

#include "../src/core/class_probe.h"

int main() {
  CHECK_EQ(MSA311_I2C_ADDR, 0x62u);
  CHECK_EQ(MSA311_PART_ID_REG, 0x01u);
  CHECK_EQ(MSA311_PART_ID, 0x13u);
  {
    ProbeBits b = {true, false, true, true, false, false};
    CHECK_EQ(probeBitsMask(b), 0x0Du);
  }
  {
    ProbeBits b = {true, false, true, true, true, true};
    CHECK_EQ(probeBitsMask(b), 0x3Du);
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_DOWNLIGHT);
    CHECK(!d.mismatch); // time anchors never perturb physical fixture class
  }
  // Clean decision table on first boot (no last, no override).
  {
    ProbeBits b = {true, false, false, true, false, false};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_DOWNLIGHT);
    CHECK(!d.mismatch);
    CHECK_EQ(d.persistLast, (uint8_t)FIXTURE_DOWNLIGHT);
  }
  {
    ProbeBits b = {false, true, false, true, false, false};
    CHECK_EQ(classDecide(b, 0, 0).cls, (uint8_t)FIXTURE_PERIMETER);
  }
  {
    ProbeBits b = {false, false, false, true, false, false};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(!d.mismatch);
    CHECK_EQ(d.persistLast, (uint8_t)FIXTURE_UPLIGHT);
  }
  {
    ProbeBits b = {false, false, false, false, false, false};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(!d.mismatch);
    CHECK_EQ(d.persistLast, (uint8_t)FIXTURE_UPLIGHT);
  }
  // TMF + VL53 conflict -> downlight with mismatch flag.
  {
    ProbeBits b = {true, true, false, false, false, false};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_DOWNLIGHT);
    CHECK(d.mismatch);
  }
  // Override wins outright and does not teach class_last.
  {
    ProbeBits b = {true, false, false, false, false, false};
    ClassDecision d = classDecide(b, FIXTURE_PERIMETER, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_PERIMETER);
    CHECK(d.mismatch); // probe says downlight
    CHECK_EQ(d.persistLast, 0);
  }
  // No sensors uses the installation-stage uplight fallback.
  {
    ProbeBits b = {false, false, false, false, false, false};
    ClassDecision d = classDecide(b, FIXTURE_UPLIGHT, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(!d.mismatch);
    CHECK_EQ(d.persistLast, 0);
  }
  // Future chandelier MACs are explicit overrides. Their expected sensorless
  // signature is valid and must not raise a false mismatch.
  {
    ProbeBits b = {false, false, false, false, false, false};
    ClassDecision d = classDecide(b, FIXTURE_CHANDELIER, FIXTURE_UPLIGHT);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_CHANDELIER);
    CHECK(!d.mismatch);
    CHECK_EQ(d.persistLast, 0);
  }
  {
    ProbeBits b = {false, false, false, true, false, false};
    ClassDecision d = classDecide(b, FIXTURE_CHANDELIER, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_CHANDELIER);
    CHECK(d.mismatch); // an MSA-bearing board is not a clean chandelier
  }
  // Sensor death: an MSA-only result is ambiguous when class_last remembers a
  // ToF-bearing class. Keep the remembered class and do not overwrite it.
  {
    ProbeBits b = {false, false, false, true, false, false}; // TMF died, MSA alive
    ClassDecision d = classDecide(b, 0, FIXTURE_DOWNLIGHT);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_DOWNLIGHT);
    CHECK(d.mismatch);
    CHECK_EQ(d.persistLast, 0);
  }
  {
    ProbeBits b = {false, false, false, false, false, false}; // VL53 died
    ClassDecision d = classDecide(b, 0, FIXTURE_PERIMETER);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_PERIMETER);
    CHECK(d.mismatch);
  }
  {
    ProbeBits b = {false, false, false, false, false, false}; // MSA died
    ClassDecision d = classDecide(b, 0, FIXTURE_UPLIGHT);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(!d.mismatch); // 2026 policy explicitly tolerates sensorless uplights
    CHECK_EQ(d.persistLast, 0);
  }
  // Old sensorless firmware taught chandelier. The new no-sensor policy
  // migrates that remembered value to uplight on the next boot.
  {
    ProbeBits b = {false, false, false, false, false, false};
    ClassDecision d = classDecide(b, 0, FIXTURE_CHANDELIER);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(!d.mismatch);
    CHECK_EQ(d.persistLast, (uint8_t)FIXTURE_UPLIGHT);
  }
  // BMP581 is environmental and never determines fixture class. A lone BMP is
  // anomalous: use the installation-stage uplight fallback without persisting.
  {
    ProbeBits b = {false, false, true, false, false, false};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(d.mismatch);
    CHECK_EQ(d.persistLast, 0);
  }
  // BMP581 does not perturb a valid MSA-only uplight signature.
  {
    ProbeBits b = {false, false, true, true, false, false};
    ClassDecision d = classDecide(b, 0, 0);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_UPLIGHT);
    CHECK(!d.mismatch);
  }
  // A changed sensor complement (repurposed hardware) is accepted and learned.
  {
    ProbeBits b = {false, true, false, false, false, false}; // was downlight, now has VL53
    ClassDecision d = classDecide(b, 0, FIXTURE_DOWNLIGHT);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_PERIMETER);
    CHECK_EQ(d.persistLast, (uint8_t)FIXTURE_PERIMETER);
  }
  // Stable class re-probe does not rewrite NVS.
  {
    ProbeBits b = {true, false, false, true, false, false};
    ClassDecision d = classDecide(b, 0, FIXTURE_DOWNLIGHT);
    CHECK_EQ(d.cls, (uint8_t)FIXTURE_DOWNLIGHT);
    CHECK_EQ(d.persistLast, 0);
  }

  return testReport("test_class_probe");
}
