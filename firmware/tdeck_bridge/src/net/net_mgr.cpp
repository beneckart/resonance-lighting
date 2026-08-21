#include "net_mgr.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <time.h>

#include "../store/store.h"
#include "espnow_link.h"

static NetState gState = NetState::OFF;
static uint32_t gConnectStartMs = 0;
static uint32_t gRetryAtMs = 0;
static uint8_t gApChannel = 0;
static char gIp[16] = "0.0.0.0";

#define NET_CONNECT_TIMEOUT_MS 15000
#define NET_RECONNECT_BACKOFF_MS 20000

static void pinMeshChannel() {
  // Leave the STA interface powered but unassociated; wifioff=true would make
  // esp_wifi_set_channel fail on Arduino-ESP32 3.x (cores3_bridge precedent).
  WiFi.disconnect(false, false);
  esp_wifi_set_channel(settings().channel, WIFI_SECOND_CHAN_NONE);
}

static void startConnect() {
  espnowDown();  // re-init after the channel settles (association hops channels)
  WiFi.begin(settings().ssid, settings().psk);
  gConnectStartMs = millis();
  gState = NetState::CONNECTING;
}

void netMgrBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // modem power-save drops ESP-NOW frames between DTIMs
  WiFi.setAutoReconnect(false);  // the guard owns reconnect policy
  if (storeHasWifi()) {
    startConnect();
  } else {
    pinMeshChannel();
    gState = NetState::OFF;
    espnowEnsureUp();
  }
}

void netMgrTick() {
  switch (gState) {
    case NetState::CONNECTING: {
      if (WiFi.status() == WL_CONNECTED) {
        gApChannel = (uint8_t)WiFi.channel();
        if (gApChannel != settings().channel) {
          // CHANNEL GUARD: the mesh is the primary function. Drop Wi-Fi,
          // re-pin, keep the mesh, and say so.
          Serial.printf("guard: AP on ch %u != mesh ch %u -> wifi DROPPED, mesh kept\n",
                        gApChannel, settings().channel);
          pinMeshChannel();
          gState = NetState::GUARD_BLOCKED;
        } else {
          strlcpy(gIp, WiFi.localIP().toString().c_str(), sizeof(gIp));
          configTime(0, 0, "pool.ntp.org", "time.google.com");
          Serial.printf("wifi up: %s ch=%u rssi=%d ip=%s\n", settings().ssid,
                        gApChannel, WiFi.RSSI(), gIp);
          gState = NetState::ONLINE;
        }
        espnowEnsureUp();
      } else if (millis() - gConnectStartMs > NET_CONNECT_TIMEOUT_MS) {
        Serial.println("wifi join timeout -> mesh-only, will retry");
        pinMeshChannel();
        espnowEnsureUp();
        gRetryAtMs = millis() + NET_RECONNECT_BACKOFF_MS;
        gState = NetState::OFF;
      }
      break;
    }
    case NetState::ONLINE: {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("wifi lost -> mesh-only, will retry");
        strlcpy(gIp, "0.0.0.0", sizeof(gIp));
        pinMeshChannel();
        espnowEnsureUp();
        gRetryAtMs = millis() + NET_RECONNECT_BACKOFF_MS;
        gState = NetState::OFF;
      }
      break;
    }
    case NetState::OFF: {
      if (storeHasWifi() && gRetryAtMs && (int32_t)(millis() - gRetryAtMs) >= 0) {
        gRetryAtMs = 0;
        startConnect();
      }
      break;
    }
    case NetState::GUARD_BLOCKED:
      break;  // deliberate: no auto-retry against a wrong-channel AP; fix the AP
  }
}

void netMgrRetry() {
  if (!storeHasWifi()) {
    Serial.println("no wifi credentials (use: set wifi <ssid> <psk>)");
    return;
  }
  startConnect();
}

void netMgrOff() {
  strlcpy(gIp, "0.0.0.0", sizeof(gIp));
  pinMeshChannel();
  espnowEnsureUp();
  gRetryAtMs = 0;
  gState = NetState::OFF;
}

NetState netState() { return gState; }

const char *netStateName() {
  switch (gState) {
    case NetState::OFF: return "mesh-only";
    case NetState::CONNECTING: return "connecting";
    case NetState::GUARD_BLOCKED: return "GUARD:ch-mismatch";
    case NetState::ONLINE: return "online";
  }
  return "?";
}

int netRssi() { return gState == NetState::ONLINE ? WiFi.RSSI() : 0; }
const char *netIp() { return gIp; }
uint8_t netApChannel() { return gApChannel; }

bool netSntpSynced() {
  time_t now = time(nullptr);
  return now > 1600000000;  // any post-2020 wall clock counts as synced
}
