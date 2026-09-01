#include "mesh_tx.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_random.h>
#include <freertos/semphr.h>
#include <string.h>

#include "espnow_link.h"
#include "../core/knock_event.h"
#include "../hal/hal_board.h"
#include "../store/store.h"
#include "fixture/src/core/fixture_context.h"
#include "fixture/src/core/packet.h"

static uint8_t gMyId[3] = {};
static uint32_t gTxSeq = 0;
static const uint8_t kBcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static SemaphoreHandle_t gTxMutex = nullptr;
static portMUX_TYPE gTxSeqMux = portMUX_INITIALIZER_UNLOCKED;

static NbForceLifecycle gLifecycleCampaign = {};
static bool gLifecycleCampaignActive = false;
static uint32_t gLifecycleCampaignUntilMs = 0;
static uint32_t gLifecycleCampaignNextMs = 0;
static uint32_t gLifecycleCampaignDurationMs = 0;
static portMUX_TYPE gLifecycleCampaignMux = portMUX_INITIALIZER_UNLOCKED;

static MaintenanceCampaign gMaintCampaign;
static portMUX_TYPE gMaintCampaignMux = portMUX_INITIALIZER_UNLOCKED;

static ProgramLeaseTracker gProgramLeaseTracker;
static portMUX_TYPE gProgramLeaseMux = portMUX_INITIALIZER_UNLOCKED;

static void fillHeader(NbHeader *h, uint8_t type);

void meshTxBegin() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(gMyId, mac + 3, sizeof(gMyId));  // fleet identity = last 3 MAC bytes
  if (!gTxMutex) gTxMutex = xSemaphoreCreateMutex();
  if (!gTxMutex) Serial.println("mesh-tx: mutex allocation FAILED");
}

const uint8_t *meshMyId() { return gMyId; }

uint32_t meshTxSeq() {
  portENTER_CRITICAL(&gTxSeqMux);
  uint32_t seq = gTxSeq;
  portEXIT_CRITICAL(&gTxSeqMux);
  return seq;
}

uint32_t meshTxSendOk() { return espnowSendOk(); }
uint32_t meshTxSendFail() { return espnowSendFail(); }

static bool txTake() {
  return gTxMutex && xSemaphoreTake(gTxMutex, portMAX_DELAY) == pdTRUE;
}

static void txGive() { xSemaphoreGive(gTxMutex); }

void meshTxTick() {
  uint32_t now = millis();
  NbTargetCmd maintPacket = {};
  bool sendMaint;
  portENTER_CRITICAL(&gMaintCampaignMux);
  sendMaint = gMaintCampaign.next(now, maintPacket.target_id);
  portEXIT_CRITICAL(&gMaintCampaignMux);
  if (sendMaint && txTake()) {
    fillHeader(&maintPacket.h, NB_TARGET_ENTER_MAINT);
    esp_now_send(kBcast, (const uint8_t *)&maintPacket,
                 sizeof(maintPacket));
    txGive();
  }

  NbForceLifecycle lifecyclePacket = {};
  bool sendLifecycle = false;
  portENTER_CRITICAL(&gLifecycleCampaignMux);
  if (gLifecycleCampaignActive &&
      (int32_t)(now - gLifecycleCampaignUntilMs) >= 0) {
    gLifecycleCampaignActive = false;
  } else if (gLifecycleCampaignActive &&
             (int32_t)(now - gLifecycleCampaignNextMs) >= 0) {
    lifecyclePacket = gLifecycleCampaign;
    gLifecycleCampaignNextMs = now + 2000;
    sendLifecycle = true;
  }
  portEXIT_CRITICAL(&gLifecycleCampaignMux);
  if (sendLifecycle && txTake()) {
    fillHeader(&lifecyclePacket.h, NB_FORCE_LIFECYCLE);
    esp_now_send(kBcast, (const uint8_t *)&lifecyclePacket,
                 sizeof(lifecyclePacket));
    txGive();
    // Covers a complete 300 s field sleep cadence with margin while adding only
    // 0.5 packet/s. The fresh header also makes every resend observable.
  }
}

