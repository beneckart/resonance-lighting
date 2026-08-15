// Resonance reduced-access Atom Matrix solenoid clicker.
//
// The pressable 5x5 face sends one fixed, targeted, bounded solenoid command.
// It deliberately exposes no fleet maintenance, configuration, show, WiFi, or
// serial command surface. Target and pulse width are compile-time settings.

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "../fixture/src/core/packet.h"

#define ATOM_CLICKER_VERSION "atom-clicker-2026-08-09.1"

#ifndef NB_CHANNEL
#define NB_CHANNEL 11
#endif

#ifndef RES_CLICKER_TARGET_0
#define RES_CLICKER_TARGET_0 0x9E
#endif
#ifndef RES_CLICKER_TARGET_1
#define RES_CLICKER_TARGET_1 0x5B
#endif
#ifndef RES_CLICKER_TARGET_2
#define RES_CLICKER_TARGET_2 0x8C
#endif
#ifndef RES_CLICKER_PULSE_MS
#define RES_CLICKER_PULSE_MS 40
#endif

#if RES_CLICKER_PULSE_MS < 5 || RES_CLICKER_PULSE_MS > 300
#error "RES_CLICKER_PULSE_MS must be 5..300"
#endif

static constexpr uint8_t BUTTON_PIN = 39; // pressable Atom Matrix face, active LOW
static constexpr uint8_t PIXEL_PIN = 27;
static constexpr uint8_t PIXEL_COUNT = 25;
static constexpr uint8_t CENTER_PIXEL = 12;
static constexpr uint32_t DEBOUNCE_MS = 35;
static constexpr uint32_t MIN_CLICK_INTERVAL_MS = 1000;
static constexpr uint32_t TARGET_FRESH_MS = 15000;
static constexpr uint32_t SENT_FLASH_MS = 350;
static constexpr uint8_t SEND_COUNT = 6;
static constexpr uint16_t SEND_GAP_MS = 8;

static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF,
                                         0xFF, 0xFF, 0xFF};
static const uint8_t TARGET_ID[3] = {RES_CLICKER_TARGET_0,
                                     RES_CLICKER_TARGET_1,
                                     RES_CLICKER_TARGET_2};

Adafruit_NeoPixel pixels(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint8_t myId[3] = {};
uint32_t txSeq = 0;
bool radioReady = false;

bool buttonRawReleased = true;
bool buttonStableReleased = true;
bool buttonArmed = false;
uint32_t buttonChangedMs = 0;
uint32_t lastClickMs = 0;
uint32_t sentFlashUntilMs = 0;
uint32_t clickCount = 0;

volatile uint32_t sendOk = 0;
volatile uint32_t sendFail = 0;
volatile uint32_t targetHeardMs = 0;
volatile int8_t targetRssi = 0;

uint32_t rgb(uint8_t red, uint8_t green, uint8_t blue) {
  return pixels.Color(red, green, blue);
}

void showCenter(uint32_t color) {
  pixels.clear();
  pixels.setPixelColor(CENTER_PIXEL, color);
  pixels.show();
}

void showSolid(uint32_t color) {
  pixels.fill(color);
  pixels.show();
}

void fillHeader(NbHeader *header, uint8_t type) {
  header->ver = NB_PROTO_VER;
  header->type = type;
  memcpy(header->src_id, myId, sizeof(myId));
  header->seq = txSeq++;
  header->uptime_ms = millis();
}

void onEspNowSend(const esp_now_send_info_t *, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) ++sendOk;
  else ++sendFail;
}

void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < (int)sizeof(NbHeader)) return;
  const NbHeader *header = (const NbHeader *)data;
  if (header->ver != NB_PROTO_VER || header->type != NB_HEARTBEAT) return;
  if (memcmp(header->src_id, TARGET_ID, sizeof(TARGET_ID)) != 0) return;
  targetRssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  targetHeardMs = millis();
}

bool setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect(false, false);
  if (esp_wifi_set_channel(NB_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("esp_wifi_set_channel FAILED");
    return false;
  }
  if (esp_now_init() != ESP_OK) {
    Serial.println("esp_now_init FAILED");
    return false;
  }
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BROADCAST_MAC, sizeof(BROADCAST_MAC));
  peer.channel = NB_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_err_t result = esp_now_add_peer(&peer);
  if (result != ESP_OK && result != ESP_ERR_ESPNOW_EXIST) {
    Serial.printf("esp_now_add_peer FAILED: %d\n", (int)result);
    return false;
  }
  return true;
}

