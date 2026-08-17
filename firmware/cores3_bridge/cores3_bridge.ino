// Resonance CoreS3 desk bridge.
//
// Dedicated ESP-NOW <-> USB bridge for the production fixture fleet. This is
// intentionally separate from net_bench: the CoreS3 has no PowerFeather power
// stack, and M5Unified must initialize its AXP2101 before the display/power path
// is treated as ready.

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#include <stdarg.h>
#include <string.h>

#ifndef CORES3_CAMBIUM_MODE
#define CORES3_CAMBIUM_MODE 0
#endif

#ifndef CORES3_AUDIO_REACTIVE_MODE
#define CORES3_AUDIO_REACTIVE_MODE 0
#endif

#ifndef CORES3_AUDIO_MODULE
#define CORES3_AUDIO_MODULE 0
#endif

#if CORES3_CAMBIUM_MODE && CORES3_AUDIO_REACTIVE_MODE
#error "Cambium binary modem and audio-reactive modes are separate builds"
#endif

#if CORES3_AUDIO_MODULE && !CORES3_AUDIO_REACTIVE_MODE
#error "CORES3_AUDIO_MODULE requires CORES3_AUDIO_REACTIVE_MODE"
#endif

#if CORES3_AUDIO_MODULE
#include <M5Module_Audio.h>
#endif

#include "../fixture/src/core/fixture_context.h"
#include "../fixture/src/core/packet.h"
#include "audio_reactive.h"
#include "cobs.h"

#define CORES3_BRIDGE_VERSION "cores3-bridge-2026-08-16.1"

#define CORES3_CAMBIUM_FW "cores3-cb-0.1"

#ifndef NB_CHANNEL
#define NB_CHANNEL 11
#endif

#ifndef NB_MAX_TRACKED
#define NB_MAX_TRACKED 192
#endif

#ifndef NB_BRIDGE_HZ
#define NB_BRIDGE_HZ 1
#endif

#ifndef NB_WDT_S
#define NB_WDT_S 8
#endif

#define NB_MAINTAIN_MIN_V10 46
#define NB_MAINTAIN_MAX_V10 168
#define NB_CAPACITY_MIN_MAH 100
#define NB_CAPACITY_MAX_MAH 30000
#define NB_CHARGE_MIN_MA 40
#define NB_CHARGE_MAX_MA 2000
#define NB_SOLENOID_MIN_MS 5
#define NB_SOLENOID_MAX_MS 300
#define NB_SOLENOID_DEFAULT_MS 40
#define NB_REMOTE_SLEEP_S 21600
#define NB_TARGET_SLEEP_DEFAULT_S 3600
#define NB_DRAWDOWN_DEFAULT_MAH 3500

static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t myMac[6] = {};
uint8_t myId[3] = {};
volatile uint32_t sendOk = 0;
volatile uint32_t sendFail = 0;
uint32_t txSeq = 0;
uint8_t gRateHz = 10;
bool radioReady = false;
M5Canvas displayCanvas(&M5.Display);
bool displayCanvasReady = false;

struct RxItem {
  uint8_t mac[6];
  int8_t rssi;
  uint8_t len;
  uint8_t data[250];
};

static_assert(sizeof(NbHeartbeat) <= sizeof(RxItem::data),
              "heartbeat outgrew the CoreS3 bridge receive buffer");

QueueHandle_t rxQueue = nullptr;
volatile uint32_t rxQueueDrops = 0;
struct PeerStat;

#if CORES3_CAMBIUM_MODE
// Cambium serial contract: COBS([frame type][payload][CRC16 LE]) + 0x00.
// This mode deliberately emits no bare serial text; all diagnostics travel in
// LOG frames so a dashboard message cannot corrupt the binary stream.
static const uint8_t FTYPE_RADIO_TX = 0x01;
static const uint8_t FTYPE_RADIO_RX = 0x02;
static const uint8_t FTYPE_CTRL = 0x03;
static const uint8_t FTYPE_STATUS = 0x04;
static const uint8_t FTYPE_LOG = 0x05;
static const uint8_t CTRL_STATUS_REQ = 0x01;
static const uint8_t CTRL_SET_CHANNEL = 0x02;
static const uint8_t CTRL_REBOOT = 0x03;
static const size_t ESPNOW_MAX = 250;
static const size_t RX_PAYLOAD_MAX = 6 + 1 + ESPNOW_MAX;
static const size_t BODY_MAX = 1 + RX_PAYLOAD_MAX + 2;

static volatile uint32_t cambiumRxPkts = 0;
static uint16_t cambiumCrcErr = 0;
static uint8_t cambiumChannel = NB_CHANNEL;

struct __attribute__((packed)) CambiumBridgeStatus {
  uint8_t proto;
  uint8_t mac[6];
  uint8_t channel;
  uint32_t uptime_ms;
  uint32_t tx_ok;
  uint32_t tx_fail;
  uint32_t rx_pkts;
  uint32_t rx_drop;
  uint16_t crc_err;
  char fw[16];
};
static_assert(sizeof(CambiumBridgeStatus) == 46,
              "Cambium STATUS layout drifted from framing.py");

static void sendCambiumFrame(uint8_t ftype, const uint8_t *payload, size_t len) {
  uint8_t body[BODY_MAX];
  uint8_t encoded[COBS_ENCODE_MAX(BODY_MAX)];
  if (len > BODY_MAX - 3) return;
  body[0] = ftype;
  memcpy(body + 1, payload, len);
  uint16_t crc = crc16_ccitt(body, len + 1);
  body[len + 1] = (uint8_t)(crc & 0xFF);
  body[len + 2] = (uint8_t)(crc >> 8);
  size_t encodedLen = cobs_encode(body, len + 3, encoded);
  Serial.write(encoded, encodedLen);
  Serial.write((uint8_t)0x00);
}

static void cambiumLogf(const char *fmt, ...) {
  char buffer[200];
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  if (len < 0) return;
  if (len > (int)sizeof(buffer) - 1) len = sizeof(buffer) - 1;
  sendCambiumFrame(FTYPE_LOG, (const uint8_t *)buffer, (size_t)len);
}

static void sendCambiumStatus() {
  CambiumBridgeStatus status = {};
  status.proto = 1;
  memcpy(status.mac, myMac, sizeof(status.mac));
  status.channel = cambiumChannel;
  status.uptime_ms = millis();
  status.tx_ok = sendOk;
  status.tx_fail = sendFail;
  status.rx_pkts = cambiumRxPkts;
  status.rx_drop = rxQueueDrops;
  status.crc_err = cambiumCrcErr;
  strncpy(status.fw, CORES3_CAMBIUM_FW, sizeof(status.fw));
  sendCambiumFrame(FTYPE_STATUS, (const uint8_t *)&status, sizeof(status));
}

static void handleCambiumFrame(uint8_t ftype, const uint8_t *payload, size_t len) {
  if (ftype == FTYPE_RADIO_TX) {
    if (len == 0 || len > ESPNOW_MAX) {
      cambiumLogf("RADIO_TX len=%u rejected", (unsigned)len);
      return;
    }
    if (esp_now_send(BCAST, payload, len) != ESP_OK) ++sendFail;
    return;
  }
  if (ftype != FTYPE_CTRL || len < 1) {
    cambiumLogf("unexpected ftype 0x%02x len=%u", ftype, (unsigned)len);
    return;
  }
  switch (payload[0]) {
  case CTRL_STATUS_REQ:
    sendCambiumStatus();
    break;
  case CTRL_SET_CHANNEL:
    if (len >= 2 && payload[1] >= 1 && payload[1] <= 14 &&
        esp_wifi_set_channel(payload[1], WIFI_SECOND_CHAN_NONE) == ESP_OK) {
      cambiumChannel = payload[1];
      cambiumLogf("channel -> %u", cambiumChannel);
    } else {
      cambiumLogf("SET_CHANNEL rejected");
    }
    break;
  case CTRL_REBOOT:
    cambiumLogf("rebooting");
    Serial.flush();
    ESP.restart();
    break;
  default:
    cambiumLogf("unknown CTRL cmd 0x%02x", payload[0]);
    break;
  }
}

static uint8_t cambiumSerialBuffer[512];
static size_t cambiumSerialLen = 0;

