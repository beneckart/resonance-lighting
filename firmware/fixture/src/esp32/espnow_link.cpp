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
static uint32_t gLastControlRxMs = 0;

static void resetRxQueue() {
  if (gRxQueue) xQueueReset(gRxQueue);
}

static void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < (int)sizeof(NbHeader) || len > (int)sizeof(RxItem::data)) return;
  uint32_t now = millis();
  gLastRxMs = now;
  RxItem it = {};
  memcpy(it.mac, info->src_addr, 6);
  it.rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  it.len = (uint8_t)len;
  it.rx_ms = now;
  memcpy(it.data, data, len);
  BaseType_t hpw = pdFALSE;
  xQueueSendFromISR(gRxQueue, &it, &hpw); // recv cb is WiFi-task ctx; ISR-safe send is fine
}

static void onEspNowSend(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) gSendOk++;
  else gSendFail++;
}

bool espNowInit() {
  if (gUp) return true;
  if (!gRxQueue) gRxQueue = xQueueCreate(32, sizeof(RxItem));
  if (!gRxQueue) {
    Serial.println("esp-now rx queue allocation FAILED");
    return false;
  }

  // Maintenance tears ESP-NOW down while its loop-owned queue remains
  // allocated. Never replay packets received before that boundary: a stale
  // ENTER_MAINT would immediately pull a resumed fixture back out of comms.
  resetRxQueue();
  gLastRxMs = 0;
  gLastControlRxMs = 0;

  esp_err_t err = esp_wifi_set_channel(gCfg.channel, WIFI_SECOND_CHAN_NONE);
  if (err != ESP_OK) {
    Serial.printf("esp-now channel %u FAILED: %d\n", gCfg.channel, (int)err);
    return false;
  }
  err = esp_now_init();
  if (err != ESP_OK) {
    Serial.printf("esp_now_init FAILED: %d\n", (int)err);
    return false;
  }
  err = esp_now_register_recv_cb(onEspNowRecv);
  if (err != ESP_OK) {
    Serial.printf("esp-now recv callback FAILED: %d\n", (int)err);
    esp_now_deinit();
    return false;
  }
  err = esp_now_register_send_cb(onEspNowSend);
  if (err != ESP_OK) {
    Serial.printf("esp-now send callback FAILED: %d\n", (int)err);
    esp_now_unregister_recv_cb();
    esp_now_deinit();
    return false;
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BCAST, 6);
  peer.channel = gCfg.channel;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  err = esp_now_add_peer(&peer);
  if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
    Serial.printf("esp-now broadcast peer FAILED: %d\n", (int)err);
    esp_now_unregister_recv_cb();
    esp_now_unregister_send_cb();
    esp_now_deinit();
    return false;
  }
  gUp = true;
  Serial.printf("esp-now up, ch=%d, broadcast peer registered\n", gCfg.channel);
  return true;
}

void espNowDeinit() {
  if (gUp) {
    esp_now_unregister_recv_cb();
    esp_now_unregister_send_cb();
    esp_now_deinit();
  }
  gUp = false;
  gLastRxMs = 0;
  gLastControlRxMs = 0;
  resetRxQueue();
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
uint32_t espNowLastControlRxMs() { return gLastControlRxMs; }
void espNowNoteControlRx() { gLastControlRxMs = millis(); }
