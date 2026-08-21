#include "mesh_tx.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_random.h>
#include <string.h>

#include "espnow_link.h"
#include "fixture/src/core/packet.h"

static uint8_t gMyId[3] = {};
static uint32_t gTxSeq = 0;
static const uint8_t kBcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void meshTxBegin() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(gMyId, mac + 3, sizeof(gMyId));  // fleet identity = last 3 MAC bytes
}

const uint8_t *meshMyId() { return gMyId; }
uint32_t meshTxSeq() { return gTxSeq; }
uint32_t meshTxSendOk() { return espnowSendOk(); }
uint32_t meshTxSendFail() { return espnowSendFail(); }

static void fillHeader(NbHeader *h, uint8_t type) {
  h->ver = NB_PROTO_VER;
  h->type = type;
  memcpy(h->src_id, gMyId, sizeof(gMyId));
  h->seq = gTxSeq++;
  h->uptime_ms = millis();
}

static void sendPacketRepeated(const void *packet, size_t len, uint8_t count,
                               uint16_t gapMs) {
  for (uint8_t i = 0; i < count; ++i) {
    esp_now_send(kBcast, (const uint8_t *)packet, len);
    if (i + 1 < count) delay(gapMs);
  }
}

void meshIdentify(const uint8_t target[3], uint8_t secs, uint8_t color,
                  uint8_t blink, uint8_t value) {
  NbIdentify cmd = {};
  fillHeader(&cmd.h, NB_IDENTIFY);
  memcpy(cmd.target_id, target, 3);
  cmd.secs = secs;
  cmd.color = color;
  cmd.blink = blink;
  cmd.value = value;
  sendPacketRepeated(&cmd, sizeof(cmd), 6, 8);
}

bool meshStrike(const uint8_t id[3], uint16_t pulseMs) {
  // Strikes are never broadcast — the fixture rejects 00:00:00 and so do we.
  static const uint8_t kAll[3] = {0, 0, 0};
  if (memcmp(id, kAll, 3) == 0) return false;
  if (pulseMs < 5) pulseMs = 5;
  if (pulseMs > 300) pulseMs = 300;
  NbTargetU16 cmd = {};
  fillHeader(&cmd.h, NB_TARGET_SOLENOID);
  memcpy(cmd.target_id, id, 3);
  cmd.value = pulseMs;
  sendPacketRepeated(&cmd, sizeof(cmd), 6, 8);
  return true;
}

void meshDirectFrame(const MeshDirectEntry *entries, uint8_t count,
                     uint8_t flags) {
  if (count == 0) return;
  if (count > NB_DIRECT_MAX_ENTRIES) count = NB_DIRECT_MAX_ENTRIES;
  NbDirectFrame frame = {};
  fillHeader(&frame.h, NB_DIRECT_FRAME);
  frame.flags = flags;
  frame.count = count;
  for (uint8_t i = 0; i < count; ++i) {
    memcpy(frame.entries[i].id, entries[i].id, 3);
    frame.entries[i].r = entries[i].r;
    frame.entries[i].g = entries[i].g;
    frame.entries[i].b = entries[i].b;
    frame.entries[i].w = entries[i].w;
  }
  // Wire length is 15 + 7*count — never sizeof (packet.h contract).
  size_t len = offsetof(NbDirectFrame, entries) + (size_t)count * sizeof(NbDirectEntry);
  esp_now_send(kBcast, (const uint8_t *)&frame, len);
}

void meshProgramLease(const uint8_t target[3], uint8_t programId,
                      uint16_t leaseS, uint8_t flags, const uint8_t params[8]) {
  NbProgramSet cmd = {};
  fillHeader(&cmd.h, NB_PROGRAM_SET);
  memcpy(cmd.target_id, target, 3);
  cmd.program_id = programId;
  cmd.lease_s = leaseS;
  cmd.seed = esp_random();
  cmd.flags = flags;
  if (params) memcpy(cmd.params, params, sizeof(cmd.params));
  sendPacketRepeated(&cmd, sizeof(cmd), 6, 8);
}
