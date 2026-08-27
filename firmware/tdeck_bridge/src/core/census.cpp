#include "census.h"

#include <string.h>

#include "fixture/src/core/packet.h"

void Census::init(PeerStat *storage, size_t cap, uint32_t freshMs,
                  uint32_t windowMs, uint32_t nowMs) {
  mPeers = storage;
  mCap = cap;
  memset(mPeers, 0, sizeof(PeerStat) * cap);
  mFreshMs = freshMs;
  mWindowMs = windowMs;
  mWindowStartMs = nowMs;
  mUnobservedMsCur = 0;
  mObservedPermille = 1000;
}

PeerStat *Census::findPeer(const uint8_t id[3], bool create, int8_t rssi,
                           uint32_t nowMs) {
  for (size_t i = 0; i < mCap; ++i)
    if (mPeers[i].used && memcmp(mPeers[i].id, id, 3) == 0) return &mPeers[i];
  if (!create) return nullptr;
  for (size_t i = 0; i < mCap; ++i) {
    if (!mPeers[i].used) {
      memset(&mPeers[i], 0, sizeof(mPeers[i]));
      mPeers[i].used = true;
      memcpy(mPeers[i].id, id, 3);
      mPeers[i].winPdrX1000 = 0xFFFF;
      return &mPeers[i];
    }
  }
  // Full. Evict the stalest entry beyond freshness first; else the weakest
  // EWMA, but only for a newcomer >6 dB stronger (hysteresis; donor:
  // neighbor_table.cpp:37-59). The cores3 census had no eviction and wedged
  // silently at capacity — this is the Bridge OS fix.
  PeerStat *stalest = nullptr;
  for (size_t i = 0; i < mCap; ++i) {
    PeerStat *cand = &mPeers[i];
    if (nowMs - cand->lastHeardMs < mFreshMs) continue;
    if (!stalest || cand->lastHeardMs < stalest->lastHeardMs) stalest = cand;
  }
  if (!stalest) {
    PeerStat *weakest = nullptr;
    for (size_t i = 0; i < mCap; ++i) {
      PeerStat *cand = &mPeers[i];
      if (!weakest || cand->rssiEwma < weakest->rssiEwma) weakest = cand;
    }
    if (!weakest || rssi <= weakest->rssiEwma + 6) return nullptr;
    stalest = weakest;
  }
  memset(stalest, 0, sizeof(*stalest));
  stalest->used = true;
  memcpy(stalest->id, id, 3);
  stalest->winPdrX1000 = 0xFFFF;
  return stalest;
}

// Donor-faithful seq accounting (cores3_bridge.ino:739-753) with the windowed
// counters riding alongside.
void Census::accountHeartbeat(PeerStat *peer, uint32_t seq, uint32_t uptimeMs) {
  bool senderRebooted = peer->recv && uptimeMs + 1000 < peer->uptimeMs;
  bool seqRestarted =
      peer->recv && seq < peer->lastSeq && peer->lastSeq - seq > 100;
  if (!peer->recv || senderRebooted || seqRestarted) {
    peer->lastSeq = seq;
    peer->recv = 1;
    peer->gaps = 0;
    ++peer->winRecv;
    return;
  }
  if (seq > peer->lastSeq) {
    uint32_t gap = seq - peer->lastSeq - 1;
    peer->gaps += gap;
    peer->winGaps += gap;
    peer->lastSeq = seq;
  }
  ++peer->recv;
  ++peer->winRecv;
}

