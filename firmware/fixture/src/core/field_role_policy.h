// Final scheduled-night role policy for minimum-viability field rendering.
// This runs after programs and local interaction, but before the platform LED
// driver's battery brightness cap. It therefore cannot bypass DIM/OFF/PROTECT.
#pragma once

#include <stdint.h>

#include "fixture_context.h"

// True when any live pixel/channel is nonzero.
bool fieldFrameVisible(const FrameBuffer &frame);

// Applies the scheduled-night field override for roles with a visibility
// floor. modeWouldLight is the pre-override artistic intent, including a
// program's explicit suppress-light result. Returns true when this class owns
// a field override and frame was replaced.
bool fieldNightRoleApply(FrameBuffer &frame, uint8_t fixtureClass,
                         bool modeWouldLight);
