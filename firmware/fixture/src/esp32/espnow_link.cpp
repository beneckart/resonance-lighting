#include "espnow_link.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "identity.h"
#include "nvs_store.h"

static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static QueueHandle_t gRxQueue = nullptr;
static bool gUp = false;
static uint32_t gTxSeq = 0;
static volatile uint32_t gSendOk = 0, gSendFail = 0;
static volatile uint32_t gLastRxMs = 0;

static void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < (int)sizeof(NbHeader) || len > (int)sizeof(RxItem::data)) return;
  gLastRxMs = millis();
  RxItem it;
  memcpy(it.mac, info->src_addr, 6);
  it.rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  it.len = (uint8_t)len;
  memcpy(it.data, data, len);
  BaseType_t hpw = pdFALSE;
  xQueueSendFromISR(gRxQueue, &it, &hpw); // recv cb is WiFi-task ctx; ISR-safe send is fine
}

static void onEspNowSend(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) gSendOk++;
  else gSendFail++;
}

bool espNowInit() {
  if (!gRxQueue) gRxQueue = xQueueCreate(32, sizeof(RxItem));
  esp_wifi_set_channel(gCfg.channel, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    Serial.println("esp_now_init FAILED");
    return false;
  }
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BCAST, 6);
  peer.channel = gCfg.channel;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  gUp = true;
  Serial.printf("esp-now up, ch=%d, broadcast peer registered\n", gCfg.channel);
  return true;
}

void espNowDeinit() {
  if (!gUp) return;
  esp_now_unregister_recv_cb();
  esp_now_deinit();
  gUp = false;
}

bool espNowUp() { return gUp; }

int espNowDrain(RxItem *out, int maxItems) {
  if (!gRxQueue) return 0;
  int n = 0;
  while (n < maxItems && xQueueReceive(gRxQueue, &out[n], 0) == pdTRUE) n++;
  return n;
}

void fillHeader(NbHeader *h, uint8_t type) {
  h->ver = NB_PROTO_VER;
  h->type = type;
  memcpy(h->src_id, gMyId, 3);
  h->seq = gTxSeq++;
  h->uptime_ms = millis();
}

bool espNowSendRaw(const void *data, size_t len) {
  if (!gUp) return false;
  return esp_now_send(BCAST, (const uint8_t *)data, len) == ESP_OK;
}

uint32_t espNowSendOk() { return gSendOk; }
uint32_t espNowSendFail() { return gSendFail; }
uint32_t espNowLastRxMs() { return gLastRxMs; }
