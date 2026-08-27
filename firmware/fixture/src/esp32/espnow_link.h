// ESP-NOW link: single unencrypted broadcast peer (the 150-node-scalable
// pattern -- encrypted peers cap at ~17), ISR-enqueue rx queue drained from the
// loop, jittered sends. Channel comes from NVS (must equal the maintenance AP
// channel or ESP-NOW silently dies).
#pragma once

#include <stdint.h>
#include "../core/packet.h"

struct RxItem {
  uint8_t mac[6];
  int8_t rssi;
  uint8_t len;
  uint32_t rx_ms; // callback receipt time; scheduled events do not use drain time
  uint8_t data[250]; // ESP-NOW payload ceiling; append-only heartbeat headroom
};
static_assert(sizeof(NbHeartbeat) <= sizeof(((RxItem *)0)->data),
              "heartbeat outgrew the rx buffer -- bump RxItem::data");

bool espNowInit();
void espNowDeinit();
bool espNowUp();

// Drain up to `maxItems` queued packets into `out`; returns count.
int espNowDrain(RxItem *out, int maxItems);

void fillHeader(NbHeader *h, uint8_t type);
bool espNowSendRaw(const void *data, size_t len);

uint32_t espNowSendOk();
uint32_t espNowSendFail();
uint32_t espNowLastRxMs(); // 0 = nothing heard since boot (diagnostics only)
uint32_t espNowLastControlRxMs(); // accepted operator command, not peer traffic
void espNowNoteControlRx();       // loop context after target/validity checks
