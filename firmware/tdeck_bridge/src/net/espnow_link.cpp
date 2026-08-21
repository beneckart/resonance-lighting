#include "espnow_link.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#include "../core/rx_ring.h"
#include "fixture/src/core/packet.h"  // the wire contract — never forked

static bool gUp = false;
static volatile MeshStats gStats = {};
static RxRing gRing;
static bool gRingAttached = false;

void espnowAttachRing(RxItem *storage, uint32_t capPow2) {
  gRing.init(storage, capPow2);
  gRingAttached = true;
}

bool espnowRingPop(RxItem *out) { return gRingAttached && gRing.pop(out); }

uint32_t espnowRingDrops() { return gRingAttached ? gRing.drops() : 0; }

static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < (int)sizeof(NbHeader) || len > (int)sizeof(RxItem::data)) {
    ++((MeshStats &)gStats).dropped;
    return;
  }
  const NbHeader *h = (const NbHeader *)data;
  if (h->ver != NB_PROTO_VER) {
    ++((MeshStats &)gStats).dropped;
    return;
  }
  MeshStats &s = (MeshStats &)gStats;
  ++s.frames;
  s.lastFrameMs = millis();
  memcpy((void *)s.lastSrcId, h->src_id, 3);
  s.lastRssi = info->rx_ctrl ? (int8_t)info->rx_ctrl->rssi : 0;
  s.lastType = h->type;

  // Timestamp + copy + return: the callback runs in the WiFi task and must
  // stay this thin (design brief §4).
  if (gRingAttached) {
    RxItem item;
    item.ms = s.lastFrameMs;
    memcpy(item.mac, info->src_addr, sizeof(item.mac));
    item.rssi = s.lastRssi;
    item.len = (uint8_t)len;
    memcpy(item.data, data, len);
    gRing.push(item);
  }
}

bool espnowUp() { return gUp; }

static volatile uint32_t gSendOk = 0, gSendFail = 0;
static void onSend(const esp_now_send_info_t *, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) ++gSendOk;
  else ++gSendFail;
}
uint32_t espnowSendOk() { return gSendOk; }
uint32_t espnowSendFail() { return gSendFail; }

bool espnowEnsureUp() {
  if (gUp) return true;
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSend);

  // Broadcast peer on channel 0 = "current channel": correct both when
  // associated to the ch-11 AP and when sitting unassociated on 11.
  static const uint8_t kBcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, kBcast, sizeof(kBcast));
  peer.channel = 0;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_err_t r = esp_now_add_peer(&peer);
  if (r != ESP_OK && r != ESP_ERR_ESPNOW_EXIST) {
    esp_now_deinit();
    return false;
  }
  gUp = true;
  return true;
}

void espnowDown() {
  if (!gUp) return;
  esp_now_deinit();
  gUp = false;
}

MeshStats espnowStats() { return (MeshStats &)gStats; }
