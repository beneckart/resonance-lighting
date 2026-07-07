// Resonance Speaker Demo -- Adafruit STEMMA speaker (#3885, PAM8302 amp + mini
// speaker) on the PowerFeather, repurposing the LED header: V+ from the
// switchable 3V3 rail (GPIO4-gated), GND, signal on A0/GPIO10.
//
// Design intent (2026-07 noisemaker shootout): people disliked square-wave
// tone() "nintendo sounds" as at odds with the bamboo-tree aesthetic. The
// #3885 is an ANALOG amp, so this sketch does not use tone() at all: it runs a
// small fixed-point synth (39 kHz sample pump into a 78 kHz / 9-bit LEDC PWM
// carrier; the amp + speaker low-pass the carrier away) and renders organic
// percussion instead -- decaying-sine bamboo knocks, marimba, chime, water
// drip, noise shaker -- plus one square "beep" kept as the A/B baseline.
// "Ripple" plays a ~2.5 s cascade of knocks and "Grove" free-runs sparse
// random knocks, to preview what 150 fixtures rippling could feel like.
//
// Wiring (STEMMA speaker JST PH 3-pin, repurposed LED header):
//   3V3 header (GPIO4-gated) -> speaker V+
//   GND                      -> speaker G
//   A0 / GPIO10              -> speaker signal (AC-coupled PAM8302 input)
//
// The dashboard "Amp power" button toggles the 3V3 header rail itself -- the
// same software kill-switch the production power policy would use.
//
// Build/flash (USB): ./build.sh --port /dev/ttyACM1
// Web: http://speakerdemo.local/ or the IP in the serial banner (115200).
// OTA: curl -F "firmware=@speaker_demo.ino.bin" http://<ip>/update

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Wire.h>
#include "soc/ledc_struct.h" // direct duty writes from the sample ISR
#include "driver/rtc_io.h"   // read back the actual EN_3V3 pad level (SDK RTC-holds it)

#define FW_VERSION "speaker-demo-2026-07-07.9"

// PowerFeather SDK: rails + telemetry + guarded charging (sway_demo pattern --
// this unit may carry a cell; charging stays OFF until the gauge reports a
// plausible LFP voltage, and the solar guard handles USB/panel supplies).
#include <PowerFeather.h>
#include "../powerfeather_solar_guard.h"
using namespace PowerFeather;
#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 (build.sh passes it) so the SDK targets the V2."
#endif
#define SPK_MAINTAIN_V 4.6f // correct for USB; re-tune toward the panel MPP for solar work
bool gPfReady = false;
bool gChargeOn = false;

#define EN_3V3_PIN 4 // switchable 3V3 header rail (speaker V+), active HIGH

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif
#define AP_SSID "ResonanceSpeaker"
#define AP_PASS "resonance"

WebServer server(80);

// ---- Audio engine -----------------------------------------------------------
// LEDC channel 0 on AUDIO_PIN at 78.125 kHz / 9-bit, GPTimer sample ISR at
// exactly carrier/2 = 39062.5 Hz (integer-locked -- see the carrier note).
// The ISR mixes up to NVOICES fixed-point voices and writes the duty directly
// to the LEDC registers (the driver calls are not ISR-safe). The DC bias
// slews 0 <-> BIAS_MID over ~13 ms so the AC-coupled amp input never sees a
// pop; after 3 s of silence the bias ramps to 0 (PWM low = amp input quiet).

#ifndef AUDIO_PIN
#define AUDIO_PIN 10 // GPIO10 / A0 on the repurposed LED header
#endif
#define LEDC_CHAN 0
// Carrier and sample clock MUST be integer-locked or their intermod beats land
// in the audible band: the first bench build (39062.5 Hz carrier / 16000 Hz
// samples, unrelated) whined at |3fc-7fs| = 5187 Hz + |fc-fs| = 23 kHz
// (measured on a phone spectrum app, 2026-07-07). The LEDC clock here is the
// 40 MHz XTAL (that's also why 78125 Hz @ 10-bit was rejected outright: it
// needs an impossible 0.5 divider -- always check the ledcAttach* return).
// Divider-1.0 (dither-free) combos, sample ISR fixed at 39062.5 Hz (GPTimer
// 40 MHz / 1024, same crystal, so clock beats land at n x 39 kHz or DC):
//   carrier=0: 78125 Hz @ 9-bit  (fc = 2 x fs) -- DEFAULT, see below
//   carrier=1: 39062.5 Hz @ 10-bit (fc = fs)   -- A/B for other amp units
// Carrier choice is about the PAM8302's FREE-RUNNING ~250 kHz (loose) class-D
// oscillator beating the carrier's odd harmonics (controlled A/B, fixed
// placement, 2026-07-07): on this unit the 39 kHz carrier whined hard at
// ~6 kHz (consistent with its 5th harmonic ~195 kHz sitting near this chip's
// oscillator) while 78 kHz was beat-free (nearest strong harmonics beat at
// 33-45 kHz, ultrasonic). The oscillator varies chip to chip, so ACROSS A
// FLEET no carrier is universally safe -- the robust fixes are an RC low-pass
// on SIG (~1k + 10 nF) or an I2S amp; the A/B button is the per-unit probe.
// A 156250 Hz @ 8-bit mode was tried and REMOVED: it exceeds the modulator's
// ~125 kHz input Nyquist and plays grossly distorted. Keep carriers 40-80 kHz.
uint32_t gPwmHz = 0;
uint8_t gCarrierSel = 0;
int32_t gPwmBits = 9;
int32_t gDutyMax = (1 << 9) - 1;
int32_t gBiasMid = 1 << 8;
int32_t gMixShift = 16 - 9; // full-scale mix (+-32767) -> +-gBiasMid
int32_t gAmpFloor = 1 << (7 + 7); // Q23 amp whose peak mix contribution ~ 0.5 duty LSB
#define TIMER_HZ 40000000
#define TIMER_TICKS 1024  // 40 MHz / 1024 = 39062.5 Hz
#define SAMPLE_HZ 39062   // integer approx for synth math (13 ppm off)
#define IDLE_HOLD_SAMPLES (12 * SAMPLE_HZ / 10) // mute 1.2 s after last voice
#define NVOICES 12
#define Q30_ONE (1 << 30)