static void fillHeader(NbHeader *h, uint8_t type) {
  h->ver = NB_PROTO_VER;
  h->type = type;
  memcpy(h->src_id, gMyId, sizeof(gMyId));
  portENTER_CRITICAL(&gTxSeqMux);
  h->seq = gTxSeq++;
  portEXIT_CRITICAL(&gTxSeqMux);
  h->uptime_ms = millis();
}

// Caller holds gTxMutex for the whole repeated burst so another task cannot
// interleave a different command between copies.
static void sendPacketRepeatedLocked(const void *packet, size_t len,
                                     uint8_t count, uint16_t gapMs) {
  for (uint8_t i = 0; i < count; ++i) {
    esp_now_send(kBcast, (const uint8_t *)packet, len);
    if (i + 1 < count) delay(gapMs);
  }
}

// Caller holds gTxMutex. Checkpoint before RF so a reboot cannot erase the
// intent or the exact mesh sequence of an availability-changing command.
static bool auditActionBeforeSend(uint8_t action, uint32_t value,
                                  const NbHeader &header,
                                  const uint8_t target[3], bool required) {
  GpsUtcObservation gps = halGpsUtc();
  uint32_t ageMs = gps.valid ? millis() - gps.receivedMs : UINT32_MAX;
  bool utcValid = gps.valid && ageMs <= 10000UL;
  uint32_t utcS =
      utcValid ? gps.utcS + (gps.subMs + ageMs) / 1000UL : 0;
  bool ok = storeRecordAction(action, value, header.seq, header.uptime_ms,
                              target, utcValid, utcS);
  if (!ok)
    Serial.printf("action-audit: persist FAILED for %s seq=%lu%s\n",
                  actionAuditName(action), (unsigned long)header.seq,
                  required ? "; command refused" : "; restorative send allowed");
  return ok || !required;
}

void meshIdentify(const uint8_t target[3], uint8_t secs, uint8_t color,
                  uint8_t blink, uint8_t value) {
  NbIdentify cmd = {};
  memcpy(cmd.target_id, target, 3);
  cmd.secs = secs;
  cmd.color = color;
  cmd.blink = blink;
  cmd.value = value;
  if (!txTake()) return;
  fillHeader(&cmd.h, NB_IDENTIFY);
  sendPacketRepeatedLocked(&cmd, sizeof(cmd), 6, 8);
  txGive();
}

bool meshStrike(const uint8_t id[3], uint16_t pulseMs) {
  // Strikes are never broadcast: the fixture rejects 00:00:00 and so do we.
  static const uint8_t kAll[3] = {0, 0, 0};
  if (memcmp(id, kAll, 3) == 0) return false;
  if (pulseMs < 5) pulseMs = 5;
  if (pulseMs > 300) pulseMs = 300;
  NbTargetU16 cmd = {};
  memcpy(cmd.target_id, id, 3);
  cmd.value = pulseMs;
  if (!txTake()) return false;
  fillHeader(&cmd.h, NB_TARGET_SOLENOID);
  sendPacketRepeatedLocked(&cmd, sizeof(cmd), 6, 8);
  txGive();
  return true;
}

bool meshStrikeBroadcast(uint16_t pulseMs, uint32_t fireInMs) {
  NbEvent event = {};
  uint32_t eventId = esp_random();
  if (!eventId) eventId = 1;
  if (!knockBuildBroadcastEvent(event, eventId, pulseMs, fireInMs)) return false;
  if (!txTake()) return false;
  fillHeader(&event.h, NB_EVENT);
  // One logical multicast event repeated for RF reliability. Later copies
  // carry less remaining delay, so a fixture that misses copy one still arms
  // for the original bridge deadline. Fixtures deduplicate by event_id.
  uint32_t fireAtMs = millis() + fireInMs;
  for (uint8_t i = 0; i < 6; ++i) {
    if (fireInMs) {
      int32_t remaining = (int32_t)(fireAtMs - millis());
      if (remaining <= 0) break; // never turn a missed deadline into a late strike
      event.fire_in_ms = (uint32_t)remaining;
    }
    esp_now_send(kBcast, (const uint8_t *)&event, sizeof(event));
    if (i + 1 < 6) delay(8);
  }
  txGive();
  return true;
}

