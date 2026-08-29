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

// Rx hooks (called by net_peer from loop context).
void behaviorOnChoreoState(const uint8_t srcId[3], int8_t rssi, const NbChoreoState &cs);
void behaviorOnPeerHeartbeat(const uint8_t srcId[3], int8_t rssi, uint8_t caState,
                             uint8_t seenLen);
void behaviorOnProgramSet(const NbProgramSet &ps);
void behaviorOnNeighborSet(const NbNeighborSet &ns);
void behaviorOnEvent(const NbEvent &event);
void behaviorOnTimeQuality(const NbTimeQuality &time, const uint8_t srcId[3]);
// NB_DIRECT_FRAME entry naming us (net_peer already scanned for our id).
void behaviorOnDirectFrame(uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                           uint8_t flags);

// Read-only strongest/fresh view used by the explicitly bounded locate survey.
uint8_t behaviorNeighborSnapshot(NeighborView *out, uint8_t maxOut);

// Serial 'N' force-night override: -1 auto, 0 day, 1 night.
void behaviorForceNight(int8_t force);
int8_t behaviorForcedNight();
uint8_t behaviorLifeState();
bool behaviorStrikesAllowed();
uint16_t behaviorDaySleepS();
uint32_t behaviorWakeListenMs();

// RTC-retained evidence for the last scheduled ritual hour. Exact-target
// canaries expose their target/hour lock so maintenance retrieval can prove
// that the intended fixture attempted, fired, or safely refused every act.
struct DaytimeRitualAudit {
  uint32_t hourKey;
  uint8_t expectedMask;
  uint8_t attemptedMask;
  uint8_t firedMask;
  uint8_t policyRefusedMask;
  uint8_t mechanismBlockedMask;
  uint16_t lastUncertaintyMs;
  uint8_t flags;
};

enum DaytimeRitualAuditFlags : uint8_t {
  DAYTIME_RITUAL_AUDIT_WINDOW_SEEN = 0x01,
  DAYTIME_RITUAL_AUDIT_WINDOW_COMPLETE = 0x02,
};

bool behaviorDaytimeRitualCanaryBuild();
uint32_t behaviorDaytimeRitualCanaryTarget();
bool behaviorDaytimeRitualCanaryTargetMatches();
uint32_t behaviorDaytimeRitualCanaryHourKey();
DaytimeRitualAudit behaviorDaytimeRitualAudit();
// Read-only observability for exact-target motion traces. These expose the
// same learned TMF gate that owns the visible listener response.
bool behaviorTofPresenceActive();
bool behaviorTofPresenceRising();

// The autonomous-program strike gate: field = DAY_ACTIVE + solar surplus +
// FULL tier; commission relaxes the surplus requirement but never the night
// gate or power veto. Deliberate radio/operator knocks use StrikeOrigin::
// OPERATOR_CONTROL and go directly to the hard solenoid mechanism gates.
bool behaviorStrikePermitted();
