#include "net_peer.h"

#include <Arduino.h>
#include "esp_random.h"
#include "esp_system.h"

#include "../core/fixture_context.h"
#include "../core/version.h"
#include "behavior_glue.h"
#include "board_power.h"
#include "identity.h"
#include "led_driver.h"
#include "maintenance.h"
#include "nvs_store.h"
#include "solenoid.h"
#include "status_led.h"
#include "telemetry.h"

uint8_t gNetCaState = 0;
uint8_t gNetLifeState = 0;
uint8_t gNetPowerTier = 0;
uint8_t gNetProgram = 0;
uint16_t gNetNightMin = 0;

static uint8_t gRateHz = 0; // 0 = profile default cadence
static uint32_t gHbSeq = 0;
static uint32_t gNextShortMs = 0;
static uint32_t gNextFullMs = 0;
static bool gLocateActive = false;
static uint32_t gLocateUntilMs = 0;
static uint32_t gNextLocateMs = 0;
static uint16_t gLocatePeriodMs = 2000;
static NeighborView gLocateViews[NEIGHBOR_TABLE_SIZE];

// Downlink (bridge SHOWFRAME/broadcast) accounting, donor semantics.
static uint32_t gDlLastSeq = 0;
static uint32_t gDlRx = 0, gDlGaps = 0;
static int8_t gDlRssi = 0;
static bool gDlSeen = false;
static uint32_t gDirectSeen = 0, gDirectMatched = 0;

static ShowFrameIn gShowFrame = {};
static uint8_t gIdentColor = 0, gIdentBlink = 0, gIdentValue = 255;
static uint32_t gIdentUntil = 0;

const ShowFrameIn &netPeerLastShowFrame() { return gShowFrame; }
uint8_t netPeerIdentifyColor() { return gIdentColor; }
uint8_t netPeerIdentifyBlink() { return gIdentBlink; }
uint8_t netPeerIdentifyValue() { return gIdentValue; }
bool netPeerIdentifyActive() { return millis() < gIdentUntil; }
uint16_t netPeerDlPdrX1000() {
  uint32_t tot = gDlRx + gDlGaps;
  return tot ? (uint16_t)((uint64_t)gDlRx * 1000 / tot) : 0;
}
int8_t netPeerDlRssi() { return gDlRssi; }
uint32_t netPeerDirectSeen() { return gDirectSeen; }
uint32_t netPeerDirectMatched() { return gDirectMatched; }

void netPeerSetRateHz(uint8_t hz) { gRateHz = hz; }

static uint32_t jittered(uint32_t periodMs) {
  uint32_t jit = (periodMs * RES_JITTER_PCT) / 100;
  return periodMs + (jit ? (esp_random() % (2 * jit)) - jit : 0);
}

static uint32_t shortPeriodMs() {
  if (gRateHz > 0) return 1000UL / gRateHz;
  return gCfg.profile == PROFILE_DEV ? RES_HB_SHORT_PERIOD_DEV_MS
                                     : RES_HB_SHORT_PERIOD_PROD_MS;
}

static uint32_t fullPeriodMs() {
  return gCfg.profile == PROFILE_DEV ? RES_HB_FULL_PERIOD_DEV_MS
                                     : RES_HB_FULL_PERIOD_PROD_MS;
}

