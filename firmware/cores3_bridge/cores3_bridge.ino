// Resonance CoreS3 desk bridge.
//
// Dedicated ESP-NOW <-> USB bridge for the production fixture fleet. This is
// intentionally separate from net_bench: the CoreS3 has no PowerFeather power
// stack, and M5Unified must initialize its AXP2101 before the display/power path
// is treated as ready.

#include <Arduino.h>
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
#if CORES3_CAMBIUM_MODE
#define CORES3_AUDIO_REACTIVE_MODE 0
#else
// The ordinary CoreS3 image is now a two-app, untethered Bridge OS. Audio is
// compiled in beside Listener instead of requiring a separate reflash.
#define CORES3_AUDIO_REACTIVE_MODE 1
#endif
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
// Module Audio already includes M5Unified. Do not include the same M5GFX graph
// first through a differently spelled Windows path: GCC can then miss
// `#pragma once` identity and report hundreds of duplicate graphics types.
#include <M5Module_Audio.h>
#else
#include <M5Unified.h>
#endif

#include "../fixture/src/core/fixture_context.h"
#include "../fixture/src/core/packet.h"
#include "app_model.h"
#include "audio_reactive.h"
#include "cobs.h"

#define CORES3_BRIDGE_VERSION "cores3-os-0.2.1-dev"

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
#define NB_DARK_LEASE_DEFAULT_S 3600
#define NB_PROGRAM_COMMISSION_DARK 4
#define NB_TRANSPORT_MAX_HOURS 168
#define NB_LOCATE_MAX_S 900
#define NB_LOCATE_PERIOD_DS 200

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
#if CORES3_AUDIO_REACTIVE_MODE
M5Canvas audioPlotCanvas(&M5.Display);
bool audioPlotCanvasReady = false;
#endif
void drawDisplay();
#if !CORES3_CAMBIUM_MODE
CoreS3App currentApp = CORES3_APP_HOME;
size_t listenerPage = 0;
int listenerDetailPeer = -1;
void openCoreS3App(CoreS3App app);
#endif

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
AudioSpectrum audioSpectrum;
AudioCalibrationClock audioCalibration;
AudioRuntimeTiming audioTiming;
bool audioActive = false;
bool audioInputReady = false;
bool audioUsingModule = false;
bool audioModuleReady = false;
bool audioModuleInitAttempted = false;
uint32_t audioFrames = 0;
uint32_t audioReadFailures = 0;
AudioVisualMode audioMode = AUDIO_MODE_CLASSIC;
AudioOutputGain audioOutputGain = AUDIO_GAIN_2X;
float audioFastEnv = 0.0f;      // PULSE: fast follower of the companded level
float audioSlowEnv = 0.0f;      // PULSE: slow reference a transient must beat
bool audioPulseActive = false;  // PULSE: a triggered flash is still decaying
uint32_t audioPulseStartMs = 0; // PULSE: trigger time of the current flash
bool audioStaleBlackSent = false;
static constexpr size_t AUDIO_SPECTROGRAM_COLUMNS = 76;
uint8_t audioSpectrogram[AUDIO_SPECTROGRAM_COLUMNS][AUDIO_SPECTRUM_ROWS] = {};
size_t audioSpectrogramHead = 0; // next column to write; also the oldest column
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

void sendFleetProgramLease(uint8_t programId, uint16_t leaseS) {
  NbProgramSet cmd = {};
  fillHeader(&cmd.h, NB_PROGRAM_SET);
  // target_id stays 00:00:00: every receiver applies the same bounded lease.
  cmd.program_id = programId;
  cmd.lease_s = leaseS;
  cmd.seed = esp_random();
  cmd.flags = 0x01; // hard cut when starting a program; ignored for release
  sendPacketRepeated(&cmd, sizeof(cmd), 6, 8);
}

void sendTransportSleep(uint16_t hours) {
  NbTransportSleep cmd = {};
  fillHeader(&cmd.h, NB_TRANSPORT_SLEEP);
  // target_id remains 00:00:00: field packing is one fleet-wide operation.
  cmd.seconds = (uint32_t)hours * 3600UL;
  sendPacketRepeated(&cmd, sizeof(cmd), 8, 12);
}

void sendLocateControl(uint16_t durationS) {
  NbLocateControl cmd = {};
  fillHeader(&cmd.h, NB_LOCATE_CONTROL);
  // target_id remains 00:00:00. Duration zero is the explicit stop command.
  cmd.duration_s = durationS;
  cmd.period_ds = NB_LOCATE_PERIOD_DS;
  sendPacketRepeated(&cmd, sizeof(cmd), 6, 8);
}

#if CORES3_AUDIO_REACTIVE_MODE
const char *audioSourceName() {
  if (!audioInputReady) return "FAILED";
  return audioUsingModule ? "MODULE TRS" : "BUILTIN DUAL MIC";
}

const char *audioInputLabel() {
  if (!audioInputReady) return "INPUT FAILED";
  return audioUsingModule ? "AUX INPUT" : "AMBIENT MIC";
}

uint32_t audioSampleRate() {
  return audioUsingModule ? 44100UL : 16000UL;
}

bool audioCanCycleInput() {
#if CORES3_AUDIO_MODULE
  return true;
#else
  return false;
#endif
}

bool audioAuxReady() {
#if CORES3_AUDIO_MODULE
  return audioModuleReady;
#else
  return false;
#endif
}

const char *audioModeName(AudioVisualMode mode) {
  switch (mode) {
  case AUDIO_MODE_CLASSIC: return "CLASSIC";
  case AUDIO_MODE_EMBER: return "EMBER";
  case AUDIO_MODE_HUECYCLE: return "HUECYCLE";
  case AUDIO_MODE_PULSE: return "PULSE";
  case AUDIO_MODE_BAND_RGB: return "BANDS RGB";
  case AUDIO_MODE_BAND_SPLIT: return "BANDS SPLIT";
  case AUDIO_MODE_TIMBRE_HUE: return "TIMBRE HUE";
  default: return "UNKNOWN";
  }
}

void nextAudioMode() {
  audioMode = (AudioVisualMode)((audioMode + 1) % AUDIO_MODE_COUNT);
  Serial.printf("audio mode=%s\n", audioModeName(audioMode));
}

const char *audioOutputGainName(AudioOutputGain gain) {
  switch (gain) {
  case AUDIO_GAIN_1_5X: return "1.5X";
  case AUDIO_GAIN_2X: return "2X";
  case AUDIO_GAIN_3X: return "3X";
  default: return "1X";
  }
}

void nextAudioOutputGain() {
  audioOutputGain = audioNextOutputGain(audioOutputGain);
  Serial.printf("audio output gain=%s\n", audioOutputGainName(audioOutputGain));
}

void resetAudioAnalysis() {
  audioEnvelope = AudioEnvelope{};
  audioSpectrum.reset();
  audioCalibration.reset();
  audioTiming.reset(millis());
  memset(audioSpectrogram, 0, sizeof(audioSpectrogram));
  audioSpectrogramHead = 0;
  audioPulseActive = false;
  audioStaleBlackSent = false;
  audioFastEnv = 0.0f;
  audioSlowEnv = 0.0f;
}

bool audioAnalysisReady() {
  return audioCalibration.calibrated() && audioEnvelope.calibrated() &&
         audioSpectrum.calibrated() && audioCalibration.captureFresh(millis());
}

void appendAudioSpectrogram() {
  memcpy(audioSpectrogram[audioSpectrogramHead], audioSpectrum.rows,
         AUDIO_SPECTRUM_ROWS);
  audioSpectrogramHead =
      (audioSpectrogramHead + 1) % AUDIO_SPECTROGRAM_COLUMNS;
}

