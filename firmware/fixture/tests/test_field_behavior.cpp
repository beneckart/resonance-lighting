#include "test_util.h"

#include "../src/core/field_behavior.h"

int main() {
  PresenceSeedGate gate;
  presenceSeedGateInit(gate);
  CHECK(presenceSeedGateObserve(gate, true, true, 1000, 300, 30));
  CHECK(!presenceSeedGateObserve(gate, true, true, 2000, 300, 30));
  CHECK(!presenceSeedGateObserve(gate, false, false, 280000, 300, 30));
  CHECK(!presenceSeedGateObserve(gate, true, true, 310000, 300, 30));
  CHECK(!presenceSeedGateObserve(gate, false, false, 311000, 300, 30));
  CHECK(!presenceSeedGateObserve(gate, false, false, 340999, 300, 30));
  CHECK(!presenceSeedGateObserve(gate, false, false, 341000, 300, 30));
  CHECK(presenceSeedGateObserve(gate, true, true, 341001, 300, 30));

  PerimeterCloseHold close;
  perimeterCloseHoldInit(close);
  CHECK(!perimeterCloseHoldObserve(close, 1, false, 0, true, 100));
  CHECK(perimeterCloseHoldObserve(close, 2, true, 300, false, 200));
  CHECK(perimeterCloseHoldObserve(close, 3, false, 0, true, 1000));
  CHECK(perimeterCloseHoldObserve(close, 3, false, 0, true, 2499));
  CHECK(!perimeterCloseHoldObserve(close, 3, false, 0, true, 2500));
  CHECK(perimeterCloseHoldObserve(close, 4, true, 350, false, 3000));
  CHECK(!perimeterCloseHoldObserve(close, 5, true, 600, false, 3100));

  CHECK(!fieldChanceSelected(0, 0));
  CHECK(fieldChanceSelected(32, 31));
  CHECK(!fieldChanceSelected(32, 32));
  CHECK(fieldChanceSelected(64, 63));
  CHECK(fieldChanceSelected(255, 255));

  return testReport("test_field_behavior");
}
