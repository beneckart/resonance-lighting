#pragma once

#include <stddef.h>
#include <stdint.h>

static constexpr size_t ATOM_FLEET_MAX_PEERS = 160;
static constexpr size_t ATOM_FLEET_MAX_EDGES = 8;

struct AtomFleetEdge {
  uint8_t id[3];
  int8_t rssi;
};

struct AtomFleetPeer {
  uint8_t id[3];
  bool heartbeatSeen;
  bool classKnown;
  uint8_t fixtureClass;
  uint32_t lastSeenMs;
  int8_t atomRssi;
  uint8_t edgeCount;
  AtomFleetEdge edges[ATOM_FLEET_MAX_EDGES];
};

struct AtomFleet {
  size_t count;
  AtomFleetPeer peers[ATOM_FLEET_MAX_PEERS];
};

struct AtomWaveTarget {
  uint8_t id[3];
  uint8_t layer;
  int8_t atomRssi;
};

void atomFleetInit(AtomFleet &fleet);
bool atomFleetObserveHeartbeat(AtomFleet &fleet, const uint8_t id[3],
                               int8_t rssi, uint32_t nowMs,
                               bool classKnown, uint8_t fixtureClass);
bool atomFleetObserveEdge(AtomFleet &fleet, const uint8_t reporterId[3],
                          const uint8_t neighborId[3], int8_t rssi);
size_t atomFleetFreshCount(const AtomFleet &fleet, uint32_t nowMs,
                           uint32_t freshMs, uint8_t fixtureClass);
size_t atomFleetDirectedEdgeCount(const AtomFleet &fleet);

// Build a breadth-first wave over the reported strongest-neighbor graph. A
// directed report is treated as an undirected proximity edge. Disconnected
// components start in Atom-RSSI bands, so incomplete/legacy survey data still
// produces a bounded radial fallback rather than silently dropping fixtures.
size_t atomFleetPlanWave(const AtomFleet &fleet, uint32_t nowMs,
                         uint32_t freshMs, uint8_t fixtureClass,
                         AtomWaveTarget *out, size_t outCap);