bool Census::ingest(const RxItem &item, uint32_t nowMs) {
  if (item.len < (int)sizeof(NbHeader)) return false;
  const NbHeader *h = (const NbHeader *)item.data;
  if (h->ver != NB_PROTO_VER || h->type != NB_HEARTBEAT) return false;
  if (item.len < (int)offsetof(NbHeartbeat, supply_mv)) return false;
  const NbHeartbeat *hb = (const NbHeartbeat *)item.data;

  PeerStat *peer = findPeer(hb->h.src_id, true, item.rssi, nowMs);
  if (!peer) return false;

  accountHeartbeat(peer, hb->h.seq, hb->h.uptime_ms);
  peer->rssi = item.rssi;
  peer->rssiEwma = peer->lastHeardMs == 0
                       ? item.rssi
                       : (int8_t)(((int)peer->rssiEwma * 7 + (int)item.rssi) / 8);
  peer->lastHeardMs = nowMs;
  peer->uptimeMs = hb->h.uptime_ms;
  peer->battMv = hb->batt_mv;
  peer->battMa = hb->batt_ma;
  peer->soc = hb->soc_pct;
  peer->resetReason = hb->reset_reason;
  peer->caState = hb->ca_state;
  peer->mode = hb->mode;
  peer->dlPdrX1000 = hb->dl_pdr_x1000;
  peer->dlRssi = hb->dl_rssi;

  const int len = item.len;
  if (NB_HAS_HB_FIELD(len, supply_good)) {
    peer->supplyMv = hb->supply_mv;
    peer->supplyMa = hb->supply_ma;
    peer->supplyGood = hb->supply_good;
  } else {
    peer->supplyMv = 0;
    peer->supplyMa = 0;
    peer->supplyGood = 0;
  }

  peer->hasEnv = NB_HAS_HB_FIELD(len, btemp_cx10);
  if (peer->hasEnv) {
    peer->luxX10 = hb->lux_x10;
    peer->lightCh0 = hb->light_ch0;
    peer->lightCh1 = hb->light_ch1;
    peer->panelTempCx10 = hb->ptemp_cx10;
    peer->panelRhPct = hb->prh_pct;
    peer->battTempCx10 = hb->btemp_cx10;
  }

  peer->hasIna = NB_HAS_HB_FIELD(len, ina_ba_ma);
  if (peer->hasIna) {
    peer->inaPvMv = hb->ina_pv_mv;
    peer->inaPaMa = hb->ina_pa_ma;
    peer->inaBvMv = hb->ina_bv_mv;
    peer->inaBaMa = hb->ina_ba_ma;
  }

  peer->hasConfig = NB_HAS_HB_FIELD(len, cfg_charge_ma);
  if (peer->hasConfig) {
    peer->capacityMah = hb->cfg_cap_mah;
    peer->chargeMa = hb->cfg_charge_ma;
  }

  peer->hasDrawdown = NB_HAS_HB_FIELD(len, drawdown_active);
  if (peer->hasDrawdown) {
    peer->drawdownMahX10 = hb->drawdown_mah_x10;
    peer->drawdownBudgetMah = hb->drawdown_budget_mah;
    peer->drawdownActive = hb->drawdown_active;
  }

  if (NB_HAS_HB_FIELD(len, fw_rev)) {
    peer->hasFw = true;  // latched, donor behavior
    memcpy(peer->fwRev, hb->fw_rev, sizeof(peer->fwRev));
    peer->fwRev[sizeof(peer->fwRev) - 1] = '\0';
  }

  peer->hasMaint = NB_HAS_HB_FIELD(len, maint_status);
  if (peer->hasMaint) peer->maintStatus = hb->maint_status;

  peer->hasField = NB_HAS_HB_FIELD(len, field_max_mv);
  if (peer->hasField) {
    peer->fieldPhase = hb->field_phase;
    peer->fieldReason = hb->field_reason;
    peer->fieldCycle = hb->field_cycle;
    peer->fieldElapsedS = hb->field_elapsed_s;
    peer->fieldChargeMah = hb->field_charge_mah;
    peer->fieldDischargeMah = hb->field_discharge_mah;
    peer->fieldMinMv = hb->field_min_mv;
    peer->fieldMaxMv = hb->field_max_mv;
  }

  peer->hasBq = NB_HAS_HB_FIELD(len, bq_part);
  if (peer->hasBq) {
    peer->bqVindpmMv = hb->bq_vindpm_mv;
    peer->bqIchgMa = hb->bq_ichg_ma;
    peer->bqVregMv = hb->bq_vreg_mv;
    peer->bqReg16 = hb->bq_reg16;
    peer->bqReg18 = hb->bq_reg18;
    peer->bqStat0 = hb->bq_stat0;
    peer->bqStat1 = hb->bq_stat1;
    peer->bqFault0 = hb->bq_fault0;
    peer->bqFlag0 = hb->bq_flag0;
    peer->bqFlag1 = hb->bq_flag1;
    peer->bqFaultFlag0 = hb->bq_fault_flag0;
    peer->bqPart = hb->bq_part;
  }

  peer->hasFieldSummary = NB_HAS_HB_FIELD(len, field_protect_min);
  if (peer->hasFieldSummary) {
    peer->fieldChargeWhX10 = hb->field_charge_wh_x10;
    peer->fieldDischargeWhX10 = hb->field_discharge_wh_x10;
    peer->fieldPeakPanelWX100 = hb->field_peak_panel_w_x100;
    peer->fieldPeakChargeWX100 = hb->field_peak_charge_w_x100;
    peer->fieldPeakDrawWX100 = hb->field_peak_draw_w_x100;
    peer->fieldLowS = hb->field_low_s;
    peer->fieldChargeMin = hb->field_charge_min;
    peer->fieldWaitMin = hb->field_wait_min;
    peer->fieldDrawMin = hb->field_draw_min;
    peer->fieldProtectMin = hb->field_protect_min;
  }

  peer->hasMppt = NB_HAS_HB_FIELD(len, mppt_p50_w_x100);
  if (peer->hasMppt) {
    peer->mpptStatus = hb->mppt_status;
    peer->mpptReason = hb->mppt_reason;
    peer->mpptRuns = hb->mppt_runs;
    peer->mpptActiveV10 = hb->mppt_active_v10;
    peer->mpptBestV10 = hb->mppt_best_v10;
    peer->mpptLastV10 = hb->mppt_last_v10;
    peer->mpptP46WX100 = hb->mppt_p46_w_x100;
    peer->mpptP48WX100 = hb->mppt_p48_w_x100;
    peer->mpptP50WX100 = hb->mppt_p50_w_x100;
  }

  peer->hasFieldLatches = NB_HAS_HB_FIELD(len, field_protect_latched);
  if (peer->hasFieldLatches) {
    peer->fieldLoadDimmed = hb->field_load_dimmed;
    peer->fieldProtectLatched = hb->field_protect_latched;
  }

  peer->hasFixtureState = NB_HAS_HB_FIELD(len, night_min);
  if (peer->hasFixtureState) {
    peer->profile = hb->profile;
    peer->lifeState = hb->life_state;
    peer->powerTier = hb->power_tier;
    peer->activeProgram = hb->active_program;
    peer->nightMin = hb->night_min;
  }

  peer->hasLedOutput = NB_HAS_HB_FIELD(len, led_lit_pixels);
  if (peer->hasLedOutput) {
    peer->fixtureClass = hb->fixture_class;
    peer->ledRailOn = hb->led_rail_on;
    peer->ledR = hb->led_r;
    peer->ledG = hb->led_g;
    peer->ledB = hb->led_b;
    peer->ledW = hb->led_w;
    peer->ledLitPixels = hb->led_lit_pixels;
    if (hb->fixture_class != 0) peer->classLatched = hb->fixture_class;
  }

  peer->hasIdentityRecovery = NB_HAS_HB_FIELD(len, recovery_detect_mv);
  if (peer->hasIdentityRecovery) {
    peer->sensorBits = hb->sensor_bits;
    peer->classMismatch = hb->class_mismatch;
    peer->recoveryState = hb->recovery_state;
    peer->recoveryDetectMv = hb->recovery_detect_mv;
  }
  if (NB_HAS_HB_FIELD(len, last_protect_batt_mv)) {
    peer->hasSleepAudit = true;
    peer->sleepAuditFlags = hb->sleep_audit_flags;
    peer->lastSleepCause = hb->last_sleep_cause;
    peer->lastSleepS = hb->last_sleep_s;
    peer->lastSleepBattMv = hb->last_sleep_batt_mv;
    peer->lastSleepProfile = hb->last_sleep_profile;
    peer->lastSleepLifeState = hb->last_sleep_life_state;
    peer->lastSleepPowerTier = hb->last_sleep_power_tier;
    memcpy(peer->lastSleepSourceId, hb->last_sleep_source_id,
           sizeof(peer->lastSleepSourceId));
    peer->lastSleepSourceSeq = hb->last_sleep_source_seq;
    peer->lastCommandSleepCause = hb->last_command_sleep_cause;
    peer->lastCommandSleepS = hb->last_command_sleep_s;
    memcpy(peer->lastCommandSleepSourceId, hb->last_command_sleep_source_id,
           sizeof(peer->lastCommandSleepSourceId));
    peer->lastCommandSleepSourceSeq = hb->last_command_sleep_source_seq;
    peer->lastProtectBattMv = hb->last_protect_batt_mv;
  }
  return true;
}

