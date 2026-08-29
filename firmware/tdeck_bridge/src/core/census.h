#pragma once

#include <stddef.h>
#include <stdint.h>

#include "rx_ring.h"

// Passive fleet census over heartbeats — port of the cores3_bridge PeerStat
// table (all 16 NB_HAS_HB_FIELD-gated tails, donor cores3_bridge.ino:270-912)
// plus the Bridge OS additions:
//   - per-peer RSSI EWMA, alpha 1/8 (donor: fixture neighbor_table.cpp:22)
//   - windowed PDR next to the donor's cumulative-since-reset ratio
//   - eviction when full: stalest-beyond-fresh first, else weakest EWMA with
//     a 6 dB newcomer hysteresis (donor: neighbor_table.cpp:37-59)
//   - fixture-class latching across hb-short frames (the dashboard does this
//     host-side at net_bench_dashboard.py:482)
//   - a radio-observation ledger so duty-cycled listening reads as
//     "unobserved", never as "quiet" (ADR 0037 §11)
// Pure: no Arduino includes; time is injected. Native tests: test_census.cpp.

struct PeerStat {
  bool used;
  uint8_t id[3];
  uint32_t lastSeq;
  uint32_t recv;
  uint32_t gaps;
  int8_t rssi;      // last packet
  int8_t rssiEwma;  // alpha 1/8
  uint32_t lastHeardMs;
  uint32_t uptimeMs;

  // Windowed PDR (Bridge OS addition).
  uint32_t winRecv;
  uint32_t winGaps;
  uint16_t winPdrX1000;  // last CLOSED window; 0xFFFF = no data yet

  int16_t battMv;
  int16_t battMa;
  uint8_t soc;
  uint8_t resetReason;
  uint8_t caState;
  uint8_t mode;
  uint16_t dlPdrX1000;
  int8_t dlRssi;

  int16_t supplyMv;
  int16_t supplyMa;
  uint8_t supplyGood;

  bool hasEnv;
  uint32_t luxX10;
  uint16_t lightCh0;
  uint16_t lightCh1;
  int16_t panelTempCx10;
  uint8_t panelRhPct;
  int16_t battTempCx10;

  bool hasIna;
  int16_t inaPvMv;
  int16_t inaPaMa;
  int16_t inaBvMv;
  int16_t inaBaMa;

  bool hasConfig;
  uint16_t capacityMah;
  uint16_t chargeMa;

  bool hasDrawdown;
  uint16_t drawdownMahX10;
  uint16_t drawdownBudgetMah;
  uint8_t drawdownActive;

  bool hasFw;
  char fwRev[24];
  bool hasMaint;
  uint8_t maintStatus;

  bool hasField;
  uint8_t fieldPhase;
  uint8_t fieldReason;
  uint16_t fieldCycle;
  uint16_t fieldElapsedS;
  uint16_t fieldChargeMah;
  uint16_t fieldDischargeMah;
  uint16_t fieldMinMv;
  uint16_t fieldMaxMv;

  bool hasBq;
  uint16_t bqVindpmMv;
  uint16_t bqIchgMa;
  uint16_t bqVregMv;
  uint8_t bqReg16;
  uint8_t bqReg18;
  uint8_t bqStat0;
  uint8_t bqStat1;
  uint8_t bqFault0;
  uint8_t bqFlag0;
  uint8_t bqFlag1;
  uint8_t bqFaultFlag0;
  uint8_t bqPart;

  // Tail-17 proof that cached power values are usable. This is deliberately
  // absent/false for old firmware rather than inferring validity from a number.
  bool hasPowerSampleFlags;
  uint8_t powerSampleFlags;

  bool hasFieldSummary;
  uint16_t fieldChargeWhX10;
  uint16_t fieldDischargeWhX10;
  uint16_t fieldPeakPanelWX100;
  uint16_t fieldPeakChargeWX100;
  uint16_t fieldPeakDrawWX100;
  uint8_t fieldLowS;
  uint8_t fieldChargeMin;
  uint8_t fieldWaitMin;
  uint8_t fieldDrawMin;
  uint8_t fieldProtectMin;

  bool hasMppt;
  uint8_t mpptStatus;
  uint8_t mpptReason;
  uint8_t mpptRuns;
  uint8_t mpptActiveV10;
  uint8_t mpptBestV10;
  uint8_t mpptLastV10;
  uint16_t mpptP46WX100;
  uint16_t mpptP48WX100;
  uint16_t mpptP50WX100;

  bool hasFieldLatches;
  uint8_t fieldLoadDimmed;
  uint8_t fieldProtectLatched;

  bool hasFixtureState;
  // Full-heartbeat fixture state is sparse (~60 s in production). Keep the
  // last explicit sample across hb-short frames and expose its own age so a
  // consumer never has to turn "not present in this packet" into IDLE.
  uint32_t fixtureStateHeardMs;
  uint8_t profile;
  uint8_t lifeState;
  uint8_t powerTier;
  uint8_t activeProgram;
  uint16_t nightMin;

  bool hasLedOutput;
  uint8_t fixtureClass;
  uint8_t ledRailOn;
  uint8_t ledR;
  uint8_t ledG;
  uint8_t ledB;
  uint8_t ledW;
  uint8_t ledLitPixels;

