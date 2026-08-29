#include "nb_emit.h"

#include <Arduino.h>
#include <esp_system.h>

#include "../core/census.h"
#include "../core/version.h"
#include "../hal/hal_board.h"
#include "../store/store.h"
#include "census_svc.h"
#include "fixture/src/core/packet.h"
#include "mesh_tx.h"

static bool gEnabled = true;

void nbEmitEnable(bool on) { gEnabled = on; }
bool nbEmitEnabled() { return gEnabled; }

static const char *resetReasonName(uint8_t raw) {
  switch ((esp_reset_reason_t)raw) {
    case ESP_RST_POWERON: return "poweron";
    case ESP_RST_EXT: return "external";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_watchdog";
    case ESP_RST_TASK_WDT: return "task_watchdog";
    case ESP_RST_WDT: return "other_watchdog";
    case ESP_RST_DEEPSLEEP: return "deepsleep";
    case ESP_RST_BROWNOUT: return "brownout";
    default: return "unknown";
  }
}

// Donor-faithful port of cores3_bridge emitBridgeStats (:982-1115): same field
// names, same order, same formats — the dashboard regex is the contract.
void nbEmitTick(uint32_t nowMs) {
  static uint32_t lastMs = 0;
  // A fleet-sized snapshot is tens of kilobytes.  At 115200 baud a 1 Hz full
  // census never drains, starving serial commands and their acknowledgements.
  // Ten seconds keeps health telemetry useful while leaving deterministic
  // headroom for exact-target maintenance and other operator commands.
  if (!gEnabled || nowMs - lastMs < 10000) return;
  lastMs = nowMs;

  const uint8_t *myId = meshMyId();
  ActionAuditRecord action = {};
  bool hasAction = storeLatestAction(action);
  Serial.printf(
      "nb-master id=%02X%02X%02X ch=%d frames=%lu sendok=%lu sendfail=%lu "
      "up=%lu bv=%.3f fw=tdeck-" TDECK_FW_VERSION " "
      "act=%u actv=%lu actseq=%lu actup=%lu actutc=%lu actf=%02X "
      "acttgt=%02X%02X%02X actn=%u\n",
      myId[0], myId[1], myId[2], settings().channel,
      (unsigned long)meshTxSeq(), (unsigned long)meshTxSendOk(),
      (unsigned long)meshTxSendFail(), (unsigned long)nowMs,
      halBatteryMv() / 1000.0f, hasAction ? action.action : 0,
      (unsigned long)(hasAction ? action.value : 0),
      (unsigned long)(hasAction ? action.mesh_seq : 0),
      (unsigned long)(hasAction ? action.bridge_uptime_ms : 0),
      (unsigned long)(hasAction ? action.utc_s : 0),
      hasAction ? action.flags : 0,
      hasAction ? action.target_id[0] : 0,
      hasAction ? action.target_id[1] : 0,
      hasAction ? action.target_id[2] : 0, storeActionCount());

  char line[1152];
  for (size_t i = 0; i < census().capacity(); ++i) {
    const PeerStat *p = census().at(i);
    if (!p) continue;
    uint32_t total = p->recv + p->gaps;
    float pdr = total ? (float)p->recv / (float)total : 0.0f;
    int n = snprintf(
        line, sizeof(line),
        "nb-peer id=%02X%02X%02X seq=%lu rx=%lu gaps=%lu pdr=%.4f rssi=%d "
        "bv=%.3f ima=%d soc=%d rr=%s ca=%d mode=%d dlpdr=%.3f dlrssi=%d "
        "up=%lu age=%lu sv=%.3f sma=%d sgood=%d",
        p->id[0], p->id[1], p->id[2], (unsigned long)p->lastSeq,
        (unsigned long)p->recv, (unsigned long)p->gaps, pdr, p->rssi,
        p->battMv / 1000.0f, p->battMa, p->soc == 255 ? -1 : p->soc,
        resetReasonName(p->resetReason), p->caState, p->mode,
        p->dlPdrX1000 / 1000.0f, p->dlRssi, (unsigned long)p->uptimeMs,
        (unsigned long)(nowMs - p->lastHeardMs), p->supplyMv / 1000.0f,
        p->supplyMa, p->supplyGood);

    if (p->hasEnv && n < (int)sizeof(line)) {
      char lux[16], panelTemp[12], battTemp[12];
      if (p->luxX10 == 0xFFFFFFFF) snprintf(lux, sizeof(lux), "nan");
      else if (p->luxX10 == 0xFFFFFFFE) snprintf(lux, sizeof(lux), "sat");
      else snprintf(lux, sizeof(lux), "%.1f", p->luxX10 / 10.0f);
      if (p->panelTempCx10 == INT16_MIN) snprintf(panelTemp, sizeof(panelTemp), "nan");
      else snprintf(panelTemp, sizeof(panelTemp), "%.1f", p->panelTempCx10 / 10.0f);
      if (p->battTempCx10 == INT16_MIN) snprintf(battTemp, sizeof(battTemp), "nan");
      else snprintf(battTemp, sizeof(battTemp), "%.1f", p->battTempCx10 / 10.0f);
      n += snprintf(line + n, sizeof(line) - n,
                    " lux=%s ch0=%u ch1=%u ptc=%s prh=%d btc=%s", lux,
                    p->lightCh0, p->lightCh1, panelTemp,
                    p->panelRhPct == 255 ? -1 : p->panelRhPct, battTemp);
    }
    if (p->hasIna && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n, " ipv=%d ipa=%d ibv=%d iba=%d",
                    p->inaPvMv, p->inaPaMa, p->inaBvMv, p->inaBaMa);
    if (p->hasConfig && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n, " cap=%u chg=%u",
                    p->capacityMah, p->chargeMa);
    if (p->hasDrawdown && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n, " dd=%.1f ddb=%u dda=%u",
                    p->drawdownMahX10 / 10.0f, p->drawdownBudgetMah,
                    p->drawdownActive);
    if (p->hasFw && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n, " fw=%s", p->fwRev);
    if (p->hasMaint && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n, " mt=%u", p->maintStatus);
    if (p->hasField && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n,
                    " fc=%u fcr=%u fcc=%u fce=%u fcchg=%u fcdis=%u fcmin=%u fcmax=%u",
                    p->fieldPhase, p->fieldReason, p->fieldCycle,
                    p->fieldElapsedS, p->fieldChargeMah, p->fieldDischargeMah,
                    p->fieldMinMv, p->fieldMaxMv);
    if (p->hasBq && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n,
                    " bqv=%u bqichg=%u bqvreg=%u bq16=%02X bq18=%02X bq1d=%02X "
                    "bq1e=%02X bq1f=%02X bq20=%02X bq21=%02X bq22=%02X bq38=%02X",
                    p->bqVindpmMv, p->bqIchgMa, p->bqVregMv, p->bqReg16,
                    p->bqReg18, p->bqStat0, p->bqStat1, p->bqFault0, p->bqFlag0,
                    p->bqFlag1, p->bqFaultFlag0, p->bqPart);
    if (p->hasFieldSummary && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n,
                    " fcwhc=%u fcwhd=%u fcpw=%u fcbw=%u fcdw=%u fclow=%u "
                    "fcmchg=%u fcmwait=%u fcmdraw=%u fcmprot=%u",
                    p->fieldChargeWhX10, p->fieldDischargeWhX10,
                    p->fieldPeakPanelWX100, p->fieldPeakChargeWX100,
                    p->fieldPeakDrawWX100, p->fieldLowS, p->fieldChargeMin,
                    p->fieldWaitMin, p->fieldDrawMin, p->fieldProtectMin);
    if (p->hasMppt && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n,
                    " mppts=%u mpptr=%u mpptn=%u mpptv=%u mpptbest=%u "
                    "mpptlast=%u mppt46=%u mppt48=%u mppt50=%u",
                    p->mpptStatus, p->mpptReason, p->mpptRuns, p->mpptActiveV10,
                    p->mpptBestV10, p->mpptLastV10, p->mpptP46WX100,
                    p->mpptP48WX100, p->mpptP50WX100);
    if (p->hasFieldLatches && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n, " fcdim=%u fclat=%u",
                    p->fieldLoadDimmed, p->fieldProtectLatched);
    if (p->hasFixtureState && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n,
                    " prof=%u life=%u ptier=%u prog=%u nmin=%u", p->profile,
                    p->lifeState, p->powerTier, p->activeProgram, p->nightMin);
    if (p->hasLedOutput && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n,
                    " cls=%u ledrail=%u ledr=%u ledg=%u ledb=%u ledw=%u ledn=%u",
                    p->fixtureClass, p->ledRailOn, p->ledR, p->ledG, p->ledB,
                    p->ledW, p->ledLitPixels);
    if (p->hasIdentityRecovery && n < (int)sizeof(line))
      n += snprintf(line + n, sizeof(line) - n, " sens=%u cmis=%u rec=%u recmv=%u",
                    p->sensorBits, p->classMismatch, p->recoveryState,
                     p->recoveryDetectMv);
    if (p->hasSleepAudit && n < (int)sizeof(line))
      n += snprintf(
          line + n, sizeof(line) - n,
          " audf=%u slpr=%u slps=%lu slpmv=%d slpprof=%u slplife=%u "
          "slptier=%u slpsrc=%02X%02X%02X slpseq=%lu cmdslpr=%u cmdslps=%lu "
          "cmdslpsrc=%02X%02X%02X cmdslpseq=%lu protmv=%d",
          p->sleepAuditFlags, p->lastSleepCause,
          (unsigned long)p->lastSleepS, p->lastSleepBattMv,
          p->lastSleepProfile, p->lastSleepLifeState, p->lastSleepPowerTier,
          p->lastSleepSourceId[0], p->lastSleepSourceId[1],
          p->lastSleepSourceId[2], (unsigned long)p->lastSleepSourceSeq,
          p->lastCommandSleepCause, (unsigned long)p->lastCommandSleepS,
          p->lastCommandSleepSourceId[0], p->lastCommandSleepSourceId[1],
          p->lastCommandSleepSourceId[2],
          (unsigned long)p->lastCommandSleepSourceSeq,
          p->lastProtectBattMv);
    if (p->hasProtectContext && n < (int)sizeof(line))
      n += snprintf(
          line + n, sizeof(line) - n,
          " protorig=%u protprev=%u protrst=%u protarm=%u protstreak=%u",
          p->lastProtectOrigin, p->lastProtectPredecessorStage,
          p->lastProtectResetReason, p->lastProtectLoadArmed,
          p->lastProtectResetStreak);
    if (p->hasRitualAudit && n < (int)sizeof(line))
      n += snprintf(
          line + n, sizeof(line) - n,
          " ritf=%u ritexp=%u ritat=%u ritfire=%u ritref=%u ritblk=%u "
          "ritu=%u rith=%lu ritcanh=%lu ritcantgt=%02X%02X%02X",
          p->ritualFlags, p->ritualExpectedMask, p->ritualAttemptedMask,
          p->ritualFiredMask, p->ritualPolicyRefusedMask,
          p->ritualMechanismBlockedMask, p->ritualLastUncertaintyMs,
          (unsigned long)p->ritualHourKey,
          (unsigned long)p->ritualCanaryHourKey,
          p->ritualCanaryTargetId[0], p->ritualCanaryTargetId[1],
          p->ritualCanaryTargetId[2]);
    if (n < 0) continue;
    // Clamp before the newline so a future tail cannot turn truncation into
    // an out-of-bounds write (donor comment, kept true here).
    size_t outputLen = min((size_t)n, sizeof(line) - 2);
    line[outputLen++] = '\n';
    line[outputLen] = '\0';
    Serial.write((const uint8_t *)line, outputLen);
  }
}

