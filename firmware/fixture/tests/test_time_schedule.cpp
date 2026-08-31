#include "test_util.h"

#include "../src/core/show_schedule.h"
#include "../src/core/time_consensus.h"

#include <cstring>

static NbTimeQuality report(uint32_t utc, uint8_t source, uint16_t boot = 1) {
  NbTimeQuality q = {};
  q.h.ver = NB_PROTO_VER;
  q.h.type = NB_TIME_QUALITY;
  q.utc_s = utc;
  q.source = source;
  q.boot_id = boot;
  q.uncert_ms = 250;
  q.flags = NB_TIME_FLAG_VALID | NB_TIME_FLAG_DATE_VALID;
  return q;
}

int main() {
  // 2026-08-30: civil dawn around 05:52 PDT and dusk around 19:59 PDT.
  // Inspection light starts one hour before dusk but does not end early.
  CHECK(showScheduleAt(1788087600UL).night);  // 04:00 PDT, before civil dawn
  CHECK(!showScheduleAt(1788098400UL).night); // 07:00 PDT, after civil dawn
  CHECK(!showScheduleAt(1788139800UL).night); // 18:30 PDT, >1 h before dusk
  CHECK(showScheduleAt(1788143400UL).night);  // 19:30 PDT, pre-dusk hour
  CHECK(showScheduleAt(1788147000UL).night);  // 20:30 PDT, after civil dusk

  TimeConsensus tc;
  timeConsensusInit(tc);
  uint8_t gps[3] = {1, 2, 3};
  NbTimeQuality q = report(1788098400UL, NB_TIME_GPS);
  CHECK(timeConsensusObserve(tc, q, gps, 1000));
  TimeEstimate e = timeConsensusEstimate(tc, 2500);
  CHECK(e.valid);
  CHECK_EQ(e.utcS, 1788098401UL);
  CHECK_EQ(e.source, (uint8_t)NB_TIME_GPS);
  CHECK_EQ(e.votes, 1u);

  // Invalid date and a lone peer relay are not accepted.
  TimeConsensus peers;
  timeConsensusInit(peers);
  uint8_t p1[3] = {4, 5, 6};
  uint8_t p2[3] = {7, 8, 9};
  NbTimeQuality peer = report(1788098400UL, NB_TIME_PEER);
  peer.hops = 1;
  CHECK(timeConsensusObserve(peers, peer, p1, 1000));
  CHECK(!timeConsensusEstimate(peers, 1100).valid);
  peer.utc_s += 1;
  CHECK(timeConsensusObserve(peers, peer, p2, 1200));
  CHECK(timeConsensusEstimate(peers, 1300).valid);
  q.flags = 0;
  CHECK(!timeConsensusObserve(tc, q, gps, 3000));

  // A later slow report can refresh quality but cannot move wall time back.
  q = report(1788098390UL, NB_TIME_GPS, 2);
  CHECK(timeConsensusObserve(tc, q, gps, 5000));
  e = timeConsensusEstimate(tc, 5000);
  CHECK(e.utcS >= 1788098404UL);

  // A wildly mis-set direct clock cannot jump accepted wall time.
  q = report(1788099000UL, NB_TIME_GPS, 3);
  CHECK(timeConsensusObserve(tc, q, gps, 6000));
  e = timeConsensusEstimate(tc, 6000);
  CHECK(e.utcS < 1788098500UL);
  // Authority expires 30 minutes after the last accepted observation.
  CHECK(!timeConsensusEstimate(tc, 1805001UL).valid);

  return testReport("test_time_schedule");
}