enum Wave : uint8_t { W_SINE = 0, W_SQUARE, W_NOISE };

struct Voice {
  volatile bool active;
  uint8_t wave;
  uint8_t lpK;      // noise one-pole low-pass strength (0 = white)
  uint32_t phase;   // Q32 phase accumulator
  uint32_t inc;     // phase increment per sample
  int32_t glide;    // Q30 per-sample multiplier on inc (Q30_ONE = none)
  int32_t amp;      // Q23 current amplitude (Q15 would truncate long tails to ~250 ms)
  int32_t decay;    // Q30 per-sample amplitude multiplier
  uint32_t hold;    // samples of constant amp before decay starts
  uint32_t delay;   // samples to wait before sounding (bias ramp / scheduling)
  int32_t lp;       // noise filter state
};

int16_t gSinLut[1024];            // filled in setup -> lives in DRAM, ISR-safe
Voice gV[NVOICES];
uint32_t gLfsr = 0x2468ACE1;
volatile int32_t gBias = 0;
volatile uint32_t gIdleSamples = 0x7FFFFFFF; // start "long idle" -> bias stays 0
volatile bool gAudioHalt = false;            // set during OTA flash + carrier swap
volatile uint32_t gIsrN = 0;                 // ISR invocations (rate check via /diag)
volatile int32_t gLastDuty = -1;             // skip redundant LEDC writes
int32_t gPrevTpdf = 0;                       // high-pass dither shaping state
bool gLedcOk = false;
hw_timer_t *gTimer = nullptr;

void IRAM_ATTR audioIsr() {
  gIsrN = gIsrN + 1;
  if (gAudioHalt) return;
  int32_t mix = 0;
  bool any = false;
  for (int i = 0; i < NVOICES; i++) {
    Voice &v = gV[i];
    if (!v.active) continue;
    any = true;
    if (v.delay) { v.delay--; continue; }
    int32_t s;
    if (v.wave == W_SINE) {
      s = gSinLut[v.phase >> 22];
    } else if (v.wave == W_SQUARE) {
      s = (v.phase & 0x80000000u) ? 24000 : -24000;
    } else {
      gLfsr ^= gLfsr << 13; gLfsr ^= gLfsr >> 17; gLfsr ^= gLfsr << 5;
      int32_t n = (int32_t)(int16_t)(gLfsr & 0xFFFF);
      v.lp += (n - v.lp) >> v.lpK;
      s = v.lp;
    }
    v.phase += v.inc;
    if (v.glide != Q30_ONE) v.inc = (uint32_t)(((uint64_t)v.inc * (uint32_t)v.glide) >> 30);
    mix += (int32_t)(((int64_t)s * v.amp) >> 23);
    if (v.hold) {
      v.hold--;
    } else {
      v.amp = (int32_t)(((int64_t)v.amp * v.decay) >> 30);
      // Kill at ~half a duty LSB: below that the tail is pure limit-cycle
      // whine (bench-measured, 2026-07-07), not audible program.
      if (v.amp < gAmpFloor) v.active = false;
    }
  }
  if (any) gIdleSamples = 0;
  else if (gIdleSamples < 0x7FFFFFFF) gIdleSamples = gIdleSamples + 1;
  int32_t tgt = (gIdleSamples > IDLE_HOLD_SAMPLES) ? 0 : gBiasMid;
  int32_t bias = gBias;
  if (bias < tgt) bias++;
  else if (bias > tgt) bias--;
  gBias = bias;
  int32_t duty = 0;
  if (bias > 0) {
    // High-pass-shaped TPDF dither, GATED on voice activity: decorrelates the
    // quantizer during notes (kills tonal limit cycles) while the shaping
    // (t[n]-t[n-1]) pushes the dither energy up toward fs/2 = 19.5 kHz where
    // neither ears nor this speaker respond. Idle-awake output is a constant
    // duty and needs no dither -- gating keeps the between-notes carrier clean.
    int32_t dith = 0;
    if (any) {
      gLfsr ^= gLfsr << 13; gLfsr ^= gLfsr >> 17; gLfsr ^= gLfsr << 5;
      uint32_t r1 = gLfsr;
      gLfsr ^= gLfsr << 13; gLfsr ^= gLfsr >> 17; gLfsr ^= gLfsr << 5;
      int32_t mask = (1 << gMixShift) - 1;
      int32_t t = (int32_t)(r1 & mask) + (int32_t)(gLfsr & mask) - mask;
      dith = t - gPrevTpdf;
      gPrevTpdf = t;
    }
    duty = bias + ((mix + dith) >> gMixShift); // full-scale mix swings +-gBiasMid
    if (duty < 0) duty = 0;
    if (duty > gDutyMax) duty = gDutyMax;
  }
  // ESP32-S3 LEDC: write duty, strobe duty_start, latch with low_speed_update.
  // sig_out_en is re-asserted with each write: ledcWrite(pin, 0) (and some
  // driver paths) can clear it via ledc_stop, and then duty writes go nowhere.
  // Skip the write entirely when duty is unchanged (idle carrier stays clean).
  if (duty != gLastDuty) {
    gLastDuty = duty;
    LEDC.channel_group[0].channel[LEDC_CHAN].duty.duty = (uint32_t)duty << 4;
    LEDC.channel_group[0].channel[LEDC_CHAN].conf0.sig_out_en = 1;
    LEDC.channel_group[0].channel[LEDC_CHAN].conf1.duty_start = 1;
    LEDC.channel_group[0].channel[LEDC_CHAN].conf0.low_speed_update = 1;
  }
}