void Census::tickWindow(uint32_t nowMs) {
  if (nowMs - mWindowStartMs < mWindowMs) return;
  for (size_t i = 0; i < mCap; ++i) {
    PeerStat &p = mPeers[i];
    if (!p.used) continue;
    uint32_t total = p.winRecv + p.winGaps;
    p.winPdrX1000 =
        total ? (uint16_t)((uint64_t)p.winRecv * 1000 / total) : 0xFFFF;
    p.winRecv = 0;
    p.winGaps = 0;
  }
  uint32_t unobs = mUnobservedMsCur;
  if (unobs > mWindowMs) unobs = mWindowMs;
  mObservedPermille = (uint16_t)(1000 - (uint64_t)unobs * 1000 / mWindowMs);
  mUnobservedMsCur = 0;
  mWindowStartMs = nowMs;
}

void Census::noteRadioGap(uint32_t offMs, uint32_t nowMs) {
  (void)nowMs;
  mUnobservedMsCur += offMs;
}

void Census::fillView(CensusView &v, const PeerStat &p, uint32_t nowMs) const {
  memcpy(v.id, p.id, 3);
  v.ageMs = nowMs - p.lastHeardMs;
  v.rssi = p.rssi;
  v.rssiEwma = p.rssiEwma;
  uint32_t total = p.recv + p.gaps;
  v.pdrX1000 = total ? (uint16_t)((uint64_t)p.recv * 1000 / total) : 0;
  v.winPdrX1000 = p.winPdrX1000;
  v.battMv = p.battMv;
  v.soc = p.soc;
  v.fixtureClass = p.classLatched;
  v.lifeState = p.hasFixtureState ? p.lifeState : 0;
  v.activeProgram = p.hasFixtureState ? p.activeProgram : 0;
  v.powerTier = p.hasFixtureState ? p.powerTier : 0;
  v.ledKnown = p.hasLedOutput;
  v.ledOn = p.ledRailOn && p.ledLitPixels > 0;
  v.ledR = p.ledR;
  v.ledG = p.ledG;
  v.ledB = p.ledB;
  v.ledW = p.ledW;
}

