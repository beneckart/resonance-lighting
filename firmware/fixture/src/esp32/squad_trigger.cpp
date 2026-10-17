#include "squad_trigger.h"
#include "../core/squad_trigger.h"
#include "../core/chime_light.h"
#include "../core/decompression_session.h"
#include "../core/fixture_context.h"
#include "board_power.h"
#include "boot_guard_io.h"
#include "identity.h"
#include "maintenance.h"
#include "ota_verify.h"
#include "power_glue.h"
#include "protect_recovery.h"
#include "sensors/sensors.h"
#include "telemetry.h"
#include "decompression.h"
#include "esp_random.h"
#include "esp_timer.h"

struct TriggerPeer {
  uint8_t mac[6] = {};
  bool known=false, ready=false;
  uint32_t readyMs=0, requestSeq=0;
  char fw[25] = {};
};
static TriggerPeer gPeers[SQUAD_COUNT];
static DecompressionSession gSession;
static SquadCycle gCycle;
static SquadPresence gPresence;
static uint32_t gBoot=0, gNextLease=0, gVlReads=0, gVlMs=0;
static uint8_t gLeaseIndex=0;
static uint16_t gJobMask=0, gSentMask=0;
static uint8_t gJobCursor=0, gJobStep=0;
static ChimeLight gTriggerLight;
static NbCyclePress gPress;
static uint8_t gPressCopies=0;
static uint32_t gForwarded=0;
static uint32_t gDeadline=0, gEventId=0, gTxErrors=0, gRefused=0;
static const char *gLast="idle";
static bool gWakePending=false;
static SquadRadioRecovery gRadioRecovery;
static int32_t gLastRadioError=0;

