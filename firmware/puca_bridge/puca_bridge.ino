// Resonance PUCA performance-audio bridge (ADR 0035) -- PROOF OF CONCEPT.
//
// *** STATUS: UNVERIFIED. NEVER RUN ON HARDWARE. ***
// Written 2026-08-19 while the PUCA was not USB-enumerated; it compiles but has
// never touched the board. Every pin and register value below is sourced from
// the vendor repo (github.com/ohmic-net/puca_dsp) and marked with provenance.
// Treat the bring-up checklist in README.md as mandatory before trusting any
// of it.
//
// Target: PUCA DSP **Original Edition** (ESP32-PICO-D4 + WM8978 codec, 8 MB
// PSRAM) on the Ohmic 6 HP Eurorack expansion. This is a classic dual-core
// ESP32 -- NOT an S3; it does not run the CoreS3 bridge binary.
//
// Role (ADR 0035 / hardware/puca-audio-bridge/README.md):
//   audio in (onboard MEMS mics or 3.5 mm line-in from the RODE NTG)
//     -> WM8978 ADC -> I2S RX @ 16 kHz mono
//     -> shared envelope tracker (firmware/cores3_bridge/audio_reactive.h)
//     -> NB_DIRECT_FRAME broadcast at ~10 Hz on fixed ESP-NOW channel 11
//        to fixtures heard via NB_HEARTBEAT.
// Constraints honored:
//   - reuses the canonical wire contract (fixture/src/core/packet.h); no new
//     packet type (ADR 0035 6 forbids one until explicitly specced);
//   - preserves the fixture 3 s stale-frame fallback + 10 s micro-lease: any
//     silence/reboot/cable-pull just stops frames and the fleet goes back to
//     autonomous behavior;
//   - no WiFi AP (the factory image's "PUCA DSP" softAP must not exist here);
//     STA stays unassociated, channel pinned to 11 and printed at boot;
//   - no raw audio on the air, ever.
//
// ---------------------------------------------------------------------------
// PIN PROVENANCE (all sources fetched 2026-08-19 from
// https://github.com/ohmic-net/puca_dsp @ main)
//
//   [IDF]  puca_dsp-esp-idf/main/main.cpp             (I2S gpio_cfg block)
//   [CVT]  puca-eurorack/hardware_test_arduino/Puca_Eurorack_CV_test/
//          Puca_Eurorack_CV_test.ino                  (knob/CV ADC channels)
//   [TRG]  puca-eurorack/hardware_test_arduino/Puca_Eurorack_trigger_test/
//          Puca_Eurorack_trigger_test.ino             (TOUCH/TRIG/LED pins)
//   [WMH]  .../Puca_Eurorack_CV_test/WM8978.h          (I2C pins + 0x1A addr)
//   [WMC]  .../Puca_Eurorack_CV_test/WM8978.cpp        (register semantics)
//   [FAU]  .../Puca_Eurorack_CV_test/sine_add.h/.cpp   (PICO_DSP=true branch:
//          bck=23 ws=25 dout=26 din=27; MCLK via CLK_OUT1 on GPIO0)
//   [DSH]  documentation/puca_dsp_datasheet_v1.1.pdf   (GPIO header table)
//   [RDM]  README.md (line-in 1 Mohm / 3.3 Vpp max; BT_LVL jumper -> GPIO14)
//
// UNCONFIRMED (could not be pinned from sources; bench-verify before trusting):
//   - which stereo slot (L/R) the mono I2S RX captures, and which physical
//     mic / line channel that is;
//   - paw touch polarity: [TRG] lights an LED while touchState==1, so HIGH is
//     assumed = touched, but the carrier's touch circuit is not documented;
//   - knob rotation direction vs ADC value (CW = up is assumed);
//   - whether the CV2/CV3 jacks share the ADC nets with the pots (eurorack
//     schematic PDF not parsed) -- a patched CV cable may fight the knobs;
//   - MICBEN requirement for the Knowles MEMS pair (upstream enables it);
//   - no amp/enable GPIO exists in any upstream source; the 1 W speaker driver
//     hangs off WM8978 LOUT2/ROUT2 which this firmware leaves powered down.
// ---------------------------------------------------------------------------

#include <ESP_I2S.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#include <math.h>
#include <string.h>