size_t Census::snapshot(CensusView *out, size_t maxOut, uint32_t nowMs) const {
  size_t n = 0;
  for (size_t i = 0; i < mCap && n < maxOut; ++i)
    if (mPeers[i].used) fillView(out[n++], mPeers[i], nowMs);
  return n;
}

size_t Census::quietList(uint32_t quietS, CensusView *out, size_t maxOut,
                         uint32_t nowMs) const {
  size_t n = 0;
  for (size_t i = 0; i < mCap && n < maxOut; ++i) {
    const PeerStat &p = mPeers[i];
    if (!p.used) continue;
    if ((nowMs - p.lastHeardMs) / 1000 >= quietS) fillView(out[n++], p, nowMs);
  }
  return n;
}

const PeerStat *Census::byId(const uint8_t id[3]) const {
  for (size_t i = 0; i < mCap; ++i)
    if (mPeers[i].used && memcmp(mPeers[i].id, id, 3) == 0) return &mPeers[i];
  return nullptr;
}

const PeerStat *Census::at(size_t i) const {
  return (i < mCap && mPeers[i].used) ? &mPeers[i] : nullptr;
}

int Census::liveCount(uint32_t nowMs) const {
  int n = 0;
  for (size_t i = 0; i < mCap; ++i)
    if (mPeers[i].used && nowMs - mPeers[i].lastHeardMs < mFreshMs) ++n;
  return n;
}

int Census::seenCount() const {
  int n = 0;
  for (size_t i = 0; i < mCap; ++i)
    if (mPeers[i].used) ++n;
  return n;
}
