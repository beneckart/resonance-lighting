// Local sensor modulation applied after a choreography program renders. This
// keeps every visible program eligible for interaction without multiplying
// sensor logic across programs or changing the mesh wire contract.
#pragma once

#include <stdint.h>

#include "fixture_context.h"

#define RES_TOF_INTERACTION_NEAR_MM 150u
#define RES_TOF_INTERACTION_CLOSE_MM 380u
#define RES_TOF_INTERACTION_MID_MM 700u
#define RES_TOF_INTERACTION_FAR_RING_MM 1100u
#define RES_TOF_INTERACTION_MAX_MM 1800u
#define RES_MSA_INTERACTION_TRIGGER_MG 120u

struct LocalInteractionInputs {
  uint8_t fixtureClass;
  uint32_t nowMs;
  bool tofValid;
  uint16_t tofDistanceMm;
  // Downlights use the learned per-zone canopy presence gate, not perimeter's
  // absolute chest-height range mapping.
  bool tofPresenceActive;
  bool msaValid;
  uint16_t msaSwayMg;
  // MSA thresholds need one field trace before fleet enablement. Keeping the
  // switch explicit makes the plumbing canaryable without silently enabling
  // wind-triggered accents across the tree.
  bool msaInteractionEnabled;
};

// Returns true when it changed a visible frame. An all-zero frame remains zero,
// so blackout and program-suppressed-light semantics cannot be bypassed.
bool interactionApply(FrameBuffer &frame, const LocalInteractionInputs &inputs);
