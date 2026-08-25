#include "mesh_tx.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_random.h>
#include <freertos/semphr.h>
#include <string.h>

#include "espnow_link.h"
#include "../core/knock_event.h"
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
static portMUX_TYPE gLifecycleCampaignMux = portMUX_INITIALIZER_UNLOCKED;

static NbTargetCmd gMaintCampaign = {};
static bool gMaintCampaignActive = false;
static uint32_t gMaintCampaignUntilMs = 0;
static uint32_t gMaintCampaignNextMs = 0;
static portMUX_TYPE gMaintCampaignMux = portMUX_INITIALIZER_UNLOCKED;

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
  bool sendMaint = false;
  portENTER_CRITICAL(&gMaintCampaignMux);
  if (gMaintCampaignActive &&
      (int32_t)(now - gMaintCampaignUntilMs) >= 0) {
    gMaintCampaignActive = false;
  } else if (gMaintCampaignActive &&
             (int32_t)(now - gMaintCampaignNextMs) >= 0) {
    maintPacket = gMaintCampaign;
    gMaintCampaignNextMs = now + 100;
    sendMaint = true;
  }
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

void meshProgramLease(const uint8_t target[3], uint8_t programId,
                      uint16_t leaseS, uint8_t flags,
                      const uint8_t params[8]) {
  NbProgramSet cmd = {};
  memcpy(cmd.target_id, target, 3);
  cmd.program_id = programId;
  cmd.lease_s = leaseS;
  cmd.seed = esp_random();
  cmd.flags = flags;
  if (params) memcpy(cmd.params, params, sizeof(cmd.params));
  if (!txTake()) return;
  fillHeader(&cmd.h, NB_PROGRAM_SET);
  sendPacketRepeatedLocked(&cmd, sizeof(cmd), 6, 8);
  txGive();
}

bool meshSleepAll(uint16_t seconds) {
  if (seconds == 0) return false;
  NbSetU16 cmd = {};
  cmd.value = seconds;
  // Existing fleet convention for a broadcast command. Receivers cut both
  // rails and enter timer deep sleep on the first copy they hear.
  if (!txTake()) return false;
  fillHeader(&cmd.h, NB_SLEEP_FOR);
  sendPacketRepeatedLocked(&cmd, sizeof(cmd), 4, 5);
  txGive();
  return true;
}

void meshForceLifecycle(uint8_t mode) {
  if (mode > 2) return;
  NbForceLifecycle packet = {};
  packet.mode = mode;
  if (!txTake()) return;
  fillHeader(&packet.h, NB_FORCE_LIFECYCLE);
  sendPacketRepeatedLocked(&packet, sizeof(packet), 4, 5);
  txGive();

  uint32_t now = millis();
  portENTER_CRITICAL(&gLifecycleCampaignMux);
  gLifecycleCampaign = packet;
  gLifecycleCampaignActive = true;
  gLifecycleCampaignNextMs = now + 2000;
  gLifecycleCampaignUntilMs = now + 360000UL;
  portEXIT_CRITICAL(&gLifecycleCampaignMux);
}

bool meshEnterMaintenance(const uint8_t target[3]) {
  static const uint8_t kAll[3] = {0, 0, 0};
  if (!target || memcmp(target, kAll, sizeof(kAll)) == 0) return false;

  NbTargetCmd packet = {};
  memcpy(packet.target_id, target, sizeof(packet.target_id));
  uint32_t now = millis();
  portENTER_CRITICAL(&gMaintCampaignMux);
  gMaintCampaign = packet;
  gMaintCampaignActive = true;
  gMaintCampaignNextMs = now;
  gMaintCampaignUntilMs = now + 35000UL;
  portEXIT_CRITICAL(&gMaintCampaignMux);
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
