#include <cassert>
#include <cstdio>
#include <cstring>

#include "core/census.h"
#include "fixture/src/core/packet.h"

static RxItem makeHb(uint8_t idLo, uint32_t seq, uint32_t uptimeMs, int len,
                     int8_t rssi, uint8_t fixtureClass = 0) {
  NbHeartbeat hb = {};
  hb.h.ver = NB_PROTO_VER;
  hb.h.type = NB_HEARTBEAT;
  hb.h.src_id[0] = 0xAA;
  hb.h.src_id[1] = 0xBB;
  hb.h.src_id[2] = idLo;
  hb.h.seq = seq;
  hb.h.uptime_ms = uptimeMs;
  hb.batt_mv = 3300;
  hb.batt_ma = -125;
  hb.soc_pct = 77;
  hb.profile = 1;
  hb.life_state = 3;
  hb.active_program = 1;
  hb.fixture_class = fixtureClass;
  hb.sleep_audit_flags = 0x07;
  hb.last_sleep_cause = 3;
  hb.last_sleep_s = 3600;
  hb.last_sleep_batt_mv = 3175;
  hb.last_sleep_source_id[0] = 0x9F;
  hb.last_sleep_source_id[1] = 0x0E;
  hb.last_sleep_source_id[2] = 0x7C;
  hb.last_sleep_source_seq = 42;
  hb.last_command_sleep_cause = 3;
  hb.last_command_sleep_s = 3600;
  hb.last_command_sleep_source_seq = 42;
  hb.last_protect_batt_mv = 3045;
  hb.power_sample_flags = NB_POWER_SAMPLE_IBAT_VALID |
                          NB_POWER_SAMPLE_VBAT_VALID |
                          NB_POWER_SAMPLE_SOC_VALID |
                          NB_POWER_SAMPLE_CHARGER_VALID;
  hb.last_protect_origin = 2;
  hb.last_protect_predecessor_stage = 2;
  hb.last_protect_reset_reason = 9;
  hb.last_protect_load_armed = 1;
  hb.last_protect_reset_streak = 2;
  hb.ritual_flags = NB_RITUAL_CANARY_BUILD |
                    NB_RITUAL_CANARY_TARGET_MATCH |
                    NB_RITUAL_WINDOW_SEEN;
  hb.ritual_expected_mask = 0x03;
  hb.ritual_attempted_mask = 0x03;
  hb.ritual_fired_mask = 0x03;
  hb.ritual_last_uncertainty_ms = 125;
  hb.ritual_hour_key = 496700;
  hb.ritual_canary_hour_key = 496700;
  hb.ritual_canary_target_id[0] = 0xAA;
  hb.ritual_canary_target_id[1] = 0xBB;
  hb.ritual_canary_target_id[2] = idLo;
  hb.bq_reg16 = 1u << 5;
  hb.bq_stat1 = 1u << 3;
  hb.bq_fault0 = 0;
  snprintf(hb.fw_rev, sizeof(hb.fw_rev), "fx-260819-abcdef0-p");
  RxItem item = {};
  item.ms = 0;
  item.rssi = rssi;
  item.len = (uint8_t)len;
  memcpy(item.data, &hb, len);
  return item;
}

static const uint8_t *pid(uint8_t idLo) {
  static uint8_t id[3];
  id[0] = 0xAA;
  id[1] = 0xBB;
  id[2] = idLo;
  return id;
}