// Wire contract: included via -I <firmware root> (see build.sh) so the fleet
// packet definitions come from the ONE canonical file -- never forked.
#include "fixture/src/core/packet.h"
// Shared envelope tracker + per-slot color, exactly the cores3 bridge's file
// (relative include, not a copy -- one tracker, one tuning).
#include "../cores3_bridge/audio_reactive.h"

#define PUCA_BRIDGE_VERSION "0.1.0-poc"

#ifndef NB_CHANNEL
#define NB_CHANNEL 11
#endif

#ifndef NB_MAX_TRACKED
#define NB_MAX_TRACKED 192
#endif

#ifndef NB_WDT_S
#define NB_WDT_S 8
#endif

// ---- pins (see provenance table above) --------------------------------------
static const int PIN_I2C_SDA = 19;   // [WMH] I2C_MASTER_SDA_IO
static const int PIN_I2C_SCL = 18;   // [WMH] I2C_MASTER_SCL_IO
static const int PIN_I2S_MCLK = 0;   // [IDF] .mclk = GPIO_NUM_0 ([FAU] CLK_OUT1)
static const int PIN_I2S_BCLK = 23;  // [IDF] .bclk = GPIO_NUM_23
static const int PIN_I2S_WS = 25;    // [IDF] .ws   = GPIO_NUM_25 (LRCLK)
static const int PIN_I2S_DOUT = 26;  // [IDF] .dout = GPIO_NUM_26 (to DAC; idle here)
static const int PIN_I2S_DIN = 27;   // [IDF] .din  = GPIO_NUM_27 (ADC -> ESP32)
static const int PIN_KNOB1 = 33;     // [CVT] CV2 top pot, ADC1_CH5 -> sensitivity
static const int PIN_KNOB2 = 34;     // [CVT] CV3 bottom pot, ADC1_CH6 -> mode param
static const int PIN_TOUCH = 15;     // [TRG] carrier capacitive paw, digital read
static const int PIN_LED_BOARD = 5;  // [TRG] LED1, PUCA_DSP onboard
static const int PIN_LED_TOP = 2;    // [TRG] LED2, carrier (LOW after boot)
static const int PIN_LED_BTM = 4;    // [TRG] LED3, carrier (LOW after boot)
// Not used but known: BUTTON=36 onboard, TRIG1=13, TRIG2=14 (HIGH after boot)
// [TRG]; BT_LVL jumper routes VBAT to GPIO14 [RDM] -- conflicts with TRIG2.

// ---- shared types (defined before the first function definition so the
// Arduino-injected prototypes that mention them stay valid) --------------------
struct PeerLite {
  bool used;
  uint8_t id[3];
  uint32_t lastHeardMs;
  bool hasFw;
  char fwRev[24];
};

struct RxItem {
  uint8_t len;
  uint8_t data[250];
};
static_assert(sizeof(NbHeartbeat) <= sizeof(RxItem::data),
              "heartbeat outgrew the PUCA bridge receive buffer");

// ---- WM8978 minimal register driver -----------------------------------------
// 7-bit I2C address 0x1A [WMH]. Registers are 7-bit addr + 9-bit value packed
// into two bytes: byte0 = (reg<<1)|val[8], byte1 = val[7:0] [WMC writeReg].
// The chip is write-only over I2C; there is no readback.
static const uint8_t WM8978_ADDR = 0x1A;

static uint32_t wm8978WriteErrors = 0;

static bool wm8978WriteReg(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(WM8978_ADDR);
  Wire.write((uint8_t)((reg << 1) | ((val >> 8) & 0x01)));
  Wire.write((uint8_t)(val & 0xFF));
  bool ok = Wire.endTransmission() == 0;
  if (!ok) ++wm8978WriteErrors;
  return ok;
}