void netPeerSendHeartbeat(bool full) {
  NbHeartbeat hb;
  memset(&hb, 0, sizeof(hb));
  fillHeader(&hb.h, NB_HEARTBEAT);
  // Heartbeats carry their OWN contiguous seq: the bridge derives uplink PDR
  // from it, while the shared txSeq is also consumed by other sends.
  hb.h.seq = gHbSeq++;
  hb.batt_mv = (int16_t)(batteryVolts() * 1000.0f);
  hb.batt_ma = (int16_t)batteryMa();
  hb.soc_pct = (batterySocPct() < 0) ? 255 : (uint8_t)batterySocPct();
  hb.reset_reason = (uint8_t)esp_reset_reason();
  hb.ca_state = gNetCaState;
  hb.mode = (uint8_t)maintMode();
  hb.dl_pdr_x1000 = netPeerDlPdrX1000();
  hb.dl_rssi = gDlRssi;
  hb.supply_mv = (int16_t)(supplyVolts() * 1000.0f);
  hb.supply_ma = (int16_t)supplyMa();
  hb.supply_good = supplyGood() ? 1 : 0;
  if (!full) {
    espNowSendRaw(&hb, NB_HB_SHORT_LEN);
    return;
  }
  // Bench-only sources ride as absent sentinels so old parsers keep decoding.
  hb.lux_x10 = 0xFFFFFFFF;
  hb.ptemp_cx10 = INT16_MIN;
  hb.prh_pct = 255;
  hb.btemp_cx10 = INT16_MIN;
  hb.ina_pv_mv = INT16_MIN;
  hb.ina_pa_ma = INT16_MIN;
  hb.ina_bv_mv = INT16_MIN;
  hb.ina_ba_ma = INT16_MIN;
  hb.cfg_cap_mah = gCfg.capMah;
  hb.cfg_charge_ma = gCfg.chargeMa;
  strncpy(hb.fw_rev, RES_FIXTURE_VERSION, sizeof(hb.fw_rev) - 1);
  hb.maint_status = maintStatus();
  hb.field_phase = gNetLifeState; // lifecycle enum rides the legacy field slot
  hb.field_min_mv = (uint16_t)(batteryVolts() * 1000.0f);
  hb.field_max_mv = (uint16_t)(batteryVolts() * 1000.0f);
  const BqSnapshot &bq = bqSnapshot();
  hb.bq_vindpm_mv = bq.vindpm_mv;
  hb.bq_ichg_ma = bq.ichg_ma;
  hb.bq_vreg_mv = bq.vreg_mv;
  hb.bq_reg16 = bq.reg16;
  hb.bq_reg18 = bq.reg18;
  hb.bq_stat0 = bq.stat0;
  hb.bq_stat1 = bq.stat1;
  hb.bq_fault0 = bq.fault0;
  hb.bq_flag0 = bq.flag0;
  hb.bq_flag1 = bq.flag1;
  hb.bq_fault_flag0 = bq.fault_flag0;
  hb.bq_part = bq.part;
  hb.mppt_status = 0; // bench MPPT perturb retired in production
  hb.profile = gCfg.profile;
  hb.life_state = gNetLifeState;
  hb.power_tier = gNetPowerTier;
  hb.active_program = gNetProgram;
  hb.night_min = gNetNightMin;
  hb.fixture_class = gTelemetryFixtureClass;
  LedOutputSnapshot led = ledOutputSnapshot();
  hb.led_rail_on = led.railOn;
  hb.led_r = led.r;
  hb.led_g = led.g;
  hb.led_b = led.b;
  hb.led_w = led.w;
  hb.led_lit_pixels = led.litPixels;
  hb.sensor_bits = gTelemetrySensorBits;
  hb.class_mismatch = gTelemetryClassMismatch ? 1 : 0;
  hb.recovery_state = lowVbatRecoveryState();
  hb.recovery_detect_mv = lowVbatRecoveryDetectMv();
  espNowSendRaw(&hb, NB_HB_FULL_LEN);
}

static void accountDownlink(const NbHeader *h, int8_t rssi) {
  gDlRssi = rssi;
  if (!gDlSeen) {
    gDlSeen = true;
    gDlLastSeq = h->seq;
    gDlRx = 1;
    return;
  }
  if (h->seq > gDlLastSeq) {
    gDlGaps += (h->seq - gDlLastSeq - 1);
    gDlRx++;
    gDlLastSeq = h->seq;
  } else if (gDlLastSeq - h->seq > 100) { // bridge reboot: fresh start
    gDlLastSeq = h->seq;
    gDlRx = 1;
    gDlGaps = 0;
  } else {
    gDlRx++;
  }
}