static void handleCambiumChunk(const uint8_t *chunk, size_t len) {
  uint8_t body[sizeof(cambiumSerialBuffer)];
  size_t bodyLen = 0;
  if (cobs_decode(chunk, len, body, &bodyLen) != 0 || bodyLen < 3) {
    ++cambiumCrcErr;
    return;
  }
  uint16_t expected = (uint16_t)body[bodyLen - 2] |
                      ((uint16_t)body[bodyLen - 1] << 8);
  if (crc16_ccitt(body, bodyLen - 2) != expected) {
    ++cambiumCrcErr;
    return;
  }
  handleCambiumFrame(body[0], body + 1, bodyLen - 3);
}

static void pumpCambiumSerial() {
  while (Serial.available() > 0) {
    int value = Serial.read();
    if (value < 0) break;
    if (value == 0) {
      if (cambiumSerialLen > 0) {
        handleCambiumChunk(cambiumSerialBuffer, cambiumSerialLen);
      }
      cambiumSerialLen = 0;
    } else if (cambiumSerialLen < sizeof(cambiumSerialBuffer)) {
      cambiumSerialBuffer[cambiumSerialLen++] = (uint8_t)value;
    } else {
      cambiumSerialLen = 0;
      ++cambiumCrcErr;
    }
  }
}

static void sendCambiumRadioRx(const RxItem &item) {
  uint8_t payload[RX_PAYLOAD_MAX];
  memcpy(payload, item.mac, sizeof(item.mac));
  payload[6] = (uint8_t)item.rssi;
  memcpy(payload + 7, item.data, item.len);
  sendCambiumFrame(FTYPE_RADIO_RX, payload, 7 + (size_t)item.len);
}
#endif

struct PeerStat {
  bool used;
  uint8_t id[3];
  uint32_t lastSeq;
  uint32_t recv;
  uint32_t gaps;
  int8_t rssi;
  uint32_t lastHeardMs;
  uint32_t uptimeMs;

  int16_t battMv;
  int16_t battMa;
  uint8_t soc;
  uint8_t resetReason;
  uint8_t caState;
  uint8_t mode;
  uint16_t dlPdrX1000;
  int8_t dlRssi;

  int16_t supplyMv;
  int16_t supplyMa;
  uint8_t supplyGood;

  bool hasEnv;
  uint32_t luxX10;
  uint16_t lightCh0;
  uint16_t lightCh1;
  int16_t panelTempCx10;
  uint8_t panelRhPct;
  int16_t battTempCx10;

  bool hasIna;
  int16_t inaPvMv;
  int16_t inaPaMa;
  int16_t inaBvMv;
  int16_t inaBaMa;

  bool hasConfig;
  uint16_t capacityMah;
  uint16_t chargeMa;

  bool hasDrawdown;
  uint16_t drawdownMahX10;
  uint16_t drawdownBudgetMah;
  uint8_t drawdownActive;

  bool hasFw;
  char fwRev[24];
  bool hasMaint;
  uint8_t maintStatus;

  bool hasField;
  uint8_t fieldPhase;
  uint8_t fieldReason;
  uint16_t fieldCycle;
  uint16_t fieldElapsedS;
  uint16_t fieldChargeMah;
  uint16_t fieldDischargeMah;
  uint16_t fieldMinMv;
  uint16_t fieldMaxMv;

  bool hasBq;
  uint16_t bqVindpmMv;
  uint16_t bqIchgMa;
  uint16_t bqVregMv;
  uint8_t bqReg16;
  uint8_t bqReg18;
  uint8_t bqStat0;
  uint8_t bqStat1;
  uint8_t bqFault0;
  uint8_t bqFlag0;
  uint8_t bqFlag1;
  uint8_t bqFaultFlag0;
  uint8_t bqPart;

  bool hasFieldSummary;
  uint16_t fieldChargeWhX10;
  uint16_t fieldDischargeWhX10;
  uint16_t fieldPeakPanelWX100;
  uint16_t fieldPeakChargeWX100;
  uint16_t fieldPeakDrawWX100;
  uint8_t fieldLowS;
  uint8_t fieldChargeMin;
  uint8_t fieldWaitMin;
  uint8_t fieldDrawMin;
  uint8_t fieldProtectMin;

  bool hasMppt;
  uint8_t mpptStatus;
  uint8_t mpptReason;
  uint8_t mpptRuns;
  uint8_t mpptActiveV10;
  uint8_t mpptBestV10;
  uint8_t mpptLastV10;
  uint16_t mpptP46WX100;
  uint16_t mpptP48WX100;
  uint16_t mpptP50WX100;

  bool hasFieldLatches;
  uint8_t fieldLoadDimmed;
  uint8_t fieldProtectLatched;

  bool hasFixtureState;
  uint8_t profile;
  uint8_t lifeState;
  uint8_t powerTier;
  uint8_t activeProgram;
  uint16_t nightMin;

  bool hasLedOutput;
  uint8_t fixtureClass;
  uint8_t ledRailOn;
  uint8_t ledR;
  uint8_t ledG;
  uint8_t ledB;
  uint8_t ledW;
  uint8_t ledLitPixels;

  bool hasIdentityRecovery;
  uint8_t sensorBits;
  uint8_t classMismatch;
  uint8_t recoveryState;
  uint16_t recoveryDetectMv;
};

PeerStat peers[NB_MAX_TRACKED] = {};

#if CORES3_AUDIO_REACTIVE_MODE
AudioEnvelope audioEnvelope;
bool audioActive = true;
bool audioInputReady = false;
bool audioUsingModule = false;
uint32_t audioFrames = 0;
uint32_t audioReadFailures = 0;
#if CORES3_AUDIO_MODULE
M5ModuleAudio audioModule;
#endif
#endif

struct MaintBurst {
  bool active;
  bool targeted;
  uint8_t target[3];
  uint32_t endMs;
  uint32_t nextSendMs;
};

MaintBurst maintBurst = {};