void meshDirectFrame(const MeshDirectEntry *entries, uint8_t count,
                     uint8_t flags) {
  if (count == 0) return;
  if (count > NB_DIRECT_MAX_ENTRIES) count = NB_DIRECT_MAX_ENTRIES;
  NbDirectFrame frame = {};
  frame.flags = flags;
  frame.count = count;
  for (uint8_t i = 0; i < count; ++i) {
    memcpy(frame.entries[i].id, entries[i].id, 3);
    frame.entries[i].r = entries[i].r;
    frame.entries[i].g = entries[i].g;
    frame.entries[i].b = entries[i].b;
    frame.entries[i].w = entries[i].w;
  }
  // Wire length is 15 + 7*count; never send sizeof(frame).
  size_t len =
      offsetof(NbDirectFrame, entries) + (size_t)count * sizeof(NbDirectEntry);
  if (!txTake()) return;
  fillHeader(&frame.h, NB_DIRECT_FRAME);
  esp_now_send(kBcast, (const uint8_t *)&frame, len);
  txGive();
}

bool meshProgramLease(const uint8_t target[3], uint8_t programId,
                      uint16_t leaseS, uint8_t flags,
                      const uint8_t params[8]) {
  NbProgramSet cmd = {};
  memcpy(cmd.target_id, target, 3);
  cmd.program_id = programId;
  cmd.lease_s = leaseS;
  cmd.seed = esp_random();
  cmd.flags = flags;
  if (params) memcpy(cmd.params, params, sizeof(cmd.params));
  if (!txTake()) return false;
  fillHeader(&cmd.h, NB_PROGRAM_SET);
  static const uint8_t kAll[3] = {0, 0, 0};
  bool fleetWide = memcmp(target, kAll, sizeof(kAll)) == 0;
  uint8_t auditAction = ACTION_AUDIT_NONE;
  bool auditRequired = false;
  if (fleetWide && leaseS == 0) {
    auditAction = ACTION_AUDIT_RELEASE_ALL;
  } else if (fleetWide && programId == 4) {
    auditAction = ACTION_AUDIT_DARK_ALL;
    auditRequired = true;
  }
  if (auditAction != ACTION_AUDIT_NONE &&
      !auditActionBeforeSend(auditAction, leaseS, cmd.h, target,
                             auditRequired)) {
    txGive();
    return false;
  }
  sendPacketRepeatedLocked(&cmd, sizeof(cmd), 6, 8);
  txGive();
  portENTER_CRITICAL(&gProgramLeaseMux);
  gProgramLeaseTracker.note(target, programId, leaseS, millis());
  portEXIT_CRITICAL(&gProgramLeaseMux);
  return true;
}

ProgramLeaseActivity meshProgramActivity() {
  portENTER_CRITICAL(&gProgramLeaseMux);
  ProgramLeaseActivity activity = gProgramLeaseTracker.snapshot(millis());
  portEXIT_CRITICAL(&gProgramLeaseMux);
  return activity;
}

bool meshStopTrackedProgramActivity() {
  ProgramLeaseActivity activity = meshProgramActivity();
  if (!activity.active) return false;
  return meshProgramLease(activity.target, 0, 0, 0x01, nullptr);
}

bool meshSleepAll(uint16_t seconds) {
  if (seconds == 0) return false;
  NbSetU16 cmd = {};
  cmd.value = seconds;
  // Existing fleet convention for a broadcast command. Receivers cut both
  // rails and enter timer deep sleep on the first copy they hear.
  if (!txTake()) return false;
  fillHeader(&cmd.h, NB_SLEEP_FOR);
  static const uint8_t kAll[3] = {0, 0, 0};
  if (!auditActionBeforeSend(ACTION_AUDIT_SLEEP_ALL, seconds, cmd.h, kAll,
                             true)) {
    txGive();
    return false;
  }
  sendPacketRepeatedLocked(&cmd, sizeof(cmd), 4, 5);
  txGive();
  return true;
}