static void sendNeighborReports() {
  uint8_t total = behaviorNeighborSnapshot(gLocateViews, NEIGHBOR_TABLE_SIZE);
  // Fragment a full heard-roster snapshot into existing 16-entry packets.
  // At the 20 s default, even a 100-neighbor reporter averages <0.4 pkt/s.
  for (uint8_t offset = 0; offset < total; offset += NB_NEIGHBOR_REPORT_MAX) {
    uint8_t count = min((uint8_t)NB_NEIGHBOR_REPORT_MAX,
                        (uint8_t)(total - offset));
    NbNeighborReport report;
    memset(&report, 0, sizeof(report));
    fillHeader(&report.h, NB_NEIGHBOR_REPORT);
    report.count = count;
    // Each row is one ranked EWMA snapshot. The host aggregates repeated rows;
    // n=1/expected=1 avoids pretending this is an on-device median.
    report.n_expected = 1;
    for (uint8_t i = 0; i < count; ++i) {
      const NeighborView &view = gLocateViews[offset + i];
      memcpy(report.entries[i].id, view.id, 3);
      report.entries[i].med_dbm = view.rssi;
      report.entries[i].n = 1;
      report.entries[i].flags = 0;
    }
    size_t wireLen = offsetof(NbNeighborReport, entries) +
                     (size_t)count * sizeof(NbNeighborEntry);
    espNowSendRaw(&report, wireLen);
    delay(3);
  }
}