// Local HSV -> RGB for HUECYCLE. hue/sat/val are 0..1 and hue wraps. The wire
// contract (packet.h) stays untouched; this is bridge-side color math only.
void audioHsvToRgb(float hue, float sat, float val,
                   uint8_t *r, uint8_t *g, uint8_t *b) {
  hue -= floorf(hue);
  float hf = hue * 6.0f;
  int sector = ((int)hf) % 6;
  float f = hf - (float)sector;
  float p = val * (1.0f - sat);
  float q = val * (1.0f - sat * f);
  float t = val * (1.0f - sat * (1.0f - f));
  float rf, gf, bf;
  switch (sector) {
  case 0: rf = val; gf = t; bf = p; break;
  case 1: rf = q; gf = val; bf = p; break;
  case 2: rf = p; gf = val; bf = t; break;
  case 3: rf = p; gf = q; bf = val; break;
  case 4: rf = t; gf = p; bf = val; break;
  default: rf = val; gf = p; bf = q; break;
  }
  *r = (uint8_t)(rf * 255.0f + 0.5f);
  *g = (uint8_t)(gf * 255.0f + 0.5f);
  *b = (uint8_t)(bf * 255.0f + 0.5f);
}

// Shared (same color for every fixture) modes; CLASSIC keeps per-slot colors.
AudioColor audioSharedColor(float level, uint32_t nowMs) {
  if (level < 0.0f) level = 0.0f;
  if (level > 1.0f) level = 1.0f;
  switch (audioMode) {
  case AUDIO_MODE_EMBER:
    return {(uint8_t)(level * 255.0f + 0.5f),
            (uint8_t)(level * 0.55f * 255.0f + 0.5f),
            (uint8_t)(level * 0.10f * 255.0f + 0.5f),
            (uint8_t)(level * 0.60f * 255.0f + 0.5f)};
  case AUDIO_MODE_HUECYCLE: {
    // Slow shared rotation: one full hue revolution every 20 s.
    float hue = (float)(nowMs % 20000UL) / 20000.0f;
    uint8_t r, g, b;
    audioHsvToRgb(hue, 1.0f, level, &r, &g, &b);
    return {r, g, b, 0};
  }
  case AUDIO_MODE_PULSE: {
    float value = level * 0.2f; // dim floor between transients
    if (audioPulseActive) {
      uint32_t elapsed = nowMs - audioPulseStartMs;
      if (elapsed < 400) {
        float pulse = 1.0f - (float)elapsed / 400.0f;
        if (pulse > value) value = pulse;
      }
    }
    uint8_t v = (uint8_t)(value * 255.0f + 0.5f);
    return {v, v, v, v};
  }
  case AUDIO_MODE_BAND_RGB:
    return audioBandRgbColor(audioSpectrum.bass.level,
                             audioSpectrum.mid.level,
                             audioSpectrum.treble.level);
  case AUDIO_MODE_TIMBRE_HUE: {
    float value = max(level, max(audioSpectrum.bass.level,
                                 max(audioSpectrum.mid.level,
                                     audioSpectrum.treble.level)));
    // Low frequencies are warm amber; high frequencies travel toward violet.
    float hue = 0.03f + audioSpectrum.centroid * 0.72f;
    uint8_t r, g, b;
    audioHsvToRgb(hue, 1.0f, value, &r, &g, &b);
    return {r, g, b, 0};
  }
  default:
    return {0, 0, 0, 0};
  }
}

bool initializeModuleAudio() {
#if CORES3_AUDIO_MODULE
  if (audioModuleReady) return true;
  // A USB reset does not remove power from the stacked module. Its controller
  // can therefore appear later than the CoreS3. A missing I2C response is safe
  // to retry from the INPUT control; an I2S/codec failure after detection is
  // not retried repeatedly because this library does not expose an end().
  if (!M5.In_I2C.isEnabled() || !M5.In_I2C.scanID(I2C_ADDR, 400000)) {
    Serial.println("audio: Module Audio controller not ready; Aux can retry later");
    return false;
  }
  if (audioModuleInitAttempted) {
    Serial.println("audio: Module Audio initialization already failed; power-cycle to retry");
    return false;
  }
  audioModuleInitAttempted = true;
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
    audioModuleReady = true;
    Serial.println("audio: Module Audio LINE/MIC input ready (TRS, 24 dB)");
    return true;
  }
  audioModuleReady = false;
  Serial.println("audio: Module Audio I2S/codec init failed");
#endif
  return false;
}

bool setupAudioInput() {
#if CORES3_AUDIO_MODULE
  if (initializeModuleAudio()) {
    audioUsingModule = true;
    return true;
  }
  Serial.println("audio: Aux unavailable at boot; falling back to CoreS3 microphones");
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
  if (audioUsingModule) {
    // Module Audio records interleaved stereo. Collapse complete L/R frames
    // before envelope/FFT analysis; treating the interleave as a mono stream
    // manufactures a false near-Nyquist component when the channels differ.
    static int16_t stereo[AUDIO_FFT_SIZE * 2];
    if (count > AUDIO_FFT_SIZE ||
        !audioModule.record((uint8_t *)stereo,
                            (int)(count * 2 * sizeof(int16_t))))
      return false;
    audioStereoToMono(stereo, samples, count);
    return true;
  }
#endif
  return M5.Mic.record(samples, count, 16000);
}

size_t collectLiveAudioIds(uint8_t ids[NB_MAX_TRACKED][3]) {
  size_t count = 0;
  uint32_t now = millis();
  for (size_t i = 0; i < NB_MAX_TRACKED && count < NB_MAX_TRACKED; ++i) {
    if (!peers[i].used || now - peers[i].lastHeardMs > 5000) continue;
    // A nearby legacy net-bench peer still heartbeats on channel 11 but cannot
    // consume type-25 frames. Once its full heartbeat identifies that firmware,
    // keep it from stealing a stable color slot from a fixture. ADR 0040 fleet
    // fixtures report immutable fx-* revisions, not the older fixture-* prefix.
    if (!cores3AudioPeerEligible(peers[i].hasFw, peers[i].fwRev)) continue;
    memcpy(ids[count++], peers[i].id, 3);
  }
  // Stable colors independent of heartbeat arrival order.
  for (size_t i = 1; i < count; ++i) {
    uint8_t key[3];
    memcpy(key, ids[i], 3);
    int j = (int)i - 1;
    while (j >= 0 && memcmp(ids[j], key, 3) > 0) {
      memcpy(ids[j + 1], ids[j], 3);
      --j;
    }
    memcpy(ids[j + 1], key, 3);
  }
  return count;
}

void sendAudioLevel(float level, bool forceBlack = false) {
  if (!radioReady) return;
  uint8_t ids[NB_MAX_TRACKED][3];
  size_t total = collectLiveAudioIds(ids);
  if (!total) return;

  AudioColor shared = {};
  bool perSlot = audioMode == AUDIO_MODE_CLASSIC ||
                 audioMode == AUDIO_MODE_BAND_SPLIT;
  if (!forceBlack && !perSlot)
    shared = audioSharedColor(level, millis());

  // NbDirectFrame holds 18 entries. Chunk the complete sorted live census so
  // an audio publisher scales beyond one packet while preserving stable slot
  // colors across every chunk.
  for (size_t offset = 0; offset < total;
       offset += AUDIO_DIRECT_ENTRIES_PER_FRAME) {
    NbDirectFrame frame = {};
    fillHeader(&frame.h, NB_DIRECT_FRAME);
    frame.flags = 0x03; // 10 s micro-lease + hard-cut; envelope owns smoothing
    size_t chunk = min((size_t)AUDIO_DIRECT_ENTRIES_PER_FRAME, total - offset);
    frame.count = (uint8_t)chunk;
    for (size_t i = 0; i < chunk; ++i) {
      size_t slot = offset + i;
      memcpy(frame.entries[i].id, ids[slot], 3);
      AudioColor color = shared;
      if (forceBlack) {
        color = {0, 0, 0, 0};
      } else if (audioMode == AUDIO_MODE_CLASSIC) {
        color = audioColorForSlot((uint8_t)slot, level);
      } else if (audioMode == AUDIO_MODE_BAND_SPLIT) {
        color = audioBandSplitColor((uint8_t)slot,
                                    audioSpectrum.bass.level,
                                    audioSpectrum.mid.level,
                                    audioSpectrum.treble.level);
      }
      color = audioApplyOutputGain(color, audioOutputGain);
      frame.entries[i].r = color.r;
      frame.entries[i].g = color.g;
      frame.entries[i].b = color.b;
      frame.entries[i].w = color.w;
    }
    size_t wireLen = offsetof(NbDirectFrame, entries) +
                     chunk * sizeof(NbDirectEntry);
    if (esp_now_send(BCAST, (const uint8_t *)&frame, wireLen) != ESP_OK)
      ++sendFail;
    else
      ++audioFrames;
  }
}

