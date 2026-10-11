#include "inspection_posture.h"

void inspectionControlInit(InspectionControlWindow &state) {
  state = InspectionControlWindow{};
}

void inspectionControlArm(InspectionControlWindow &state, uint32_t nowMs,
                          uint32_t durationMs) {
  if (durationMs == 0) {
    inspectionControlInit(state);
    return;
  }
  state.armed = true;
  state.deadlineMs = nowMs + durationMs;
}

void inspectionControlClose(InspectionControlWindow &state) {
  inspectionControlInit(state);
}

bool inspectionControlActive(InspectionControlWindow &state, uint32_t nowMs) {
  if (!state.armed) return false;
  if ((int32_t)(state.deadlineMs - nowMs) > 0) return true;
  inspectionControlInit(state);
  return false;
}

void inspectionRadioDutyInit(InspectionRadioDutyState &state) {
  state = InspectionRadioDutyState{};
  state.radioOn = true;
}

InspectionRadioAction inspectionRadioDutyTick(
    InspectionRadioDutyState &state, bool eligible, uint32_t nowMs,
    uint32_t listenMs, uint32_t radioOffMs) {
  if (!eligible || listenMs == 0 || radioOffMs == 0) {
    InspectionRadioAction action =
        state.active && !state.radioOn ? INSPECTION_RADIO_RESUME
                                      : INSPECTION_RADIO_KEEP;
    inspectionRadioDutyInit(state);
    return action;
  }

  if (!state.active) {
    state.active = true;
    state.radioOn = true;
    state.phaseStartMs = nowMs;
    return INSPECTION_RADIO_KEEP;
  }

  uint32_t elapsed = nowMs - state.phaseStartMs;
  if (state.radioOn && elapsed >= listenMs) {
    state.radioOn = false;
    state.phaseStartMs = nowMs;
    return INSPECTION_RADIO_PAUSE;
  }
  if (!state.radioOn && elapsed >= radioOffMs) {
    state.radioOn = true;
    state.phaseStartMs = nowMs;
    return INSPECTION_RADIO_RESUME;
  }
  return INSPECTION_RADIO_KEEP;
}