static void processPacket(const RxItem &it) {
  const NbHeader *h = (const NbHeader *)it.data;
  if (h->ver != NB_PROTO_VER) return;
  if (memcmp(h->src_id, gMyId, 3) == 0) return; // our own broadcast echo

  switch (h->type) {
  case NB_HEARTBEAT: {
    // Peer heartbeat (fixture or old net_bench): tracks the neighbor and
    // mirrors its ca_state so mixed benches still form a CA graph.
    if (it.len < (int)(sizeof(NbHeader) + 11)) return; // through ca_state+mode
    const NbHeartbeat *hb = (const NbHeartbeat *)it.data;
    behaviorOnPeerHeartbeat(h->src_id, it.rssi, hb->ca_state, it.len);
    break;
  }
  case NB_CHOREO_STATE: {
    if (it.len < (int)sizeof(NbChoreoState)) return;
    behaviorOnChoreoState(h->src_id, it.rssi, *(const NbChoreoState *)it.data);
    break;
  }
  case NB_PROGRAM_SET: {
    if (it.len < (int)sizeof(NbProgramSet)) return;
    behaviorOnProgramSet(*(const NbProgramSet *)it.data);
    break;
  }
  case NB_NEIGHBOR_SET: {
    if (it.len < (int)sizeof(NbNeighborSet)) return;
    behaviorOnNeighborSet(*(const NbNeighborSet *)it.data);
    break;
  }
  case NB_EVENT: {
    if (it.len < (int)sizeof(NbEvent)) return;
    behaviorOnEvent(*(const NbEvent *)it.data);
    break;
  }
  case NB_SHOWFRAME: {
    if (it.len < (int)(sizeof(NbHeader) + 4)) return;
    accountDownlink(h, it.rssi);
    const NbShowFrame *f = (const NbShowFrame *)it.data;
    gShowFrame.rx_ms = millis();
    gShowFrame.phase = f->phase;
    gShowFrame.hue = f->hue;
    gShowFrame.flags = f->flags;
    // Fixture-era tail (old masters send 17 B): default to full value/bright.
    bool hasTail = it.len >= (int)sizeof(NbShowFrame);
    gShowFrame.val = hasTail ? f->val : 255;
    gShowFrame.bright = hasTail ? f->bright : 255;
    gShowFrame.effect = hasTail ? f->effect : 0;
    gShowFrame.beat_phase = hasTail ? f->beat_phase : 0;
    gShowFrame.energy = hasTail ? f->energy : 0;
    break;
  }
  case NB_DIRECT_FRAME: {
    // Variable length: 15 B preamble + 7 B per entry; a frame with no entries
    // is not a frame. Per-fixture addressing rides IN the entries (there is no
    // target_id), so downlink accounting happens whether or not we're named.
    if (it.len < (int)(offsetof(NbDirectFrame, entries) + sizeof(NbDirectEntry))) return;
    ++gDirectSeen;
    accountDownlink(h, it.rssi);
    const NbDirectFrame *df = (const NbDirectFrame *)it.data;
    const NbDirectEntry *e = nbDirectFindEntry(df, it.len, gMyId);
    if (!e) break; // frame doesn't name us: not ours, ignore
    ++gDirectMatched;
    behaviorOnDirectFrame(e->r, e->g, e->b, e->w, df->flags);
    break;
  }
  case NB_FORCE_LIFECYCLE: {
    if (it.len < (int)sizeof(NbForceLifecycle)) return;
    const NbForceLifecycle *fl = (const NbForceLifecycle *)it.data;
    if (!nbTargetMatches(fl->target_id, gMyId)) return;
    if (fl->mode > 2) return;
    // Radio twin of serial 'N': RAM-only, a reboot always returns to auto.
    behaviorForceNight(fl->mode == 2 ? -1 : (int8_t)fl->mode);
    Serial.printf("force_night -> %s (radio)\n",
                  fl->mode == 2 ? "auto" : (fl->mode ? "night" : "day"));
    break;
  }
  case NB_TRANSPORT_SLEEP: {
    if (it.len < (int)sizeof(NbTransportSleep)) return;
    const NbTransportSleep *ts = (const NbTransportSleep *)it.data;
    if (!nbTargetMatches(ts->target_id, gMyId)) return;
    if (ts->seconds == 0 || ts->seconds > 7UL * 24UL * 3600UL) return;
    enterTransportSleep(ts->seconds, "radio-transport");
    break;
  }
  case NB_LOCATE_CONTROL: {
    if (it.len < (int)sizeof(NbLocateControl)) return;
    const NbLocateControl *lc = (const NbLocateControl *)it.data;
    if (!nbTargetMatches(lc->target_id, gMyId)) return;
    if (lc->duration_s == 0) {
      gLocateActive = false;
      Serial.println("locate survey: stopped");
      break;
    }
    if (lc->duration_s > 900 || lc->period_ds < 10 || lc->period_ds > 250) return;
    gLocatePeriodMs = (uint16_t)lc->period_ds * 100U;
    gLocateUntilMs = millis() + (uint32_t)lc->duration_s * 1000UL;
    gNextLocateMs = millis() + 100 + (esp_random() % gLocatePeriodMs);
    gLocateActive = true;
    Serial.printf("locate survey: %us every %ums\n", lc->duration_s,
                  gLocatePeriodMs);
    break;
  }
  case NB_ENTER_MAINT:
    if (maintMode() == MODE_COMMS) enterMaintenance();
    break;
  case NB_TARGET_ENTER_MAINT: {
    if (it.len < (int)sizeof(NbTargetCmd)) return;
    const NbTargetCmd *c = (const NbTargetCmd *)it.data;
    if (nbTargetMatches(c->target_id, gMyId) && maintMode() == MODE_COMMS)
      enterMaintenance();
    break;
  }
  case NB_RESUME:
    if (maintMode() == MODE_MAINT) enterComms();
    break;
  case NB_SET_RATE: {
    if (it.len < (int)sizeof(NbCmd)) return;
    const NbCmd *c = (const NbCmd *)it.data;
    if (c->arg >= 1 && c->arg <= 100) {
      netPeerSetRateHz(c->arg);
      Serial.printf("rate set -> %u Hz\n", c->arg);
    }
    break;
  }
  case NB_IDENTIFY: {
    if (it.len < (int)(sizeof(NbHeader) + 4)) return;
    const NbIdentify *id = (const NbIdentify *)it.data;
    if (!nbTargetMatches(id->target_id, gMyId)) return;
    bool hasColor = it.len >= (int)(offsetof(NbIdentify, blink) + sizeof(id->blink));
    bool hasValue = it.len >= (int)sizeof(NbIdentify);
    gIdentColor = hasColor ? id->color : 0;
    gIdentBlink = hasColor ? id->blink : 0;
    gIdentValue = hasValue && id->value ? id->value : 255;
    gIdentUntil = millis() + (uint32_t)id->secs * 1000UL;
    statusLedIdentify(id->secs);
    break;
  }
  case NB_SET_MAINTAIN: {
    if (it.len < (int)sizeof(NbCmd)) return;
    const NbCmd *c = (const NbCmd *)it.data;
    applyMaintainV10(c->arg);
    break;
  }
  case NB_SET_CAPACITY:
  case NB_TARGET_CAPACITY: {
    uint16_t mah;
    if (h->type == NB_SET_CAPACITY) {
      if (it.len < (int)sizeof(NbSetU16)) return;
      mah = ((const NbSetU16 *)it.data)->value;
    } else {
      if (it.len < (int)sizeof(NbTargetU16)) return;
      const NbTargetU16 *c = (const NbTargetU16 *)it.data;
      if (!nbTargetMatches(c->target_id, gMyId)) return;
      mah = c->value;
    }
    applyCapacityAndReboot(mah);
    break;
  }
  case NB_SET_CHARGE_MA:
  case NB_TARGET_CHARGE_MA: {
    uint16_t ma;
    if (h->type == NB_SET_CHARGE_MA) {
      if (it.len < (int)sizeof(NbSetU16)) return;
      ma = ((const NbSetU16 *)it.data)->value;
    } else {
      if (it.len < (int)sizeof(NbTargetU16)) return;
      const NbTargetU16 *c = (const NbTargetU16 *)it.data;
      if (!nbTargetMatches(c->target_id, gMyId)) return;
      ma = c->value;
    }
    applyChargeMa(ma);
    break;
  }
  case NB_SLEEP_FOR: {
    if (it.len < (int)sizeof(NbSetU16)) return;
    const NbSetU16 *c = (const NbSetU16 *)it.data;
    if (c->value > 0) enterTimedDeepSleep(c->value, "radio");
    break;
  }
  case NB_TARGET_SLEEP_FOR: {
    if (it.len < (int)sizeof(NbTargetU16)) return;
    const NbTargetU16 *c = (const NbTargetU16 *)it.data;
    if (memcmp(c->target_id, gMyId, 3) != 0) return; // never all-sleep by 00:00:00 here
    if (c->value > 0) enterTimedDeepSleep(c->value, "radio-target");
    break;
  }
  case NB_TARGET_SOLENOID: {
    if (it.len < (int)sizeof(NbTargetU16)) return;
    const NbTargetU16 *c = (const NbTargetU16 *)it.data;
    if (memcmp(c->target_id, gMyId, 3) != 0) return; // strikes are never broadcast
    // Lifecycle gate: daytime-surplus only (ADR 0030; dev relaxes surplus, not
    // night). Mechanism safety (clamp, rest, failsafe, arm) is in solenoidStrike.
    if (!behaviorStrikePermitted()) {
      Serial.println("solenoid: radio strike refused (lifecycle/power gate)");
      break;
    }
    solenoidStrike(c->value, "radio");
    break;
  }
  case NB_PROFILE: {
    if (it.len < (int)sizeof(NbProfile)) return;
    const NbProfile *p = (const NbProfile *)it.data;
    if (!nbTargetMatches(p->target_id, gMyId)) return;
    if (p->profile > PROFILE_PROD) return;
    if (p->flags & 0x01) {
      nvsPersistProfile(p->profile);
    } else {
      gCfg.profile = p->profile; // RAM-only until reboot
    }
    Serial.printf("profile -> %s (%s)\n",
                  p->profile == PROFILE_DEV ? "commission" : "field",
                  (p->flags & 0x01) ? "persisted" : "until reboot");
    break;
  }
  case NB_TIME_QUALITY: // reserved: defined, ignored until ADR 0031 lands
  default:
    break;
  }
}