void setAudioActive(bool active) {
  if (active == audioActive) return;
  audioPulseActive = false;
  audioFastEnv = 0.0f;
  audioSlowEnv = 0.0f;
  if (!active) sendAudioLevel(0.0f, true);
  audioActive = active;
  if (active) {
    // Explicit program leases (CA Studio, Contagion, Dark, and similar) win
    // over direct-frame micro-leases in fixture arbitration. Entering Audio is
    // an operator ownership handoff, so release any prior fleet program lease
    // in RAM before publishing the first direct frame. This changes no profile,
    // lifecycle setting, NVS, or autonomous default.
    sendFleetProgramLease(0, 0);
    resetAudioAnalysis();
    Serial.println("audio reactive -> ON (fleet program lease released)");
  } else {
    Serial.println("audio reactive -> OFF");
  }
}

bool selectAudioInput(CoreS3AudioInput target) {
  CoreS3AudioInput current = audioUsingModule ? CORES3_AUDIO_INPUT_AUX
                                               : CORES3_AUDIO_INPUT_AMBIENT;
  if (target == current && audioInputReady) return true;

#if !CORES3_AUDIO_MODULE
  if (target == CORES3_AUDIO_INPUT_AUX) {
    Serial.println("audio input: AUX unavailable in built-in hardware image");
    return false;
  }
#endif

  // A source handoff is an ownership boundary. Send an explicit zero frame and
  // suspend sampling before touching either I2S path, then restore the prior
  // publishing state with a fresh two-second calibration on the new source.
  bool wasActive = audioActive;
  if (wasActive) setAudioActive(false);

  bool switched = false;
  if (target == CORES3_AUDIO_INPUT_AMBIENT) {
    M5.Speaker.end();
    if (M5.Mic.begin()) {
#if CORES3_AUDIO_MODULE
      if (audioModuleReady) {
        audioModule.setMICStatus(AUDIO_MIC_CLOSE);
        audioModule.setAllRGBLED(0x000030);
      }
#endif
      audioUsingModule = false;
      audioInputReady = true;
      switched = true;
    } else {
      Serial.println("audio input: CoreS3 ambient microphones failed to start");
    }
  } else {
#if CORES3_AUDIO_MODULE
    if (initializeModuleAudio()) {
      M5.Mic.end();
      audioModule.setHPMICStatus(AUDIO_MIC_CLOSE);
      audioModule.setMICStatus(AUDIO_MIC_OPEN);
      audioModule.setAllRGBLED(0x003000);
      audioUsingModule = true;
      audioInputReady = true;
      switched = true;
    } else {
      Serial.println("audio input: Aux is not ready; staying on Ambient");
    }
#endif
  }

  if (!switched) {
#if CORES3_AUDIO_MODULE
    // The old Aux path was kept open until Ambient successfully started, so a
    // failed handoff can return to the prior working input without a reboot.
    if (current == CORES3_AUDIO_INPUT_AUX && audioModuleReady) {
      audioModule.setMICStatus(AUDIO_MIC_OPEN);
      audioModule.setAllRGBLED(0x003000);
      audioUsingModule = true;
      audioInputReady = true;
    }
#endif
    if (wasActive && audioInputReady) setAudioActive(true);
    return false;
  }

  resetAudioAnalysis();
  Serial.printf("audio input -> %s\n", audioInputLabel());
  if (wasActive) setAudioActive(true);
  return true;
}

void nextAudioInput() {
  CoreS3AudioInput current = audioUsingModule ? CORES3_AUDIO_INPUT_AUX
                                               : CORES3_AUDIO_INPUT_AMBIENT;
  selectAudioInput(cores3NextAudioInput(current, audioCanCycleInput()));
}

void audioReactiveTick() {
  if (!audioActive) return;

  // Publishing is independently phase-locked. Check it before the blocking
  // capture so a 32 ms microphone read cannot turn the 100 ms deadline into a
  // permanent 120 ms cadence. It sends the newest complete feature snapshot.
  uint32_t now = millis();
  if (audioTiming.publishDeadline.take(now, AUDIO_FIXTURE_PERIOD_MS) &&
      audioAnalysisReady()) {
    audioTiming.publish.note(now);
    uint32_t startedUs = micros();
    sendAudioLevel(audioEnvelope.level);
    AudioRuntimeTiming::noteMax(micros() - startedUs,
                                &audioTiming.publishMaxUs);
  }

  now = millis();
  if (!audioTiming.analysisDeadline.take(now, AUDIO_ANALYSIS_PERIOD_MS)) return;
  uint32_t analysisStartedUs = micros();

  static int16_t samples[AUDIO_FFT_SIZE];
  uint32_t captureStartedUs = micros();
  if (!readAudioSamples(samples, sizeof(samples) / sizeof(samples[0]))) {
    ++audioReadFailures;
    AudioRuntimeTiming::noteMax(micros() - captureStartedUs,
                                &audioTiming.captureMaxUs);
    AudioRuntimeTiming::noteMax(micros() - analysisStartedUs,
                                &audioTiming.analysisMaxUs);
    // Decoupled publishing must not keep replaying the last bright feature if
    // an I2S source fails. Stop within the same freshness bound used to restart
    // calibration; the fixture's ordinary three-second fallback remains intact.
    if (!audioStaleBlackSent && audioCalibration.observations &&
        !audioCalibration.captureFresh(millis())) {
      sendAudioLevel(0.0f, true);
      audioStaleBlackSent = true;
    }
    return;
  }
  AudioRuntimeTiming::noteMax(micros() - captureStartedUs,
                              &audioTiming.captureMaxUs);
  uint32_t capturedMs = millis();
  audioStaleBlackSent = false;
  audioTiming.capture.note(capturedMs);
  audioCalibration.noteCapture(capturedMs);
  bool calibrating = audioCalibration.calibratingCurrentCapture();
  uint16_t calibrationObservation = audioCalibration.observationCount16();

  float level = audioEnvelope.update(samples,
                                     sizeof(samples) / sizeof(samples[0]),
                                     calibrating, calibrationObservation);
  if (!audioSpectrum.update(samples, sizeof(samples) / sizeof(samples[0]),
                            audioSampleRate(), calibrating,
                            calibrationObservation)) {
    ++audioReadFailures;
    AudioRuntimeTiming::noteMax(micros() - analysisStartedUs,
                                &audioTiming.analysisMaxUs);
    return;
  }
  appendAudioSpectrogram();
  uint32_t analyzedMs = millis();
  audioTiming.analysis.note(analyzedMs);
  AudioRuntimeTiming::noteMax(micros() - analysisStartedUs,
                              &audioTiming.analysisMaxUs);

  // PULSE transient tracking runs in every mode so switching in is seamless.
  audioFastEnv += (level - audioFastEnv) * 0.33f;
  audioSlowEnv += (level - audioSlowEnv) * 0.025f;
  if (audioPulseActive && analyzedMs - audioPulseStartMs >= 400)
    audioPulseActive = false;
  if (!audioPulseActive && audioAnalysisReady() &&
      audioFastEnv > audioSlowEnv * 1.4f && audioFastEnv > 0.08f) {
    audioPulseActive = true;
    audioPulseStartMs = analyzedMs;
  }
}
#endif