// Input path selection. false = onboard Knowles MEMS pair (differential into
// the input PGAs, routing per [WMC] inputCfg(1,0,0)); true = 3.5 mm stereo
// line-in (L2/R2 straight into the boost mixer at 0 dB, PGAs muted, per [WMC]
// lineinGain(5)). ALC/AGC stays at its power-on default (OFF, R32) on purpose:
// gain staging lives at the RODE dial + KNOB1, not in a codec loop
// (docs/research/AUDIO_INGEST_NTG_PUCA_2026-08-04.md item 2).
static bool wm8978SetInput(bool lineIn) {
  bool ok = true;
  if (lineIn) {
    ok &= wm8978WriteReg(44, 0x000);        // R44: nothing into the PGAs
    ok &= wm8978WriteReg(45, 0x040);        // R45: left PGA muted
    ok &= wm8978WriteReg(46, 0x140);        // R46: right PGA muted (+update)
    ok &= wm8978WriteReg(47, 0x050);        // R47: L2->boost 0 dB, PGABOOST off
    ok &= wm8978WriteReg(48, 0x050);        // R48: R2->boost 0 dB, PGABOOST off
  } else {
    ok &= wm8978WriteReg(44, 0x033);        // R44: LIP/LIN+RIP/RIN -> PGAs [WMC]
    ok &= wm8978WriteReg(45, 30);           // R45: left PGA +10.5 dB ([CVT] micGain(30))
    ok &= wm8978WriteReg(46, 0x100 | 30);   // R46: right PGA +10.5 dB, update
    ok &= wm8978WriteReg(47, 0x100);        // R47: +20 dB PGABOOSTL [WMC init]
    ok &= wm8978WriteReg(48, 0x100);        // R48: +20 dB PGABOOSTR [WMC init]
  }
  return ok;
}

// Minimal RX-only init: power/bias, ADC path, I2S slave, 16 kHz. Derived from
// the upstream init()/addaCfg()/sampleRate()/i2sCfg() sequence [WMC][CVT] with
// the DAC/headphone/speaker output enables stripped (this bridge never plays
// audio). Register fields cross-checked against documentation/WM8978_v4.5
// datasheet.pdf in the same repo.
static bool wm8978Init() {
  bool ok = true;
  ok &= wm8978WriteReg(0, 0x000);  // R0: software reset [WMC init]
  delay(10);
  ok &= wm8978WriteReg(1, 0x01B);  // R1: VMIDSEL=11, BIASEN, MICBEN (upstream
                                   //     0x9B minus OUT4MIXEN -- no outputs)
  ok &= wm8978WriteReg(2, 0x03F);  // R2: BOOSTENL/R, INPPGAENL/R, ADCENL/R
                                   //     (upstream 0x1B0|addaCfg minus LOUT1/ROUT1)
  ok &= wm8978WriteReg(3, 0x000);  // R3: DAC + output mixers all OFF (RX only)
  ok &= wm8978WriteReg(4, 0x010);  // R4: AIF = I2S format, 16-bit ([CVT] i2sCfg(2,0))
  ok &= wm8978WriteReg(6, 0x000);  // R6: codec = clock SLAVE, MCLK undivided;
                                   //     ESP32 masters BCLK/LRCLK/MCLK ([WMC]
                                   //     writeReg(6,0) + [IDF] I2S_ROLE_MASTER)
  ok &= wm8978WriteReg(7, 0x006);  // R7: SMPLR=011 -> 16 kHz filters ([WMC]
                                   //     sampleRate(3))
  ok &= wm8978WriteReg(14, 0x108); // R14: HPFEN (DC block) + ADCOSR128 (upstream
                                   //     sets OSR only; HPF is the chip default
                                   //     we keep instead of clearing)
  return ok;
}

// ---- global state ------------------------------------------------------------
static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t myMac[6] = {};
uint8_t myId[3] = {};
volatile uint32_t sendOk = 0;
volatile uint32_t sendFail = 0;
uint32_t txSeq = 0;
bool radioReady = false;
bool codecReady = false;
bool audioInputReady = false;
bool audioActive = true;
bool inputLineIn = false;  // boot on the zero-cable MEMS mics; 'I' flips to line
uint32_t audioFrames = 0;
uint32_t audioReadFailures = 0;

I2SClass i2sIn;
AudioEnvelope audioEnvelope;

enum BridgeMode : uint8_t {
  MODE_CLASSIC = 0, // per-slot R/G/B envelope (cores3 CLASSIC); KNOB2 = ceiling
  MODE_EMBER,       // shared warm white envelope; KNOB2 = ceiling
  MODE_HUE,         // KNOB2 picks the hue, envelope drives the value
  MODE_OFF,         // stop sending; fleet reverts via 3 s staleness + lease
  MODE_COUNT,
};
BridgeMode mode = MODE_CLASSIC;

float knob1Filt = 0.0f;   // smoothed 0..1
float knob2Filt = 0.0f;   // smoothed 0..1
bool knobsPrimed = false;
float knobGain = 1.0f;    // 0.25x..4x sensitivity multiplier from KNOB1
float lastShownLevel = 0.0f;