int main() {
  static PeerStat storage[8];
  Census c;
  c.init(storage, 8, 5000, 10000, 0);

  // --- basic ingest + tail gating (hb-short) ---
  assert(c.ingest(makeHb(1, 1, 1000, NB_HB_SHORT_LEN, -50), 1000));
  const PeerStat *p1 = c.byId(pid(1));
  assert(p1 && p1->recv == 1 && p1->soc == 77);
  assert(!p1->hasEnv && !p1->hasFixtureState && !p1->hasLedOutput);
  assert(p1->classLatched == 0);
  assert(p1->rssiEwma == -50);  // seeded on first packet

  // non-heartbeat and wrong-version frames are not consumed
  RxItem bad = makeHb(1, 2, 2000, NB_HB_SHORT_LEN, -50);
  bad.data[1] = NB_SHOWFRAME;
  assert(!c.ingest(bad, 2000));
  bad = makeHb(1, 2, 2000, NB_HB_SHORT_LEN, -50);
  bad.data[0] = 99;
  assert(!c.ingest(bad, 2000));

  // --- hb-full latches class; later hb-short keeps it ---
  assert(c.ingest(makeHb(1, 2, 2000, (int)NB_HB_FULL_LEN, -50, 2), 2000));
  assert(p1->hasLedOutput && p1->classLatched == 2 && p1->hasFw);
  assert(p1->hasFixtureState && p1->fixtureStateHeardMs == 2000);
  assert(p1->hasPowerSampleFlags);
  assert(p1->powerSampleFlags & NB_POWER_SAMPLE_IBAT_VALID);
  assert(p1->hasBq);
  assert(p1->hasSleepAudit && p1->sleepAuditFlags == 0x07);
  assert(p1->lastSleepCause == 3 && p1->lastSleepS == 3600);
  assert(p1->lastSleepBattMv == 3175 && p1->lastSleepSourceSeq == 42);
  assert(p1->lastProtectBattMv == 3045);
  assert(p1->hasProtectContext && p1->lastProtectOrigin == 2);
  assert(p1->lastProtectPredecessorStage == 2);
  assert(p1->lastProtectResetReason == 9 && p1->lastProtectLoadArmed == 1);
  assert(p1->lastProtectResetStreak == 2);
  assert(p1->hasRitualAudit);
  assert(p1->ritualFlags & NB_RITUAL_CANARY_TARGET_MATCH);
  assert(p1->ritualExpectedMask == 0x03 && p1->ritualFiredMask == 0x03);
  assert(p1->ritualHourKey == 496700 &&
         p1->ritualCanaryHourKey == 496700);
  assert(p1->ritualCanaryTargetId[2] == 1);
  assert(strncmp(p1->fwRev, "fx-260819", 9) == 0);
  assert(c.ingest(makeHb(1, 3, 3000, NB_HB_SHORT_LEN, -50), 3000));
  assert(!p1->hasLedOutput && p1->classLatched == 2);  // latched across short
  assert(p1->hasFixtureState && p1->activeProgram == 1);
  assert(p1->hasSleepAudit); // provenance remains visible across hb-short
  assert(p1->hasPowerSampleFlags); // sample proof also latches across hb-short
  assert(p1->hasBq); // charger status also latches across hb-short
  CensusView v[8];
  size_t n = c.snapshot(v, 8, 3000);
  assert(n == 1 && v[0].fixtureClass == 2);
  assert(v[0].hasFixtureState);
  assert(v[0].activeProgram == 1 && v[0].battMa == -125);
  assert(v[0].battMaValid && v[0].hasBq);
  assert(v[0].fwFingerprint ==
         censusFirmwareFingerprint("fx-260819-abcdef0-p"));

  // --- seq gaps + cumulative PDR ---
  assert(c.ingest(makeHb(1, 6, 4000, NB_HB_SHORT_LEN, -50), 4000));  // 4,5 lost
  assert(p1->gaps == 2 && p1->recv == 4);
  n = c.snapshot(v, 8, 4000);
  assert(v[0].pdrX1000 == 4000 / 6);  // 4/(4+2) = 666

  // --- EWMA blends toward new RSSI ---
  assert(c.ingest(makeHb(1, 7, 5000, NB_HB_SHORT_LEN, -70), 5000));
  assert(p1->rssiEwma == (-50 * 7 + -70) / 8);  // -52

  // --- reboot detection resets accounting ---
  assert(c.ingest(makeHb(1, 1, 500, NB_HB_SHORT_LEN, -50), 6000));
  assert(p1->recv == 1 && p1->gaps == 0);
  assert(!p1->hasFixtureState && !p1->hasFw);
  assert(!p1->hasPowerSampleFlags && !p1->hasBq);

  // --- seq restart (counter reset without uptime reset) ---
  assert(c.ingest(makeHb(1, 500, 7000, NB_HB_SHORT_LEN, -50), 7000));
  assert(c.ingest(makeHb(1, 2, 8000, NB_HB_SHORT_LEN, -50), 8000));
  assert(p1->recv == 1 && p1->lastSeq == 2);

  // --- windowed PDR ---
  Census w;
  static PeerStat wStorage[4];
  w.init(wStorage, 4, 5000, 10000, 0);
  w.ingest(makeHb(9, 1, 1000, NB_HB_SHORT_LEN, -40), 1000);
  w.ingest(makeHb(9, 4, 2000, NB_HB_SHORT_LEN, -40), 2000);  // 2,3 lost
  const PeerStat *p9 = w.byId(pid(9));
  assert(p9->winPdrX1000 == 0xFFFF);  // window not closed yet
  w.tickWindow(9000);                 // too early — still open
  assert(p9->winPdrX1000 == 0xFFFF);
  w.tickWindow(10000);  // closes: 2 recv, 2 gaps -> 500
  assert(p9->winPdrX1000 == 500);
  w.tickWindow(20001);  // idle window -> no data
  assert(p9->winPdrX1000 == 0xFFFF);

  // --- observation ledger ---
  Census o;
  static PeerStat oStorage[2];
  o.init(oStorage, 2, 5000, 10000, 0);
  o.noteRadioGap(2500, 5000);
  o.tickWindow(10000);
  assert(o.observedPermille() == 750);
  o.tickWindow(20000);
  assert(o.observedPermille() == 1000);

  // --- eviction: stalest-beyond-fresh first ---
  Census e;
  static PeerStat eStorage[2];
  e.init(eStorage, 2, 5000, 10000, 0);
  e.ingest(makeHb(10, 1, 1000, NB_HB_SHORT_LEN, -40), 1000);
  e.ingest(makeHb(11, 1, 2000, NB_HB_SHORT_LEN, -60), 2000);
  e.ingest(makeHb(12, 1, 20000, NB_HB_SHORT_LEN, -55), 20000);
  assert(e.byId(pid(10)) == nullptr);  // stalest evicted
  assert(e.byId(pid(11)) && e.byId(pid(12)));

  // --- eviction hysteresis when all entries are fresh ---
  Census h;
  static PeerStat hStorage[1];
  h.init(hStorage, 1, 60000, 10000, 0);
  h.ingest(makeHb(20, 1, 1000, NB_HB_SHORT_LEN, -50), 1000);
  h.ingest(makeHb(21, 1, 2000, NB_HB_SHORT_LEN, -47), 2000);  // only 3 dB up
  assert(h.byId(pid(20)) && h.byId(pid(21)) == nullptr);
  h.ingest(makeHb(22, 1, 3000, NB_HB_SHORT_LEN, -40), 3000);  // 10 dB up
  assert(h.byId(pid(20)) == nullptr && h.byId(pid(22)));

  // --- quiet list + live count ---
  Census q;
  static PeerStat qStorage[4];
  q.init(qStorage, 4, 5000, 10000, 0);
  q.ingest(makeHb(30, 1, 1000, NB_HB_SHORT_LEN, -40), 1000);
  q.ingest(makeHb(31, 1, 60000, NB_HB_SHORT_LEN, -40), 60000);
  assert(q.liveCount(61000) == 1 && q.seenCount() == 2);
  n = q.quietList(30, v, 8, 61000);
  assert(n == 1 && v[0].id[2] == 30);

  // --- rx ring ---
  static RxItem ringBuf[4];
  RxRing ring;
  ring.init(ringBuf, 4);
  RxItem it = makeHb(1, 1, 1, NB_HB_SHORT_LEN, -40);
  for (int i = 0; i < 4; ++i) assert(ring.push(it));
  assert(!ring.push(it) && ring.drops() == 1);
  RxItem outItem;
  int popped = 0;
  while (ring.pop(&outItem)) ++popped;
  assert(popped == 4 && ring.pending() == 0);

  printf("census ok\n");
  return 0;
}
