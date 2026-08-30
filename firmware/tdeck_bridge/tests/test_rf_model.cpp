#include <cassert>
#include <cstdio>
#include <cstring>

#include "core/rf_model.h"

static RfPeerObservation peer(uint32_t id, uint32_t ageMs, int8_t rssi,
                              bool rssiKnown, uint16_t pdr,
                              uint16_t windowPdr, bool roster) {
  RfPeerObservation p = {};
  p.id[0] = (uint8_t)(id >> 16);
  p.id[1] = (uint8_t)(id >> 8);
  p.id[2] = (uint8_t)id;
  p.ageMs = ageMs;
  p.rssiEwma = rssi;
  p.rssiAvailable = rssiKnown;
  p.pdrX1000 = pdr;
  p.windowPdrX1000 = windowPdr;
  p.inPhysicalRoster = roster;
  p.inProductionRoster = roster;
  return p;
}

static uint32_t idOf(const RfRankedPeer &p) {
  return ((uint32_t)p.id[0] << 16) | ((uint32_t)p.id[1] << 8) | p.id[2];
}

int main() {
  RfPeerObservation peers[] = {
      peer(0x100003, 1000, -40, true, 900, 950, true),
      peer(0x100001, 2000, -40, true, 980, 950, true),
      peer(0x100002, 2000, -40, true, 980, 950, true),
      peer(0x200001, 4999, -80, true, 800, RF_PDR_UNAVAILABLE, false),
      peer(0x100004, 5000, -95, true, 200, 100, true),
      peer(0x100005, 100, 0, false, RF_PDR_UNAVAILABLE,
           RF_PDR_UNAVAILABLE, true),
      peer(0x200002, 5001, -10, true, 1000, 1000, false),
  };
  RfReport report = {};
  rfBuildReport(peers, sizeof(peers) / sizeof(peers[0]), 5000, 8, 750,
                &report);

  assert(report.summary.live == 5);
  assert(report.summary.seen == 7);
  assert(report.summary.stale == 2);
  assert(report.summary.rosterSeen == 5);
  assert(report.summary.rosterLive == 4);
  assert(report.summary.rosterUnobserved == 3);
  assert(report.summary.foreignSeen == 2);
  assert(report.summary.foreignLive == 1);
  assert(report.summary.unrankableFresh == 1);
  assert(report.summary.coverage == RfCoverageState::PARTIAL);
  assert(report.summary.observedPermille == 750);

  assert(report.strongestCount == 3);
  assert(idOf(report.strongest[0]) == 0x100003);  // same RSSI/PDR, younger
  assert(idOf(report.strongest[1]) == 0x100001);  // exact tie, lower ID
  assert(idOf(report.strongest[2]) == 0x100002);
  assert(report.strongest[0].pdrX1000 == 950);
  assert(report.strongest[0].pdrSource == RfPdrSource::WINDOW);

  assert(report.weakestCount == 3);
  assert(idOf(report.weakest[0]) == 0x200001);
  assert(report.weakest[0].pdrX1000 == 800);
  assert(report.weakest[0].pdrSource == RfPdrSource::CUMULATIVE);
  assert(idOf(report.weakest[1]) == 0x100001);  // tied RSSI/PDR, older first
  assert(idOf(report.weakest[2]) == 0x100002);  // exact tie, lower ID first

  // A known camp/repair fixture remains part of the overall census but does
  // not satisfy the site denominator and is not mislabeled as foreign.
  RfPeerObservation camp =
      peer(0x100006, 100, -55, true, 1000, 1000, false);
  camp.inPhysicalRoster = true;
  rfBuildReport(&camp, 1, 5000, 8, 1000, &report);
  assert(report.summary.seen == 1 && report.summary.live == 1);
  assert(report.summary.rosterSeen == 0);
  assert(report.summary.rosterUnobserved == 8);
  assert(report.summary.foreignSeen == 0 && report.summary.foreignLive == 0);

  // The freshness boundary is strict and unavailable metrics stay explicit.
  RfPeerObservation unavailable =
      peer(0x300001, 0, -50, true, 1001, RF_PDR_UNAVAILABLE, true);
  rfBuildReport(&unavailable, 1, 1, 1, 1001, &report);
  assert(report.summary.live == 1);
  assert(report.summary.coverage == RfCoverageState::UNAVAILABLE);
  assert(report.summary.observedPermille == RF_PDR_UNAVAILABLE);
  assert(report.strongest[0].pdrSource == RfPdrSource::UNAVAILABLE);
  assert(report.strongest[0].pdrX1000 == RF_PDR_UNAVAILABLE);

  RfPeerObservation boundary =
      peer(0x300002, 5000, -50, true, 1000, 1000, true);
  rfBuildReport(&boundary, 1, 5000, 1, 1000, &report);
  assert(report.summary.live == 0 && report.summary.stale == 1);
  assert(report.strongestCount == 0 && report.weakestCount == 0);
  assert(report.summary.coverage == RfCoverageState::COMPLETE);

  // A stale foreign entry never reduces roster-unobserved; overfull input is
  // clamped instead of wrapping the unsigned count.
  RfPeerObservation foreign =
      peer(0x400001, 9000, -60, true, 1000, 1000, false);
  rfBuildReport(&foreign, 1, 5000, 0, 0, &report);
  assert(report.summary.rosterUnobserved == 0);
  assert(report.summary.foreignSeen == 1 && report.summary.foreignLive == 0);

  assert(rfGuardState(RfWifiState::MESH_ONLY, 11, 0) ==
         RfGuardState::MESH_ONLY);
  assert(rfGuardState(RfWifiState::CONNECTING, 11, 0) ==
         RfGuardState::CHECKING);
  assert(rfGuardState(RfWifiState::ONLINE, 11, 11) == RfGuardState::MATCH);
  assert(rfGuardState(RfWifiState::ONLINE, 11, 6) ==
         RfGuardState::INCONSISTENT);
  assert(rfGuardState(RfWifiState::GUARD_BLOCKED, 11, 6) ==
         RfGuardState::BLOCKED);
  assert(rfGuardState(RfWifiState::GUARD_BLOCKED, 11, 11) ==
         RfGuardState::INCONSISTENT);
  assert(rfGuardState(RfWifiState::UNKNOWN, 11, 0) ==
         RfGuardState::UNAVAILABLE);
  assert(rfGuardState(RfWifiState::MESH_ONLY, 0, 0) ==
         RfGuardState::UNAVAILABLE);

  std::printf("rf_model ok\n");
  return 0;
}