PeerLite peers[NB_MAX_TRACKED] = {};

QueueHandle_t rxQueue = nullptr;
volatile uint32_t rxQueueDrops = 0;

// uint8_t (not BridgeMode) so the Arduino-generated prototype -- which is
// injected above the enum definition -- stays a valid declaration.
const char *modeName(uint8_t m) {
  switch (m) {
  case MODE_CLASSIC: return "CLASSIC";
  case MODE_EMBER: return "EMBER";
  case MODE_HUE: return "HUE";
  case MODE_OFF: return "OFF";
  default: return "UNKNOWN";
  }
}

// ---- ESP-NOW (ported from cores3_bridge; same discipline) --------------------
void fillHeader(NbHeader *h, uint8_t type) {
  h->ver = NB_PROTO_VER;
  h->type = type;
  memcpy(h->src_id, myId, sizeof(myId));
  h->seq = txSeq++;
  h->uptime_ms = millis();
}

void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
  if (!rxQueue || len < (int)sizeof(NbHeader) || len > (int)sizeof(RxItem::data))
    return;
  const NbHeader *h = (const NbHeader *)data;
  if (h->ver != NB_PROTO_VER || h->type != NB_HEARTBEAT) return;

  RxItem item = {};
  item.len = (uint8_t)len;
  memcpy(item.data, data, len);
  if (xQueueSend(rxQueue, &item, 0) != pdTRUE) ++rxQueueDrops;
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
  // This also guarantees the factory "PUCA DSP" softAP behavior does not exist
  // in this firmware (ADR 0035).
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
  memcpy(peer.peer_addr, BCAST, sizeof(BCAST));
  peer.channel = NB_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_err_t result = esp_now_add_peer(&peer);
  if (result != ESP_OK && result != ESP_ERR_ESPNOW_EXIST) {
    Serial.printf("esp_now_add_peer FAILED: %d\n", (int)result);
    return false;
  }
  Serial.printf("esp-now up, ch=%d, broadcast peer registered\n", NB_CHANNEL);
  return true;
}