void netPeerInit() {
  uint32_t now = millis();
  gNextShortMs = now + jittered(shortPeriodMs());
  gNextFullMs = now + jittered(fullPeriodMs());
}

void netPeerTick() {
  RxItem items[6];
  int n = espNowDrain(items, 6);
  for (int i = 0; i < n; i++) {
    if (items[i].len >= sizeof(NbHeader)) processPacket(items[i]);
    // A command can flip us into maintenance mid-drain; stop touching the radio.
    if (maintMode() == MODE_MAINT) return;
  }

  uint32_t now = millis();
  if ((int32_t)(now - gNextShortMs) >= 0) {
    netPeerSendHeartbeat(false);
    gNextShortMs = now + jittered(shortPeriodMs());
  }
  if ((int32_t)(now - gNextFullMs) >= 0) {
    netPeerSendHeartbeat(true);
    gNextFullMs = now + jittered(fullPeriodMs());
  }
  if (gLocateActive) {
    if ((int32_t)(now - gLocateUntilMs) >= 0) {
      gLocateActive = false;
    } else if ((int32_t)(now - gNextLocateMs) >= 0) {
      sendNeighborReports();
      // Independent jitter prevents a 60-node survey from phase-locking.
      uint32_t spread = gLocatePeriodMs / 4U;
      gNextLocateMs = now + gLocatePeriodMs - spread / 2U +
                      (spread ? esp_random() % spread : 0);
    }
  }
}