const char *resetReasonName(uint8_t raw) {
  switch ((esp_reset_reason_t)raw) {
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

void fillHeader(NbHeader *h, uint8_t type) {
  h->ver = NB_PROTO_VER;
  h->type = type;
  memcpy(h->src_id, myId, sizeof(myId));
  h->seq = txSeq++;
  h->uptime_ms = millis();
}

void sendPacketRepeated(const void *packet, size_t len, uint8_t count, uint16_t gapMs) {
  for (uint8_t i = 0; i < count; ++i) {
    esp_now_send(BCAST, (const uint8_t *)packet, len);
    if (i + 1 < count) delay(gapMs);
  }
}

void sendCmd(uint8_t type, uint8_t arg) {
  NbCmd cmd = {};
  fillHeader(&cmd.h, type);
  cmd.arg = arg;
  sendPacketRepeated(&cmd, sizeof(cmd), 4, 5);
}

void sendSetU16(uint8_t type, uint16_t value) {
  NbSetU16 cmd = {};
  fillHeader(&cmd.h, type);
  cmd.value = value;
  sendPacketRepeated(&cmd, sizeof(cmd), 4, 5);
}

void sendTargetU16(uint8_t type, const uint8_t target[3], uint16_t value) {
  NbTargetU16 cmd = {};
  fillHeader(&cmd.h, type);
  memcpy(cmd.target_id, target, 3);
  cmd.value = value;
  sendPacketRepeated(&cmd, sizeof(cmd), 6, 8);
}

void sendIdentify(const uint8_t target[3], uint8_t seconds,
                  uint8_t color = 0, uint8_t blink = 0,
                  uint8_t value = 255) {
  NbIdentify cmd = {};
  fillHeader(&cmd.h, NB_IDENTIFY);
  memcpy(cmd.target_id, target, 3);
  cmd.secs = seconds;
  cmd.color = color;
  cmd.blink = blink;
  cmd.value = value;
  sendPacketRepeated(&cmd, sizeof(cmd), 6, 8);
}

void sendProfile(const uint8_t target[3], uint8_t profile, bool persist) {
  NbProfile cmd = {};
  fillHeader(&cmd.h, NB_PROFILE);
  memcpy(cmd.target_id, target, 3);
  cmd.profile = profile;
  cmd.flags = persist ? 0x01 : 0x00;
  sendPacketRepeated(&cmd, sizeof(cmd), 6, 8);
}

#if CORES3_AUDIO_REACTIVE_MODE
const char *audioSourceName() {
  if (!audioInputReady) return "FAILED";
  return audioUsingModule ? "MODULE TRS" : "BUILTIN DUAL MIC";
}

bool setupAudioInput() {
#if CORES3_AUDIO_MODULE
  if (audioModule.begin(M5.In_I2C)) {
    // The LINE/MIC TRS jack is input 1. Keep the TRRS headset-mic path closed
    // because both routes share LIN1 on the module.
    audioModule.setHPMICStatus(AUDIO_MIC_CLOSE);
    audioModule.setMICStatus(AUDIO_MIC_OPEN);
    audioModule.setMicInputLine(ADC_INPUT_LINPUT1_RINPUT1);
    audioModule.setMicGain(MIC_GAIN_24DB);
    audioModule.setMicAdcVolume(100);
    audioModule.setBitsSample(ES_MODULE_ADC, BIT_LENGTH_16BITS);
    audioModule.setSampleRate(SAMPLE_RATE_44K);
    audioModule.setRGBBrightness(20);
    audioModule.setAllRGBLED(0x003000);
    audioUsingModule = true;
    Serial.println("audio: Module Audio LINE/MIC input ready (TRS, 24 dB)");
    return true;
  }
  Serial.println("audio: Module Audio not found; falling back to CoreS3 microphones");
#endif

  M5.Speaker.end();
  if (!M5.Mic.begin()) {
    Serial.println("audio: CoreS3 built-in microphone init FAILED");
    return false;
  }
  audioUsingModule = false;
  Serial.println("audio: CoreS3 built-in dual microphone ready");
  return true;
}

bool readAudioSamples(int16_t *samples, size_t count) {
  if (!audioInputReady || !samples || !count) return false;
#if CORES3_AUDIO_MODULE
  if (audioUsingModule)
    return audioModule.record((uint8_t *)samples,
                              (int)(count * sizeof(int16_t)));
#endif
  return M5.Mic.record(samples, count, 16000);
}

uint8_t collectLiveAudioIds(uint8_t ids[18][3]) {
  uint8_t count = 0;
  uint32_t now = millis();
  for (size_t i = 0; i < NB_MAX_TRACKED && count < 18; ++i) {
    if (!peers[i].used || now - peers[i].lastHeardMs > 5000) continue;
    // A nearby legacy net-bench peer still heartbeats on channel 11 but cannot
    // consume type-25 frames. Once its full heartbeat identifies that firmware,
    // keep it from stealing a stable color slot from a fixture.
    if (peers[i].hasFw &&
        strncmp(peers[i].fwRev, "fixture-", strlen("fixture-")) != 0)
      continue;
    memcpy(ids[count++], peers[i].id, 3);
  }
  // Stable colors independent of heartbeat arrival order.
  for (uint8_t i = 1; i < count; ++i) {
    uint8_t key[3];
    memcpy(key, ids[i], 3);
    int j = i - 1;
    while (j >= 0 && memcmp(ids[j], key, 3) > 0) {
      memcpy(ids[j + 1], ids[j], 3);
      --j;
    }
    memcpy(ids[j + 1], key, 3);
  }
  return count;
}

void sendAudioLevel(float level) {
  if (!radioReady) return;
  uint8_t ids[18][3];
  uint8_t count = collectLiveAudioIds(ids);
  if (!count) return;

  NbDirectFrame frame = {};
  fillHeader(&frame.h, NB_DIRECT_FRAME);
  frame.flags = 0x03; // 10 s micro-lease + hard-cut; envelope owns smoothing
  frame.count = count;
  for (uint8_t i = 0; i < count; ++i) {
    memcpy(frame.entries[i].id, ids[i], 3);
    AudioColor color = audioColorForSlot(i, level);
    frame.entries[i].r = color.r;
    frame.entries[i].g = color.g;
    frame.entries[i].b = color.b;
    frame.entries[i].w = color.w;
  }
  size_t wireLen = offsetof(NbDirectFrame, entries) +
                   count * sizeof(NbDirectEntry);
  if (esp_now_send(BCAST, (const uint8_t *)&frame, wireLen) != ESP_OK)
    ++sendFail;
  else
    ++audioFrames;
}

void setAudioActive(bool active) {
  if (active == audioActive) return;
  if (!active) sendAudioLevel(0.0f);
  audioActive = active;
  if (active) audioEnvelope = AudioEnvelope{};
  Serial.printf("audio reactive -> %s\n", active ? "ON" : "OFF");
}

void audioReactiveTick() {
  static uint32_t nextAudioMs = 0;
  uint32_t now = millis();
  if (!audioActive || (int32_t)(now - nextAudioMs) < 0) return;
  nextAudioMs = now + 100; // fixture render cap is 10 Hz

  static int16_t samples[512];
  if (!readAudioSamples(samples, sizeof(samples) / sizeof(samples[0]))) {
    ++audioReadFailures;
    return;
  }
  float level = audioEnvelope.update(samples,
                                     sizeof(samples) / sizeof(samples[0]));
  if (audioEnvelope.calibrated()) sendAudioLevel(level);
}
#endif

void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (!rxQueue || len <= 0 || len > (int)sizeof(RxItem::data)) return;
#if !CORES3_CAMBIUM_MODE
  if (len < (int)sizeof(NbHeader)) return;
  const NbHeader *h = (const NbHeader *)data;
  if (h->ver != NB_PROTO_VER ||
      (h->type != NB_HEARTBEAT && h->type != NB_SCANAP)) return;
#endif

  RxItem item = {};
  memcpy(item.mac, info->src_addr, sizeof(item.mac));
  item.rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  item.len = (uint8_t)len;
  memcpy(item.data, data, len);
  if (xQueueSend(rxQueue, &item, 0) != pdTRUE) ++rxQueueDrops;
#if CORES3_CAMBIUM_MODE
  else ++cambiumRxPkts;
#endif
}

void onEspNowSend(const esp_now_send_info_t *, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) ++sendOk;
  else ++sendFail;
}

bool setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  // Leave the STA interface powered but unassociated. Passing wifioff=true here
  // stops the radio and makes esp_wifi_set_channel fail on Arduino-ESP32 3.x.
  WiFi.disconnect(false, false);
  if (esp_wifi_set_channel(NB_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
#if CORES3_CAMBIUM_MODE
    cambiumLogf("esp_wifi_set_channel FAILED");
#else
    Serial.println("esp_wifi_set_channel FAILED");
#endif
    return false;
  }
  if (esp_now_init() != ESP_OK) {
#if CORES3_CAMBIUM_MODE
    cambiumLogf("esp_now_init FAILED");
#else
    Serial.println("esp_now_init FAILED");
#endif
    return false;
  }
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BCAST, sizeof(BCAST));
#if CORES3_CAMBIUM_MODE
  peer.channel = 0;
#else
  peer.channel = NB_CHANNEL;
#endif
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_err_t result = esp_now_add_peer(&peer);
  if (result != ESP_OK && result != ESP_ERR_ESPNOW_EXIST) {
#if CORES3_CAMBIUM_MODE
    cambiumLogf("esp_now_add_peer FAILED: %d", (int)result);
#else
    Serial.printf("esp_now_add_peer FAILED: %d\n", (int)result);
#endif
    return false;
  }
#if CORES3_CAMBIUM_MODE
  cambiumLogf("esp-now up, ch=%d", NB_CHANNEL);
#else
  Serial.printf("esp-now up, ch=%d, broadcast peer registered\n", NB_CHANNEL);
#endif
  return true;
}

PeerStat *findPeer(const uint8_t id[3], bool create) {
  for (size_t i = 0; i < NB_MAX_TRACKED; ++i) {
    if (peers[i].used && memcmp(peers[i].id, id, 3) == 0) return &peers[i];
  }
  if (!create) return nullptr;
  for (size_t i = 0; i < NB_MAX_TRACKED; ++i) {
    if (!peers[i].used) {
      memset(&peers[i], 0, sizeof(peers[i]));
      peers[i].used = true;
      memcpy(peers[i].id, id, 3);
      return &peers[i];
    }
  }
  return nullptr;
}

void accountHeartbeat(PeerStat *peer, const NbHeader *h) {
  bool senderRebooted = peer->recv && h->uptime_ms + 1000 < peer->uptimeMs;
  bool seqRestarted = peer->recv && h->seq < peer->lastSeq && peer->lastSeq - h->seq > 100;
  if (!peer->recv || senderRebooted || seqRestarted) {
    peer->lastSeq = h->seq;
    peer->recv = 1;
    peer->gaps = 0;
    return;
  }
  if (h->seq > peer->lastSeq) {
    peer->gaps += h->seq - peer->lastSeq - 1;
    peer->lastSeq = h->seq;
  }
  ++peer->recv;
}

