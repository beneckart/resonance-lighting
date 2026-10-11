#include "fleet_graph.h"

#include <string.h>

static bool realId(const uint8_t id[3]) {
  return id && (id[0] != 0 || id[1] != 0 || id[2] != 0);
}

static int findPeer(const AtomFleet &fleet, const uint8_t id[3]) {
  for (size_t i = 0; i < fleet.count; ++i)
    if (memcmp(fleet.peers[i].id, id, 3) == 0) return (int)i;
  return -1;
}

static int upsertPeer(AtomFleet &fleet, const uint8_t id[3]) {
  if (!realId(id)) return -1;
  int found = findPeer(fleet, id);
  if (found >= 0) return found;
  if (fleet.count >= ATOM_FLEET_MAX_PEERS) return -1;
  AtomFleetPeer &peer = fleet.peers[fleet.count];
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.id, id, 3);
  return (int)fleet.count++;
}

void atomFleetInit(AtomFleet &fleet) { memset(&fleet, 0, sizeof(fleet)); }

bool atomFleetObserveHeartbeat(AtomFleet &fleet, const uint8_t id[3],
                               int8_t rssi, uint32_t nowMs,
                               bool classKnown, uint8_t fixtureClass) {
  int index = upsertPeer(fleet, id);
  if (index < 0) return false;
  AtomFleetPeer &peer = fleet.peers[index];
  peer.heartbeatSeen = true;
  peer.lastSeenMs = nowMs;
  peer.atomRssi = rssi;
  if (classKnown) {
    peer.classKnown = true;
    peer.fixtureClass = fixtureClass;
  }
  return true;
}

bool atomFleetObserveEdge(AtomFleet &fleet, const uint8_t reporterId[3],
                          const uint8_t neighborId[3], int8_t rssi) {
  if (!realId(reporterId) || !realId(neighborId) ||
      memcmp(reporterId, neighborId, 3) == 0)
    return false;
  int reporterIndex = upsertPeer(fleet, reporterId);
  if (reporterIndex < 0 || upsertPeer(fleet, neighborId) < 0) return false;
  AtomFleetPeer &reporter = fleet.peers[reporterIndex];
  for (uint8_t i = 0; i < reporter.edgeCount; ++i) {
    if (memcmp(reporter.edges[i].id, neighborId, 3) == 0) {
      reporter.edges[i].rssi = rssi;
      return true;
    }
  }
  if (reporter.edgeCount < ATOM_FLEET_MAX_EDGES) {
    AtomFleetEdge &edge = reporter.edges[reporter.edgeCount++];
    memcpy(edge.id, neighborId, 3);
    edge.rssi = rssi;
    return true;
  }

  uint8_t weakest = 0;
  for (uint8_t i = 1; i < reporter.edgeCount; ++i)
    if (reporter.edges[i].rssi < reporter.edges[weakest].rssi) weakest = i;
  if (rssi <= reporter.edges[weakest].rssi) return true;
  memcpy(reporter.edges[weakest].id, neighborId, 3);
  reporter.edges[weakest].rssi = rssi;
  return true;
}

static bool eligible(const AtomFleetPeer &peer, uint32_t nowMs,
                     uint32_t freshMs, uint8_t fixtureClass) {
  if (!peer.heartbeatSeen || (uint32_t)(nowMs - peer.lastSeenMs) >= freshMs)
    return false;
  return fixtureClass == 0 ||
         (peer.classKnown && peer.fixtureClass == fixtureClass);
}

size_t atomFleetFreshCount(const AtomFleet &fleet, uint32_t nowMs,
                           uint32_t freshMs, uint8_t fixtureClass) {
  size_t count = 0;
  for (size_t i = 0; i < fleet.count; ++i)
    if (eligible(fleet.peers[i], nowMs, freshMs, fixtureClass)) ++count;
  return count;
}

