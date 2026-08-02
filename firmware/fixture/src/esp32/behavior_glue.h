// Ties lifecycle + choreo runtime + neighbor table into the loop, and receives
// the fixture-era packets from net_peer.
#pragma once

#include <stdint.h>
#include "../core/fixture_context.h"
#include "../core/packet.h"

void behaviorInit(uint8_t fixtureClass, uint16_t pixelCount, uint32_t seed);
void behaviorTick(); // lifecycle + program tick + choreo tx + day-sleep

// Frame source for renderTick: true when the show frame in `f` should render
// (NIGHT_SHOW with power headroom). Identify/smoke still take precedence.
bool behaviorFrame(FrameBuffer &f);

// Rx hooks (called by net_peer from loop context).
void behaviorOnChoreoState(const uint8_t srcId[3], int8_t rssi, const NbChoreoState &cs);
void behaviorOnPeerHeartbeat(const uint8_t srcId[3], int8_t rssi, uint8_t caState,
                             uint8_t seenLen);
void behaviorOnProgramSet(const NbProgramSet &ps);
void behaviorOnNeighborSet(const NbNeighborSet &ns);
// NB_DIRECT_FRAME entry naming us (net_peer already scanned for our id).
void behaviorOnDirectFrame(uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                           uint8_t flags);

// Serial 'N' force-night override: -1 auto, 0 day, 1 night.
void behaviorForceNight(int8_t force);
int8_t behaviorForcedNight();
uint8_t behaviorLifeState();
bool behaviorStrikesAllowed();

// The radio-strike gate: production = DAY_ACTIVE + solar surplus + FULL tier;
// dev profile relaxes the surplus requirement (bench boards on USB idle) but
// never the night gate or the power veto.
bool behaviorStrikePermitted();