  bool hasIdentityRecovery;
  uint8_t sensorBits;
  uint8_t classMismatch;
  uint8_t recoveryState;
  uint16_t recoveryDetectMv;

  // Latched after the first full heartbeat: provenance changes only on a new
  // sleep/protection event, so hb-short frames must not hide it from emitters.
  bool hasSleepAudit;
  uint8_t sleepAuditFlags;
  uint8_t lastSleepCause;
  uint32_t lastSleepS;
  int16_t lastSleepBattMv;
  uint8_t lastSleepProfile;
  uint8_t lastSleepLifeState;
  uint8_t lastSleepPowerTier;
  uint8_t lastSleepSourceId[3];
  uint32_t lastSleepSourceSeq;
  uint8_t lastCommandSleepCause;
  uint32_t lastCommandSleepS;
  uint8_t lastCommandSleepSourceId[3];
  uint32_t lastCommandSleepSourceSeq;
  int16_t lastProtectBattMv;
  bool hasProtectContext;
  uint8_t lastProtectOrigin;
  uint8_t lastProtectPredecessorStage;
  uint8_t lastProtectResetReason;
  uint8_t lastProtectLoadArmed;
  uint16_t lastProtectResetStreak;

  bool hasRitualAudit;
  uint8_t ritualFlags;
  uint8_t ritualExpectedMask;
  uint8_t ritualAttemptedMask;
  uint8_t ritualFiredMask;
  uint8_t ritualPolicyRefusedMask;
  uint8_t ritualMechanismBlockedMask;
  uint16_t ritualLastUncertaintyMs;
  uint32_t ritualHourKey;
  uint32_t ritualCanaryHourKey;
  uint8_t ritualCanaryTargetId[3];

  // Latched across hb-short frames (hb-full arrives every ~60 s in prod).
  uint8_t classLatched;  // 0 = never seen
};

// Compact row for UI/tool consumers.
struct CensusView {
  uint8_t id[3];
  uint32_t ageMs;
  int8_t rssi;
  int8_t rssiEwma;
  uint16_t pdrX1000;     // cumulative since (re)sync
  uint16_t winPdrX1000;  // last closed window; 0xFFFF = no data
  int16_t battMv;
  int16_t battMa;        // + charging, - discharging
  bool battMaValid;
  uint8_t soc;
  int16_t supplyMv;
  int16_t supplyMa;
  bool supplyGood;
  bool hasBq;
  uint8_t bqReg16;
  uint8_t bqStat1;
  uint8_t bqFault0;
  uint8_t fixtureClass;  // latched
  bool hasFixtureState;
  uint8_t lifeState;
  uint8_t activeProgram;
  uint8_t powerTier;
  uint32_t fwFingerprint;  // 0 = no revision evidence
  // Reported LED output (hb-full tail 14; ADR 0043: reported color, not the
  // requested default, is the dashboard's source of truth).
  bool ledKnown;
  uint8_t ledOn;  // rail on AND >0 lit pixels
  uint8_t ledR, ledG, ledB, ledW;
};

// Compact equality key for firmware filtering. Full revision strings remain
// in PeerStat (one PSRAM record per peer), not in every consumer's cached row.
uint32_t censusFirmwareFingerprint(const char *revision);

class Census {
 public:
  void init(PeerStat *storage, size_t cap, uint32_t freshMs, uint32_t windowMs,
            uint32_t nowMs);
  // Heartbeats only; returns false for anything it did not consume.
  bool ingest(const RxItem &item, uint32_t nowMs);
  // Closes PDR/observation windows; call at loop cadence.
  void tickWindow(uint32_t nowMs);
  // Radio was off/elsewhere for offMs ending at nowMs (duty cycle, Wi-Fi scan).
  void noteRadioGap(uint32_t offMs, uint32_t nowMs);

  size_t snapshot(CensusView *out, size_t maxOut, uint32_t nowMs) const;
  size_t quietList(uint32_t quietS, CensusView *out, size_t maxOut,
                   uint32_t nowMs) const;
  const PeerStat *byId(const uint8_t id[3]) const;
  const PeerStat *at(size_t i) const;  // raw slot access for emitters
  size_t capacity() const { return mCap; }
  int liveCount(uint32_t nowMs) const;
  int seenCount() const;
  uint32_t freshMs() const { return mFreshMs; }
  // 1000 = the last closed window was fully observed; lower = we were deaf
  // for part of it. Surfaced so "quiet node" and "unobserved node" never look
  // the same (ADR 0037 §11).
  uint16_t observedPermille() const { return mObservedPermille; }

 private:
  PeerStat *findPeer(const uint8_t id[3], bool create, int8_t rssi,
                     uint32_t nowMs);
  void accountHeartbeat(PeerStat *p, uint32_t seq, uint32_t uptimeMs);
  void fillView(CensusView &v, const PeerStat &p, uint32_t nowMs) const;

  PeerStat *mPeers = nullptr;
  size_t mCap = 0;
  uint32_t mFreshMs = 5000;
  uint32_t mWindowMs = 60000;
  uint32_t mWindowStartMs = 0;
  uint32_t mUnobservedMsCur = 0;
  uint16_t mObservedPermille = 1000;
};