// ---- Trigger-side synth (task context only) ----------------------------------
uint8_t gVol = 35;    // 0..100 master volume
uint8_t gPitch = 50;  // 1..100 -> x0.25..x4 (2^((v-50)/25))
uint8_t gDecay = 50;  // 1..100 -> x0.25..x4 decay-time scale
bool gAmpOn = true;   // 3V3 header rail state
char gLast[28] = "-";

float volF() { float f = gVol / 100.0f; return f * f; } // perceptual-ish taper
float pitchMulF() { return powf(2.0f, (gPitch - 50) / 25.0f); }
float decayMulF() { return powf(2.0f, (gDecay - 50) / 25.0f); }
float frand() { return (esp_random() & 0xFFFF) / 65535.0f; }

uint32_t phaseInc(float hz) { return (uint32_t)(hz * (4294967296.0 / SAMPLE_HZ)); }
int32_t decayQ30(float sec) { // time to fall ~60 dB
  if (sec < 0.001f) sec = 0.001f;
  return (int32_t)(exp(-6.907755 / (sec * SAMPLE_HZ)) * (double)Q30_ONE);
}
uint32_t biasDelay() { int32_t b = gBias; return b >= gBiasMid ? 0 : (uint32_t)(gBiasMid - b); }

int voiceAlloc() {
  int32_t worst = 0x7FFFFFFF, wi = 0;
  for (int i = 0; i < NVOICES; i++) {
    if (!gV[i].active) return i;
    if (gV[i].amp < worst) { worst = gV[i].amp; wi = i; }
  }
  return wi; // steal the quietest
}

void startVoice(uint8_t wave, float hz, float amp, float decaySec, float holdSec,
                float glideRatio, float glideSec, uint8_t lpK, uint32_t delaySamp) {
  if (amp <= 0.0f) return;
  if (amp > 1.0f) amp = 1.0f;
  int i = voiceAlloc();
  Voice &v = gV[i];
  v.active = false; // ISR stops touching the slot before we rewrite it
  v.wave = wave;
  v.lpK = lpK;
  v.phase = 0;
  v.inc = phaseInc(hz);
  v.glide = (glideRatio > 0 && fabsf(glideRatio - 1.0f) > 1e-4f && glideSec > 0)
                ? (int32_t)(exp(log(glideRatio) / (glideSec * SAMPLE_HZ)) * (double)Q30_ONE)
                : Q30_ONE;
  v.amp = (int32_t)(8388607.0f * amp); // Q23
  v.decay = decayQ30(decaySec);
  v.hold = (uint32_t)(holdSec * SAMPLE_HZ);
  v.delay = delaySamp;
  v.lp = 0;
  v.active = true;
}

// Shorthand: plain decaying sine partial.
void partial(float hz, float amp, float decaySec, uint32_t d0) {
  startVoice(W_SINE, hz, amp, decaySec, 0, 1.0f, 0, 0, d0);
}

enum Snd : uint8_t { SND_KNOCK = 0, SND_TOCK, SND_TICK, SND_SHAKER, SND_MARIMBA,
                     SND_CHIME, SND_DRIP, SND_BEEP, SND_COUNT };
const char *SND_NAMES[SND_COUNT] = {"knock", "tock",  "tick", "shaker",
                                    "marimba", "chime", "drip", "beep"};
// A-minor pentatonic; enough range to read as musical without being a tune.
const float MARIMBA_HZ[] = {220.0f, 261.63f, 293.66f, 329.63f, 392.0f, 440.0f, 523.25f};

