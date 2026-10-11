#include "test_util.h"

#include "../src/core/inspection_posture.h"

int main() {
  CHECK(inspectionStaticEnabled());

  InspectionControlWindow control;
  inspectionControlInit(control);
  CHECK(!inspectionControlActive(control, 100));
  inspectionControlArm(control, 100, 600000);
  CHECK(inspectionControlActive(control, 100));
  CHECK(inspectionControlActive(control, 600099));
  CHECK(!inspectionControlActive(control, 600100));
  CHECK(!control.armed);

  // The signed deadline comparison remains correct across millis() wrap.
  inspectionControlArm(control, 0xFFFFFF00u, 1000);
  CHECK(inspectionControlActive(control, 0xFFFFFF80u));
  CHECK(inspectionControlActive(control, 0x00000200u));
  CHECK(!inspectionControlActive(control, 0x000002E8u));

  inspectionControlArm(control, 5000, 600000);
  inspectionControlClose(control);
  CHECK(!inspectionControlActive(control, 5001));

  InspectionRadioDutyState state;
  inspectionRadioDutyInit(state);
  CHECK(state.radioOn);
  CHECK(!state.active);

  CHECK_EQ(inspectionRadioDutyTick(state, true, 1000, 12000, 120000),
           INSPECTION_RADIO_KEEP);
  CHECK(state.active && state.radioOn);
  CHECK_EQ(inspectionRadioDutyTick(state, true, 12999, 12000, 120000),
           INSPECTION_RADIO_KEEP);
  CHECK_EQ(inspectionRadioDutyTick(state, true, 13000, 12000, 120000),
           INSPECTION_RADIO_PAUSE);
  CHECK(!state.radioOn);
  CHECK_EQ(inspectionRadioDutyTick(state, true, 132999, 12000, 120000),
           INSPECTION_RADIO_KEEP);
  CHECK_EQ(inspectionRadioDutyTick(state, true, 133000, 12000, 120000),
           INSPECTION_RADIO_RESUME);
  CHECK(state.radioOn);

  // Loss of trustworthy schedule restores continuous radio immediately.
  CHECK_EQ(inspectionRadioDutyTick(state, true, 145000, 12000, 120000),
           INSPECTION_RADIO_PAUSE);
  CHECK_EQ(inspectionRadioDutyTick(state, false, 145001, 12000, 120000),
           INSPECTION_RADIO_RESUME);
  CHECK(!state.active && state.radioOn);

  // A later inspection interval starts with a fresh listen window.
  CHECK_EQ(inspectionRadioDutyTick(state, true, 200000, 12000, 120000),
           INSPECTION_RADIO_KEEP);
  CHECK(state.active && state.radioOn);

  return testReport("inspection_posture");
}
