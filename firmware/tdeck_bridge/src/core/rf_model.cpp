#include "rf_model.h"

#include <string.h>

static int compareId(const uint8_t a[3], const uint8_t b[3]) {
  for (size_t i = 0; i < 3; ++i) {
    if (a[i] < b[i]) return -1;
    if (a[i] > b[i]) return 1;
  }
  return 0;
}

static RfRankedPeer rankedPeer(const RfPeerObservation &peer) {
  RfRankedPeer out = {};
  memcpy(out.id, peer.id, sizeof(out.id));
  out.ageMs = peer.ageMs;
  out.rssi = peer.rssiEwma;
  if (peer.windowPdrX1000 <= 1000) {
    out.pdrX1000 = peer.windowPdrX1000;
    out.pdrSource = RfPdrSource::WINDOW;
  } else if (peer.pdrX1000 <= 1000) {
    out.pdrX1000 = peer.pdrX1000;
    out.pdrSource = RfPdrSource::CUMULATIVE;
  } else {
    out.pdrX1000 = RF_PDR_UNAVAILABLE;
    out.pdrSource = RfPdrSource::UNAVAILABLE;
  }
  return out;
}

static bool pdrBefore(const RfRankedPeer &a, const RfRankedPeer &b,
                      bool strongest) {
  bool aKnown = a.pdrSource != RfPdrSource::UNAVAILABLE;
  bool bKnown = b.pdrSource != RfPdrSource::UNAVAILABLE;
  if (aKnown != bKnown) return aKnown;
  if (aKnown && a.pdrX1000 != b.pdrX1000)
    return strongest ? a.pdrX1000 > b.pdrX1000
                     : a.pdrX1000 < b.pdrX1000;
  if (a.ageMs != b.ageMs)
    return strongest ? a.ageMs < b.ageMs : a.ageMs > b.ageMs;
  return compareId(a.id, b.id) < 0;
}

static bool rankBefore(const RfRankedPeer &a, const RfRankedPeer &b,
                       bool strongest) {
  if (a.rssi != b.rssi)
    return strongest ? a.rssi > b.rssi : a.rssi < b.rssi;
  return pdrBefore(a, b, strongest);
}

static void insertRanked(RfRankedPeer *rows, size_t *count,
                         const RfRankedPeer &peer, bool strongest) {
  size_t n = *count;
  size_t position = 0;
  while (position < n && !rankBefore(peer, rows[position], strongest))
    ++position;
  if (position >= RF_RANK_LIMIT) return;
  if (n < RF_RANK_LIMIT) ++n;
  for (size_t i = n - 1; i > position; --i) rows[i] = rows[i - 1];
  rows[position] = peer;
  *count = n;
}

void rfBuildReport(const RfPeerObservation *peers, size_t peerCount,
                   uint32_t freshMs, size_t productionRosterCount,
                   uint16_t observedPermille, RfReport *out) {
  if (!out) return;
  *out = {};
  if (observedPermille <= 1000) {
    out->summary.observedPermille = observedPermille;
    out->summary.coverage = observedPermille == 1000
                                ? RfCoverageState::COMPLETE
                                : RfCoverageState::PARTIAL;
  } else {
    out->summary.observedPermille = RF_PDR_UNAVAILABLE;
    out->summary.coverage = RfCoverageState::UNAVAILABLE;
  }

  for (size_t i = 0; peers && i < peerCount; ++i) {
    const RfPeerObservation &peer = peers[i];
    bool fresh = peer.ageMs < freshMs;
    ++out->summary.seen;
    if (fresh) ++out->summary.live;
    else ++out->summary.stale;

    if (peer.inProductionRoster) {
      ++out->summary.rosterSeen;
      if (fresh) ++out->summary.rosterLive;
    } else {
      ++out->summary.foreignSeen;
      if (fresh) ++out->summary.foreignLive;
    }

    if (!fresh) continue;
    if (!peer.rssiAvailable) {
      ++out->summary.unrankableFresh;
      continue;
    }
    RfRankedPeer ranked = rankedPeer(peer);
    insertRanked(out->strongest, &out->strongestCount, ranked, true);
    insertRanked(out->weakest, &out->weakestCount, ranked, false);
  }

  size_t seen = out->summary.rosterSeen;
  size_t unobserved = productionRosterCount > seen
                          ? productionRosterCount - seen
                          : 0;
  out->summary.rosterUnobserved =
      unobserved > UINT16_MAX ? UINT16_MAX : (uint16_t)unobserved;
}

static bool validChannel(uint8_t channel) {
  return channel >= 1 && channel <= 13;
}

RfGuardState rfGuardState(RfWifiState wifi, uint8_t meshChannel,
                          uint8_t apChannel) {
  bool meshKnown = validChannel(meshChannel);
  bool apKnown = validChannel(apChannel);
  switch (wifi) {
    case RfWifiState::MESH_ONLY:
      return meshKnown ? RfGuardState::MESH_ONLY : RfGuardState::UNAVAILABLE;
    case RfWifiState::CONNECTING:
      return meshKnown ? RfGuardState::CHECKING : RfGuardState::UNAVAILABLE;
    case RfWifiState::ONLINE:
      if (!meshKnown || !apKnown) return RfGuardState::INCONSISTENT;
      return meshChannel == apChannel ? RfGuardState::MATCH
                                      : RfGuardState::INCONSISTENT;
    case RfWifiState::GUARD_BLOCKED:
      if (!meshKnown || !apKnown || meshChannel == apChannel)
        return RfGuardState::INCONSISTENT;
      return RfGuardState::BLOCKED;
    case RfWifiState::UNKNOWN:
      return RfGuardState::UNAVAILABLE;
  }
  return RfGuardState::UNAVAILABLE;
}