void nbEmitScanAp(const RxItem &item) {
  if (item.len < sizeof(NbScanAp)) return;
  const NbScanAp *scan = (const NbScanAp *)item.data;
  char ssid[21] = {};
  memcpy(ssid, scan->ssid, 20);
  Serial.printf(
      "nb-scanap from=%02X%02X%02X scan=%u idx=%u count=%u "
      "bssid=%02x:%02x:%02x:%02x:%02x:%02x ap_rssi=%d ch=%u enc=%u "
      "linkrssi=%d ssid=%s\n",
      scan->h.src_id[0], scan->h.src_id[1], scan->h.src_id[2], scan->scan_id,
      scan->idx, scan->count, scan->bssid[0], scan->bssid[1], scan->bssid[2],
      scan->bssid[3], scan->bssid[4], scan->bssid[5], scan->ap_rssi,
      scan->channel, scan->enc, item.rssi, ssid);
}

void nbEmitNeighborReport(const RxItem &item) {
  if (item.len < (int)offsetof(NbNeighborReport, entries)) return;
  const NbNeighborReport *report = (const NbNeighborReport *)item.data;
  uint8_t available = (uint8_t)((item.len - offsetof(NbNeighborReport, entries)) /
                                sizeof(NbNeighborEntry));
  uint8_t count = min((uint8_t)NB_NEIGHBOR_REPORT_MAX,
                      min(report->count, available));
  for (uint8_t i = 0; i < count; ++i) {
    const NbNeighborEntry &entry = report->entries[i];
    Serial.printf(
        "nb-rssi report=%lu rx=%02X%02X%02X tx=%02X%02X%02X rssi=%d n=%u "
        "expected=%u censored=%u idx=%u count=%u linkrssi=%d\n",
        (unsigned long)report->h.seq, report->h.src_id[0], report->h.src_id[1],
        report->h.src_id[2], entry.id[0], entry.id[1], entry.id[2],
        entry.med_dbm, entry.n, report->n_expected, entry.flags & 0x01, i,
        count, item.rssi);
  }
}

