#pragma once

#include <stddef.h>
#include <stdint.h>

// Pure, read-only RF diagnostics model. Time, roster membership, and radio
// metrics are injected so ranking behavior stays native-testable.

static constexpr uint16_t RF_PDR_UNAVAILABLE = 0xFFFF;
static constexpr size_t RF_RANK_LIMIT = 3;

enum class RfPdrSource : uint8_t {
  UNAVAILABLE = 0,
  WINDOW,
  CUMULATIVE,
};

enum class RfCoverageState : uint8_t {
  UNAVAILABLE = 0,
  PARTIAL,
  COMPLETE,
};

struct RfPeerObservation {
  uint8_t id[3];
  uint32_t ageMs;
  int8_t rssiEwma;
  bool rssiAvailable;
  uint16_t pdrX1000;
  uint16_t windowPdrX1000;
  bool inPhysicalRoster;
  bool inProductionRoster;
};

struct RfRankedPeer {
  uint8_t id[3];
  uint32_t ageMs;
  int8_t rssi;
  uint16_t pdrX1000;
  RfPdrSource pdrSource;
};

struct RfFleetSummary {
  uint16_t live;
  uint16_t seen;
  uint16_t stale;
  uint16_t rosterSeen;
  uint16_t rosterLive;
  uint16_t rosterUnobserved;
  uint16_t foreignSeen;
  uint16_t foreignLive;
  uint16_t unrankableFresh;
  uint16_t observedPermille;
  RfCoverageState coverage;
};

struct RfReport {
  RfFleetSummary summary;
  RfRankedPeer strongest[RF_RANK_LIMIT];
  size_t strongestCount;
  RfRankedPeer weakest[RF_RANK_LIMIT];
  size_t weakestCount;
};

// Input IDs are expected to be unique, as guaranteed by the census. Freshness
// is strict: age == freshMs is stale. "Unobserved" means a production-roster
// ID has no retained census entry; it is not a claim that the fixture is
// powered off. The census is bounded and can evict entries if it fills.
// Known camp/repair peers count in overall census metrics but are neither site
// roster observations nor foreign devices.
// RSSI-unavailable fresh peers are counted but not ranked.
// Link ranks use RSSI first, then known PDR, then age, then ascending short ID.
// Strong ties prefer higher PDR and younger age; weak ties prefer lower PDR
// and older age. A closed 60-second PDR window wins over cumulative PDR.
void rfBuildReport(const RfPeerObservation *peers, size_t peerCount,
                   uint32_t freshMs, size_t productionRosterCount,
                   uint16_t observedPermille, RfReport *out);

enum class RfWifiState : uint8_t {
  UNKNOWN = 0,
  MESH_ONLY,
  CONNECTING,
  GUARD_BLOCKED,
  ONLINE,
};

enum class RfGuardState : uint8_t {
  UNAVAILABLE = 0,
  MESH_ONLY,
  CHECKING,
  MATCH,
  BLOCKED,
  INCONSISTENT,
};

// A channel value outside 1..13 is unavailable. ONLINE is healthy only when
// the AP and mesh channels are both known and equal. GUARD_BLOCKED is honest
// only when the remembered AP channel is known and differs from the mesh.
RfGuardState rfGuardState(RfWifiState wifi, uint8_t meshChannel,
                          uint8_t apChannel);