void processHeartbeat(const RxItem &item) {
  if (item.len < (int)offsetof(NbHeartbeat, supply_mv)) return;
  const NbHeartbeat *hb = (const NbHeartbeat *)item.data;
  PeerStat *peer = findPeer(hb->h.src_id, true);
  if (!peer) return;

  accountHeartbeat(peer, &hb->h);
  peer->rssi = item.rssi;
  peer->lastHeardMs = millis();
  peer->uptimeMs = hb->h.uptime_ms;
  peer->battMv = hb->batt_mv;
  peer->battMa = hb->batt_ma;
  peer->soc = hb->soc_pct;
  peer->resetReason = hb->reset_reason;
  peer->caState = hb->ca_state;
  peer->mode = hb->mode;
  peer->dlPdrX1000 = hb->dl_pdr_x1000;
  peer->dlRssi = hb->dl_rssi;

  if (NB_HAS_HB_FIELD(item.len, supply_good)) {
    peer->supplyMv = hb->supply_mv;
    peer->supplyMa = hb->supply_ma;
    peer->supplyGood = hb->supply_good;
  } else {
    peer->supplyMv = 0;
    peer->supplyMa = 0;
    peer->supplyGood = 0;
  }

  peer->hasEnv = NB_HAS_HB_FIELD(item.len, btemp_cx10);
  if (peer->hasEnv) {
    peer->luxX10 = hb->lux_x10;
    peer->lightCh0 = hb->light_ch0;
    peer->lightCh1 = hb->light_ch1;
    peer->panelTempCx10 = hb->ptemp_cx10;
    peer->panelRhPct = hb->prh_pct;
    peer->battTempCx10 = hb->btemp_cx10;
  }

  peer->hasIna = NB_HAS_HB_FIELD(item.len, ina_ba_ma);
  if (peer->hasIna) {
    peer->inaPvMv = hb->ina_pv_mv;
    peer->inaPaMa = hb->ina_pa_ma;
    peer->inaBvMv = hb->ina_bv_mv;
    peer->inaBaMa = hb->ina_ba_ma;
  }

  peer->hasConfig = NB_HAS_HB_FIELD(item.len, cfg_charge_ma);
  if (peer->hasConfig) {
    peer->capacityMah = hb->cfg_cap_mah;
    peer->chargeMa = hb->cfg_charge_ma;
  }

  peer->hasDrawdown = NB_HAS_HB_FIELD(item.len, drawdown_active);
  if (peer->hasDrawdown) {
    peer->drawdownMahX10 = hb->drawdown_mah_x10;
    peer->drawdownBudgetMah = hb->drawdown_budget_mah;
    peer->drawdownActive = hb->drawdown_active;
  }

  if (NB_HAS_HB_FIELD(item.len, fw_rev)) {
    peer->hasFw = true;
    memcpy(peer->fwRev, hb->fw_rev, sizeof(peer->fwRev));
    peer->fwRev[sizeof(peer->fwRev) - 1] = '\0';
  }

  peer->hasMaint = NB_HAS_HB_FIELD(item.len, maint_status);
  if (peer->hasMaint) peer->maintStatus = hb->maint_status;

  peer->hasField = NB_HAS_HB_FIELD(item.len, field_max_mv);
  if (peer->hasField) {
    peer->fieldPhase = hb->field_phase;
    peer->fieldReason = hb->field_reason;
    peer->fieldCycle = hb->field_cycle;
    peer->fieldElapsedS = hb->field_elapsed_s;
    peer->fieldChargeMah = hb->field_charge_mah;
    peer->fieldDischargeMah = hb->field_discharge_mah;
    peer->fieldMinMv = hb->field_min_mv;
    peer->fieldMaxMv = hb->field_max_mv;
  }

  peer->hasBq = NB_HAS_HB_FIELD(item.len, bq_part);
  if (peer->hasBq) {
    peer->bqVindpmMv = hb->bq_vindpm_mv;
    peer->bqIchgMa = hb->bq_ichg_ma;
    peer->bqVregMv = hb->bq_vreg_mv;
    peer->bqReg16 = hb->bq_reg16;
    peer->bqReg18 = hb->bq_reg18;
    peer->bqStat0 = hb->bq_stat0;
    peer->bqStat1 = hb->bq_stat1;
    peer->bqFault0 = hb->bq_fault0;
    peer->bqFlag0 = hb->bq_flag0;
    peer->bqFlag1 = hb->bq_flag1;
    peer->bqFaultFlag0 = hb->bq_fault_flag0;
    peer->bqPart = hb->bq_part;
  }

  peer->hasFieldSummary = NB_HAS_HB_FIELD(item.len, field_protect_min);
  if (peer->hasFieldSummary) {
    peer->fieldChargeWhX10 = hb->field_charge_wh_x10;
    peer->fieldDischargeWhX10 = hb->field_discharge_wh_x10;
    peer->fieldPeakPanelWX100 = hb->field_peak_panel_w_x100;
    peer->fieldPeakChargeWX100 = hb->field_peak_charge_w_x100;
    peer->fieldPeakDrawWX100 = hb->field_peak_draw_w_x100;
    peer->fieldLowS = hb->field_low_s;
    peer->fieldChargeMin = hb->field_charge_min;
    peer->fieldWaitMin = hb->field_wait_min;
    peer->fieldDrawMin = hb->field_draw_min;
    peer->fieldProtectMin = hb->field_protect_min;
  }

  peer->hasMppt = NB_HAS_HB_FIELD(item.len, mppt_p50_w_x100);
  if (peer->hasMppt) {
    peer->mpptStatus = hb->mppt_status;
    peer->mpptReason = hb->mppt_reason;
    peer->mpptRuns = hb->mppt_runs;
    peer->mpptActiveV10 = hb->mppt_active_v10;
    peer->mpptBestV10 = hb->mppt_best_v10;
    peer->mpptLastV10 = hb->mppt_last_v10;
    peer->mpptP46WX100 = hb->mppt_p46_w_x100;
    peer->mpptP48WX100 = hb->mppt_p48_w_x100;
    peer->mpptP50WX100 = hb->mppt_p50_w_x100;
  }

  peer->hasFieldLatches = NB_HAS_HB_FIELD(item.len, field_protect_latched);
  if (peer->hasFieldLatches) {
    peer->fieldLoadDimmed = hb->field_load_dimmed;
    peer->fieldProtectLatched = hb->field_protect_latched;
  }

  peer->hasFixtureState = NB_HAS_HB_FIELD(item.len, night_min);
  if (peer->hasFixtureState) {
    peer->profile = hb->profile;
    peer->lifeState = hb->life_state;
    peer->powerTier = hb->power_tier;
    peer->activeProgram = hb->active_program;
    peer->nightMin = hb->night_min;
  }

  peer->hasLedOutput = NB_HAS_HB_FIELD(item.len, led_lit_pixels);
  if (peer->hasLedOutput) {
    peer->fixtureClass = hb->fixture_class;
    peer->ledRailOn = hb->led_rail_on;
    peer->ledR = hb->led_r;
    peer->ledG = hb->led_g;
    peer->ledB = hb->led_b;
    peer->ledW = hb->led_w;
    peer->ledLitPixels = hb->led_lit_pixels;
  }

  peer->hasIdentityRecovery = NB_HAS_HB_FIELD(item.len, recovery_detect_mv);
  if (peer->hasIdentityRecovery) {
    peer->sensorBits = hb->sensor_bits;
    peer->classMismatch = hb->class_mismatch;
    peer->recoveryState = hb->recovery_state;
    peer->recoveryDetectMv = hb->recovery_detect_mv;
  }
}

void emitScanAp(const RxItem &item) {
  if (item.len < sizeof(NbScanAp)) return;
  const NbScanAp *scan = (const NbScanAp *)item.data;
  char ssid[21] = {};
  memcpy(ssid, scan->ssid, 20);
  Serial.printf("nb-scanap from=%02X%02X%02X scan=%u idx=%u count=%u "
                "bssid=%02x:%02x:%02x:%02x:%02x:%02x ap_rssi=%d ch=%u enc=%u linkrssi=%d ssid=%s\n",
                scan->h.src_id[0], scan->h.src_id[1], scan->h.src_id[2],
                scan->scan_id, scan->idx, scan->count,
                scan->bssid[0], scan->bssid[1], scan->bssid[2], scan->bssid[3],
                scan->bssid[4], scan->bssid[5], scan->ap_rssi, scan->channel,
                scan->enc, item.rssi, ssid);
}