void sendStrike() {
  if (!radioReady) {
    showSolid(rgb(24, 0, 0));
    Serial.println("click refused: radio not ready");
    return;
  }

  NbTargetU16 command = {};
  fillHeader(&command.h, NB_TARGET_SOLENOID);
  memcpy(command.target_id, TARGET_ID, sizeof(TARGET_ID));
  command.value = RES_CLICKER_PULSE_MS;

  uint32_t okBefore = sendOk;
  uint32_t failBefore = sendFail;
  for (uint8_t i = 0; i < SEND_COUNT; ++i) {
    esp_now_send(BROADCAST_MAC, (const uint8_t *)&command, sizeof(command));
    if (i + 1 < SEND_COUNT) delay(SEND_GAP_MS);
  }

  ++clickCount;
  sentFlashUntilMs = millis() + SENT_FLASH_MS;
  showSolid(rgb(22, 10, 0)); // amber means sent, not acknowledged
  Serial.printf("click #%lu -> target %02X%02X%02X pulse=%u ms seq=%lu "
                "callbacks_before=%lu/%lu\n",
                (unsigned long)clickCount, TARGET_ID[0], TARGET_ID[1],
                TARGET_ID[2], (unsigned)RES_CLICKER_PULSE_MS,
                (unsigned long)command.h.seq, (unsigned long)okBefore,
                (unsigned long)failBefore);
}

void buttonTick() {
  bool released = digitalRead(BUTTON_PIN) == HIGH;
  uint32_t now = millis();
  if (released != buttonRawReleased) {
    buttonRawReleased = released;
    buttonChangedMs = now;
  }
  if (released == buttonStableReleased || now - buttonChangedMs < DEBOUNCE_MS)
    return;

  buttonStableReleased = released;
  if (released) {
    buttonArmed = true;
    return;
  }
  if (!buttonArmed || now - lastClickMs < MIN_CLICK_INTERVAL_MS) return;

  buttonArmed = false;
  lastClickMs = now;
  sendStrike();
}

void displayTick() {
  static uint32_t nextMs = 0;
  uint32_t now = millis();
  if ((int32_t)(now - nextMs) < 0) return;
  nextMs = now + 100;

  if (!radioReady) {
    showCenter(rgb(24, 0, 0));
  } else if ((int32_t)(sentFlashUntilMs - now) > 0) {
    showSolid(rgb(22, 10, 0));
  } else if (targetHeardMs != 0 && now - targetHeardMs <= TARGET_FRESH_MS) {
    showCenter(rgb(0, 18, 0));
  } else {
    showCenter(rgb(0, 0, 18));
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pixels.begin();
  pixels.setBrightness(32);
  pixels.clear();
  pixels.show();

  // GPIO39 is input-only and the Atom Matrix board supplies its button pull-up.
  pinMode(BUTTON_PIN, INPUT);
  delay(2);
  buttonRawReleased = digitalRead(BUTTON_PIN) == HIGH;
  buttonStableReleased = buttonRawReleased;
  buttonArmed = buttonStableReleased;
  buttonChangedMs = millis();
  lastClickMs = millis() - MIN_CLICK_INTERVAL_MS;

  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  memcpy(myId, mac + 3, sizeof(myId));

  Serial.println();
  Serial.println("=== Resonance " ATOM_CLICKER_VERSION " ===");
  Serial.printf("node=%02X%02X%02X target=%02X%02X%02X pulse=%u ms channel=%d\n",
                myId[0], myId[1], myId[2], TARGET_ID[0], TARGET_ID[1],
                TARGET_ID[2], (unsigned)RES_CLICKER_PULSE_MS, NB_CHANNEL);
  Serial.println("controls: press/release face only; no serial commands");

  radioReady = setupEspNow();
  Serial.printf("radio=%s; blue=waiting green=target-fresh amber=sent red=error\n",
                radioReady ? "ready" : "FAILED");
  displayTick();
}

void loop() {
  buttonTick();
  displayTick();
  delay(2);
}