void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (!rxQueue || len <= 0 || len > (int)sizeof(RxItem::data)) return;
#if !CORES3_CAMBIUM_MODE
  if (len < (int)sizeof(NbHeader)) return;
  const NbHeader *h = (const NbHeader *)data;
  if (h->ver != NB_PROTO_VER ||
      (h->type != NB_HEARTBEAT && h->type != NB_SCANAP &&
       h->type != NB_NEIGHBOR_REPORT)) return;
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

void emitNeighborReport(const RxItem &item) {
  if (item.len < (int)offsetof(NbNeighborReport, entries)) return;
  const NbNeighborReport *report = (const NbNeighborReport *)item.data;
  uint8_t available = (uint8_t)((item.len - offsetof(NbNeighborReport, entries)) /
                                sizeof(NbNeighborEntry));
  uint8_t count = min((uint8_t)NB_NEIGHBOR_REPORT_MAX,
                      min(report->count, available));
  for (uint8_t i = 0; i < count; ++i) {
    const NbNeighborEntry &entry = report->entries[i];
    Serial.printf("nb-rssi report=%lu rx=%02X%02X%02X tx=%02X%02X%02X "
                  "rssi=%d n=%u expected=%u censored=%u idx=%u count=%u "
                  "linkrssi=%d\n",
                  (unsigned long)report->h.seq,
                  report->h.src_id[0], report->h.src_id[1], report->h.src_id[2],
                  entry.id[0], entry.id[1], entry.id[2], entry.med_dbm,
                  entry.n, report->n_expected, entry.flags & 0x01, i, count,
                  item.rssi);
  }
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
    else if (h->type == NB_NEIGHBOR_REPORT) emitNeighborReport(item);
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
  Serial.printf("audio source=%s ready=%d auxready=%d active=%d mode=%s gain=%s calibrated=%d "
                "rms=%.1f noise=%.1f level=%.3f bass=%.3f mid=%.3f treble=%.3f "
                "centroid=%.3f analysis=%lu frames=%lu readfail=%lu\n",
                audioSourceName(), audioInputReady, audioAuxReady(), audioActive,
                audioModeName(audioMode), audioOutputGainName(audioOutputGain),
                audioAnalysisReady(),
                audioEnvelope.rms, audioEnvelope.noise, audioEnvelope.level,
                audioSpectrum.bass.level, audioSpectrum.mid.level,
                audioSpectrum.treble.level, audioSpectrum.centroid,
                (unsigned long)audioSpectrum.analysisFrames,
                (unsigned long)audioFrames,
                (unsigned long)audioReadFailures);
  Serial.printf(
      "audio-timing cal_ms=%lu cal_obs=%lu capture_hz_x1000=%lu "
      "analysis_hz_x1000=%lu analysis_ms=%lu/%lu "
      "tx_hz_x1000=%lu tx_ms=%lu/%lu display_hz_x1000=%lu "
      "skip=%lu/%lu/%lu late_max_ms=%lu/%lu/%lu "
      "sendfail=%lu rxdrops=%lu "
      "max_us_capture_analysis_tx_display_loop=%lu/%lu/%lu/%lu/%lu\n",
      (unsigned long)audioCalibration.elapsedMs(),
      (unsigned long)audioCalibration.observations,
      (unsigned long)audioTiming.capture.rateMilliHz(),
      (unsigned long)audioTiming.analysis.rateMilliHz(),
      (unsigned long)audioTiming.analysis.intervalMinMs(),
      (unsigned long)audioTiming.analysis.intervalMaxMs(),
      (unsigned long)audioTiming.publish.rateMilliHz(),
      (unsigned long)audioTiming.publish.intervalMinMs(),
      (unsigned long)audioTiming.publish.intervalMaxMs(),
      (unsigned long)audioTiming.display.rateMilliHz(),
      (unsigned long)audioTiming.analysisDeadline.skippedPeriods,
      (unsigned long)audioTiming.publishDeadline.skippedPeriods,
      (unsigned long)audioTiming.displayDeadline.skippedPeriods,
      (unsigned long)audioTiming.analysisDeadline.maxLatenessMs,
      (unsigned long)audioTiming.publishDeadline.maxLatenessMs,
      (unsigned long)audioTiming.displayDeadline.maxLatenessMs,
      (unsigned long)sendFail,
      (unsigned long)rxQueueDrops,
      (unsigned long)audioTiming.captureMaxUs,
      (unsigned long)audioTiming.analysisMaxUs,
      (unsigned long)audioTiming.publishMaxUs,
      (unsigned long)audioTiming.displayMaxUs,
      (unsigned long)audioTiming.loopMaxUs);
#endif

#if CORES3_AUDIO_REACTIVE_MODE
  // Keep the compact master/audio timing lines at 1 Hz, but avoid making the
  // real-time loop format and flush every live peer once per second. The full
  // peer table remains at 1 Hz in Listener and at 0.2 Hz while Audio is active.
  static AudioPeriodicDeadline audioPeerStatsDeadline;
  uint32_t statsNow = millis();
  if (audioActive) {
    if (!audioPeerStatsDeadline.take(statsNow, 5000)) return;
  } else {
    audioPeerStatsDeadline.reset(statsNow);
  }
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
    // 2026-08-20 incident fix: a bare 'm' used to cycle presets STARTING AT
    // 5.5 V — one stray byte on this port (e.g. a ModemManager probe of the
    // freshly re-enumerated tty) broadcast a VINDPM that collapses USB-powered
    // fixtures' input and PERSISTS in their NVS. Explicit digits are now
    // required; there is no bare-'m' action.
    int explicitV10 = readSerialUint(50, NB_MAINTAIN_MAX_V10);
    if (explicitV10 < 0) {
      Serial.println("m requires an explicit value, e.g. m46 (4.6 V solar "
                     "std) / m50 / m52; 55 collapses USB-powered boards");
      break;
    }
    if (explicitV10 < NB_MAINTAIN_MIN_V10 || explicitV10 > NB_MAINTAIN_MAX_V10) {
      Serial.printf("SET_MAINTAIN %d rejected (range %d..%d)\n", explicitV10,
                    NB_MAINTAIN_MIN_V10, NB_MAINTAIN_MAX_V10);
      break;
    }
    sendCmd(NB_SET_MAINTAIN, (uint8_t)explicitV10);
    Serial.printf("broadcast SET_MAINTAIN %.1f V\n", explicitV10 / 10.0f);
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
  case 'Q': {
    int hours = readSerialUint(100, NB_TRANSPORT_MAX_HOURS);
    if (hours < 1 || hours > NB_TRANSPORT_MAX_HOURS) {
      Serial.printf("TRANSPORT_SLEEP rejected: use Q<hours>, 1..%d\n",
                    NB_TRANSPORT_MAX_HOURS);
      break;
    }
    sendTransportSleep((uint16_t)hours);
    Serial.printf("broadcast TRANSPORT_SLEEP %dh (%lus); timer wake stays dark "
                  "until bridge program release\n",
                  hours, (unsigned long)hours * 3600UL);
    break;
  }
  case 'L': {
    int seconds = readSerialUint(100, NB_LOCATE_MAX_S);
    if (seconds < 0) seconds = 120;
    if (seconds > NB_LOCATE_MAX_S) {
      Serial.printf("LOCATE rejected: use L[seconds], 0..%d\n", NB_LOCATE_MAX_S);
      break;
    }
    sendLocateControl((uint16_t)seconds);
    Serial.printf("broadcast LOCATE %s, %d s, reports every %.1f s\n",
                  seconds ? "start" : "stop", seconds,
                  NB_LOCATE_PERIOD_DS / 10.0f);
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
  case 'B': {
    int seconds = readSerialUint(80, 65535);
    uint16_t leaseS = seconds < 0 ? NB_DARK_LEASE_DEFAULT_S : (uint16_t)seconds;
    if (!leaseS) {
      Serial.println("DARK rejected: use B[seconds], seconds 1..65535");
      break;
    }
    sendFleetProgramLease(NB_PROGRAM_COMMISSION_DARK, leaseS);
    Serial.printf("broadcast DARK lease %us (RAM-only; profile unchanged)\n", leaseS);
    break;
  }
  case 'b':
    sendFleetProgramLease(0, 0);
    Serial.println("broadcast program lease release");
    break;
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
    if (currentApp != CORES3_APP_AUDIO)
      openCoreS3App(CORES3_APP_AUDIO);
    else
      setAudioActive(!audioActive);
    break;
  case 'M':
    nextAudioMode();
    break;
  case 'N':
    nextAudioInput();
    break;
  case 'V':
    nextAudioOutputGain();
    break;
#endif
  case 'h':
  case '?':
    Serial.println("commands: r t U[id] c +/- R<hz> i[id][:s] I F[id:]<0|1> B[s] b m<v10> C[id:]mAh G[id:]mA K<id>:ms S[s] Q<hours> L[seconds] P<id>[:s] D[<id>][:mAh]"
#if CORES3_AUDIO_REACTIVE_MODE
                   " A M N V"
#endif
    );
    break;
  default:
    break;
  }
}