void nbEmitTimeQuality(const RxItem &item) {
  if (item.len < sizeof(NbTimeQuality)) return;
  const NbTimeQuality *quality = (const NbTimeQuality *)item.data;

  GpsUtcObservation gps = halGpsUtc();
  uint32_t gpsAgeMs = gps.valid ? millis() - gps.receivedMs : 0;
  bool gpsFresh = gps.valid && gpsAgeMs <= 10000UL;
  int64_t deltaMs = 0;
  if (gpsFresh) {
    uint64_t reportedMs = (uint64_t)quality->utc_s * 1000ULL +
                          quality->sub_ms +
                          (uint64_t)quality->age_s * 1000ULL;
    uint64_t gpsNowMs = (uint64_t)gps.utcS * 1000ULL + gps.subMs + gpsAgeMs;
    deltaMs = (int64_t)reportedMs - (int64_t)gpsNowMs;
  }

  Serial.printf(
      "nb-time from=%02X%02X%02X utc=%lu sub=%u src=%u hops=%u age=%u "
      "uncert=%u boot=%u flags=%02X linkrssi=%d gps=%u gpsutc=%lu "
      "gpssub=%u gpsage=%lu delta=%lld\n",
      quality->h.src_id[0], quality->h.src_id[1], quality->h.src_id[2],
      (unsigned long)quality->utc_s, quality->sub_ms, quality->source,
      quality->hops, quality->age_s, quality->uncert_ms, quality->boot_id,
      quality->flags, item.rssi, gpsFresh ? 1U : 0U,
      (unsigned long)(gpsFresh ? gps.utcS : 0U), gpsFresh ? gps.subMs : 0U,
      (unsigned long)(gpsFresh ? gpsAgeMs : 0U), (long long)deltaMs);
}

void nbEmitLocalGps(const uint8_t sourceId[3], uint32_t utcS, uint16_t subMs,
                    uint32_t ageMs, uint16_t uncertaintyMs, uint16_t bootId) {
  Serial.printf(
      "nb-time from=%02X%02X%02X utc=%lu sub=%u src=%u hops=0 age=%u "
      "uncert=%u boot=%u flags=%02X linkrssi=0 gps=1 gpsutc=%lu "
      "gpssub=%u gpsage=%lu delta=0\n",
      sourceId[0], sourceId[1], sourceId[2], (unsigned long)utcS, subMs,
      (unsigned)NB_TIME_GPS, (unsigned)(ageMs / 1000U), uncertaintyMs, bootId,
      (unsigned)(NB_TIME_FLAG_VALID | NB_TIME_FLAG_DATE_VALID),
      (unsigned long)utcS, subMs, (unsigned long)ageMs);
}
