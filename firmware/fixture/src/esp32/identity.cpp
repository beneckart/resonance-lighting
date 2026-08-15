#include "identity.h"

#include "esp_mac.h"

uint8_t gMyMac[6] = {0};
uint8_t gMyId[3] = {0};
String gShortId;

void identityInit() {
  esp_read_mac(gMyMac, ESP_MAC_WIFI_STA);
  gMyId[0] = gMyMac[3];
  gMyId[1] = gMyMac[4];
  gMyId[2] = gMyMac[5];
  char idb[7];
  snprintf(idb, sizeof(idb), "%02X%02X%02X", gMyId[0], gMyId[1], gMyId[2]);
  gShortId = String(idb);
  Serial.printf("node id=%s mac=%02X:%02X:%02X:%02X:%02X:%02X\n", idb, gMyMac[0],
                gMyMac[1], gMyMac[2], gMyMac[3], gMyMac[4], gMyMac[5]);
}

const char *resetReasonName(esp_reset_reason_t r) {
  switch (r) {
  case ESP_RST_POWERON: return "poweron";
  case ESP_RST_EXT: return "external";
  case ESP_RST_SW: return "software";
  case ESP_RST_PANIC: return "panic";
  case ESP_RST_INT_WDT: return "interrupt_watchdog";
  case ESP_RST_TASK_WDT: return "task_watchdog";
  case ESP_RST_WDT: return "other_watchdog";
  case ESP_RST_DEEPSLEEP: return "deepsleep";
  case ESP_RST_BROWNOUT: return "brownout";
  default: return "unknown";
  }
}