#if !CORES3_CAMBIUM_MODE
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((uint16_t)(r & 0xF8) << 8) |
                    ((uint16_t)(g & 0xFC) << 3) | (b >> 3));
}

void drawTextButton(int16_t x, int16_t y, int16_t w, int16_t h,
                    uint16_t color, const char *label) {
  displayCanvas.fillRoundRect(x, y, w, h, 7, color);
  displayCanvas.drawRoundRect(x, y, w, h, 7, TFT_WHITE);
  displayCanvas.setTextColor(TFT_WHITE, color);
  displayCanvas.setTextSize(2);
  int16_t labelWidth = (int16_t)strlen(label) * 12;
  displayCanvas.setCursor(x + max(5, (w - labelWidth) / 2), y + (h - 16) / 2);
  displayCanvas.print(label);
}

void drawAppHeader(const char *title, bool showApps = true) {
  displayCanvas.fillRect(0, 0, 320, 32, rgb565(9, 25, 36));
  if (showApps) {
    displayCanvas.fillRoundRect(4, 4, 56, 24, 5, rgb565(24, 73, 91));
    displayCanvas.setTextColor(TFT_WHITE, rgb565(24, 73, 91));
    displayCanvas.setTextSize(1);
    displayCanvas.setCursor(15, 12);
    displayCanvas.print("APPS");
  }
  displayCanvas.setTextSize(2);
  displayCanvas.setTextColor(TFT_CYAN, rgb565(9, 25, 36));
  displayCanvas.setCursor(showApps ? 70 : 8, 8);
  displayCanvas.print(title);

  int pct = M5.Power.getBatteryLevel();
  displayCanvas.setTextSize(1);
  displayCanvas.setTextColor(M5.Power.isCharging() ? TFT_GREEN : TFT_WHITE,
                             rgb565(9, 25, 36));
  displayCanvas.setCursor(276, 12);
  if (pct >= 0)
    displayCanvas.printf("%d%%", pct);
  else
    displayCanvas.print("--%");
}

size_t sortedPeerIndexes(uint16_t out[NB_MAX_TRACKED]) {
  size_t count = 0;
  for (size_t i = 0; i < NB_MAX_TRACKED; ++i) {
    if (!peers[i].used) continue;
    size_t at = count;
    while (at > 0 && memcmp(peers[out[at - 1]].id, peers[i].id, 3) > 0) {
      out[at] = out[at - 1];
      --at;
    }
    out[at] = (uint16_t)i;
    ++count;
  }
  return count;
}

uint16_t batteryBandColor(CoreS3BatteryBand band) {
  switch (band) {
  case CORES3_BATTERY_GOOD: return rgb565(27, 138, 58);
  case CORES3_BATTERY_NEAR_LOW: return rgb565(217, 165, 0);
  case CORES3_BATTERY_LOW: return rgb565(213, 43, 43);
  case CORES3_BATTERY_UNKNOWN: return rgb565(47, 128, 201);
  default: return rgb565(52, 58, 64);
  }
}

uint16_t peerLedColor(const PeerStat &peer) {
  if (!peer.hasLedOutput || !peer.ledRailOn || !peer.ledLitPixels)
    return rgb565(35, 39, 43);
  uint16_t white = peer.ledW / 2;
  return rgb565((uint8_t)min(255, (int)peer.ledR + white),
                (uint8_t)min(255, (int)peer.ledG + white),
                (uint8_t)min(255, (int)peer.ledB + white));
}

void drawFixtureShape(int16_t x, int16_t y, uint8_t fixtureClass,
                      uint16_t color) {
  switch (fixtureClass) {
  case FIXTURE_DOWNLIGHT:
    displayCanvas.fillCircle(x, y, 8, color);
    break;
  case FIXTURE_PERIMETER:
    displayCanvas.fillRect(x - 4, y - 8, 8, 16, color);
    displayCanvas.fillTriangle(x - 9, y, x - 4, y - 8, x - 4, y + 8,
                               color);
    displayCanvas.fillTriangle(x + 9, y, x + 4, y - 8, x + 4, y + 8,
                               color);
    break;
  case FIXTURE_UPLIGHT:
    displayCanvas.fillTriangle(x, y - 9, x - 9, y + 8, x + 9, y + 8,
                               color);
    break;
  case FIXTURE_CHANDELIER:
    displayCanvas.fillTriangle(x, y - 10, x - 9, y, x + 9, y, color);
    displayCanvas.fillTriangle(x, y + 10, x - 9, y, x + 9, y, color);
    break;
  default:
    displayCanvas.fillRoundRect(x - 8, y - 8, 16, 16, 4, color);
    break;
  }
}

void drawLauncher() {
  drawAppHeader("BRIDGE OS", false);
  displayCanvas.setTextSize(1);
  displayCanvas.setTextColor(radioReady ? TFT_GREEN : TFT_RED, TFT_BLACK);
  displayCanvas.setCursor(10, 41);
  displayCanvas.printf("WIRELESS  ch%d  radio %s", NB_CHANNEL,
                       radioReady ? "UP" : "FAIL");
  displayCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
  displayCanvas.setCursor(198, 41);
  displayCanvas.printf("%d live / %d seen", peerCount(true), peerCount(false));

  displayCanvas.fillRoundRect(10, 65, 145, 154, 10, rgb565(14, 55, 70));
  displayCanvas.drawRoundRect(10, 65, 145, 154, 10, TFT_CYAN);
  displayCanvas.setTextColor(TFT_CYAN, rgb565(14, 55, 70));
  displayCanvas.setTextSize(2);
  displayCanvas.setCursor(30, 84);
  displayCanvas.print("LISTENER");
  displayCanvas.setTextColor(TFT_WHITE, rgb565(14, 55, 70));
  displayCanvas.setTextSize(1);
  displayCanvas.setCursor(23, 119);
  displayCanvas.println("Fleet health grid");
  displayCanvas.setCursor(23, 137);
  displayCanvas.println("Tap any fixture");
  displayCanvas.setCursor(23, 155);
  displayCanvas.println("USB is optional");
  displayCanvas.setTextSize(3);
  displayCanvas.setCursor(59, 181);
  displayCanvas.print("24");

  uint16_t audioCard = audioInputReady ? rgb565(74, 25, 75) : rgb565(58, 45, 58);
  displayCanvas.fillRoundRect(165, 65, 145, 154, 10, audioCard);
  displayCanvas.drawRoundRect(165, 65, 145, 154, 10,
                              audioInputReady ? TFT_MAGENTA : TFT_RED);
  displayCanvas.setTextColor(audioInputReady ? TFT_MAGENTA : TFT_RED,
                             audioCard);
  displayCanvas.setTextSize(2);
  displayCanvas.setCursor(191, 84);
  displayCanvas.print("AUDIO");
  displayCanvas.setTextColor(TFT_WHITE, audioCard);
  displayCanvas.setTextSize(1);
  displayCanvas.setCursor(180, 119);
  displayCanvas.println(audioSourceName());
  displayCanvas.setCursor(180, 137);
  displayCanvas.println("10 Hz direct frames");
  displayCanvas.setCursor(180, 155);
  displayCanvas.println("Touch controls");
  displayCanvas.setTextSize(3);
  displayCanvas.setCursor(214, 181);
  displayCanvas.print("~");
}