static bool forceLifecycleFor(uint8_t mode, uint32_t campaignDurationMs) {
  if (mode > 2) return false;
  NbForceLifecycle packet = {};
  packet.mode = mode;
  if (!txTake()) return false;
  fillHeader(&packet.h, NB_FORCE_LIFECYCLE);
  static const uint8_t kAll[3] = {0, 0, 0};
  uint8_t action = mode == 0 ? ACTION_AUDIT_FORCE_DAY
                             : (mode == 1 ? ACTION_AUDIT_FORCE_NIGHT
                                          : ACTION_AUDIT_FORCE_AUTO);
  if (!auditActionBeforeSend(action, mode, packet.h, kAll, true)) {
    txGive();
    return false;
  }
  sendPacketRepeatedLocked(&packet, sizeof(packet), 4, 5);
  txGive();

  uint32_t now = millis();
  portENTER_CRITICAL(&gLifecycleCampaignMux);
  gLifecycleCampaign = packet;
  gLifecycleCampaignActive = true;
  gLifecycleCampaignNextMs = now + 2000;
  gLifecycleCampaignUntilMs = now + campaignDurationMs;
  gLifecycleCampaignDurationMs = campaignDurationMs;
  portEXIT_CRITICAL(&gLifecycleCampaignMux);
  return true;
}

bool meshForceLifecycle(uint8_t mode) {
  return forceLifecycleFor(mode, 360000UL);
}

bool meshPerformanceHold() {
  return forceLifecycleFor(0, 3600000UL);
}

MeshLifecycleCampaignStatus meshLifecycleCampaignStatus() {
  MeshLifecycleCampaignStatus status = {};
  uint32_t now = millis();
  portENTER_CRITICAL(&gLifecycleCampaignMux);
  if (gLifecycleCampaignActive &&
      (int32_t)(now - gLifecycleCampaignUntilMs) < 0) {
    status.active = true;
    status.mode = gLifecycleCampaign.mode;
    status.durationMs = gLifecycleCampaignDurationMs;
    status.remainingMs = gLifecycleCampaignUntilMs - now;
  }
  portEXIT_CRITICAL(&gLifecycleCampaignMux);
  return status;
}

bool meshEnterMaintenance(const uint8_t target[3]) {
  static const uint8_t kAll[3] = {0, 0, 0};
  if (!target || memcmp(target, kAll, sizeof(kAll)) == 0) return false;

  uint32_t now = millis();
  uint32_t jobId = now | 0x80000000UL;
  if (jobId == 0) jobId = 0x80000001UL;
  bool ok = false;
  portENTER_CRITICAL(&gMaintCampaignMux);
  MaintenanceCampaignStatus current = gMaintCampaign.status(now);
  // A legacy one-target request must never replace an explicit fleet gather.
  if (current.phase != MAINT_CAMPAIGN_GATHER &&
      gMaintCampaign.begin(jobId, 35000UL, now)) {
    ok = gMaintCampaign.add(jobId, target);
  }
  portEXIT_CRITICAL(&gMaintCampaignMux);
  return ok;
}

bool meshProfile(const uint8_t target[3], uint8_t profile, bool persist) {
  static const uint8_t kAll[3] = {0, 0, 0};
  if (!target || memcmp(target, kAll, sizeof(kAll)) == 0 ||
      profile > PROFILE_PROD)
    return false;
  NbProfile packet = {};
  memcpy(packet.target_id, target, sizeof(packet.target_id));
  packet.profile = profile;
  packet.flags = persist ? 0x01 : 0;
  if (!txTake()) return false;
  fillHeader(&packet.h, NB_PROFILE);
  sendPacketRepeatedLocked(&packet, sizeof(packet), 6, 8);
  txGive();
  return true;
}

bool meshMaintenanceBegin(uint32_t jobId, uint16_t durationS) {
  if (durationS == 0 || durationS > 3600) return false;
  uint32_t now = millis();
  portENTER_CRITICAL(&gMaintCampaignMux);
  bool ok = gMaintCampaign.begin(jobId, (uint32_t)durationS * 1000UL, now);
  portEXIT_CRITICAL(&gMaintCampaignMux);
  return ok;
}

bool meshMaintenanceAdd(uint32_t jobId, const uint8_t target[3]) {
  portENTER_CRITICAL(&gMaintCampaignMux);
  bool ok = gMaintCampaign.add(jobId, target);
  portEXIT_CRITICAL(&gMaintCampaignMux);
  return ok;
}