void processRx() {
  if (!rxQueue) return;
  RxItem item;
  while (xQueueReceive(rxQueue, &item, 0) == pdTRUE) {
#if CORES3_CAMBIUM_MODE
    sendCambiumRadioRx(item);
    if (item.len < sizeof(NbHeader)) continue;
#endif
    const NbHeader *h = (const NbHeader *)item.data;
    if (memcmp(h->src_id, myId, sizeof(myId)) == 0) continue;
    if (h->type == NB_HEARTBEAT) processHeartbeat(item);
#if !CORES3_CAMBIUM_MODE
    else if (h->type == NB_SCANAP) emitScanAp(item);
#endif
  }
}

int peerCount(bool liveOnly = false) {
  int count = 0;
  uint32_t now = millis();
  for (size_t i = 0; i < NB_MAX_TRACKED; ++i) {
    if (!peers[i].used) continue;
    if (liveOnly && now - peers[i].lastHeardMs > 5000) continue;
    ++count;
  }
  return count;
}

int16_t bridgeBatteryMv() {
  int16_t mv = M5.Power.getBatteryVoltage();
  return mv > 0 ? mv : 0;
}

void emitBridgeStats() {
  Serial.printf("nb-master id=%02X%02X%02X ch=%d frames=%lu sendok=%lu sendfail=%lu up=%lu bv=%.3f fw=%s\n",
                myId[0], myId[1], myId[2], NB_CHANNEL,
                (unsigned long)txSeq, (unsigned long)sendOk,
                (unsigned long)sendFail, (unsigned long)millis(),
                bridgeBatteryMv() / 1000.0f, CORES3_BRIDGE_VERSION);
#if CORES3_AUDIO_REACTIVE_MODE
  Serial.printf("audio source=%s ready=%d active=%d calibrated=%d rms=%.1f "
                "noise=%.1f level=%.3f frames=%lu readfail=%lu\n",
                audioSourceName(), audioInputReady, audioActive,
                audioEnvelope.calibrated(), audioEnvelope.rms,
                audioEnvelope.noise, audioEnvelope.level,
                (unsigned long)audioFrames,
                (unsigned long)audioReadFailures);
#endif

  char line[1024];
  for (size_t i = 0; i < NB_MAX_TRACKED; ++i) {
    PeerStat *p = &peers[i];
    if (!p->used) continue;
    uint32_t total = p->recv + p->gaps;
    float pdr = total ? (float)p->recv / (float)total : 0.0f;
    int n = snprintf(
        line, sizeof(line),
        "nb-peer id=%02X%02X%02X seq=%lu rx=%lu gaps=%lu pdr=%.4f rssi=%d bv=%.3f "
        "ima=%d soc=%d rr=%s ca=%d mode=%d dlpdr=%.3f dlrssi=%d up=%lu age=%lu "
        "sv=%.3f sma=%d sgood=%d",
        p->id[0], p->id[1], p->id[2], (unsigned long)p->lastSeq,
        (unsigned long)p->recv, (unsigned long)p->gaps, pdr, p->rssi,
        p->battMv / 1000.0f, p->battMa, p->soc == 255 ? -1 : p->soc,
        resetReasonName(p->resetReason), p->caState, p->mode,
        p->dlPdrX1000 / 1000.0f, p->dlRssi, (unsigned long)p->uptimeMs,
        (unsigned long)(millis() - p->lastHeardMs), p->supplyMv / 1000.0f,
        p->supplyMa, p->supplyGood);

    if (p->hasEnv && n < (int)sizeof(line)) {
      char lux[16];
      char panelTemp[12];
      char battTemp[12];
      if (p->luxX10 == 0xFFFFFFFF) snprintf(lux, sizeof(lux), "nan");
      else if (p->luxX10 == 0xFFFFFFFE) snprintf(lux, sizeof(lux), "sat");
      else snprintf(lux, sizeof(lux), "%.1f", p->luxX10 / 10.0f);
      if (p->panelTempCx10 == INT16_MIN) snprintf(panelTemp, sizeof(panelTemp), "nan");
      else snprintf(panelTemp, sizeof(panelTemp), "%.1f", p->panelTempCx10 / 10.0f);
      if (p->battTempCx10 == INT16_MIN) snprintf(battTemp, sizeof(battTemp), "nan");
      else snprintf(battTemp, sizeof(battTemp), "%.1f", p->battTempCx10 / 10.0f);
      n += snprintf(line + n, sizeof(line) - n,
                    " lux=%s ch0=%u ch1=%u ptc=%s prh=%d btc=%s",
                    lux, p->lightCh0, p->lightCh1, panelTemp,
                    p->panelRhPct == 255 ? -1 : p->panelRhPct, battTemp);
    }
    if (p->hasIna && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n, " ipv=%d ipa=%d ibv=%d iba=%d",
                    p->inaPvMv, p->inaPaMa, p->inaBvMv, p->inaBaMa);
    }
    if (p->hasConfig && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n, " cap=%u chg=%u",
                    p->capacityMah, p->chargeMa);
    }
    if (p->hasDrawdown && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n, " dd=%.1f ddb=%u dda=%u",
                    p->drawdownMahX10 / 10.0f, p->drawdownBudgetMah,
                    p->drawdownActive);
    }
    if (p->hasFw && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n, " fw=%s", p->fwRev);
    }
    if (p->hasMaint && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n, " mt=%u", p->maintStatus);
    }
    if (p->hasField && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n,
                    " fc=%u fcr=%u fcc=%u fce=%u fcchg=%u fcdis=%u fcmin=%u fcmax=%u",
                    p->fieldPhase, p->fieldReason, p->fieldCycle, p->fieldElapsedS,
                    p->fieldChargeMah, p->fieldDischargeMah, p->fieldMinMv,
                    p->fieldMaxMv);
    }
    if (p->hasBq && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n,
                    " bqv=%u bqichg=%u bqvreg=%u bq16=%02X bq18=%02X bq1d=%02X "
                    "bq1e=%02X bq1f=%02X bq20=%02X bq21=%02X bq22=%02X bq38=%02X",
                    p->bqVindpmMv, p->bqIchgMa, p->bqVregMv, p->bqReg16,
                    p->bqReg18, p->bqStat0, p->bqStat1, p->bqFault0, p->bqFlag0,
                    p->bqFlag1, p->bqFaultFlag0, p->bqPart);
    }
    if (p->hasFieldSummary && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n,
                    " fcwhc=%u fcwhd=%u fcpw=%u fcbw=%u fcdw=%u fclow=%u "
                    "fcmchg=%u fcmwait=%u fcmdraw=%u fcmprot=%u",
                    p->fieldChargeWhX10, p->fieldDischargeWhX10,
                    p->fieldPeakPanelWX100, p->fieldPeakChargeWX100,
                    p->fieldPeakDrawWX100, p->fieldLowS, p->fieldChargeMin,
                    p->fieldWaitMin, p->fieldDrawMin, p->fieldProtectMin);
    }
    if (p->hasMppt && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n,
                    " mppts=%u mpptr=%u mpptn=%u mpptv=%u mpptbest=%u mpptlast=%u "
                    "mppt46=%u mppt48=%u mppt50=%u",
                    p->mpptStatus, p->mpptReason, p->mpptRuns, p->mpptActiveV10,
                    p->mpptBestV10, p->mpptLastV10, p->mpptP46WX100,
                    p->mpptP48WX100, p->mpptP50WX100);
    }
    if (p->hasFieldLatches && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n, " fcdim=%u fclat=%u",
                    p->fieldLoadDimmed, p->fieldProtectLatched);
    }
    if (p->hasFixtureState && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n,
                    " prof=%u life=%u ptier=%u prog=%u nmin=%u",
                    p->profile, p->lifeState, p->powerTier,
                    p->activeProgram, p->nightMin);
    }
    if (p->hasLedOutput && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n,
                    " cls=%u ledrail=%u ledr=%u ledg=%u ledb=%u ledw=%u ledn=%u",
                    p->fixtureClass, p->ledRailOn, p->ledR, p->ledG,
                    p->ledB, p->ledW, p->ledLitPixels);
    }
    if (p->hasIdentityRecovery && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n,
                    " sens=%u cmis=%u rec=%u recmv=%u",
                    p->sensorBits, p->classMismatch, p->recoveryState,
                    p->recoveryDetectMv);
    }
    if (n < 0) continue;
    // snprintf returns the length it wanted to write. Clamp before appending a
    // newline so a future telemetry tail cannot turn truncation into an
    // out-of-bounds Serial.write.
    size_t outputLen = min((size_t)n, sizeof(line) - 2);
    line[outputLen++] = '\n';
    line[outputLen] = '\0';
    Serial.write((const uint8_t *)line, outputLen);
  }
}