void drawListenerGrid() {
  drawAppHeader("LISTENER");
  uint16_t order[NB_MAX_TRACKED];
  size_t count = sortedPeerIndexes(order);
  listenerPage = cores3ClampPage(listenerPage, count);
  size_t pageCount = cores3PageCount(count);
  size_t start = cores3PageStart(listenerPage, count);
  size_t end = min(count, start + CORES3_LISTENER_PAGE_SIZE);
  uint32_t now = millis();

  displayCanvas.setTextSize(1);
  displayCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
  displayCanvas.setCursor(7, 39);
  displayCanvas.printf("%d live / %d seen", peerCount(true), peerCount(false));
  displayCanvas.setCursor(247, 39);
  displayCanvas.printf("page %u/%u", (unsigned)listenerPage + 1,
                       (unsigned)pageCount);

  for (size_t position = start; position < end; ++position) {
    size_t cell = position - start;
    int16_t col = cell % 6;
    int16_t row = cell / 6;
    int16_t x = 4 + col * 52;
    int16_t y = 53 + row * 37;
    PeerStat &peer = peers[order[position]];
    bool live = now - peer.lastHeardMs <= 5000;
    CoreS3BatteryBand band = cores3BatteryBand(live, peer.battMv);
    uint16_t bg = rgb565(18, 22, 26);
    displayCanvas.fillRoundRect(x, y, 48, 34, 4, bg);
    displayCanvas.drawRoundRect(x, y, 48, 34, 4,
                                live ? TFT_WHITE : TFT_DARKGREY);
    displayCanvas.fillRect(x + 3, y + 2, 42, 3, peerLedColor(peer));
    drawFixtureShape(x + 24, y + 15,
                     peer.hasLedOutput ? peer.fixtureClass : FIXTURE_UNKNOWN,
                     batteryBandColor(band));
    displayCanvas.setTextSize(1);
    displayCanvas.setTextColor(TFT_WHITE, bg);
    displayCanvas.setCursor(x + 17, y + 24);
    displayCanvas.printf("%02X", peer.id[2]);
  }

  if (!count) {
    displayCanvas.setTextSize(2);
    displayCanvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    displayCanvas.setCursor(35, 105);
    displayCanvas.print("Waiting for heartbeats");
  }

  drawTextButton(4, 205, 72, 31, rgb565(37, 62, 73), "PREV");
  displayCanvas.setTextSize(1);
  displayCanvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  displayCanvas.setCursor(88, 216);
  displayCanvas.print("tap tile for detail");
  drawTextButton(244, 205, 72, 31, rgb565(37, 62, 73), "NEXT");
}

void drawListenerDetail() {
  drawAppHeader("FIXTURE");
  if (listenerDetailPeer < 0 || listenerDetailPeer >= NB_MAX_TRACKED ||
      !peers[listenerDetailPeer].used) {
    displayCanvas.setTextSize(2);
    displayCanvas.setTextColor(TFT_RED, TFT_BLACK);
    displayCanvas.setCursor(20, 90);
    displayCanvas.print("Fixture unavailable");
    drawTextButton(88, 197, 144, 35, rgb565(37, 62, 73), "BACK");
    return;
  }

  const PeerStat &p = peers[listenerDetailPeer];
  uint32_t ageS = (millis() - p.lastHeardMs) / 1000;
  displayCanvas.setTextSize(2);
  displayCanvas.setTextColor(TFT_CYAN, TFT_BLACK);
  displayCanvas.setCursor(8, 38);
  displayCanvas.printf("%02X%02X%02X  %s", p.id[0], p.id[1], p.id[2],
                       p.hasLedOutput ? fixtureClassName(p.fixtureClass)
                                      : "class pending");

  displayCanvas.setTextSize(1);
  displayCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
  int y = 62;
  displayCanvas.setCursor(8, y);
  displayCanvas.printf("last %lus  RSSI %d dBm  PDR %.1f%%", (unsigned long)ageS,
                       p.rssi, p.recv ? 100.0f * p.recv / (p.recv + p.gaps) : 0);
  y += 15;
  displayCanvas.setCursor(8, y);
  displayCanvas.printf("battery %.3f V  %+d mA", p.battMv / 1000.0f,
                       p.battMa);
  y += 15;
  displayCanvas.setCursor(8, y);
  if (p.soc != 255) displayCanvas.printf("SOC %u%%  ", p.soc);
  displayCanvas.printf("input %.3f V  %+d mA  %s", p.supplyMv / 1000.0f,
                       p.supplyMa, p.supplyGood ? "GOOD" : "not good");
  y += 15;
  displayCanvas.setCursor(8, y);
  if (p.hasFixtureState)
    displayCanvas.printf("profile %u  life %u  power %u  program %u", p.profile,
                         p.lifeState, p.powerTier, p.activeProgram);
  else
    displayCanvas.print("fixture state unavailable");
  y += 15;
  displayCanvas.setCursor(8, y);
  if (p.hasIdentityRecovery)
    displayCanvas.printf("sensors 0x%02X  mismatch %u  recovery %u",
                         p.sensorBits, p.classMismatch, p.recoveryState);
  else
    displayCanvas.print("sensor/recovery detail unavailable");
  y += 15;
  displayCanvas.setCursor(8, y);
  if (p.hasLedOutput)
    displayCanvas.printf("LED rail %s  lit %u  RGBW %u/%u/%u/%u",
                         p.ledRailOn ? "ON" : "OFF", p.ledLitPixels, p.ledR,
                         p.ledG, p.ledB, p.ledW);
  else
    displayCanvas.print("reported LED output unavailable");
  y += 15;
  displayCanvas.setCursor(8, y);
  displayCanvas.printf("firmware %s", p.hasFw ? p.fwRev : "unknown");
  y += 15;
  displayCanvas.setCursor(8, y);
  displayCanvas.printf("bridge TX ok/fail %lu/%lu  RX drops %lu",
                       (unsigned long)sendOk, (unsigned long)sendFail,
                       (unsigned long)rxQueueDrops);

  drawTextButton(88, 201, 144, 34, rgb565(37, 62, 73), "BACK");
}

uint16_t audioSpectrogramColor(uint8_t value) {
  if (value < 12) return TFT_BLACK;
  if (value < 96) {
    return rgb565(0, (uint8_t)(value / 3), value);
  }
  if (value < 176) {
    uint8_t ramp = value - 96;
    return rgb565(0, (uint8_t)(64 + ramp * 2),
                  (uint8_t)(255 - ramp));
  }
  uint8_t ramp = value - 176;
  return rgb565((uint8_t)min(255, (int)ramp * 3), 255,
                (uint8_t)min(255, (int)ramp * 2));
}

void drawAudioBandMeter(int16_t x, const char *label, float level,
                        uint16_t color) {
  displayCanvas.setTextSize(1);
  displayCanvas.setTextColor(color, TFT_BLACK);
  displayCanvas.setCursor(x, 165);
  displayCanvas.printf("%s %2d", label,
                       (int)(audioClampUnit(level) * 99.0f + 0.5f));
  displayCanvas.drawRoundRect(x, 176, 96, 9, 3, TFT_DARKGREY);
  int width = (int)(audioClampUnit(level) * 92.0f + 0.5f);
  if (width > 0) displayCanvas.fillRect(x + 2, 178, width, 5, color);
}