bool meshMaintenanceFreeze(uint32_t jobId) {
  uint32_t now = millis();
  portENTER_CRITICAL(&gMaintCampaignMux);
  bool ok = gMaintCampaign.freeze(jobId, now);
  portEXIT_CRITICAL(&gMaintCampaignMux);
  return ok;
}

MaintenanceCampaignStatus meshMaintenanceStatus() {
  uint32_t now = millis();
  portENTER_CRITICAL(&gMaintCampaignMux);
  MaintenanceCampaignStatus status = gMaintCampaign.status(now);
  portEXIT_CRITICAL(&gMaintCampaignMux);
  return status;
}

void meshMaintenancePrintStatus() {
  MaintenanceCampaignStatus status = meshMaintenanceStatus();
  Serial.printf(
      "nb-maint job=%08lX phase=%u active=%u targets=%u dispatch=%lu "
      "remain=%lu cycle=%lu\n",
      (unsigned long)status.jobId, (unsigned)status.phase,
      status.phase == MAINT_CAMPAIGN_GATHER ? 1U : 0U,
      (unsigned)status.targetCount, (unsigned long)status.dispatchCount,
      (unsigned long)status.remainingMs, (unsigned long)status.cycleMs);
}

bool meshCommissionDefault(const uint8_t target[3], uint8_t mode,
                           bool persist) {
  static const uint8_t kAll[3] = {0, 0, 0};
  if (!target || memcmp(target, kAll, sizeof(kAll)) == 0 ||
      mode > COMMISSION_DEFAULT_DARK)
    return false;
  NbCommissionDefault packet = {};
  memcpy(packet.target_id, target, sizeof(packet.target_id));
  packet.mode = mode;
  packet.flags = persist ? 0x01 : 0;
  if (!txTake()) return false;
  fillHeader(&packet.h, NB_COMMISSION_DEFAULT);
  sendPacketRepeatedLocked(&packet, sizeof(packet), 6, 8);
  txGive();
  return true;
}

bool meshFieldTuning(uint8_t dayChimeChanceX256, uint8_t showSchedule,
                     uint16_t presenceSeedMinS,
                     uint16_t presenceRearmClearS, bool persist) {
  if (showSchedule > 1 || presenceSeedMinS < 10 ||
      presenceSeedMinS > 3600 || presenceRearmClearS < 1 ||
      presenceRearmClearS > 600)
    return false;
  NbFieldTuning packet = {};
  packet.flags = persist ? 0x01 : 0;
  packet.day_chime_chance_x256 = dayChimeChanceX256;
  packet.show_schedule = showSchedule;
  packet.presence_seed_min_s = presenceSeedMinS;
  packet.presence_rearm_clear_s = presenceRearmClearS;
  if (!txTake()) return false;
  fillHeader(&packet.h, NB_FIELD_TUNING);
  uint32_t auditValue = (uint32_t)dayChimeChanceX256 |
                        ((uint32_t)showSchedule << 8) |
                        ((uint32_t)presenceSeedMinS << 9) |
                        ((uint32_t)presenceRearmClearS << 21);
  static const uint8_t kAll[3] = {0, 0, 0};
  if (!auditActionBeforeSend(ACTION_AUDIT_FIELD_TUNING, auditValue,
                             packet.h, kAll, true)) {
    txGive();
    return false;
  }
  sendPacketRepeatedLocked(&packet, sizeof(packet), 6, 8);
  txGive();
  return true;
}

bool meshTimeQuality(uint32_t utcS, uint16_t subMs, uint16_t ageS,
                     uint16_t uncertaintyMs, uint16_t bootId) {
  NbTimeQuality q = {};
  q.utc_s = utcS;
  q.sub_ms = subMs;
  q.source = NB_TIME_GPS;
  q.hops = 0;
  q.age_s = ageS;
  q.uncert_ms = uncertaintyMs;
  q.boot_id = bootId;
  q.flags = NB_TIME_FLAG_VALID | NB_TIME_FLAG_DATE_VALID;
  if (!txTake()) return false;
  fillHeader(&q.h, NB_TIME_QUALITY);
  bool queued = esp_now_send(kBcast, (const uint8_t *)&q, sizeof(q)) == ESP_OK;
  txGive();
  return queued;
}