int hexNibble(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

int readSerialUint(uint32_t windowMs = 70, int maxValue = 100000) {
  int value = -1;
  uint32_t deadline = millis() + windowMs;
  while ((int32_t)(deadline - millis()) > 0) {
    if (!Serial.available()) {
      delay(1);
      continue;
    }
    int next = Serial.peek();
    if (next < '0' || next > '9') break;
    Serial.read();
    value = (value < 0 ? 0 : value * 10) + next - '0';
    if (value > maxValue) break;
  }
  return value;
}

bool readSerialHexId(uint8_t out[3], uint32_t windowMs = 120) {
  uint8_t nibbles[6];
  uint8_t count = 0;
  uint32_t deadline = millis() + windowMs;
  while (count < 6 && (int32_t)(deadline - millis()) > 0) {
    if (!Serial.available()) {
      delay(1);
      continue;
    }
    int nibble = hexNibble(Serial.peek());
    if (nibble < 0) break;
    Serial.read();
    nibbles[count++] = (uint8_t)nibble;
  }
  if (count != 6) return false;
  out[0] = (nibbles[0] << 4) | nibbles[1];
  out[1] = (nibbles[2] << 4) | nibbles[3];
  out[2] = (nibbles[4] << 4) | nibbles[5];
  return true;
}

void consumeOptionalSeparator() {
  uint32_t deadline = millis() + 40;
  while ((int32_t)(deadline - millis()) > 0) {
    if (!Serial.available()) {
      delay(1);
      continue;
    }
    int next = Serial.peek();
    if (next == ':' || next == ',' || next == '=') Serial.read();
    return;
  }
}

size_t readSerialArg(char *out, size_t outLen, uint32_t windowMs = 90) {
  size_t count = 0;
  bool sawData = false;
  uint32_t deadline = millis() + windowMs;
  while ((int32_t)(deadline - millis()) > 0) {
    if (!Serial.available()) {
      delay(1);
      continue;
    }
    int next = Serial.peek();
    if (next == '\r' || next == '\n' || next == ' ' || next == '\t') {
      Serial.read();
      if (sawData) break;
      continue;
    }
    Serial.read();
    sawData = true;
    if (count + 1 < outLen) out[count++] = (char)next;
    deadline = millis() + 20;
  }
  if (outLen) out[count] = '\0';
  return count;
}

bool parseHexIdText(const char *text, uint8_t out[3]) {
  if (strlen(text) != 6) return false;
  uint8_t nibbles[6];
  for (int i = 0; i < 6; ++i) {
    int nibble = hexNibble(text[i]);
    if (nibble < 0) return false;
    nibbles[i] = (uint8_t)nibble;
  }
  out[0] = (nibbles[0] << 4) | nibbles[1];
  out[1] = (nibbles[2] << 4) | nibbles[3];
  out[2] = (nibbles[4] << 4) | nibbles[5];
  return true;
}

bool parseUint16Text(const char *text, uint16_t minValue, uint16_t maxValue,
                     uint16_t *out) {
  if (!text || !*text) return false;
  uint32_t value = 0;
  for (const char *p = text; *p; ++p) {
    if (*p < '0' || *p > '9') return false;
    value = value * 10 + (uint32_t)(*p - '0');
    if (value > maxValue) return false;
  }
  if (value < minValue) return false;
  *out = (uint16_t)value;
  return true;
}

bool parseTargetU16Arg(const char *arg, uint8_t target[3], bool *haveTarget,
                       uint16_t minValue, uint16_t maxValue, uint16_t *value) {
  *haveTarget = false;
  const char *separator = strchr(arg, ':');
  if (!separator) separator = strchr(arg, ',');
  if (!separator) separator = strchr(arg, '=');
  if (!separator) return parseUint16Text(arg, minValue, maxValue, value);
  if (separator - arg != 6) return false;
  char id[7] = {};
  memcpy(id, arg, 6);
  if (!parseHexIdText(id, target)) return false;
  if (!parseUint16Text(separator + 1, minValue, maxValue, value)) return false;
  *haveTarget = true;
  return true;
}

void startMaintBurst(const uint8_t *target) {
  maintBurst.active = true;
  maintBurst.targeted = target != nullptr;
  if (target) memcpy(maintBurst.target, target, 3);
  maintBurst.endMs = millis() + 35000;
  maintBurst.nextSendMs = 0;
}

void maintBurstTick() {
  if (!maintBurst.active) return;
  uint32_t now = millis();
  if ((int32_t)(now - maintBurst.endMs) >= 0) {
    maintBurst.active = false;
    Serial.println("sustained ENTER_MAINT done -> sweep for the peer + OTA");
    return;
  }
  if ((int32_t)(now - maintBurst.nextSendMs) < 0) return;
  maintBurst.nextSendMs = now + 100;
  if (maintBurst.targeted) {
    NbTargetCmd cmd = {};
    fillHeader(&cmd.h, NB_TARGET_ENTER_MAINT);
    memcpy(cmd.target_id, maintBurst.target, 3);
    esp_now_send(BCAST, (const uint8_t *)&cmd, sizeof(cmd));
  } else {
    NbCmd cmd = {};
    fillHeader(&cmd.h, NB_ENTER_MAINT);
    esp_now_send(BCAST, (const uint8_t *)&cmd, sizeof(cmd));
  }
}

void handleSerial() {
  if (!Serial.available()) return;
  char command = (char)Serial.read();
  static const uint8_t rates[] = {1, 2, 5, 10, 20, 50};
  static uint8_t rateIndex = 3;

  switch (command) {
  case 'U': {
    uint8_t target[3];
    bool targeted = readSerialHexId(target);
    startMaintBurst(targeted ? target : nullptr);
    if (targeted) {
      Serial.printf("sustained TARGET_ENTER_MAINT %02X%02X%02X 35s (nonblocking)\n",
                    target[0], target[1], target[2]);
    } else {
      Serial.println("sustained ENTER_MAINT 35s (nonblocking)");
    }
    break;
  }
  case 'c':
    sendCmd(NB_RESUME, 0);
    Serial.println("broadcast RESUME");
    break;
  case '+':
    if (rateIndex + 1 < sizeof(rates)) ++rateIndex;
    gRateHz = rates[rateIndex];
    sendCmd(NB_SET_RATE, gRateHz);
    Serial.printf("rate -> %u Hz\n", gRateHz);
    break;
  case '-':
    if (rateIndex) --rateIndex;
    gRateHz = rates[rateIndex];
    sendCmd(NB_SET_RATE, gRateHz);
    Serial.printf("rate -> %u Hz\n", gRateHz);
    break;
  case 'R': {
    int hz = readSerialUint(80, 100);
    if (hz < 1 || hz > 100) {
      Serial.println("SET_RATE rejected (range 1..100 Hz)");
      break;
    }
    gRateHz = (uint8_t)hz;
    sendCmd(NB_SET_RATE, gRateHz);
    Serial.printf("broadcast SET_RATE %u Hz\n", gRateHz);
    break;
  }
  case 'm': {
    static const uint8_t presets[] = {55, 52, 50, 48, 46};
    static uint8_t presetIndex = 0;
    int explicitV10 = readSerialUint(50, NB_MAINTAIN_MAX_V10);
    uint8_t v10;
    if (explicitV10 >= 0) {
      if (explicitV10 < NB_MAINTAIN_MIN_V10 || explicitV10 > NB_MAINTAIN_MAX_V10) {
        Serial.printf("SET_MAINTAIN %d rejected (range %d..%d)\n", explicitV10,
                      NB_MAINTAIN_MIN_V10, NB_MAINTAIN_MAX_V10);
        break;
      }
      v10 = (uint8_t)explicitV10;
    } else {
      v10 = presets[presetIndex++];
      presetIndex %= sizeof(presets);
    }
    sendCmd(NB_SET_MAINTAIN, v10);
    Serial.printf("broadcast SET_MAINTAIN %.1f V\n", v10 / 10.0f);
    break;
  }
  case 'C':
  case 'G':
  case 'K': {
    char arg[24];
    uint8_t target[3] = {};
    bool haveTarget = false;
    uint16_t value = 0;
    if (!readSerialArg(arg, sizeof(arg))) {
      Serial.println("missing command argument");
      break;
    }
    uint16_t minValue = command == 'C' ? NB_CAPACITY_MIN_MAH
                         : command == 'G' ? NB_CHARGE_MIN_MA
                                          : NB_SOLENOID_MIN_MS;
    uint16_t maxValue = command == 'C' ? NB_CAPACITY_MAX_MAH
                         : command == 'G' ? NB_CHARGE_MAX_MA
                                          : NB_SOLENOID_MAX_MS;
    if (!parseTargetU16Arg(arg, target, &haveTarget, minValue, maxValue, &value) ||
        (command == 'K' && !haveTarget)) {
      Serial.println("command argument rejected");
      break;
    }
    if (command == 'C') {
      if (haveTarget) sendTargetU16(NB_TARGET_CAPACITY, target, value);
      else sendSetU16(NB_SET_CAPACITY, value);
      Serial.printf("%s SET_CAPACITY %u mAh\n", haveTarget ? "target" : "broadcast", value);
    } else if (command == 'G') {
      if (haveTarget) sendTargetU16(NB_TARGET_CHARGE_MA, target, value);
      else sendSetU16(NB_SET_CHARGE_MA, value);
      Serial.printf("%s SET_CHARGE_MA %u mA\n", haveTarget ? "target" : "broadcast", value);
    } else {
      sendTargetU16(NB_TARGET_SOLENOID, target, value);
      Serial.printf("target SOLENOID %02X%02X%02X pulse=%ums\n",
                    target[0], target[1], target[2], value);
    }
    break;
  }
  case 'S': {
    int seconds = readSerialUint(80, 65535);
    uint16_t sleepS = seconds < 0 ? NB_REMOTE_SLEEP_S : (uint16_t)seconds;
    if (!sleepS) {
      Serial.println("SLEEP rejected");
      break;
    }
    sendSetU16(NB_SLEEP_FOR, sleepS);
    Serial.printf("broadcast SLEEP_FOR %us\n", sleepS);
    break;
  }
  case 'P': {
    uint8_t target[3];
    if (!readSerialHexId(target)) {
      Serial.println("PARK rejected: use P<id>[:seconds]");
      break;
    }
    consumeOptionalSeparator();
    int seconds = readSerialUint(80, 65535);
    uint16_t sleepS = seconds < 0 ? NB_TARGET_SLEEP_DEFAULT_S : (uint16_t)seconds;
    if (!sleepS) {
      Serial.println("PARK rejected");
      break;
    }
    sendTargetU16(NB_TARGET_SLEEP_FOR, target, sleepS);
    Serial.printf("target PARK %02X%02X%02X sleep=%us\n",
                  target[0], target[1], target[2], sleepS);
    break;
  }
  case 'D': {
    uint8_t target[3] = {};
    bool haveTarget = readSerialHexId(target, 100);
    if (haveTarget) consumeOptionalSeparator();
    int budget = readSerialUint(80, NB_CAPACITY_MAX_MAH);
    if (budget < 0) budget = NB_DRAWDOWN_DEFAULT_MAH;
    if (!haveTarget) {
      int known = 0;
      for (size_t i = 0; i < NB_MAX_TRACKED; ++i) {
        if (!peers[i].used) continue;
        memcpy(target, peers[i].id, 3);
        ++known;
      }
      if (known != 1) {
        Serial.printf("DRAWDOWN rejected: %d peers known; use D<id>[:mah]\n", known);
        break;
      }
    }
    sendTargetU16(NB_DRAWDOWN, target, (uint16_t)budget);
    Serial.printf("target DRAWDOWN %02X%02X%02X budget=%d mAh\n",
                  target[0], target[1], target[2], budget);
    break;
  }
  case 'i': {
    uint8_t target[3] = {};
    if (readSerialHexId(target, 120)) {
      consumeOptionalSeparator();
      int requested = readSerialUint(80, 255);
      uint8_t seconds = requested < 0 ? 60 : (uint8_t)requested;
      if (!seconds) {
        Serial.println("IDENTIFY rejected: use i<id>[:seconds], seconds 1..255");
        break;
      }
      sendIdentify(target, seconds);
      Serial.printf("identifying %02X%02X%02X for %us\n",
                    target[0], target[1], target[2], seconds);
      break;
    }
    static size_t nextIndex = 0;
    size_t found = NB_MAX_TRACKED;
    for (size_t offset = 0; offset < NB_MAX_TRACKED; ++offset) {
      size_t index = (nextIndex + offset) % NB_MAX_TRACKED;
      if (peers[index].used) {
        found = index;
        break;
      }
    }
    if (found == NB_MAX_TRACKED) {
      Serial.println("no peers to identify");
      break;
    }
    nextIndex = (found + 1) % NB_MAX_TRACKED;
    sendIdentify(peers[found].id, 8);
    Serial.printf("identifying %02X%02X%02X for 8s\n",
                  peers[found].id[0], peers[found].id[1], peers[found].id[2]);
    break;
  }
  case 'I': {
    uint8_t all[3] = {};
    sendIdentify(all, 8);
    Serial.println("identify ALL peers 8s");
    break;
  }
  case 'T': {
    uint8_t target[3] = {};
    if (!readSerialHexId(target, 120)) {
      Serial.println("TAG rejected: use T<id>:<0|1>");
      break;
    }
    consumeOptionalSeparator();
    int enabled = readSerialUint(80, 1);
    if (enabled < 0 || enabled > 1) {
      Serial.println("TAG rejected: use T<id>:<0|1>");
      break;
    }
    if (enabled) {
      sendIdentify(target, 255, 2, 0, 128); // steady green, half brightness
    } else {
      sendIdentify(target, 0, 0, 0, 255); // immediate release
    }
    Serial.printf("target TAG %02X%02X%02X -> %s\n",
                  target[0], target[1], target[2], enabled ? "ON" : "OFF");
    break;
  }
  case 'F': {
    char arg[24];
    uint8_t target[3] = {};
    bool haveTarget = false;
    uint16_t profile = 0;
    if (!readSerialArg(arg, sizeof(arg)) ||
        !parseTargetU16Arg(arg, target, &haveTarget, PROFILE_DEV, PROFILE_PROD,
                           &profile)) {
      Serial.println("PROFILE rejected: use F<0|1> or F<id>:<0|1>");
      break;
    }
    sendProfile(target, (uint8_t)profile, true);
    if (haveTarget) {
      Serial.printf("target PROFILE %02X%02X%02X -> %s (persisted)\n",
                    target[0], target[1], target[2],
                    profile == PROFILE_DEV ? "commission" : "field");
    } else {
      Serial.printf("broadcast PROFILE -> %s (persisted)\n",
                    profile == PROFILE_DEV ? "commission" : "field");
    }
    break;
  }
  case 't':
    Serial.printf("{\"bridge\":\"%02X%02X%02X\",\"channel\":%d,\"peers\":%d,\"live\":%d,\"queue_drops\":%lu}\n",
                  myId[0], myId[1], myId[2], NB_CHANNEL, peerCount(false),
                  peerCount(true), (unsigned long)rxQueueDrops);
    break;
  case 'r':
    Serial.printf("role=master mode=serial-bridge ch=%d rate=%uHz txseq=%lu sendok=%lu fail=%lu peers=%d live=%d queue_drops=%lu\n",
                  NB_CHANNEL, gRateHz, (unsigned long)txSeq,
                  (unsigned long)sendOk, (unsigned long)sendFail,
                  peerCount(false), peerCount(true), (unsigned long)rxQueueDrops);
    break;
#if CORES3_AUDIO_REACTIVE_MODE
  case 'A':
    setAudioActive(!audioActive);
    break;
#endif
  case 'h':
  case '?':
    Serial.println("commands: r t U[id] c +/- R<hz> i[id][:s] I F[id:]<0|1> m<v10> C[id:]mAh G[id:]mA K<id>:ms S[s] P<id>[:s] D[<id>][:mAh]"
#if CORES3_AUDIO_REACTIVE_MODE
                   " A"
#endif
    );
    break;
  default:
    break;
  }
}

void drawDisplay() {
  if (!displayCanvasReady) return;
  // Render the whole status page off-screen, then transfer the completed frame.
  // Clearing the physical LCD before each 1 Hz redraw caused a visible black
  // flash; clearing this PSRAM-backed canvas is invisible to the viewer.
  displayCanvas.fillSprite(TFT_BLACK);
  displayCanvas.setTextFont(1);
  displayCanvas.setTextSize(2);
  displayCanvas.setCursor(8, 6);
  displayCanvas.setTextColor(TFT_CYAN, TFT_BLACK);
#if CORES3_AUDIO_REACTIVE_MODE
  displayCanvas.println("RESONANCE AUDIO");
#else
  displayCanvas.println("RESONANCE BRIDGE");
#endif

  displayCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
  displayCanvas.printf("%02X%02X%02X  ESP-NOW ch %d\n", myId[0], myId[1], myId[2], NB_CHANNEL);
  displayCanvas.printf("radio %-4s  USB ready\n", radioReady ? "UP" : "FAIL");
  displayCanvas.printf("peers %d live / %d seen\n", peerCount(true), peerCount(false));

  int16_t batteryMv = bridgeBatteryMv();
  int batteryPct = M5.Power.getBatteryLevel();
  if (batteryMv > 0) {
    displayCanvas.printf("bridge %.3fV  %d%% %s\n", batteryMv / 1000.0f,
                         batteryPct, M5.Power.isCharging() ? "charging" : "");
  } else {
    displayCanvas.println("bridge battery: n/a");
  }
  displayCanvas.printf("tx ok/fail %lu/%lu drop %lu\n",
                       (unsigned long)sendOk, (unsigned long)sendFail,
                       (unsigned long)rxQueueDrops);

#if CORES3_AUDIO_REACTIVE_MODE
  displayCanvas.setTextColor(audioInputReady ? TFT_GREEN : TFT_RED, TFT_BLACK);
  displayCanvas.printf("audio %-16s %s\n", audioSourceName(),
                       audioActive ? "ON" : "OFF");
  displayCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
  displayCanvas.printf("rms %.0f floor %.0f level %3d%%\n",
                       audioEnvelope.rms, audioEnvelope.noise,
                       (int)(audioEnvelope.level * 100.0f + 0.5f));
  int barWidth = (int)(audioEnvelope.level * 300.0f);
  displayCanvas.drawRect(8, displayCanvas.getCursorY(), 304, 12, TFT_DARKGREY);
  if (barWidth > 0)
    displayCanvas.fillRect(10, displayCanvas.getCursorY() + 2,
                           barWidth, 8, TFT_MAGENTA);
  displayCanvas.setCursor(8, displayCanvas.getCursorY() + 16);
#endif

  displayCanvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  displayCanvas.println("live fixtures:");
  displayCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
  uint32_t now = millis();
  int rows = 0;
  const int rowLimit =
#if CORES3_AUDIO_REACTIVE_MODE
      3;
#else
      6;
#endif
  for (size_t i = 0; i < NB_MAX_TRACKED && rows < rowLimit; ++i) {
    const PeerStat &p = peers[i];
    if (!p.used || now - p.lastHeardMs > 5000) continue;
    displayCanvas.printf("%02X%02X%02X %4ddBm %1.2fV %3d%%\n",
                         p.id[0], p.id[1], p.id[2], p.rssi,
                         p.battMv / 1000.0f, p.soc == 255 ? -1 : p.soc);
    ++rows;
  }
  if (!rows) displayCanvas.println("(waiting for heartbeat)");
  displayCanvas.pushSprite(0, 0);
}

void setupWatchdog() {
  esp_task_wdt_config_t config = {};
  config.timeout_ms = NB_WDT_S * 1000;
  config.idle_core_mask = 0;
  config.trigger_panic = true;
  // Arduino-ESP32 normally initializes TWDT before setup(). Reconfigure first
  // to avoid an alarming (but harmless) "already initialized" boot log; fall
  // back to init for cores/configurations that do not pre-initialize it.
  esp_err_t result = esp_task_wdt_reconfigure(&config);
  if (result == ESP_ERR_INVALID_STATE) esp_task_wdt_init(&config);
  esp_task_wdt_add(nullptr);
}

void setup() {
  auto config = M5.config();
  config.serial_baudrate = 115200;
  config.clear_display = true;
  config.output_power = true;
  config.internal_imu = false;
  config.internal_rtc = false;
  config.internal_mic = CORES3_AUDIO_REACTIVE_MODE && !CORES3_AUDIO_MODULE;
  config.internal_spk = false;
  config.led_brightness = 24;
  M5.begin(config);

  delay(1200);
#if !CORES3_CAMBIUM_MODE
  Serial.println();
  Serial.println("=== Resonance net-bench " CORES3_BRIDGE_VERSION " ===");
  Serial.printf("role=master channel=%d frame_hz=0 hb_hz=0\n", NB_CHANNEL);
#if CORES3_AUDIO_REACTIVE_MODE
  Serial.println("mode: AUDIO REACTIVE (CoreS3; 10 Hz direct frames)");
#else
  Serial.println("mode: SERIAL BRIDGE (CoreS3; no WiFi; relaying nb-* to USB serial)");
#endif
#endif

  esp_read_mac(myMac, ESP_MAC_WIFI_STA);
  memcpy(myId, myMac + 3, sizeof(myId));
#if !CORES3_CAMBIUM_MODE
  Serial.printf("node id=%02X%02X%02X mac=%02X:%02X:%02X:%02X:%02X:%02X\n",
                myId[0], myId[1], myId[2], myMac[0], myMac[1], myMac[2],
                myMac[3], myMac[4], myMac[5]);
#endif

  rxQueue = xQueueCreate(64, sizeof(RxItem));
  if (!rxQueue) {
#if CORES3_CAMBIUM_MODE
    cambiumLogf("rx queue allocation FAILED");
#else
    Serial.println("rx queue allocation FAILED");
#endif
  }
  else radioReady = setupEspNow();
  setupWatchdog();

#if CORES3_AUDIO_REACTIVE_MODE
  audioInputReady = setupAudioInput();
  if (!audioInputReady) audioActive = false;
#endif

  M5.Display.setRotation(1);
  displayCanvas.setPsram(true);
  displayCanvas.setColorDepth(16);
  displayCanvasReady = displayCanvas.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
  if (displayCanvasReady) {
    drawDisplay();
  } else {
#if CORES3_CAMBIUM_MODE
    cambiumLogf("display framebuffer allocation FAILED; bridge continues headless");
#else
    Serial.println("display framebuffer allocation FAILED; bridge continues headless");
#endif
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_RED, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(8, 8);
    M5.Display.println("DISPLAY BUFFER FAIL");
    M5.Display.println("Bridge still active");
  }
#if CORES3_CAMBIUM_MODE
  sendCambiumStatus();
  cambiumLogf("%s up, ch=%u", CORES3_CAMBIUM_FW, cambiumChannel);
#endif
}

void loop() {
  esp_task_wdt_reset();
  M5.update();
#if CORES3_AUDIO_REACTIVE_MODE
  if (M5.Touch.getCount() && M5.Touch.getDetail(0).wasClicked())
    setAudioActive(!audioActive);
#endif
  processRx();
#if CORES3_CAMBIUM_MODE
  pumpCambiumSerial();
#else
  handleSerial();
  maintBurstTick();
#if CORES3_AUDIO_REACTIVE_MODE
  audioReactiveTick();
#endif
#endif

  static uint32_t nextBridgeMs = 0;
  static uint32_t nextDisplayMs = 0;
  uint32_t now = millis();
  if ((int32_t)(now - nextBridgeMs) >= 0) {
    nextBridgeMs = now + 1000 / NB_BRIDGE_HZ;
#if CORES3_CAMBIUM_MODE
    sendCambiumStatus();
#else
    emitBridgeStats();
#endif
  }
  if ((int32_t)(now - nextDisplayMs) >= 0) {
    nextDisplayMs = now + 1000;
    drawDisplay();
  }
  delay(2);
}