void drawAudioApp() {
  drawAppHeader("AUDIO");
  displayCanvas.setTextSize(1);
  displayCanvas.setTextColor(audioInputReady ? TFT_GREEN : TFT_RED, TFT_BLACK);
  displayCanvas.setCursor(8, 38);
  displayCanvas.printf("%s  %s", audioInputLabel(),
                       audioActive ? "PUBLISHING" : "PAUSED");
  displayCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
  displayCanvas.setCursor(230, 38);
  displayCanvas.printf("%d live", peerCount(true));

  displayCanvas.setTextSize(1);
  displayCanvas.setTextColor(TFT_MAGENTA, TFT_BLACK);
  displayCanvas.setCursor(8, 51);
  displayCanvas.printf("%s  GAIN %s", audioModeName(audioMode),
                       audioOutputGainName(audioOutputGain));
  displayCanvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  displayCanvas.setCursor(174, 51);
  uint32_t analysisRate = audioTiming.analysis.rateMilliHz();
  uint32_t publishRate = audioTiming.publish.rateMilliHz();
  displayCanvas.printf("FFT %lu.%lu TX %lu.%lu",
                       (unsigned long)(analysisRate / 1000),
                       (unsigned long)((analysisRate % 1000) / 100),
                       (unsigned long)(publishRate / 1000),
                       (unsigned long)((publishRate % 1000) / 100));

  // Three seconds of log-frequency history at 25 columns/s. Low frequencies
  // are at the bottom; the newest spectrum is the rightmost column.
  for (size_t column = 0; column < AUDIO_SPECTROGRAM_COLUMNS; ++column) {
    size_t source = (audioSpectrogramHead + column) %
                    AUDIO_SPECTROGRAM_COLUMNS;
    for (size_t row = 0; row < AUDIO_SPECTRUM_ROWS; ++row) {
      uint8_t value = audioSpectrogram[source][row];
      displayCanvas.fillRect(8 + (int16_t)column * 4,
                             65 + (int16_t)(AUDIO_SPECTRUM_ROWS - row - 1) * 4,
                             4, 4, audioSpectrogramColor(value));
    }
  }
  displayCanvas.drawRect(7, 64, 306, 98, TFT_DARKGREY);
  if (audioActive && !audioAnalysisReady()) {
    displayCanvas.fillRoundRect(92, 98, 136, 28, 5, rgb565(24, 20, 29));
    displayCanvas.drawRoundRect(92, 98, 136, 28, 5, TFT_MAGENTA);
    displayCanvas.setTextColor(TFT_WHITE, rgb565(24, 20, 29));
    displayCanvas.setTextSize(1);
    displayCanvas.setCursor(119, 108);
    displayCanvas.print("CALIBRATING...");
  }

  drawAudioBandMeter(8, "BASS", audioSpectrum.bass.level, TFT_RED);
  drawAudioBandMeter(112, "MID", audioSpectrum.mid.level, TFT_GREEN);
  drawAudioBandMeter(216, "HIGH", audioSpectrum.treble.level, TFT_CYAN);

  displayCanvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  displayCanvas.setTextSize(1);
  displayCanvas.setCursor(8, 188);
  displayCanvas.printf("level %d  centroid %d  frames %lu  fail %lu/%lu",
                       (int)(audioEnvelope.level * 99.0f + 0.5f),
                       (int)(audioSpectrum.centroid * 99.0f + 0.5f),
                       (unsigned long)audioFrames,
                       (unsigned long)audioReadFailures,
                       (unsigned long)sendFail);

  drawTextButton(4, 199, 72, 36,
                 audioActive ? rgb565(145, 38, 45) : rgb565(27, 120, 65),
                 audioActive ? "PAUSE" : "START");
  drawTextButton(84, 199, 72, 36,
                 audioAuxReady() ? rgb565(24, 91, 125)
                                 : audioCanCycleInput() ? rgb565(145, 92, 20)
                                                        : rgb565(55, 59, 63),
                 "INPUT");
  drawTextButton(164, 199, 72, 36, rgb565(91, 35, 100), "MODE");
  char gainLabel[16];
  snprintf(gainLabel, sizeof(gainLabel), "GAIN %s",
           audioOutputGainName(audioOutputGain));
  drawTextButton(244, 199, 72, 36, rgb565(125, 76, 22), gainLabel);
}

// The full 320x240 PSRAM sprite takes about 60 ms to transfer on CoreS3. That
// caps the entire loop near 15 Hz if used for every spectral column. Keep the
// ordinary full redraw for app transitions, then transfer only the 304x96 plot
// while Audio is running. Small text/meters are updated directly at 5 Hz.
void drawAudioRealtime() {
  if (!audioPlotCanvasReady) {
    drawDisplay();
    return;
  }

  audioPlotCanvas.fillSprite(TFT_BLACK);
  for (size_t column = 0; column < AUDIO_SPECTROGRAM_COLUMNS; ++column) {
    size_t source = (audioSpectrogramHead + column) %
                    AUDIO_SPECTROGRAM_COLUMNS;
    for (size_t row = 0; row < AUDIO_SPECTRUM_ROWS; ++row) {
      audioPlotCanvas.fillRect(
          (int16_t)column * 4,
          (int16_t)(AUDIO_SPECTRUM_ROWS - row - 1) * 4,
          4, 4, audioSpectrogramColor(audioSpectrogram[source][row]));
    }
  }
  audioPlotCanvas.pushSprite(8, 65);

  static uint8_t slowUiDivider = 0;
  bool slowUiDue = (++slowUiDivider >= 5);
  if (slowUiDue) slowUiDivider = 0;

  M5.Display.startWrite();
  if (audioActive && !audioAnalysisReady()) {
    M5.Display.fillRoundRect(92, 98, 136, 28, 5, rgb565(24, 20, 29));
    M5.Display.drawRoundRect(92, 98, 136, 28, 5, TFT_MAGENTA);
    M5.Display.setTextFont(1);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_WHITE, rgb565(24, 20, 29));
    M5.Display.setCursor(119, 108);
    M5.Display.print("CALIBRATING...");
  }

  if (slowUiDue) {
    M5.Display.setTextFont(1);
    M5.Display.setTextSize(1);
    M5.Display.fillRect(8, 38, 304, 23, TFT_BLACK);
    M5.Display.setTextColor(audioInputReady ? TFT_GREEN : TFT_RED, TFT_BLACK);
    M5.Display.setCursor(8, 38);
    M5.Display.printf("%s  %s", audioInputLabel(),
                      audioActive ? "PUBLISHING" : "PAUSED");
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(230, 38);
    M5.Display.printf("%d live", peerCount(true));
    M5.Display.setTextColor(TFT_MAGENTA, TFT_BLACK);
    M5.Display.setCursor(8, 51);
    M5.Display.printf("%s  GAIN %s", audioModeName(audioMode),
                      audioOutputGainName(audioOutputGain));
    M5.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    M5.Display.setCursor(174, 51);
    uint32_t analysisRate = audioTiming.analysis.rateMilliHz();
    uint32_t publishRate = audioTiming.publish.rateMilliHz();
    M5.Display.printf("FFT %lu.%lu TX %lu.%lu",
                      (unsigned long)(analysisRate / 1000),
                      (unsigned long)((analysisRate % 1000) / 100),
                      (unsigned long)(publishRate / 1000),
                      (unsigned long)((publishRate % 1000) / 100));

    M5.Display.fillRect(8, 165, 304, 34, TFT_BLACK);
    const char *labels[3] = {"BASS", "MID", "HIGH"};
    const float levels[3] = {audioSpectrum.bass.level,
                             audioSpectrum.mid.level,
                             audioSpectrum.treble.level};
    const uint16_t colors[3] = {TFT_RED, TFT_GREEN, TFT_CYAN};
    const int16_t xs[3] = {8, 112, 216};
    for (size_t i = 0; i < 3; ++i) {
      M5.Display.setTextColor(colors[i], TFT_BLACK);
      M5.Display.setCursor(xs[i], 165);
      M5.Display.printf("%s %2d", labels[i],
                        (int)(audioClampUnit(levels[i]) * 99.0f + 0.5f));
      M5.Display.drawRoundRect(xs[i], 176, 96, 9, 3, TFT_DARKGREY);
      int width = (int)(audioClampUnit(levels[i]) * 92.0f + 0.5f);
      if (width > 0)
        M5.Display.fillRect(xs[i] + 2, 178, width, 5, colors[i]);
    }
    M5.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    M5.Display.setCursor(8, 188);
    M5.Display.printf("level %d  centroid %d  frames %lu  fail %lu/%lu",
                      (int)(audioEnvelope.level * 99.0f + 0.5f),
                      (int)(audioSpectrum.centroid * 99.0f + 0.5f),
                      (unsigned long)audioFrames,
                      (unsigned long)audioReadFailures,
                      (unsigned long)sendFail);
  }
  M5.Display.endWrite();
}

