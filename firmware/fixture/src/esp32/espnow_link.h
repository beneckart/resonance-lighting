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
  uint8_t data[192]; // headroom over the full heartbeat + fixture-era packets
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
uint32_t espNowLastRxMs(); // 0 = nothing heard since boot (bridge-hold input)