void playSound(uint8_t snd, float pMul, float vMul) {
  if (!gAmpOn) return;
  float P = pitchMulF() * pMul;
  float D = decayMulF();
  float V = volF() * vMul;
  uint32_t d0 = biasDelay();
  switch (snd) {
    case SND_KNOCK: // bamboo body + a woody overtone + a tiny contact chiff
      partial(300 * P, 0.9f * V, 0.090f * D, d0);
      partial(940 * P, 0.35f * V, 0.035f * D, d0);
      startVoice(W_NOISE, 0, 0.35f * V, 0.008f, 0, 1, 0, 1, d0);
      break;
    case SND_TOCK: // bigger, lower, longer
      partial(195 * P, 0.95f * V, 0.140f * D, d0);
      partial(590 * P, 0.30f * V, 0.050f * D, d0);
      startVoice(W_NOISE, 0, 0.30f * V, 0.010f, 0, 1, 0, 2, d0);
      break;
    case SND_TICK: // clave-like dry tick
      partial(1800 * P, 0.8f * V, 0.014f * D, d0);
      startVoice(W_NOISE, 0, 0.25f * V, 0.003f, 0, 1, 0, 0, d0);
      break;
    case SND_SHAKER: // low-passed noise burst
      startVoice(W_NOISE, 0, 0.7f * V, 0.25f * D, 0.040f, 1, 0, 2, d0);
      break;
    case SND_MARIMBA: { // random pentatonic note, fundamental + brief 4th harmonic
      float f = MARIMBA_HZ[esp_random() % (sizeof(MARIMBA_HZ) / sizeof(float))] * P;
      partial(f, 0.85f * V, 0.35f * D, d0);
      partial(4 * f, 0.25f * V, 0.070f * D, d0);
      break;
    }
    case SND_CHIME: // struck chime: inharmonic partials, long tail
      partial(740 * P, 0.75f * V, 1.30f * D, d0);
      partial(740 * 2.76f * P, 0.40f * V, 0.80f * D, d0);
      partial(740 * 5.40f * P, 0.22f * V, 0.45f * D, d0);
      break;
    case SND_DRIP: // rising "bloop" + tiny onset tick
      startVoice(W_SINE, 340 * P, 0.9f * V, 0.16f * D, 0, 2.6f, 0.10f, 0, d0);
      startVoice(W_NOISE, 0, 0.2f * V, 0.003f, 0, 1, 0, 1, d0);
      break;
    case SND_BEEP: // the square-wave baseline everyone compared against
      startVoice(W_SQUARE, 660 * P, 0.5f * V, 0.030f, 0.120f, 1, 0, 0, d0);
      break;
    default:
      return;
  }
  snprintf(gLast, sizeof(gLast), "%s p%.2f v%.2f", SND_NAMES[snd], P, vMul);
  Serial.printf("play %s\n", gLast);
}

// ---- Scheduler: ripple cascade + grove auto-mode (loop context) --------------
#define EVMAX 48
struct Ev { bool used; uint32_t atMs; uint8_t snd; float p, v; };
Ev gEv[EVMAX];
bool gGrove = false;
uint8_t gGroveRate = 24; // events per minute (mean of the exponential gaps)
uint32_t gGroveNextMs = 0;

void schedule(uint32_t inMs, uint8_t snd, float p, float v) {
  for (int i = 0; i < EVMAX; i++)
    if (!gEv[i].used) {
      gEv[i] = {true, millis() + inMs, snd, p, v};
      return;
    }
}

void rippleStart() { // a wave of knocks passing through the grove
  const int N = 20;
  for (int i = 0; i < N; i++) {
    uint32_t t = i * 115 + (uint32_t)(frand() * 90);
    float win = sinf(3.14159f * i / (N - 1)); // swells in, fades out
    float v = 0.25f + 0.75f * win * win;
    float p = 0.75f + frand() * 0.55f;
    schedule(t, frand() < 0.25f ? SND_TICK : SND_KNOCK, p, v);
  }
  Serial.println("ripple: 20 events over ~2.4 s");
}

void schedTick() {
  uint32_t now = millis();
  for (int i = 0; i < EVMAX; i++)
    if (gEv[i].used && (int32_t)(now - gEv[i].atMs) >= 0) {
      gEv[i].used = false;
      playSound(gEv[i].snd, gEv[i].p, gEv[i].v);
    }
}

void groveTick() {
  if (!gGrove) { gGroveNextMs = 0; return; }
  uint32_t now = millis();
  if (!gGroveNextMs) gGroveNextMs = now + 400;
  if ((int32_t)(now - gGroveNextMs) < 0) return;
  uint32_t r = esp_random() % 100;
  uint8_t snd = r < 55 ? SND_KNOCK : r < 75 ? SND_TOCK : r < 90 ? SND_TICK : SND_MARIMBA;
  playSound(snd, 0.8f + frand() * 0.45f, 0.4f + frand() * 0.6f);
  float u = frand();
  if (u < 0.0001f) u = 0.0001f;
  uint32_t gap = (uint32_t)(-logf(u) * 60000.0f / gGroveRate);
  if (gap < 120) gap = 120;
  if (gap > 30000) gap = 30000;
  gGroveNextMs = now + gap;
}

