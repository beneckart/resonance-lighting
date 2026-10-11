// Ties lifecycle + choreo runtime + neighbor table into the loop, and receives
// the fixture-era packets from net_peer.
#pragma once

#include <stdint.h>
#include "../core/fixture_context.h"
#include "../core/neighbor_table.h"
#include "../core/packet.h"

void behaviorInit(uint8_t fixtureClass, uint16_t pixelCount, uint32_t seed);
void behaviorTick(); // lifecycle + program tick + choreo tx + day-sleep

// Frame source for renderTick: true when the show frame in `f` should render
// (NIGHT_SHOW with power headroom). Identify/smoke still take precedence.
bool behaviorFrame(FrameBuffer &f);
// True when the emergency inspection posture owns the field renderer. This
// lets render arbitration prevent identify/smoke blink from dimming a live
// safety light.
bool behaviorStaticInspectionActive();
// True for the production inspection image even outside its scheduled light
// interval. Show/program traffic is ignored and must not create awake holds;
// only a Wake Fleet-armed direct frame is artistic authority.
bool behaviorInspectionDirectOnly();

// Rx hooks (called by net_peer from loop context).
void behaviorOnChoreoState(const uint8_t srcId[3], int8_t rssi, const NbChoreoState &cs);
void behaviorOnPeerHeartbeat(const uint8_t srcId[3], int8_t rssi, uint8_t caState,
                             uint8_t seenLen);
// Returns true only when a program command was accepted as control authority.
bool behaviorOnProgramSet(const NbProgramSet &ps);
void behaviorOnNeighborSet(const NbNeighborSet &ns);
void behaviorOnEvent(const NbEvent &event);
void behaviorOnTimeQuality(const NbTimeQuality &time, const uint8_t srcId[3]);
// NB_DIRECT_FRAME entry naming us (net_peer already scanned for our id).
// Returns true only when the frame was accepted as operator authority. The
// inspection posture requires a fresh Wake Fleet arm before accepting it.
bool behaviorOnDirectFrame(uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                           uint8_t flags);

// Read-only strongest/fresh view used by the explicitly bounded locate survey.
uint8_t behaviorNeighborSnapshot(NeighborView *out, uint8_t maxOut);

// Serial 'N' force-night override: -1 auto, 0 day, 1 night.
// Returns true when inspection field firmware interprets DAY as the bounded
// Wake Fleet direct-control arm instead of a lifecycle override.
bool behaviorForceNight(int8_t force);
int8_t behaviorForcedNight();
uint8_t behaviorLifeState();
bool behaviorStrikesAllowed();
uint16_t behaviorDaySleepS();
uint32_t behaviorWakeListenMs();
// Read-only observability for exact-target motion traces. These expose the
// same learned TMF gate that owns the visible listener response.
bool behaviorTofPresenceActive();
bool behaviorTofPresenceRising();

// The autonomous-program strike gate: field = DAY_ACTIVE + solar surplus +
// FULL tier; commission relaxes the surplus requirement but never the night
// gate or power veto. Deliberate radio/operator knocks use StrikeOrigin::
// OPERATOR_CONTROL and go directly to the hard solenoid mechanism gates.
bool behaviorStrikePermitted();
