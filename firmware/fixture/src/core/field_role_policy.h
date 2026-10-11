// Temporary scheduled inspection policy for maximum-visibility field
// rendering. This runs before the platform LED driver's battery brightness
// cap, so it cannot bypass DIM/OFF/PROTECT.
#pragma once

#include <stdint.h>

#include "fixture_context.h"

// True when any live pixel/channel is nonzero.
bool fieldFrameVisible(const FrameBuffer &frame);

// Replaces every fixture role with static linear RGB white at 255. Perimeters
// retain only their physical center pixel; all point-source and unknown roles
// use pixel zero. modeWouldLight is retained only to avoid breaking older
// callers while this emergency posture supersedes artistic intent.
bool fieldNightRoleApply(FrameBuffer &frame, uint8_t fixtureClass,
                         bool modeWouldLight);

// Preserve an explicit direct-frame color while retaining the physical role
// safety contract. Point sources use their one commanded RGBW pixel;
// perimeters use only the physical center pixel, never a 37-pixel wash.
bool fieldDirectRoleApply(FrameBuffer &frame, uint8_t fixtureClass);