void openCoreS3App(CoreS3App app) {
  if (currentApp == CORES3_APP_AUDIO && app != CORES3_APP_AUDIO && audioActive)
    setAudioActive(false);
  currentApp = app;
  if (currentApp == CORES3_APP_AUDIO && audioInputReady && !audioActive)
    setAudioActive(true);
  if (displayCanvasReady) drawDisplay();
}

void handleAppTouch(int16_t x, int16_t y) {
  if (currentApp != CORES3_APP_HOME && y < 34 && x < 66) {
    openCoreS3App(CORES3_APP_HOME);
    return;
  }
  if (currentApp == CORES3_APP_HOME) {
    if (y >= 65 && y <= 225 && x < 160)
      openCoreS3App(CORES3_APP_LISTENER);
    else if (y >= 65 && y <= 225 && x >= 160 && audioInputReady)
      openCoreS3App(CORES3_APP_AUDIO);
    return;
  }
  if (currentApp == CORES3_APP_LISTENER) {
    uint16_t order[NB_MAX_TRACKED];
    size_t count = sortedPeerIndexes(order);
    if (y >= 205) {
      if (x < 80 && listenerPage > 0) --listenerPage;
      if (x > 240 && listenerPage + 1 < cores3PageCount(count)) ++listenerPage;
      drawDisplay();
      return;
    }
    if (y >= 53 && y < 201) {
      int col = (x - 4) / 52;
      int row = (y - 53) / 37;
      if (x >= 4 && col >= 0 && col < 6 && row >= 0 && row < 4 &&
          (x - 4) % 52 < 48 && (y - 53) % 37 < 34) {
        size_t position = cores3PageStart(listenerPage, count) + row * 6 + col;
        if (position < count) {
          listenerDetailPeer = order[position];
          currentApp = CORES3_APP_LISTENER_DETAIL;
          drawDisplay();
        }
      }
    }
    return;
  }
  if (currentApp == CORES3_APP_LISTENER_DETAIL) {
    if (y >= 190) openCoreS3App(CORES3_APP_LISTENER);
    return;
  }
  if (currentApp == CORES3_APP_AUDIO && y >= 195) {
    if (x < 80)
      setAudioActive(!audioActive);
    else if (x < 160)
      nextAudioInput();
    else if (x < 240)
      nextAudioMode();
    else
      nextAudioOutputGain();
    drawDisplay();
  }
}
#endif

void drawDisplay() {
  if (!displayCanvasReady) return;
  // Render the whole app off-screen, then transfer the completed frame. This
  // avoids the black flash caused by clearing the physical LCD before redraw.
  displayCanvas.fillSprite(TFT_BLACK);
  displayCanvas.setTextFont(1);
#if CORES3_CAMBIUM_MODE
  displayCanvas.setTextSize(2);
  displayCanvas.setCursor(8, 6);
  displayCanvas.setTextColor(TFT_CYAN, TFT_BLACK);
  displayCanvas.println("RESONANCE CAMBIUM");
  displayCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
  displayCanvas.printf("%02X%02X%02X  ESP-NOW ch %d\n", myId[0], myId[1],
                       myId[2], cambiumChannel);
  displayCanvas.printf("radio %-4s  binary USB\n", radioReady ? "UP" : "FAIL");
  displayCanvas.printf("peers %d live / %d seen\n", peerCount(true),
                       peerCount(false));
  displayCanvas.printf("tx ok/fail %lu/%lu drop %lu\n",
                       (unsigned long)sendOk, (unsigned long)sendFail,
                       (unsigned long)rxQueueDrops);
#else
  switch (currentApp) {
  case CORES3_APP_LISTENER: drawListenerGrid(); break;
  case CORES3_APP_AUDIO: drawAudioApp(); break;
  case CORES3_APP_LISTENER_DETAIL: drawListenerDetail(); break;
  default: drawLauncher(); break;
  }
#endif
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
  // Let M5Unified configure the CoreS3 mic pins/callback even in a Module
  // Audio build. It does not start the mic here; the external module still
  // gets first choice in setupAudioInput(), while a missing module can now
  // fall back to M5.Mic.begin() instead of reporting a false FAILED state.
  config.internal_mic = CORES3_AUDIO_REACTIVE_MODE;
  config.internal_spk = false;
  config.led_brightness = 24;
  M5.begin(config);

  delay(1200);
#if !CORES3_CAMBIUM_MODE
  Serial.println();
  Serial.println("=== Resonance net-bench " CORES3_BRIDGE_VERSION " ===");
  Serial.printf("role=master channel=%d frame_hz=0 hb_hz=0\n", NB_CHANNEL);
  Serial.println("mode: BRIDGE OS (CoreS3; wireless Listener + Audio apps; USB optional)");
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
#if CORES3_AUDIO_REACTIVE_MODE
  audioPlotCanvas.setPsram(true);
  audioPlotCanvas.setColorDepth(16);
  audioPlotCanvasReady = audioPlotCanvas.createSprite(304, 96) != nullptr;
#endif
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
#if CORES3_AUDIO_REACTIVE_MODE
  uint32_t loopStartedUs = micros();
#endif
  esp_task_wdt_reset();
  M5.update();
#if !CORES3_CAMBIUM_MODE
  if (M5.Touch.getCount() && M5.Touch.getDetail(0).wasClicked()) {
    auto touch = M5.Touch.getDetail(0);
    handleAppTouch(touch.x, touch.y);
  }
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

  static AudioPeriodicDeadline bridgeStatusDeadline;
  static AudioPeriodicDeadline normalDisplayDeadline;
  uint32_t now = millis();
  if (bridgeStatusDeadline.take(now, 1000 / NB_BRIDGE_HZ)) {
#if CORES3_CAMBIUM_MODE
    sendCambiumStatus();
#else
    emitBridgeStats();
#endif
  }
  bool displayDue = false;
#if CORES3_AUDIO_REACTIVE_MODE && !CORES3_CAMBIUM_MODE
  bool timedAudioDisplay = currentApp == CORES3_APP_AUDIO && audioActive;
  if (timedAudioDisplay)
    displayDue = audioTiming.displayDeadline.take(now,
                                                   AUDIO_ANALYSIS_PERIOD_MS);
  else
#endif
    displayDue = normalDisplayDeadline.take(now, 1000);
  if (displayDue) {
#if CORES3_AUDIO_REACTIVE_MODE && !CORES3_CAMBIUM_MODE
    uint32_t displayStartedUs = micros();
    if (timedAudioDisplay) audioTiming.display.note(now);
#endif
#if CORES3_AUDIO_REACTIVE_MODE && !CORES3_CAMBIUM_MODE
    if (timedAudioDisplay)
      drawAudioRealtime();
    else
#endif
      drawDisplay();
#if CORES3_AUDIO_REACTIVE_MODE && !CORES3_CAMBIUM_MODE
    if (timedAudioDisplay)
      AudioRuntimeTiming::noteMax(micros() - displayStartedUs,
                                  &audioTiming.displayMaxUs);
#endif
  }
#if CORES3_AUDIO_REACTIVE_MODE
  if (audioActive)
    AudioRuntimeTiming::noteMax(micros() - loopStartedUs,
                                &audioTiming.loopMaxUs);
#endif
  delay(2);
}