void applyCarrier(uint8_t sel) { // runtime A/B: 0 = 78125 Hz/9-bit, 1 = 39062.5 Hz/10-bit
  gAudioHalt = true;
  delay(2); // let an in-flight ISR pass finish
  for (int i = 0; i < NVOICES; i++) gV[i].active = false;
  ledcDetach(AUDIO_PIN); // no-op harmlessly if not yet attached
  uint32_t hz = sel ? 39062 : 78125;
  uint8_t bits = sel ? 10 : 9;
  gLedcOk = ledcAttachChannel(AUDIO_PIN, hz, bits, LEDC_CHAN);
  if (!gLedcOk && sel) { // fall back to the default
    sel = 0; hz = 78125; bits = 9;
    gLedcOk = ledcAttachChannel(AUDIO_PIN, hz, bits, LEDC_CHAN);
  }
  if (!gLedcOk) Serial.println("WARNING: ledcAttachChannel failed");
  gCarrierSel = sel;
  gPwmHz = hz;
  gPwmBits = bits;
  gDutyMax = (1 << bits) - 1;
  gBiasMid = 1 << (bits - 1);
  gMixShift = 16 - bits;
  gAmpFloor = 1 << (gMixShift + 7);
  gBias = 0;
  gIdleSamples = 0x7FFFFFF0;
  gLastDuty = -1;
  // Idle low via the same direct-register path the ISR uses -- deliberately NOT
  // ledcWrite(pin, 0), which can park the channel with sig_out_en cleared.
  LEDC.channel_group[0].channel[LEDC_CHAN].duty.duty = 0;
  LEDC.channel_group[0].channel[LEDC_CHAN].conf0.sig_out_en = 1;
  LEDC.channel_group[0].channel[LEDC_CHAN].conf1.duty_start = 1;
  LEDC.channel_group[0].channel[LEDC_CHAN].conf0.low_speed_update = 1;
  gAudioHalt = false;
  Serial.printf("carrier: %lu Hz PWM / %d-bit (sample clock stays %d Hz)\n",
                (unsigned long)hz, bits, SAMPLE_HZ);
}

void audioInit() {
  for (int i = 0; i < 1024; i++)
    gSinLut[i] = (int16_t)(sinf(i * 6.2831853f / 1024.0f) * 32767.0f);
  applyCarrier(0);
  gTimer = timerBegin(TIMER_HZ); // 40 MHz tick, same crystal as the LEDC clock
  timerAttachInterrupt(gTimer, &audioIsr);
  timerAlarm(gTimer, TIMER_TICKS, true, 0); // 19531.25 Hz, integer-locked to fc
  Serial.printf("audio: GPIO%d, %lu Hz PWM / %ld-bit, %d Hz samples, %d voices\n",
                AUDIO_PIN, (unsigned long)gPwmHz, (long)gPwmBits, SAMPLE_HZ, NVOICES);
}

// The SDK manages EN_3V3 (GPIO4) as an RTC pin with a pad HOLD re-armed after
// every write (Mainboard::_setRTCPin) -- raw pinMode/digitalWrite on GPIO4 does
// NOTHING while the hold is set. Go through Board.enable3V3() when the SDK is
// up and CHECK the result: it try-locks a mutex and can fail silently.
int en3v3Level() {
  return gPfReady ? (int)rtc_gpio_get_level(GPIO_NUM_4) : (int)digitalRead(EN_3V3_PIN);
}

void setAmpPower(bool on) {
  gAmpOn = on;
  if (gPfReady) {
    Result r = Result::Failure;
    for (int i = 0; i < 5 && r != Result::Ok; i++) {
      r = Board.enable3V3(on);
      if (r != Result::Ok) delay(50); // try-lock miss: retry
    }
    Serial.printf("amp power (3V3 header rail): %s -> SDK %s, GPIO4 pad reads %d\n",
                  on ? "ON" : "OFF", r == Result::Ok ? "Ok" : "FAILED", en3v3Level());
  } else {
    pinMode(EN_3V3_PIN, OUTPUT);
    digitalWrite(EN_3V3_PIN, on ? HIGH : LOW);
    Serial.printf("amp power (3V3 header rail): %s via raw GPIO4 (SDK down)\n",
                  on ? "ON" : "OFF");
  }
  if (!on)
    for (int i = 0; i < NVOICES; i++) gV[i].active = false;
}

// ---- Battery stats cache (sway_demo/led_studio pattern) ----------------------
float gBatV = 0, gBatMa = 0, gSupV = 0, gSupMa = 0;
uint8_t gSoc = 0;
bool gSupGood = false;

void batteryTick() {
  static uint32_t nextMs = 0;
  static uint8_t idx = 0;
  if (!gPfReady || millis() < nextMs) return;
  nextMs = millis() + 800;
  switch (idx++ % 5) {
    case 0: Board.getBatteryVoltage(gBatV); break;
    case 1: Board.getBatteryCurrent(gBatMa); break;
    case 2: Board.getBatteryCharge(gSoc); break;
    case 3: Board.getSupplyVoltage(gSupV); break;
    case 4: Board.checkSupplyGood(gSupGood); break;
  }
}

// One-shot guarded charge-enable (sway_demo/presence_bench pattern).
void chargeTick() {
  static bool done = false;
  if (done || !gPfReady || millis() < 6000) return;
  if (gBatV < 0.1f) {
    if (millis() > 60000) {
      done = true;
      Serial.println("no battery reading after 60 s -> charging stays OFF");
    }
    return;
  }
  done = true;
  if (gBatV > 2.5f && gBatV < 4.4f) {
    Board.setBatteryChargingMaxCurrent(500);
    Board.enableBatteryCharging(true);
    gChargeOn = true;
    pfSolarGuardInit("speaker_demo", SPK_MAINTAIN_V, true);
    Serial.printf("battery %.2fV present -> charging ON (500 mA, LFP 3.65 V ceiling)\n", gBatV);
  } else {
    Serial.printf("battery %.2fV implausible -> charging stays OFF\n", gBatV);
  }
}