size_t atomFleetDirectedEdgeCount(const AtomFleet &fleet) {
  size_t count = 0;
  for (size_t i = 0; i < fleet.count; ++i) count += fleet.peers[i].edgeCount;
  return count;
}

static bool hasEdgeTo(const AtomFleetPeer &peer, const uint8_t id[3]) {
  for (uint8_t i = 0; i < peer.edgeCount; ++i)
    if (memcmp(peer.edges[i].id, id, 3) == 0) return true;
  return false;
}

static bool linked(const AtomFleet &fleet, size_t a, size_t b) {
  return hasEdgeTo(fleet.peers[a], fleet.peers[b].id) ||
         hasEdgeTo(fleet.peers[b], fleet.peers[a].id);
}

static uint8_t addLayer(uint8_t a, uint8_t b) {
  unsigned sum = (unsigned)a + b;
  return (uint8_t)(sum > 63 ? 63 : sum);
}

size_t atomFleetPlanWave(const AtomFleet &fleet, uint32_t nowMs,
                         uint32_t freshMs, uint8_t fixtureClass,
                         AtomWaveTarget *out, size_t outCap) {
  if (!out || outCap == 0) return 0;
  size_t selected[ATOM_FLEET_MAX_PEERS];
  size_t n = 0;
  for (size_t i = 0; i < fleet.count && n < outCap; ++i)
    if (eligible(fleet.peers[i], nowMs, freshMs, fixtureClass))
      selected[n++] = i;
  if (n == 0) return 0;

  uint8_t layer[ATOM_FLEET_MAX_PEERS];
  bool assigned[ATOM_FLEET_MAX_PEERS];
  memset(layer, 0, sizeof(layer));
  memset(assigned, 0, sizeof(assigned));

  int8_t strongest = fleet.peers[selected[0]].atomRssi;
  for (size_t i = 1; i < n; ++i)
    if (fleet.peers[selected[i]].atomRssi > strongest)
      strongest = fleet.peers[selected[i]].atomRssi;

  size_t remaining = n;
  while (remaining) {
    size_t root = 0;
    bool haveRoot = false;
    for (size_t i = 0; i < n; ++i) {
      if (assigned[i]) continue;
      if (!haveRoot || fleet.peers[selected[i]].atomRssi >
                           fleet.peers[selected[root]].atomRssi) {
        root = i;
        haveRoot = true;
      }
    }

    int rssiDelta = (int)strongest - (int)fleet.peers[selected[root]].atomRssi;
    if (rssiDelta < 0) rssiDelta = 0;
    uint8_t baseLayer = (uint8_t)(rssiDelta / 6);
    if (baseLayer > 63) baseLayer = 63;

    size_t queue[ATOM_FLEET_MAX_PEERS];
    size_t head = 0, tail = 0;
    queue[tail++] = root;
    assigned[root] = true;
    layer[root] = baseLayer;
    --remaining;
    while (head < tail) {
      size_t current = queue[head++];
      for (size_t candidate = 0; candidate < n; ++candidate) {
        if (assigned[candidate] ||
            !linked(fleet, selected[current], selected[candidate]))
          continue;
        assigned[candidate] = true;
        layer[candidate] = addLayer(layer[current], 1);
        queue[tail++] = candidate;
        --remaining;
      }
    }
  }

  for (size_t i = 0; i < n; ++i) {
    memcpy(out[i].id, fleet.peers[selected[i]].id, 3);
    out[i].layer = layer[i];
    out[i].atomRssi = fleet.peers[selected[i]].atomRssi;
  }
  for (size_t i = 1; i < n; ++i) {
    AtomWaveTarget value = out[i];
    size_t j = i;
    while (j > 0 &&
           (out[j - 1].layer > value.layer ||
            (out[j - 1].layer == value.layer &&
             memcmp(out[j - 1].id, value.id, 3) > 0))) {
      out[j] = out[j - 1];
      --j;
    }
    out[j] = value;
  }
  return n;
}