// ---- heartbeat listener (minimal port of the cores3 peer tracker) ------------
PeerLite *findPeer(const uint8_t id[3], bool create) {
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

void processHeartbeat(const RxItem &item) {
  const NbHeartbeat *hb = (const NbHeartbeat *)item.data;
  PeerLite *peer = findPeer(hb->h.src_id, true);
  if (!peer) return;
  peer->lastHeardMs = millis();
  if (NB_HAS_HB_FIELD(item.len, fw_rev)) {
    peer->hasFw = true;
    memcpy(peer->fwRev, hb->fw_rev, sizeof(peer->fwRev));
    peer->fwRev[sizeof(peer->fwRev) - 1] = '\0';
  }
}

void processRx() {
  if (!rxQueue) return;
  RxItem item;
  while (xQueueReceive(rxQueue, &item, 0) == pdTRUE) {
    const NbHeader *h = (const NbHeader *)item.data;
    if (memcmp(h->src_id, myId, sizeof(myId)) == 0) continue;
    processHeartbeat(item);
  }
}

int livePeerCount() {
  int count = 0;
  uint32_t now = millis();
  for (size_t i = 0; i < NB_MAX_TRACKED; ++i) {
    if (peers[i].used && now - peers[i].lastHeardMs <= 5000) ++count;
  }
  return count;
}

// Identical filter to cores3 collectLiveAudioIds: fixtures heard in the last
// 5 s, minus peers whose full heartbeat identifies non-fixture firmware, sorted
// by id so slot colors are stable regardless of heartbeat arrival order.
uint8_t collectLiveFixtureIds(uint8_t ids[NB_DIRECT_MAX_ENTRIES][3]) {
  uint8_t count = 0;
  uint32_t now = millis();
  for (size_t i = 0; i < NB_MAX_TRACKED && count < NB_DIRECT_MAX_ENTRIES; ++i) {
    if (!peers[i].used || now - peers[i].lastHeardMs > 5000) continue;
    if (peers[i].hasFw &&
        strncmp(peers[i].fwRev, "fixture-", strlen("fixture-")) != 0)
      continue;
    memcpy(ids[count++], peers[i].id, 3);
  }
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

// ---- bridge-side color math (ported from cores3_bridge.ino) ------------------
// The wire contract (packet.h) stays untouched; this is sender-side math only.
void hsvToRgb(float hue, float sat, float val, uint8_t *r, uint8_t *g, uint8_t *b) {
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

AudioColor colorForMode(uint8_t slot, float shown) {
  if (shown < 0.0f) shown = 0.0f;
  if (shown > 1.0f) shown = 1.0f;
  switch (mode) {
  case MODE_CLASSIC:
    // KNOB2 is the brightness ceiling; scale the level before the per-slot
    // color so the R/G/B ratios stay put.
    return audioColorForSlot(slot, shown * knob2Filt);
  case MODE_EMBER: {
    // Same warm-white ratios as cores3 AUDIO_MODE_EMBER; KNOB2 = ceiling.
    float v = shown * knob2Filt;
    return {(uint8_t)(v * 255.0f + 0.5f),
            (uint8_t)(v * 0.55f * 255.0f + 0.5f),
            (uint8_t)(v * 0.10f * 255.0f + 0.5f),
            (uint8_t)(v * 0.60f * 255.0f + 0.5f)};
  }
  case MODE_HUE: {
    // KNOB2 sets the hue; the envelope owns the value. No ceiling here --
    // the knob is spoken for.
    uint8_t r, g, b;
    hsvToRgb(knob2Filt, 1.0f, shown, &r, &g, &b);
    return {r, g, b, 0};
  }
  default:
    return {0, 0, 0, 0}; // MODE_OFF only sends the one blackout frame
  }
}

void sendDirectFrame(float shown) {
  if (!radioReady) return;
  uint8_t ids[NB_DIRECT_MAX_ENTRIES][3];
  uint8_t count = collectLiveFixtureIds(ids);
  if (!count) return;

  NbDirectFrame frame = {};
  fillHeader(&frame.h, NB_DIRECT_FRAME);
  frame.flags = 0x03; // 10 s micro-lease + hard-cut; the envelope owns smoothing
  frame.count = count;
  for (uint8_t i = 0; i < count; ++i) {
    memcpy(frame.entries[i].id, ids[i], 3);
    AudioColor color = colorForMode(i, shown);
    frame.entries[i].r = color.r;
    frame.entries[i].g = color.g;
    frame.entries[i].b = color.b;
    frame.entries[i].w = color.w;
  }
  size_t wireLen = offsetof(NbDirectFrame, entries) + count * sizeof(NbDirectEntry);
  if (esp_now_send(BCAST, (const uint8_t *)&frame, wireLen) == ESP_OK)
    ++audioFrames;
}

// ---- controls -----------------------------------------------------------------
void nextMode() {
  mode = (BridgeMode)((mode + 1) % MODE_COUNT);
  if (mode == MODE_OFF) sendDirectFrame(0.0f); // black now; staleness + lease
                                               // expiry return autonomy
  digitalWrite(PIN_LED_TOP, (mode != MODE_OFF && audioActive) ? HIGH : LOW);
  Serial.printf("mode=%s\n", modeName(mode));
}

void setAudioActive(bool active) {
  if (active == audioActive) return;
  if (!active) sendDirectFrame(0.0f);
  audioActive = active;
  if (active) audioEnvelope = AudioEnvelope{}; // re-run noise calibration
  digitalWrite(PIN_LED_TOP, (mode != MODE_OFF && audioActive) ? HIGH : LOW);
  Serial.printf("audio -> %s\n", active ? "ON" : "OFF");
}

void setInputLineIn(bool lineIn) {
  inputLineIn = lineIn;
  bool ok = wm8978SetInput(lineIn);
  audioEnvelope = AudioEnvelope{}; // different noise floor -> recalibrate
  Serial.printf("input -> %s (%s)\n", lineIn ? "line" : "mic",
                ok ? "codec ack" : "I2C WRITE FAILED");
}

// KNOB1: sensitivity multiplier, log-mapped 0.25x (CCW) .. 4x (CW), 1x center.
// KNOB2: CLASSIC/EMBER brightness ceiling 0..1; HUE hue 0..1 turn.
// Both ADC1 channels, so readings survive WiFi being up. 10 Hz + light EMA.
void knobsTick() {
  float k1 = (float)analogRead(PIN_KNOB1) / 4095.0f;
  float k2 = (float)analogRead(PIN_KNOB2) / 4095.0f;
  if (!knobsPrimed) {
    knob1Filt = k1;
    knob2Filt = k2;
    knobsPrimed = true;
  } else {
    knob1Filt += (k1 - knob1Filt) * 0.3f;
    knob2Filt += (k2 - knob2Filt) * 0.3f;
  }
  knobGain = 0.25f * powf(16.0f, knob1Filt);
}

// Paw: HIGH assumed = touched (see UNCONFIRMED header note). 30 ms debounce,
// mode advances on the rising edge only.
void touchTick(uint32_t now) {
  static bool lastRaw = false;
  static bool pressed = false;
  static uint32_t lastChangeMs = 0;
  bool raw = digitalRead(PIN_TOUCH) == HIGH;
  if (raw != lastRaw) {
    lastRaw = raw;
    lastChangeMs = now;
  }
  if (now - lastChangeMs >= 30 && raw != pressed) {
    pressed = raw;
    if (pressed) nextMode();
  }
}

// ---- audio: 100 ms blocking reads pace the ~10 Hz send loop -------------------
void audioTick() {
  if (!audioInputReady) return;

  // 1600 samples = exactly 100 ms of 16 kHz mono, so draining the DMA both
  // keeps the envelope current and naturally paces this loop at ~10 Hz (the
  // ESP32 masters every clock, so the read cannot hang on a dead codec -- it
  // would just deliver silence).
  static int16_t samples[1600];
  size_t got = i2sIn.readBytes((char *)samples, sizeof(samples));
  if (got < sizeof(samples)) {
    ++audioReadFailures;
    if (got < sizeof(int16_t)) {
      delay(5);
      return;
    }
  }

  knobsTick();
  if (!audioActive) return;

  float level = audioEnvelope.update(samples, got / sizeof(int16_t));
  float shown = level * knobGain;
  if (shown > 1.0f) shown = 1.0f;
  lastShownLevel = shown;

  if (mode == MODE_OFF || !audioEnvelope.calibrated()) return;

  // The blocking read is the metronome; this floor only stops a burst if the
  // DMA had a backlog and reads briefly return instantly.
  static uint32_t lastSendMs = 0;
  uint32_t now = millis();
  if (now - lastSendMs < 80) return;
  lastSendMs = now;
  sendDirectFrame(shown);
}

// ---- serial CLI + status -------------------------------------------------------
void printStatusJson() {
  Serial.printf("{\"mode\":\"%s\",\"level\":%.3f,\"gain\":%.2f,\"hue\":%d,"
                "\"peers\":%d,\"sendok\":%lu,\"active\":%d,\"input\":\"%s\","
                "\"ceil\":%.2f,\"sendfail\":%lu,\"codec\":%d}\n",
                modeName(mode), lastShownLevel, knobGain,
                (int)(knob2Filt * 360.0f), livePeerCount(),
                (unsigned long)sendOk, audioActive ? 1 : 0,
                inputLineIn ? "line" : "mic", knob2Filt,
                (unsigned long)sendFail, codecReady ? 1 : 0);
}

void handleSerial() {
  while (Serial.available() > 0) {
    int c = Serial.read();
    switch (c) {
    case 't': printStatusJson(); break;
    case 'M': nextMode(); break;
    case 'A': setAudioActive(!audioActive); break;
    case 'I': setInputLineIn(!inputLineIn); break;
    case '\r': case '\n': case ' ': break;
    default:
      Serial.println("keys: t=status-json M=next-mode A=audio-toggle I=mic/line");
      break;
    }
  }
}

void statusTick(uint32_t now) {
  static uint32_t nextStatusMs = 0;
  if ((int32_t)(now - nextStatusMs) < 0) return;
  nextStatusMs = now + 1000;
  Serial.printf("puca mode=%s active=%d input=%s calibrated=%d level=%.3f "
                "gain=%.2f ceil=%.2f hue=%d peers=%d sendok=%lu sendfail=%lu "
                "frames=%lu readfail=%lu rxdrop=%lu i2cerr=%lu codec=%d up=%lu "
                "fw=puca-bridge-" PUCA_BRIDGE_VERSION "\n",
                modeName(mode), audioActive ? 1 : 0,
                inputLineIn ? "line" : "mic",
                audioEnvelope.calibrated() ? 1 : 0, lastShownLevel, knobGain,
                knob2Filt, (int)(knob2Filt * 360.0f), livePeerCount(),
                (unsigned long)sendOk, (unsigned long)sendFail,
                (unsigned long)audioFrames, (unsigned long)audioReadFailures,
                (unsigned long)rxQueueDrops, (unsigned long)wm8978WriteErrors,
                codecReady ? 1 : 0, (unsigned long)millis());
}

// ---- setup / loop ----------------------------------------------------------------
void setupWatchdog() {
  esp_task_wdt_config_t config = {};
  config.timeout_ms = NB_WDT_S * 1000;
  config.idle_core_mask = 0;
  config.trigger_panic = true;
  esp_err_t result = esp_task_wdt_reconfigure(&config);
  if (result == ESP_ERR_INVALID_STATE) esp_task_wdt_init(&config);
  esp_task_wdt_add(nullptr);
}

bool setupI2S() {
  // ESP32 is I2S master; ESP_I2S std mode defaults MCLK to 256 x fs, matching
  // the upstream I2S_MCLK_MULTIPLE_256 [IDF]. Mono RX takes the LEFT slot
  // (which physical channel that is on this board is UNCONFIRMED).
  i2sIn.setPins(PIN_I2S_BCLK, PIN_I2S_WS, PIN_I2S_DOUT, PIN_I2S_DIN, PIN_I2S_MCLK);
  return i2sIn.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT,
                     I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  Serial.println();
  Serial.println("=== Resonance puca-bridge " PUCA_BRIDGE_VERSION " ===");
  Serial.println("*** UNVERIFIED PoC: this build has never run on hardware ***");
  Serial.printf("role=audio-bridge channel=%d target=PUCA DSP Original Edition\n",
                NB_CHANNEL);

  pinMode(PIN_LED_BOARD, OUTPUT);
  pinMode(PIN_LED_TOP, OUTPUT);
  pinMode(PIN_LED_BTM, OUTPUT);
  pinMode(PIN_TOUCH, INPUT); // upstream trigger test uses plain INPUT [TRG]
  digitalWrite(PIN_LED_BOARD, LOW);
  digitalWrite(PIN_LED_TOP, LOW);
  digitalWrite(PIN_LED_BTM, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_KNOB1, ADC_11db);
  analogSetPinAttenuation(PIN_KNOB2, ADC_11db);

  esp_read_mac(myMac, ESP_MAC_WIFI_STA);
  memcpy(myId, myMac + 3, sizeof(myId));
  Serial.printf("node id=%02X%02X%02X mac=%02X:%02X:%02X:%02X:%02X:%02X\n",
                myId[0], myId[1], myId[2], myMac[0], myMac[1], myMac[2],
                myMac[3], myMac[4], myMac[5]);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000); // [WMH] pins + 100 kHz
  codecReady = wm8978Init() && wm8978SetInput(inputLineIn);
  Serial.printf("codec: WM8978 @0x%02X %s (input=%s)\n", WM8978_ADDR,
                codecReady ? "init ok" : "INIT FAILED", inputLineIn ? "line" : "mic");

  audioInputReady = setupI2S();
  Serial.printf("i2s: 16 kHz mono RX %s (mclk=%d bclk=%d ws=%d din=%d)\n",
                audioInputReady ? "ready" : "INIT FAILED",
                PIN_I2S_MCLK, PIN_I2S_BCLK, PIN_I2S_WS, PIN_I2S_DIN);
  if (!audioInputReady) audioActive = false;

  rxQueue = xQueueCreate(32, sizeof(RxItem));
  if (!rxQueue) Serial.println("rx queue allocation FAILED");
  else radioReady = setupEspNow();
  setupWatchdog();

  digitalWrite(PIN_LED_BOARD, radioReady ? HIGH : LOW);
  digitalWrite(PIN_LED_TOP, (mode != MODE_OFF && audioActive) ? HIGH : LOW);
  Serial.printf("mode=%s gain-knob=GPIO%d mode-knob=GPIO%d paw=GPIO%d\n",
                modeName(mode), PIN_KNOB1, PIN_KNOB2, PIN_TOUCH);
  Serial.println("keys: t=status-json M=next-mode A=audio-toggle I=mic/line");
}

void loop() {
  esp_task_wdt_reset();
  uint32_t now = millis();
  processRx();
  handleSerial();
  touchTick(now);
  audioTick(); // blocks ~100 ms while audio is up; that is the loop cadence
  statusTick(millis());
  if (!audioInputReady) delay(10); // no I2S pacing -> do not spin the core
}