void solarGuardTick() {
  if (!gPfReady || !gChargeOn) return;
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  if (now - lastMs < 2000) return;
  lastMs = now;
  float sv = 0.0f, sma = 0.0f;
  bool good = false;
  if (Board.getSupplyVoltage(sv) != Result::Ok) return;
  if (Board.getSupplyCurrent(sma) != Result::Ok) return;
  if (Board.checkSupplyGood(good) != Result::Ok) return;
  gSupV = sv;
  gSupMa = sma;
  gSupGood = good;
  pfSolarGuardTick("speaker_demo", sv, sma, good, SPK_MAINTAIN_V, true);
}

// ---- Web UI -----------------------------------------------------------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>Speaker Demo</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:14px;max-width:520px}
 h2{margin:.2em 0}
 .row{margin:10px 0}
 label{display:block;font-size:13px;color:#aaa;margin-bottom:3px}
 input[type=range]{width:100%;height:30px}
 .btns{display:flex;flex-wrap:wrap;gap:6px}
 button{flex:1 1 auto;min-width:64px;padding:14px 8px;font-size:14px;border:0;border-radius:8px;background:#333;color:#eee}
 button.on{background:#0a7;color:#fff}
 button.big{background:#264;font-weight:bold}
 #vals{font-family:monospace;font-size:13px;background:#000;padding:8px;border-radius:6px;white-space:pre;color:#6f6;overflow-x:auto}
 hr{border:0;border-top:1px solid #333;margin:14px 0}
</style></head><body>
<h2>Speaker Demo <span style="font-size:12px;color:#888" id=fw></span></h2>
<div class=row><label>Percussion (the point of this bench)</label><div class=btns>
 <button onclick="play('knock')">Knock</button>
 <button onclick="play('tock')">Tock</button>
 <button onclick="play('tick')">Tick</button>
 <button onclick="play('shaker')">Shaker</button>
</div></div>
<div class=row><label>Tonal</label><div class=btns>
 <button onclick="play('marimba')">Marimba</button>
 <button onclick="play('chime')">Chime</button>
 <button onclick="play('drip')">Drip</button>
 <button onclick="play('beep')">Beep (square baseline)</button>
</div></div>
<div class=row><label>Scenes: what would 150 of these feel like</label><div class=btns>
 <button class=big onclick="fetch('/ripple')">Ripple</button>
 <button id=grove onclick="toggleGrove()">Grove auto: off</button>
</div></div>
<div class=row><label>Grove rate <span id=gratel></span></label>
 <input type=range id=grate min=2 max=120 value=24 oninput="ch('grate',this.value)"></div>
<hr>
<div class=row><label>Volume <span id=voll></span></label>
 <input type=range id=vol min=0 max=100 value=35 oninput="ch('vol',this.value)"></div>
<div class=row><label>Pitch <span id=pitchl></span></label>
 <input type=range id=pitch min=1 max=100 value=50 oninput="ch('pitch',this.value)"></div>
<div class=row><label>Decay <span id=decayl></span></label>
 <input type=range id=decay min=1 max=100 value=50 oninput="ch('decay',this.value)"></div>
<div class=row><div class=btns>
 <button id=amp onclick="toggleAmp()">Amp power: on</button>
 <button id=car onclick="toggleCar()">Carrier: 78k/9b</button>
</div></div>
<div class=row><div id=vals>...</div></div>
<div class=row><label>Battery</label><div id=bat>...</div></div>
<script>
let st={vol:35,pitch:50,decay:50,grate:24,grove:0,amp:1,carrier:0};
function send(q){fetch('/set?'+q);}
function ch(k,v){st[k]=+v;send(k+'='+v);syncLabels();}
function play(s){fetch('/play?snd='+s);}
function toggleGrove(){st.grove^=1;send('grove='+st.grove);groveBtn();}
function groveBtn(){let e=document.getElementById('grove');
 e.textContent='Grove auto: '+(st.grove?'on':'off');e.className=st.grove?'on':'';}
function toggleAmp(){st.amp^=1;send('amp='+st.amp);ampBtn();}
function ampBtn(){let e=document.getElementById('amp');
 e.textContent='Amp power: '+(st.amp?'on':'off');e.className=st.amp?'on':'';}
function toggleCar(){st.carrier^=1;send('carrier='+st.carrier);carBtn();}
function carBtn(){let e=document.getElementById('car');
 e.textContent='Carrier: '+(st.carrier?'39k/10b':'78k/9b');e.className=st.carrier?'on':'';}
function syncLabels(){
 voll.textContent=st.vol;
 pitchl.textContent='x'+Math.pow(2,(st.pitch-50)/25).toFixed(2);
 decayl.textContent='x'+Math.pow(2,(st.decay-50)/25).toFixed(2);
 gratel.textContent=st.grate+'/min';}
function tick(){fetch('/state').then(r=>r.json()).then(s=>{
 document.getElementById('fw').textContent=s.fw;
 if(document.activeElement.type!='range'){
  st.vol=s.vol;st.pitch=s.pitch;st.decay=s.decay;st.grate=s.grate;
  vol.value=s.vol;pitch.value=s.pitch;decay.value=s.decay;grate.value=s.grate;}
 st.grove=s.grove;st.amp=s.amp;st.carrier=s.carrier;groveBtn();ampBtn();carBtn();syncLabels();
 vals.textContent='last   '+s.last+'\nvoices '+s.voices+'   bias '+s.bias+'   rssi '+s.rssi+' dBm';
 let bat=document.getElementById('bat');
 if(!s.pf){bat.textContent='no battery data (SDK init failed)';}
 else{let act=s.ma>30?('charging +'+s.ma+'mA'):(s.ma<-30?('discharging '+s.ma+'mA'):'idle ~'+s.ma+'mA');
  bat.textContent='SOC '+s.soc+'%  '+s.bv.toFixed(3)+'V  '+act+
   (s.sgood?('  |  supply '+s.sv.toFixed(2)+'V ok'):'  |  on battery')+
   (s.chg?'':'  |  charger disabled');}
 setTimeout(tick,600);}).catch(()=>setTimeout(tick,1200));}
syncLabels();tick();
</script></body></html>)HTML";

void handlePlay() {
  String s = server.arg("snd");
  float p = server.hasArg("p") ? server.arg("p").toFloat() : 1.0f;
  float v = server.hasArg("v") ? server.arg("v").toFloat() : 1.0f;
  if (p <= 0) p = 1.0f;
  if (v <= 0) v = 1.0f;
  for (uint8_t i = 0; i < SND_COUNT; i++)
    if (s == SND_NAMES[i]) {
      playSound(i, p, v);
      server.send(200, "text/plain", "ok");
      return;
    }
  server.send(400, "text/plain", "unknown snd");
}

void handleSet() {
  if (server.hasArg("vol")) gVol = constrain(server.arg("vol").toInt(), 0, 100);
  if (server.hasArg("pitch")) gPitch = constrain(server.arg("pitch").toInt(), 1, 100);
  if (server.hasArg("decay")) gDecay = constrain(server.arg("decay").toInt(), 1, 100);
  if (server.hasArg("grate")) gGroveRate = constrain(server.arg("grate").toInt(), 2, 120);
  if (server.hasArg("grove")) gGrove = server.arg("grove").toInt() != 0;
  if (server.hasArg("amp")) setAmpPower(server.arg("amp").toInt() != 0);
  if (server.hasArg("carrier")) {
    uint8_t sel = server.arg("carrier").toInt() ? 1 : 0;
    if (sel != gCarrierSel) applyCarrier(sel);
  }
  server.send(200, "text/plain", "ok");
}

void handleDiag() { // raw LEDC/GPIO evidence for is-the-pin-actually-driving
  uint8_t tsel = LEDC.channel_group[0].channel[LEDC_CHAN].conf0.timer_sel;
  static char buf[512];
  snprintf(buf, sizeof(buf),
           "{\"ledcAttach\":%d,\"pwmHz\":%lu,\"pwmBits\":%ld,\"isrN\":%lu,\"ms\":%lu,"
           "\"ch_conf0\":\"%08lX\",\"sig_out_en\":%u,\"timer_sel\":%u,"
           "\"ch_conf1\":\"%08lX\",\"ch_duty\":%lu,\"ch_duty_rd\":%lu,\"ch_hpoint\":%lu,"
           "\"tmr_conf\":\"%08lX\",\"tmr_res_bits\":%u,\"tmr_div\":%u,\"tmr_pause\":%u,"
           "\"tmr_cnt\":%lu,\"bias\":%d}",
           gLedcOk ? 1 : 0, (unsigned long)gPwmHz, (long)gPwmBits,
           (unsigned long)gIsrN, (unsigned long)millis(),
           (unsigned long)LEDC.channel_group[0].channel[LEDC_CHAN].conf0.val,
           (unsigned)LEDC.channel_group[0].channel[LEDC_CHAN].conf0.sig_out_en, (unsigned)tsel,
           (unsigned long)LEDC.channel_group[0].channel[LEDC_CHAN].conf1.val,
           (unsigned long)LEDC.channel_group[0].channel[LEDC_CHAN].duty.duty,
           (unsigned long)LEDC.channel_group[0].channel[LEDC_CHAN].duty_rd.duty_read,
           (unsigned long)LEDC.channel_group[0].channel[LEDC_CHAN].hpoint.hpoint,
           (unsigned long)LEDC.timer_group[0].timer[tsel].conf.val,
           (unsigned)LEDC.timer_group[0].timer[tsel].conf.duty_resolution,
           (unsigned)LEDC.timer_group[0].timer[tsel].conf.clock_divider,
           (unsigned)LEDC.timer_group[0].timer[tsel].conf.pause,
           (unsigned long)LEDC.timer_group[0].timer[tsel].value.timer_cnt, (int)gBias);
  server.send(200, "application/json", buf);
}

void handleState() {
  int voices = 0;
  for (int i = 0; i < NVOICES; i++)
    if (gV[i].active) voices++;
  static char buf[512];
  snprintf(buf, sizeof(buf),
           "{\"fw\":\"%s\",\"vol\":%u,\"pitch\":%u,\"decay\":%u,\"carrier\":%u,"
           "\"grove\":%d,\"grate\":%u,\"amp\":%d,\"en3v3\":%d,\"voices\":%d,\"bias\":%d,"
           "\"last\":\"%s\",\"rssi\":%d,\"mac\":\"%s\","
           "\"pf\":%d,\"chg\":%d,\"bv\":%.3f,\"ma\":%.0f,\"soc\":%u,\"sv\":%.2f,\"sgood\":%d}",
           FW_VERSION, gVol, gPitch, gDecay, gCarrierSel, gGrove ? 1 : 0, gGroveRate,
           gAmpOn ? 1 : 0, en3v3Level(), voices, (int)gBias, gLast, (int)WiFi.RSSI(),
           WiFi.macAddress().c_str(),
           gPfReady ? 1 : 0, gChargeOn ? 1 : 0, gBatV, gBatMa, gSoc, gSupV,
           gSupGood ? 1 : 0);
  server.send(200, "application/json", buf);
}

void setupWifi() {
#if HAVE_SECRETS
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname("speakerdemo");
  WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);
  Serial.print("WiFi connecting");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    bool apOk = WiFi.softAP(AP_SSID, AP_PASS, WiFi.channel());
    Serial.print("Speaker Demo STA at http://");
    Serial.println(WiFi.localIP());
    if (apOk) {
      Serial.print("Speaker Demo AP '" AP_SSID "' -> http://");
      Serial.println(WiFi.softAPIP());
    }
    return;
  }
  Serial.println("station failed; starting AP fallback");
#endif
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP '" AP_SSID "' -> http://");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Speaker Demo " FW_VERSION);

  Result pf = Result::Failure;
  for (int i = 0; i < 4 && pf != Result::Ok; i++) {
    pf = Board.init(2000, Mainboard::BatteryType::Generic_LFP);
    if (pf != Result::Ok) delay(250);
  }
  if (pf == Result::Ok) {
    gPfReady = true;
    Board.enableBatteryCharging(false); // chargeTick() enables it once the gauge warms up
    Board.setSupplyMaintainVoltage(SPK_MAINTAIN_V);
    Board.enableVSQT(false); // no STEMMA-QT devices on this bench
    Result r3 = Result::Failure;
    for (int i = 0; i < 5 && r3 != Result::Ok; i++) {
      r3 = Board.enable3V3(true); // speaker V+ rail; try-locks, so verify + retry
      if (r3 != Result::Ok) delay(50);
    }
    Serial.printf("PowerFeather SDK Ok: VSQT off, charging OFF (bench); 3V3 enable %s, "
                  "GPIO4 pad reads %d\n",
                  r3 == Result::Ok ? "Ok" : "FAILED", (int)rtc_gpio_get_level(GPIO_NUM_4));
    // Clear a latched BQ EN_HIZ (REG0x16 bit 4) left by a prior image -- otherwise
    // the board can silently drain its cell while on USB (presence_bench pattern).
    uint8_t r16 = 0;
    Wire1.beginTransmission(0x6A);
    Wire1.write(0x16);
    if (Wire1.endTransmission(false) == 0 && Wire1.requestFrom(0x6A, 1) == 1) {
      r16 = Wire1.read();
      if (r16 & (1 << 4)) {
        Wire1.beginTransmission(0x6A);
        Wire1.write(0x16);
        Wire1.write(r16 & ~(1 << 4));
        Serial.printf("BQ EN_HIZ was latched (REG0x16=0x%02X) -> cleared %s\n", r16,
                      Wire1.endTransmission() == 0 ? "ok" : "FAILED");
      }
    }
  } else {
    Serial.println("WARNING: Board.init failed -- enabling 3V3 via GPIO4 manually");
    Wire1.begin(47, 48, 100000);
    // Raw fallback ONLY when the SDK is down: with the SDK up, GPIO4 is an
    // RTC-held pad and pinMode/digitalWrite are silently ignored (and pinMode
    // would remux the pad away from the SDK's RTC config).
    pinMode(EN_3V3_PIN, OUTPUT);
    digitalWrite(EN_3V3_PIN, HIGH);
  }
  delay(100); // rail settle
  Wire1.setClock(100000); // shared charger/gauge bus: 100 kHz, NEVER faster

  audioInit();
  playSound(SND_KNOCK, 1.0f, 1.0f); // boot chirp: proves the whole audio path

  setupWifi();
  if (MDNS.begin("speakerdemo")) { // http://speakerdemo.local/
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://speakerdemo.local/");
  } else {
    Serial.println("mDNS start failed (use the IP)");
  }
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/play", handlePlay);
  server.on("/ripple", []() { rippleStart(); server.send(200, "text/plain", "ok"); });
  server.on("/set", handleSet);
  server.on("/state", handleState);
  server.on("/diag", handleDiag);
  server.on("/update", HTTP_GET, []() {
    server.send(200, "text/html",
                "<form method=POST action=/update enctype=multipart/form-data>"
                "<input type=file name=firmware><input type=submit value=Flash></form>");
  });
  server.on(
      "/update", HTTP_POST,
      []() {
        bool ok = !Update.hasError();
        server.send(ok ? 200 : 500, "text/plain",
                    ok ? "Update complete. Rebooting.\n" : "Update failed.\n");
        delay(500);
        if (ok) ESP.restart();
      },
      []() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.printf("OTA upload start: %s\n", upload.filename.c_str());
          gAudioHalt = true; // mute; the ISR is deferred during flash writes anyway
          for (int i = 0; i < NVOICES; i++) gV[i].active = false;
          ledcWrite(AUDIO_PIN, 0);
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
            Update.printError(Serial);
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) Serial.printf("OTA upload done: %u bytes\n", upload.totalSize);
          else Update.printError(Serial);
        }
      });
  server.begin();
  Serial.printf("Speaker Demo ready: audio on GPIO%d, vol %u\n", AUDIO_PIN, gVol);
}

void loop() {
  server.handleClient();
  schedTick();
  groveTick();
  batteryTick();
  chargeTick();
  solarGuardTick();
}