// RES_SQUAD_TRIGGER_ANY: every perimeter light running this image is the
// trigger (swap perimeters freely; run one at a time, as each keeps its own
// cycle). Otherwise only the exact RES_SQUAD_TRIGGER_TARGET.
#ifndef RES_SQUAD_TRIGGER_ANY
#define RES_SQUAD_TRIGGER_ANY 0
#endif
bool squadTriggerEnabled() {
  if (RES_SQUAD_TRIGGER_ANY && gTelemetryFixtureClass == FIXTURE_PERIMETER) return true;
  return RES_SQUAD_TRIGGER_TARGET != 0UL &&
    (((uint32_t)gMyId[0]<<16)|((uint32_t)gMyId[1]<<8)|gMyId[2]) == RES_SQUAD_TRIGGER_TARGET;
}
static bool powerReady() {
  return squadTriggerEnabled() && maintMode()==MODE_COMMS && espNowUp() &&
    !otaVerifyPending() && !bootGuardParked() && !protectRecoveryOwnsOutputs() &&
    pfIsReady() && batteryPresent() && powerWakeSampleWindowComplete() &&
    (uint8_t)powerBudget().tier <= (uint8_t)LedTier::DIM;
}
bool squadTriggerActive() {
  return squadTriggerEnabled() && gSession.active(esp_timer_get_time());
}
bool squadTriggerSetMinMm(uint16_t mm) {
  return squadTriggerEnabled() && gPresence.setMinMm(mm);
}
bool squadTriggerSetMaxMm(uint16_t mm) {
  return squadTriggerEnabled() && gPresence.setNearMm(mm);
}
bool squadTriggerSetClearMm(uint16_t mm) {
  return squadTriggerEnabled() && gPresence.setClearMm(mm);
}
static void lease(unsigned i, uint8_t minutes) {
  if (!gBoot) gBoot=esp_random() | 1U;
  NbDecompressionControl c={};
  fillHeader(&c.h, NB_DECOMPRESSION_CONTROL);
  memcpy(c.target_id,SQUAD[i].id,3);
  c.controller_boot=gBoot; c.action=NB_DECOMP_SESSION; c.minutes=minutes;
  gPeers[i].requestSeq=c.h.seq;
  if (!espNowSendRaw(&c,sizeof(c))) ++gTxErrors;
}
bool squadTriggerSession(uint16_t minutes) {
  if (!squadTriggerEnabled() || minutes>120 || (minutes && !powerReady())) return false;
  gSession.set(esp_timer_get_time(),minutes);
  gJobMask=0; gPresence.invalidate(); gNextLease=0; gLeaseIndex=0;
  if (!minutes) {
    // Bounded local end; any missed release expires after the five-minute lease.
    for (unsigned i=0;i<SQUAD_COUNT;++i) { lease(i,0); gPeers[i].ready=false; }
  }
  gLast=minutes ? "gathering" : "ended";
  return true;
}
static uint16_t readyMask() {
  uint16_t mask=0;
  for (unsigned i=0;i<SQUAD_COUNT;++i)
    if (gPeers[i].known && gPeers[i].ready && millis()-gPeers[i].readyMs<12000)
      mask |= 1U<<i;
  return mask;
}
bool squadTriggerPress(const char *source) {
  if (!squadTriggerActive() || !powerReady() || gJobMask ||
      !gCycle.canFire(millis(),readyMask())) {
    ++gRefused; gLast="not ready / cooldown";
    Serial.printf("trigger: refused (%s), next=%u ready=%03x\n",source,gCycle.next+1,readyMask());
    return false;
  }
  gJobStep=gCycle.next;
  gJobMask=gCycle.targets() & readyMask(); gSentMask=0; gJobCursor=0;
  gDeadline=millis()+1000; gEventId=esp_random() | 1U;
  gLast="scheduled";
  Serial.printf("trigger: step=%u source=%s targets=%03x event=%08lx deadline_ms=%lu\n",
    gCycle.next+1,source,gJobMask,(unsigned long)gEventId,(unsigned long)gDeadline);
  // Consume the step before any transmission. Partial delivery cannot replay it.
  gCycle.sent(millis());
  chimeLightNoteStrikes(gTriggerLight, gCycle.count, millis());  // trigger perimeter flashes too
  return true;
}
void squadTriggerButton() {
  if (!squadTriggerEnabled()) return;
  if (!squadTriggerActive()) {
    bool ok=squadTriggerSession(60);
    Serial.printf("trigger: USER session %s; press again when ready\n",ok ? "started" : "refused");
  } else squadTriggerPress("USER");
}
void squadTriggerWake() { gWakePending=true; }
void squadTriggerOnPacket(const RxItem &it) {
  if (!squadTriggerEnabled() || millis()-it.rx_ms>500) return;
  const NbHeader &h=*(const NbHeader *)it.data;
  int i=squadIndex(h.src_id);
  if (i<0 || memcmp(h.src_id,it.mac+3,3)) return;
  TriggerPeer &p=gPeers[i];
  if (h.type==NB_HEARTBEAT && it.len>=sizeof(NbHeader)+11) {
    memcpy(p.mac,it.mac,6); p.known=true;
    if (NB_HAS_HB_FIELD(it.len,fw_rev)) {
      const NbHeartbeat &hb=*(const NbHeartbeat *)it.data;
      memcpy(p.fw,hb.fw_rev,24); p.fw[24]=0;
    }
  } else if (h.type==NB_DECOMPRESSION_RECEIPT && it.len==sizeof(NbDecompressionReceipt)) {
    const NbDecompressionReceipt &r=*(const NbDecompressionReceipt *)it.data;
    if (memcmp(r.request_source,gMyId,3) || r.controller_boot!=gBoot ||
        r.request_seq!=p.requestSeq || r.action!=NB_DECOMP_SESSION) return;
    memcpy(p.mac,it.mac,6); p.known=true;
    p.ready=r.status==NB_DECOMP_READY && r.lease_left_s>10;
    if (p.ready) p.readyMs=millis();
  }
}
void squadTriggerTick() {
  if (!squadTriggerEnabled()) return;
  if (gRadioRecovery.paused) {
    if (!gRadioRecovery.due(millis())) return;
    gRadioRecovery.resumed();
    if (!commsResumeRadio()) {
      gSession.set(esp_timer_get_time(),0); gLast="radio restart failed";
    }
  }
  if (gWakePending && powerReady()) {
    gWakePending=false;
    squadTriggerSession(60);
  }
  if (RES_SQUAD_TRIGGER_ANY && powerReady()) {
    // Event mode: the session starts by itself after boot/wake and renews while
    // the power budget allows (powerReady), instead of ending after an hour.
    uint64_t t=esp_timer_get_time();
    if (!gSession.active(t)) { squadTriggerSession(60); gLast="gathering (auto)"; }
    else if (gSession.remainingSeconds(t) < 600) gSession.set(t,60);
  }
  if (!squadTriggerActive() || !powerReady()) {
    gJobMask=0; gPresence.invalidate(); return;
  }
  uint32_t now=millis();
  if (espNowTxStalled()) {
    gLastRadioError=espNowLastTxError();
    Serial.printf("trigger: radio stalled err=%ld pending_ms=%lu ok=%lu fail=%lu; dropping pending strike\n",
      (long)gLastRadioError,(unsigned long)espNowTxPendingMs(),
      (unsigned long)espNowSendOk(),(unsigned long)espNowSendFail());
    gJobMask=0; gPresence.invalidate();
    for (auto &p:gPeers) p.ready=false;
    gLeaseIndex=0; gNextLease=0;
    if (!gRadioRecovery.start(now)) {
      gSession.set(esp_timer_get_time(),0); gLast="radio retry budget exhausted";
      return;
    }
    gLast="restarting radio";
    if (!commsPauseRadio()) {
      gRadioRecovery.resumed();gSession.set(esp_timer_get_time(),0);
      gLast="radio pause failed";
    }
    return;
  }
  if (gJobMask) {
    // Unicast is essential: legacy kind-2 receivers have no payload target.
    // Never broadcast this event, including as a fallback on a failed send.
    for (unsigned n=0;n<3 && gJobCursor<SQUAD_COUNT*3;++n) {
      if (!espNowTxAvailable()) break; // never stack a send on an unfinished TX
      unsigned i=gJobCursor++ % SQUAD_COUNT;
      if (!(gJobMask & (1U<<i))) continue;
      int32_t remaining=(int32_t)(gDeadline-millis());
      if (remaining<250) { gJobCursor=SQUAD_COUNT*3; break; }
      NbEvent event={}; fillHeader(&event.h,NB_EVENT);
      event.event_id=gEventId;
      event.fire_in_ms=remaining + squadStepOffsetMs(gJobStep, i);  // pair: the higher lantern a beat later
      event.kind=NB_EVENT_SOLENOID_STRIKE;
      event.params[NB_EVENT_STRIKE_PULSE_LO_OFFSET]=SQUAD[i].pulseMs;
      if (espNowSendTo(gPeers[i].mac,&event,sizeof(event))) gSentMask |= 1U<<i;
      else ++gTxErrors;
    }
    if (gJobCursor>=SQUAD_COUNT*3) {
      gLast=gSentMask==gJobMask ? "sent (sound unverified)" : "partial send";
      Serial.printf("trigger: queued=%03x requested=%03x; no execution receipt in legacy strike protocol\n",gSentMask,gJobMask);
      gJobMask=0;
    }
  } else if (espNowTxAvailable() && (int32_t)(now-gNextLease)>=0) {
    lease(gLeaseIndex,5);
    if (++gLeaseIndex==SQUAD_COUNT) { gLeaseIndex=0; gNextLease=now+3000; }
  }
  const SensorSnapshot &s=sensors();
  if (s.vlReads!=gVlReads) {
    gVlReads=s.vlReads; gVlMs=now;
    if (gPresence.sample(now,s.vlPresent && s.vlOk,s.vlZoneNearestMm) &&
        decompressionEventMode()==NB_MODE_PERIMETER_CYCLE) {  // lantern-dings mode: leases only
      if (RES_SQUAD_TRIGGER_ANY && SQUAD_TRIGGER_LANTERN>=0) {
        // One cycle owner: forward the approach to the trigger lantern.
        NbCyclePress press={}; fillHeader(&press.h,NB_CYCLE_PRESS);
        press.confirm=NB_CYCLE_PRESS_CONFIRM;
        gPressCopies=3; gPress=press; gLast="forwarded to trigger lantern";
        chimeLightNoteStrikes(gTriggerLight,++gForwarded,now);
      } else {
        squadTriggerPress("presence");
      }
    }
  } else if (now-gVlMs>1000) gPresence.invalidate();
  if (gPressCopies && espNowTxAvailable()) {  // copies share one seq; receiver dedupes
    if (!espNowSendRaw(&gPress,sizeof(gPress))) ++gTxErrors;
    --gPressCopies;
  }
}
bool squadTriggerFrame(FrameBuffer &f,uint8_t pixels) {
  if (!squadTriggerActive()) return false;
  frameClear(f); f.count=pixels;
  if (RES_SQUAD_TRIGGER_ANY) {
    // Same slow colour drift as the lanterns' perimeter frame; flashes on each step.
    static bool seeded=false;
    if (!seeded) {
      gTriggerLight.baseHue=chimeLightSeedHue(gMyId);
      chimeLightNoteStrikes(gTriggerLight,gCycle.count,millis());
      seeded=true;
    }
    uint8_t c[4];
    chimeLightRgbw(gTriggerLight,millis(),c);
    uint8_t flash[3];
    if (decompressionModeFlash(millis(),flash)) { c[0]=flash[0]; c[1]=flash[1]; c[2]=flash[2]; }
    for (uint16_t i=0;i<f.count;++i) {
      f.px[i][0]=(uint8_t)((uint16_t)c[0]*32/CHIME_LIGHT_AMBIENT);
      f.px[i][1]=(uint8_t)((uint16_t)c[1]*32/CHIME_LIGHT_AMBIENT);
      f.px[i][2]=(uint8_t)((uint16_t)c[2]*32/CHIME_LIGHT_AMBIENT);
    }
    return true;
  }
  // One low-current pixel: amber gathering, green next target ready, blue hit.
  if (gCycle.fired && millis()-gCycle.lastMs<1500) f.px[0][2]=48;
  else if (gCycle.canFire(millis(),readyMask())) f.px[0][1]=24;
  else { f.px[0][0]=24; f.px[0][1]=8; }
  return true;
}
void squadTriggerTelemetry(String &j) {
  if (!squadTriggerEnabled()) return;
  j+=",\"trigger_enabled\":true,\"trigger_left_s\":"+String(gSession.remainingSeconds(esp_timer_get_time()));
  j+=",\"trigger_next_step\":"+String(gCycle.next+1);
  j+=",\"trigger_count\":"+String((unsigned long)gCycle.count);
  j+=",\"trigger_ready_mask\":"+String(readyMask());
  j+=",\"trigger_presence_armed\":"+String(gPresence.armed ? "true" : "false");
  j+=",\"trigger_min_mm\":"+String(gPresence.minMm);
  j+=",\"trigger_max_mm\":"+String(gPresence.nearMm);
  j+=",\"trigger_clear_mm\":"+String(gPresence.clearMm);
  j+=",\"trigger_refused\":"+String((unsigned long)gRefused);
  j+=",\"trigger_tx_errors\":"+String((unsigned long)gTxErrors);
  j+=",\"trigger_radio_restarts\":"+String(gRadioRecovery.attempts);
  j+=",\"trigger_radio_last_error\":"+String((long)gLastRadioError);
  j+=",\"trigger_tx_pending_ms\":"+String((unsigned long)espNowTxPendingMs());
  j+=",\"trigger_tx_ok\":"+String((unsigned long)espNowSendOk());
  j+=",\"trigger_tx_fail\":"+String((unsigned long)espNowSendFail());
  j+=",\"trigger_last\":\""+String(gLast)+"\",\"trigger_peers\":[";
  for (unsigned i=0;i<SQUAD_COUNT;++i) {
    if (i) j+=',';
    char id[7]; snprintf(id,sizeof(id),"%02X%02X%02X",SQUAD[i].id[0],SQUAD[i].id[1],SQUAD[i].id[2]);
    j+="{\"id\":\""+String(id)+"\",\"fw\":\""+String(gPeers[i].fw)+"\"}";
  }
  j+=']';
}
