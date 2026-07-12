// Resonance LED+Solenoid Bench -- combined RGBW point source + solenoid strike
// driver on ONE PowerFeather, both loads fed VBAT-DIRECT (the ADR 0029 fork:
// no 3V3 header rail in the load path).
//
// Wiring (2026-07-11 bench):
//   VBAT            -> RGBW module V+ AND solenoid driver load supply
//   GND             -> both grounds
//   D13 / GPIO11    -> RGBW SK6812 data in
//   D12 / GPIO12    -> solenoid driver signal (gate) input
//   driver flyback diode MANDATORY for the coil load -- check it is populated.
//
// Because the loads sit on VBAT, EN_3V3 (GPIO4) gates NOTHING here: the coil is
// hot whenever a cell is attached, and gate discipline is the ONLY software
// control. All of solenoid_demo's coil safety is kept verbatim: every gate
// pulse ends via an esp_timer one-shot AND a loop() failsafe deadline, width is
// hard-clamped (5..300 ms), and a minimum coil-rest gap is enforced. The 3V3
// header rail defaults OFF as a wiring diagnostic -- if either load goes dark
// with the rail off, it is NOT on VBAT like it should be.
//
// Flash sync: STRIKE can flash the RGBW white for the pulse duration (toggle in
// the UI) -- first look at the percussion+light aesthetic for the fixtures.
//
// Build/flash (USB): ./build.sh --port /dev/ttyACM1
// Web: http://ledsol.local/ or the IP in the serial banner (115200).
// OTA: curl -F "firmware=@led_sol_bench.ino.bin" http://<ip>/update

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MSA301.h> // also provides Adafruit_MSA311 (sway_demo pattern)
#include "esp_timer.h"
#include "driver/rtc_io.h" // read back the actual EN_3V3 pad level (SDK RTC-holds it)

#define FW_VERSION "led-sol-bench-2026-07-11.8"

// PowerFeather SDK: rails + telemetry + guarded charging (solenoid_demo pattern --
// this unit may carry a cell; charging stays OFF until the gauge reports a
// plausible LFP voltage, and the solar guard handles USB/panel supplies).
#include <PowerFeather.h>
#include "../powerfeather_solar_guard.h"
using namespace PowerFeather;
#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 (build.sh passes it) so the SDK targets the V2."
#endif
#define BENCH_MAINTAIN_V 4.6f // correct for USB; re-tune toward the panel MPP for solar work
bool gPfReady = false;
bool gChargeOn = false;

#define EN_3V3_PIN 4 // switchable 3V3 header rail -- UNUSED by the loads on this bench

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif
#define AP_SSID "ResonanceLedSol"
#define AP_PASS "resonance"

WebServer server(80);

// ---- Solenoid gate control (solenoid_demo machinery, pin moved to D12) --------
#ifndef SOLENOID_PIN
#define SOLENOID_PIN 12 // GPIO12 / D12 -> driver signal input
#endif
constexpr uint16_t PULSE_MIN_MS = 5;
constexpr uint16_t PULSE_MAX_MS = 300;  // hard cap -- longer is heat, not strike
constexpr uint16_t COIL_REST_MS = 80;   // minimum gap between strikes
constexpr uint16_t BURST_GAP_FLOOR_MS = 150;

uint16_t gPulseMs = 40;      // default strike width
uint16_t gIntervalMs = 600;  // auto-repeat period
bool gAuto = false;
bool gArmed = true;          // software strike-enable (VBAT coil supply is always hot)
volatile bool gGateOn = false;
volatile uint32_t gLastEndMs = 0;   // when the gate last dropped
uint32_t gGateFailsafeMs = 0;       // loop() deadline: force-low if the timer missed
uint32_t gAutoNextMs = 0;
uint8_t gBurstLeft = 0;
uint32_t gBurstNextMs = 0;
uint32_t gStrikes = 0;
uint32_t gBlocked = 0;   // strike requests refused (rest gap / disarmed / mid-pulse)
uint32_t gFailsafes = 0; // should stay 0 -- nonzero means the esp_timer path missed
char gLast[40] = "-";
esp_timer_handle_t gPulseTimer = nullptr;

// ---- RGBW state (led_studio single-point subset) -------------------------------
#ifndef LED_PIN
#define LED_PIN 11 // GPIO11 / D13 -> SK6812 RGBW data
#endif
#ifndef LED_COUNT
#define LED_COUNT 1
#endif
// Order default is RGBW: slot-tested on the production 4 W module 2026-07-11.
// (led_studio's MODE_RGBW uses GRBW -- likely R/G-swapped on this module class.)
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGBW + NEO_KHZ800);

// Runtime LED data-pin / feed switch. Allowed pins: 10 (A0, legacy 3V3-rail
// header) and 11 (the D13 POSITION -- which is GPIO11, not GPIO13!). GPIO12 is
// the solenoid gate and GPIO13 is EN0, the SDK-owned FeatherWings enable
// (Mainboard.h:123) -- neither is switchable to. Feed A/B pairs the pin with
// the matching supply: A = 3V3 rail ON + A0; B = VBAT (rail off) + D13.
uint8_t gLedPin = LED_PIN;
bool ledPinAllowed(int p) { return p == 10 || p == 11; }

// Runtime color-order switch (2026-07-11 diagnosis: this RGBW unit shows R/G
// swapped vs the led_studio point source, so the wire order is in question).
const uint16_t ORDER_TYPES[] = {NEO_GRBW, NEO_RGBW, NEO_WRGB, NEO_GRB, NEO_RGB};
const char *ORDER_NAMES[] = {"GRBW", "RGBW", "WRGB", "GRB", "RGB"};
constexpr uint8_t ORDER_N = 5;
uint8_t gOrder = 1; // RGBW (matches the constructor)

// LED anims: 0 static, 1 hue, 2 breathe, 3 candle, 4 fade
uint8_t gAnim = 0;
uint8_t gR = 255, gG = 140, gB = 40, gW = 0; // warm amber default
uint8_t gB2r = 0, gB2g = 120, gB2b = 255;    // Color B for Fade
uint8_t gBri = 40;
uint8_t gSpeed = 30;
bool gGamma = true;
bool gFlashSync = true;      // STRIKE also flashes the LED white
uint32_t gFlashUntil = 0;
float rgbwPhase = 0;
float candleLevel = 1.0f, candleTarget = 1.0f;
uint32_t lastFrame = 0;

inline uint8_t gam(uint8_t v) { return gGamma ? Adafruit_NeoPixel::gamma8(v) : v; }

void switchLedPin(uint8_t p) {
  if (!ledPinAllowed(p) || p == gLedPin) return;
  gAnim = 0;
  gFlashUntil = 0;
  strip.clear();               // BLANK the outgoing module first -- with a module
  strip.show();                // on each header, a latched frame would stay lit
  delay(2);
  rmtDeinit(gLedPin);          // free the RMT channel (only 4 TX on the S3)
  pinMode(gLedPin, INPUT);     // park the old pin high-Z
  gLedPin = p;
  strip.setPin(gLedPin);       // next show() re-inits RMT on the new pin
  strip.clear();
  strip.show();
  Serial.printf("LED data pin -> GPIO%u\n", gLedPin);
}

void applyOrder(uint8_t o) {
  gOrder = o % ORDER_N;
  strip.updateType(ORDER_TYPES[gOrder] + NEO_KHZ800);
  strip.updateLength(LED_COUNT);
  strip.clear();
  strip.show();
  Serial.printf("LED wire order -> %s\n", ORDER_NAMES[gOrder]);
}

void setRGBWpix(uint8_t r, uint8_t g, uint8_t b, uint8_t w, float f) {
  float s = (float)gBri / 255.0f * f;
  if (s < 0) s = 0;
  if (s > 1) s = 1;
  for (uint16_t i = 0; i < strip.numPixels(); i++)
    strip.setPixelColor(i, gam((uint8_t)(r * s)), gam((uint8_t)(g * s)),
                        gam((uint8_t)(b * s)), gam((uint8_t)(w * s)));
  strip.show();
}

void renderStaticLed() { setRGBWpix(gR, gG, gB, gW, 1.0f); }

void renderFrameLed() {
  switch (gAnim) {
    case 1: { // hue cycle
      uint16_t hue = (uint16_t)((uint32_t)rgbwPhase & 0xFFFF);
      uint32_t c = strip.ColorHSV(hue, 255, gBri);
      if (gGamma) c = strip.gamma32(c);
      for (uint16_t i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, c);
      strip.show();
      rgbwPhase += 256;
      break;
    }
    case 2: { // breathe
      setRGBWpix(gR, gG, gB, gW, 0.5f + 0.5f * sinf(rgbwPhase));
      rgbwPhase += 0.15f;
      break;
    }
    case 3: { // candle
      candleLevel += (candleTarget - candleLevel) * 0.25f;
      if (fabsf(candleTarget - candleLevel) < 0.03f) {
        uint32_t r = esp_random();
        candleTarget = 0.45f + (float)(r & 0xFFFF) / 65535.0f * 0.55f;
        if ((r & 0x7) == 0) candleTarget *= 0.7f;
      }
      setRGBWpix(gR, gG, gB, gW, candleLevel);
      break;
    }
    case 4: { // fade current <-> color B
      float t = 0.5f + 0.5f * sinf(rgbwPhase);
      uint8_t r = gR + (int)((gB2r - gR) * t);
      uint8_t g = gG + (int)((gB2g - gG) * t);
      uint8_t b = gB + (int)((gB2b - gB) * t);
      uint8_t w = gW + (int)((0 - gW) * t);
      setRGBWpix(r, g, b, w, 1.0f);
      rgbwPhase += 0.06f;
      break;
    }
    default:
      break;
  }
}

uint32_t frameIntervalMs() { return (uint32_t)(400 - (gSpeed - 1) * (375.0f / 99.0f)); }

void ledTick() {
  uint32_t now = millis();
  static bool wasFlashing = false;
  if ((int32_t)(gFlashUntil - now) > 0) { // strike flash overrides everything
    if (!wasFlashing) {
      setRGBWpix(255, 255, 255, 255, 1.0f);
      wasFlashing = true;
    }
    return;
  }
  if (wasFlashing) {
    wasFlashing = false;
    if (gAnim == 0) renderStaticLed(); // anims repaint on their next frame anyway
    else strip.clear();
  }
  if (gAnim && now - lastFrame >= frameIntervalMs()) {
    lastFrame = now;
    renderFrameLed();
  }
}

// ---- Solenoid ------------------------------------------------------------------
void pulseEnd(void *) { // esp_timer task context
  digitalWrite(SOLENOID_PIN, LOW);
  gGateOn = false;
  gLastEndMs = millis();
}

bool strike(uint16_t ms, const char *why) {
  ms = constrain(ms, PULSE_MIN_MS, PULSE_MAX_MS);
  uint32_t now = millis();
  if (!gArmed || gGateOn || now - gLastEndMs < COIL_REST_MS) {
    gBlocked++;
    return false;
  }
  gGateFailsafeMs = now + ms + 50;
  gGateOn = true;
  digitalWrite(SOLENOID_PIN, HIGH);
  esp_timer_stop(gPulseTimer); // no-op if not running
  esp_timer_start_once(gPulseTimer, (uint64_t)ms * 1000ULL);
  if (gFlashSync) gFlashUntil = now + ms + 60; // hold past the pulse so the eye catches it
  gStrikes++;
  snprintf(gLast, sizeof(gLast), "%s %ums", why, ms);
  Serial.printf("strike #%lu: %s\n", (unsigned long)gStrikes, gLast);
  return true;
}

void stopAll() {
  gAuto = false;
  gBurstLeft = 0;
  esp_timer_stop(gPulseTimer);
  digitalWrite(SOLENOID_PIN, LOW);
  if (gGateOn) {
    gGateOn = false;
    gLastEndMs = millis();
  }
}

void burstStart(uint8_t n) {
  gBurstLeft = constrain((int)n, 1, 20);
  gBurstNextMs = millis();
}

uint32_t minIntervalMs() {
  uint32_t m = (uint32_t)gPulseMs + COIL_REST_MS;
  return m < BURST_GAP_FLOOR_MS ? BURST_GAP_FLOOR_MS : m;
}

void burstTick() {
  if (!gBurstLeft) return;
  uint32_t now = millis();
  if ((int32_t)(now - gBurstNextMs) < 0) return;
  if (strike(gPulseMs, "burst")) gBurstLeft--;
  gBurstNextMs = now + minIntervalMs();
}

void autoTick() {
  if (!gAuto) return;
  uint32_t now = millis();
  if ((int32_t)(now - gAutoNextMs) < 0) return;
  strike(gPulseMs, "auto");
  uint32_t iv = gIntervalMs;
  if (iv < minIntervalMs()) iv = minIntervalMs();
  gAutoNextMs = now + iv;
}

void failsafeTick() { // belt-and-suspenders: the coil must never stay energized
  if (gGateOn && (int32_t)(millis() - gGateFailsafeMs) >= 0) {
    digitalWrite(SOLENOID_PIN, LOW);
    gGateOn = false;
    gLastEndMs = millis();
    gFailsafes++;
    Serial.println("FAILSAFE: gate forced low (pulse timer missed)");
  }
}

// The SDK manages EN_3V3 (GPIO4) as an RTC pin with a pad HOLD re-armed after
// every write (Mainboard::_setRTCPin) -- raw pinMode/digitalWrite on GPIO4 does
// NOTHING while the hold is set. Go through Board.enable3V3() when the SDK is
// up and CHECK the result: it try-locks a mutex and can fail silently.
bool gRailOn = false; // requested 3V3 header rail state (diagnostic only here)

int en3v3Level() {
  return gPfReady ? (int)rtc_gpio_get_level(GPIO_NUM_4) : (int)digitalRead(EN_3V3_PIN);
}

void setRail(bool on) {
  gRailOn = on;
  if (gPfReady) {
    Result r = Result::Failure;
    for (int i = 0; i < 5 && r != Result::Ok; i++) {
      r = Board.enable3V3(on);
      if (r != Result::Ok) delay(50); // try-lock miss: retry
    }
    Serial.printf("3V3 header rail (unused by VBAT loads): %s -> SDK %s, GPIO4 pad reads %d\n",
                  on ? "ON" : "OFF", r == Result::Ok ? "Ok" : "FAILED", en3v3Level());
  } else {
    pinMode(EN_3V3_PIN, OUTPUT);
    digitalWrite(EN_3V3_PIN, on ? HIGH : LOW);
    Serial.printf("3V3 header rail: %s via raw GPIO4 (SDK down)\n", on ? "ON" : "OFF");
  }
}

// ---- VEML7700 lux on the STEMMA-QT port (ina_monitor register pattern) --------
// Shares Wire1 (47/48) with the charger/gauge at 100 kHz. Gain 1/8 + IT 100 ms:
// 0.4608 lx/ct, ~30 klx full scale -- covers the ~3 klx RGBW ceiling with margin.
#define VEML_ADDR 0x10
#define VEML_REG_CONF 0x00
#define VEML_REG_ALS 0x04
#define VEML_CONF 0x1000 // gain 1/8, IT 100 ms, no persistence/interrupt, powered on
#define VEML_LUX_PER_CT 0.4608f
bool gVemlPresent = false;

bool vemlWriteConf() {
  Wire1.beginTransmission(VEML_ADDR);
  Wire1.write(VEML_REG_CONF);
  Wire1.write(VEML_CONF & 0xFF); // little-endian
  Wire1.write(VEML_CONF >> 8);
  return Wire1.endTransmission() == 0;
}

bool vemlReadALS(uint16_t &raw) {
  Wire1.beginTransmission(VEML_ADDR);
  Wire1.write(VEML_REG_ALS);
  if (Wire1.endTransmission(false) != 0) return false;
  if (Wire1.requestFrom(VEML_ADDR, 2) != 2) return false;
  raw = Wire1.read();
  raw |= (uint16_t)Wire1.read() << 8;
  return true;
}

void handleLux() {
  if (!gVemlPresent) gVemlPresent = vemlWriteConf(); // late-plug re-probe
  uint16_t raw = 0;
  bool ok = gVemlPresent && vemlReadALS(raw);
  if (!ok) gVemlPresent = false;
  static char buf[120];
  snprintf(buf, sizeof(buf), "{\"veml\":%d,\"raw\":%u,\"lux\":%.1f,\"sat\":%d}",
           ok ? 1 : 0, raw, raw * VEML_LUX_PER_CT, raw >= 65000 ? 1 : 0);
  server.send(200, "application/json", buf);
}

// ---- Battery stats cache (speaker_demo/sway_demo pattern) ---------------------
float gBatV = 0, gBatMa = 0, gSupV = 0, gSupMa = 0;
uint8_t gSoc = 0;
bool gSupGood = false;

void batteryTick() {
  static uint32_t nextMs = 0;
  static uint8_t idx = 0;
  if (!gPfReady || millis() < nextMs) return;
  nextMs = millis() + 800;
  switch (idx++ % 6) {
    case 0: Board.getBatteryVoltage(gBatV); break;
    case 1: Board.getBatteryCurrent(gBatMa); break;
    case 2: Board.getBatteryCharge(gSoc); break;
    case 3: Board.getSupplyVoltage(gSupV); break;
    case 4: Board.getSupplyCurrent(gSupMa); break;
    case 5: Board.checkSupplyGood(gSupGood); break;
  }
}

// One-shot guarded charge-enable (speaker_demo/presence_bench pattern).
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
    pfSolarGuardInit("led_sol_bench", BENCH_MAINTAIN_V, true);
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
  pfSolarGuardTick("led_sol_bench", sv, sma, good, BENCH_MAINTAIN_V, true);
}

// ---- Web UI -------------------------------------------------------------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>LED+Solenoid Bench</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:14px;max-width:520px}
 h2{margin:.2em 0}
 h3{margin:.6em 0 .2em;color:#8ac}
 .row{margin:10px 0}
 label{display:block;font-size:13px;color:#aaa;margin-bottom:3px}
 input[type=range]{width:100%;height:30px}
 .btns{display:flex;flex-wrap:wrap;gap:6px}
 button{flex:1 1 auto;min-width:58px;padding:12px 8px;font-size:14px;border:0;border-radius:8px;background:#333;color:#eee}
 button.on{background:#0a7;color:#fff}
 button.big{background:#264;font-weight:bold;font-size:17px;padding:18px 8px}
 #vals{font-family:monospace;font-size:13px;background:#000;padding:8px;border-radius:6px;white-space:pre;color:#6f6;overflow-x:auto}
 hr{border:0;border-top:1px solid #333;margin:14px 0}
</style></head><body>
<h2>LED+Solenoid Bench <span style="font-size:12px;color:#888" id=fw></span></h2>

<h3>Solenoid (D12, VBAT)</h3>
<div class=row><div class=btns>
 <button class=big onclick="fetch('/strike')">STRIKE</button>
</div></div>
<div class=row><label>Fixed-width test strike</label><div class=btns>
 <button onclick="sw(10)">10ms</button>
 <button onclick="sw(15)">15ms</button>
 <button onclick="sw(20)">20ms</button>
 <button onclick="sw(30)">30ms</button>
 <button onclick="sw(50)">50ms</button>
 <button onclick="sw(80)">80ms</button>
 <button onclick="sw(120)">120ms</button>
</div></div>
<div class=row><label>Patterns</label><div class=btns>
 <button onclick="fetch('/burst?n=2')">Double</button>
 <button onclick="fetch('/burst?n=5')">Burst x5</button>
 <button onclick="fetch('/burst?n=10')">Burst x10</button>
 <button id=auto onclick="tog('auto')">Auto: off</button>
</div></div>
<div class=row><label>Pulse width <span id=pulsel></span></label>
 <input type=range id=pulse min=5 max=300 value=40 oninput="ch('pulse',this.value)"></div>
<div class=row><label>Auto interval <span id=intervall></span></label>
 <input type=range id=interval min=150 max=4000 step=50 value=600 oninput="ch('interval',this.value)"></div>

<h3>RGBW (D13, VBAT)</h3>
<div class=row><label>Color</label>
 <input type=color id=col value="#ff8c28" oninput="setCol(this.value)"></div>
<div class=row><label>W (white die) <span id=wl></span></label>
 <input type=range id=w min=0 max=255 value=0 oninput="ch('w',this.value)"></div>
<div class=row><label>Brightness <span id=bril></span></label>
 <input type=range id=bri min=0 max=255 value=40 oninput="ch('bri',this.value)"></div>
<div class=row><label>Speed <span id=spl></span></label>
 <input type=range id=sp min=1 max=100 value=30 oninput="ch('speed',this.value)"></div>
<div class=row><label>Animation</label><div class=btns>
 <button id=an0 onclick="anim(0)">Static</button>
 <button id=an1 onclick="anim(1)">Hue</button>
 <button id=an2 onclick="anim(2)">Breathe</button>
 <button id=an3 onclick="anim(3)">Candle</button>
 <button id=an4 onclick="anim(4)">Fade</button>
</div></div>
<div class=row><label>Presets</label><div class=btns>
 <button onclick="preset(0,0,0,255)">W only</button>
 <button onclick="preset(255,255,255,0)">RGB white</button>
 <button onclick="preset(255,255,255,255)">RGBW full</button>
 <button onclick="preset(255,120,25,40)">Warm amber</button>
</div></div>
<div class=row><label>Wire order (module byte order; picker R/G swapped = try RGBW)</label><div class=btns>
 <button id=or0 onclick="order(0)">GRBW</button>
 <button id=or1 onclick="order(1)">RGBW</button>
 <button id=or2 onclick="order(2)">WRGB</button>
 <button id=or3 onclick="order(3)">GRB</button>
 <button id=or4 onclick="order(4)">RGB</button>
</div></div>
<div class=row><label>Wire slot test (raw byte, bypasses mapping; note the color each slot lights)</label><div class=btns>
 <button onclick="slot(0)">Slot 0</button>
 <button onclick="slot(1)">Slot 1</button>
 <button onclick="slot(2)">Slot 2</button>
 <button onclick="slot(3)">Slot 3</button>
 <button onclick="slot(-1)">Raw off</button>
</div></div>
<div class=row><label>Feed A/B (blank LED, re-plug module onto the other header, then toggle)</label><div class=btns>
 <button id=fd0 onclick="feed(0)">A: 3V3 rail + A0</button>
 <button id=fd1 onclick="feed(1)">B: VBAT + D13</button>
</div></div>
<div class=row><label>GND probe (pulldown high% &gt; 0 = return current on that line)</label><div class=btns>
 <button onclick="probe(10)">Probe 10</button>
 <button onclick="probe(11)">Probe 11</button>
</div><div id=probeout style="font-family:monospace;font-size:12px;color:#fc6;white-space:pre-wrap"></div></div>

<hr>
<div class=row><div class=btns>
 <button id=flash onclick="tog('flash')">Flash sync: on</button>
 <button id=gamma onclick="tog('gamma')">Gamma: on</button>
 <button id=arm onclick="tog('arm')">Armed: on</button>
</div></div>
<div class=row><div class=btns>
 <button id=rail onclick="tog('rail')">3V3 rail: off</button>
 <button onclick="fetch('/stop')">Stop / LED off</button>
</div></div>
<div class=row><div id=vals>...</div></div>
<div class=row><label>Battery</label><div id=bat>...</div></div>
<script>
let st={pulse:40,interval:600,auto:0,arm:1,rail:0,flash:1,gamma:1,r:255,g:140,b:40,w:0,bri:40,speed:30,anim:0};
function send(q){fetch('/set?'+q);}
function ch(k,v){st[k]=+v;send(k+'='+v);syncLabels();}
function sw(ms){fetch('/strike?ms='+ms);}
function hx(v){return ('0'+(+v).toString(16)).slice(-2);}
function setCol(hex){let r=parseInt(hex.substr(1,2),16),g=parseInt(hex.substr(3,2),16),b=parseInt(hex.substr(5,2),16);
 st.r=r;st.g=g;st.b=b;send('r='+r+'&g='+g+'&b='+b);}
function preset(r,g,b,w){st.r=r;st.g=g;st.b=b;st.w=w;w0.value=w;
 col.value='#'+hx(r)+hx(g)+hx(b);send('r='+r+'&g='+g+'&b='+b+'&w='+w);syncLabels();}
function anim(n){st.anim=n;send('anim='+n);animBtns();}
function animBtns(){for(let i=0;i<5;i++)document.getElementById('an'+i).className=(i==st.anim?'on':'');}
function order(n){st.order=n;send('order='+n);orderBtns();}
function orderBtns(){for(let i=0;i<5;i++)document.getElementById('or'+i).className=(i==st.order?'on':'');}
function slot(n){let q=[0,1,2,3].map(i=>'b'+i+'='+(i==n?255:0)).join('&');fetch('/raw?'+q);}
function feed(f){send('feed='+f);}
function pinBtns(){document.getElementById('fd0').className=(st.ledpin==10?'on':'');
 document.getElementById('fd1').className=(st.ledpin==11?'on':'');}
function probe(p){document.getElementById('probeout').textContent='probing GPIO'+p+'...';
 fetch('/gndprobe?pin='+p).then(r=>r.json()).then(j=>{
  document.getElementById('probeout').textContent='GPIO'+j.pin+': pulldown '+j.pulldown_high_pct+'% high ('+j.pulldown_edges+' edges), pullup '+j.pullup_high_pct+'% high';});}
function tog(k){st[k]^=1;send(k+'='+st[k]);togBtns();}
function togBtns(){
 for(const [k,lbl] of [['auto','Auto'],['flash','Flash sync'],['gamma','Gamma'],['arm','Armed'],['rail','3V3 rail']]){
  let e=document.getElementById(k);e.textContent=lbl+': '+(st[k]?'on':'off');e.className=st[k]?'on':'';}}
function syncLabels(){
 pulsel.textContent=st.pulse+' ms';
 intervall.textContent=st.interval+' ms';
 wl.textContent=st.w;bril.textContent=st.bri;spl.textContent=st.speed;}
const w0=document.getElementById('w');
function tick(){fetch('/state').then(r=>r.json()).then(s=>{
 document.getElementById('fw').textContent=s.fw;
 if(document.activeElement.type!='range'){
  st.pulse=s.pulse;st.interval=s.interval;st.w=s.w;st.bri=s.bri;st.speed=s.speed;
  pulse.value=s.pulse;interval.value=s.interval;w0.value=s.w;bri.value=s.bri;sp.value=s.speed;}
 st.auto=s.auto;st.arm=s.arm;st.rail=s.rail;st.flash=s.flash;st.gamma=s.gamma;st.anim=s.anim;st.order=s.order;st.ledpin=s.ledpin;
 togBtns();animBtns();orderBtns();pinBtns();syncLabels();
 vals.textContent='last    '+s.last+'\nstrikes '+s.strikes+'   blocked '+s.blocked+
  '   failsafe '+s.failsafes+'\ngate '+(s.gate?'HIGH':'low')+'   en3v3 '+s.en3v3+
  '   rssi '+s.rssi+' dBm';
 let bat=document.getElementById('bat');
 if(!s.pf){bat.textContent='no battery data (SDK init failed)';}
 else{let act=s.ma>30?('charging +'+s.ma+'mA'):(s.ma<-30?('discharging '+s.ma+'mA'):'idle ~'+s.ma+'mA');
  bat.textContent='SOC '+s.soc+'%  '+s.bv.toFixed(3)+'V  '+act+
   (s.sgood?('  |  supply '+s.sv.toFixed(2)+'V '+s.sma+'mA'):'  |  on battery')+
   (s.chg?'':'  |  charger disabled');}
 setTimeout(tick,600);}).catch(()=>setTimeout(tick,1200));}
syncLabels();togBtns();animBtns();tick();
</script></body></html>)HTML";

void handleStrike() {
  uint16_t ms = gPulseMs;
  const char *why = "web";
  if (server.hasArg("ms")) {
    ms = constrain(server.arg("ms").toInt(), (long)PULSE_MIN_MS, (long)PULSE_MAX_MS);
    why = "test";
  }
  const bool ok = strike(ms, why);
  server.send(ok ? 200 : 409, "text/plain", ok ? "ok" : "blocked");
}

void handleBurst() {
  burstStart(server.hasArg("n") ? server.arg("n").toInt() : 5);
  server.send(200, "text/plain", "ok");
}

void handleSet() {
  if (server.hasArg("pulse"))
    gPulseMs = constrain(server.arg("pulse").toInt(), (long)PULSE_MIN_MS, (long)PULSE_MAX_MS);
  if (server.hasArg("interval"))
    gIntervalMs = constrain(server.arg("interval").toInt(), 150L, 4000L);
  if (server.hasArg("auto")) {
    gAuto = server.arg("auto").toInt() != 0;
    gAutoNextMs = millis();
  }
  if (server.hasArg("arm")) {
    gArmed = server.arg("arm").toInt() != 0;
    if (!gArmed) stopAll();
  }
  if (server.hasArg("rail")) setRail(server.arg("rail").toInt() != 0);
  if (server.hasArg("flash")) gFlashSync = server.arg("flash").toInt() != 0;
  if (server.hasArg("r")) gR = server.arg("r").toInt();
  if (server.hasArg("g")) gG = server.arg("g").toInt();
  if (server.hasArg("b")) gB = server.arg("b").toInt();
  if (server.hasArg("w")) gW = server.arg("w").toInt();
  if (server.hasArg("bri")) gBri = server.arg("bri").toInt();
  if (server.hasArg("speed")) gSpeed = constrain(server.arg("speed").toInt(), 1L, 100L);
  if (server.hasArg("gamma")) gGamma = server.arg("gamma").toInt() != 0;
  if (server.hasArg("b2r")) gB2r = server.arg("b2r").toInt();
  if (server.hasArg("b2g")) gB2g = server.arg("b2g").toInt();
  if (server.hasArg("b2b")) gB2b = server.arg("b2b").toInt();
  if (server.hasArg("order")) applyOrder(server.arg("order").toInt());
  if (server.hasArg("ledpin")) switchLedPin(server.arg("ledpin").toInt());
  if (server.hasArg("feed")) { // A/B: 0 = 3V3 rail + A0, 1 = VBAT + D13(GPIO11)
    if (server.arg("feed").toInt() == 0) {
      switchLedPin(10);
      setRail(true);
    } else {
      switchLedPin(11);
      setRail(false);
    }
  }
  if (server.hasArg("anim")) {
    gAnim = constrain(server.arg("anim").toInt(), 0L, 4L);
    rgbwPhase = 0;
    candleLevel = candleTarget = 1.0f;
    strip.clear();
  }
  if (gAnim == 0) renderStaticLed();
  server.send(200, "text/plain", "ok");
}

// Write raw wire bytes b0..b3 straight into the frame, bypassing the logical
// color mapping: definitive die-per-slot test. Forces the 4-byte GRBW type
// (wire = [g,r,b,w] logical), so under it logical(r=b1, g=b0, b=b2, w=b3)
// emits exactly [b0,b1,b2,b3] on the wire. Resets the order setting to GRBW.
void handleRaw() {
  uint8_t v[4] = {0, 0, 0, 0};
  const char *keys[4] = {"b0", "b1", "b2", "b3"};
  for (int i = 0; i < 4; i++)
    if (server.hasArg(keys[i])) v[i] = constrain(server.arg(keys[i]).toInt(), 0L, 255L);
  applyOrder(0); // GRBW
  gAnim = 0;
  gFlashUntil = 0;
  for (uint16_t i = 0; i < strip.numPixels(); i++)
    strip.setPixelColor(i, v[1], v[0], v[2], v[3]);
  strip.show();
  Serial.printf("raw wire bytes: [%u,%u,%u,%u]\n", v[0], v[1], v[2], v[3]);
  server.send(200, "text/plain", "ok");
}

// GND-fault probe (2026-07-11): if the LED module's ground is open/floating,
// its return current can only exit via the data wire, so a tri-stated data pin
// with the weak (~45k) pulldown gets pulled up / flutters. A healthy grounded
// module presents a high-Z DIN and the pulldown reads solid LOW. A solid LOW
// does NOT fully exonerate the harness (an open V+ also reads LOW -- the module
// would be entirely dead). Optional ?strike=1 fires a 60 ms coil pulse mid-
// window: VBAT sag modulating the floating level is corroborating evidence.
// The pin is re-parked OUTPUT LOW and the RMT driver re-attached afterward.
uint32_t sampleHighPct(uint8_t pin, uint32_t ms, uint32_t *edges) {
  uint32_t highs = 0, n = 0, e = 0;
  int prev = digitalRead(pin);
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    int v = digitalRead(pin);
    highs += v;
    if (v != prev) e++;
    prev = v;
    n++;
    delayMicroseconds(800);
  }
  if (edges) *edges = e;
  return n ? highs * 100 / n : 0;
}

void handleGndProbe() {
  const bool doStrike = server.hasArg("strike");
  uint8_t pin = gLedPin;
  if (server.hasArg("pin")) {
    int p = server.arg("pin").toInt();
    if (!ledPinAllowed(p)) {
      server.send(400, "text/plain", "pin must be 10, 11 or 13");
      return;
    }
    pin = (uint8_t)p;
  }
  stopAll();
  gAnim = 0;
  gFlashUntil = 0;
  if (pin == gLedPin) rmtDeinit(pin); // release the pad from RMT so pinMode owns it

  pinMode(pin, INPUT_PULLDOWN);
  delay(10);
  uint32_t pdEdges = 0, puEdges = 0;
  uint32_t pdHigh = sampleHighPct(pin, 250, &pdEdges);

  pinMode(pin, INPUT_PULLUP); // baseline: reads high either way
  delay(10);
  uint32_t puHigh = sampleHighPct(pin, 250, &puEdges);

  // Strike correlation: pulldown again, 150 ms pre / 60 ms pulse / 200 ms post.
  int32_t preHigh = -1, durHigh = -1, postHigh = -1;
  bool struck = false;
  if (doStrike) {
    pinMode(pin, INPUT_PULLDOWN);
    delay(10);
    preHigh = sampleHighPct(pin, 150, nullptr);
    struck = strike(60, "probe"); // respects arm/rest-gap; esp_timer ends the pulse
    if (struck) {
      durHigh = sampleHighPct(pin, 60, nullptr);
      postHigh = sampleHighPct(pin, 200, nullptr);
    }
  }

  if (pin == gLedPin) {
    pinMode(pin, OUTPUT); // re-park: sink any return current at 0 V
    digitalWrite(pin, LOW);
    strip.clear();
    strip.show(); // re-attaches RMT (rmtInit runs again on show) + repaints dark
  } else {
    pinMode(pin, INPUT); // not ours -- leave it high-Z
  }

  static char buf[320];
  snprintf(buf, sizeof(buf),
           "{\"pin\":%u,\"pulldown_high_pct\":%lu,\"pulldown_edges\":%lu,"
           "\"pullup_high_pct\":%lu,\"pullup_edges\":%lu,"
           "\"struck\":%d,\"pre_high_pct\":%ld,\"during_high_pct\":%ld,"
           "\"post_high_pct\":%ld,"
           "\"note\":\"pulldown high>0 => return current on data line (GND fault); "
           "solid 0 => GND likely intact OR module fully unpowered (open V+)\"}",
           pin, (unsigned long)pdHigh, (unsigned long)pdEdges, (unsigned long)puHigh,
           (unsigned long)puEdges, struck ? 1 : 0, (long)preHigh, (long)durHigh,
           (long)postHigh);
  Serial.printf("gndprobe: %s\n", buf);
  server.send(200, "application/json", buf);
}

// ---- MSA311 impact sensing (strike-energy meter, 2026-07-11) -------------------
// Accelerometer strapped to the strike surface, daisy-chained on STEMMA-QT
// (Wire1, addr 0x62 -- coexists with the VEML at 0x10). Peak |mag - baseline|
// during the pulse window = objective strike energy; replaces by-ear scoring.
Adafruit_MSA311 msa;
bool gMsaOk = false;

bool msaInit() {
  if (!msa.begin(MSA311_I2CADDR_DEFAULT, &Wire1)) return false;
  msa.setRange(MSA301_RANGE_16_G);          // impacts are sharp; don't clip
  msa.setDataRate(MSA301_DATARATE_1000_HZ); // fastest ODR to catch the peak
  msa.setBandwidth(MSA301_BANDWIDTH_500_HZ);
  msa.setPowerMode(MSA301_NORMALMODE);
  return true;
}

float msaMag() {
  msa.read();
  return sqrtf(msa.x_g * msa.x_g + msa.y_g * msa.y_g + msa.z_g * msa.z_g);
}

// Strike probe (2026-07-11 VDC-tap bench): fire ONE gate pulse and sample the
// supply node (= VDC when the PSU/panel feeds it) before / mid-pulse / after.
// The BQ ADC has its own conversion cadence, so mid-pulse points are
// indicative, not oscilloscope truth -- the pre/post recovery pattern across
// repeated strikes is the reliable signal. Blocks loop() for ~ms+400.
void handleProbeStrike() {
  uint16_t ms = server.hasArg("ms")
                    ? (uint16_t)constrain(server.arg("ms").toInt(), (long)PULSE_MIN_MS,
                                          (long)PULSE_MAX_MS)
                    : gPulseMs;
  float svPre = 0, smaPre = 0, svMid = 0, smaMid = 0, svEnd = 0, sv150 = 0, sv400 = 0;
  bool goodPre = false, goodPost = false;
  if (gPfReady) {
    Board.getSupplyVoltage(svPre);
    Board.getSupplyCurrent(smaPre);
    Board.checkSupplyGood(goodPre);
  }
  if (!gMsaOk) gMsaOk = msaInit(); // hot-plug friendly, like the VEML
  float base = 0, peak = 0;
  if (gMsaOk) {
    for (int i = 0; i < 5; i++) base += msaMag();
    base /= 5.0f;
  }
  const bool ok = strike(ms, "probe");
  if (ok && gPfReady) {
    // Poll the accelerometer through pulse + ring-down; grab the (slow) BQ ADC
    // checkpoints on schedule inside the same loop.
    uint32_t t0 = millis();
    bool didMid = false, didEnd = false, did150 = false;
    while ((int32_t)(millis() - (t0 + ms + 220)) < 0) {
      if (gMsaOk) {
        float d = fabsf(msaMag() - base);
        if (d > peak) peak = d;
      }
      uint32_t el = millis() - t0;
      if (!didMid && el >= ms / 2) {
        Board.getSupplyVoltage(svMid);
        Board.getSupplyCurrent(smaMid);
        didMid = true;
      } else if (!didEnd && el >= (uint32_t)ms + 20) {
        Board.getSupplyVoltage(svEnd);
        didEnd = true;
      } else if (!did150 && el >= (uint32_t)ms + 150) {
        Board.getSupplyVoltage(sv150);
        did150 = true;
      }
    }
    Board.getSupplyVoltage(sv400);
    Board.checkSupplyGood(goodPost);
  }
  static char buf[340];
  snprintf(buf, sizeof(buf),
           "{\"ok\":%d,\"ms\":%u,\"msa\":%d,\"peak_mg\":%d,"
           "\"sv_pre\":%.3f,\"sma_pre\":%.0f,\"sv_mid\":%.3f,"
           "\"sma_mid\":%.0f,\"sv_end\":%.3f,\"sv_150\":%.3f,\"sv_400\":%.3f,"
           "\"good_pre\":%d,\"good_post\":%d,\"bv\":%.3f,\"failsafes\":%lu}",
           ok ? 1 : 0, ms, gMsaOk ? 1 : 0, (int)(peak * 1000.0f),
           svPre, smaPre, svMid, smaMid, svEnd, sv150, sv400,
           goodPre ? 1 : 0, goodPost ? 1 : 0, gBatV, (unsigned long)gFailsafes);
  server.send(200, "application/json", buf);
}

void handleState() {
  static char buf[704];
  snprintf(buf, sizeof(buf),
           "{\"fw\":\"%s\",\"pulse\":%u,\"interval\":%u,\"auto\":%d,\"arm\":%d,"
           "\"rail\":%d,\"flash\":%d,\"gamma\":%d,\"anim\":%u,\"order\":%u,\"ledpin\":%u,"
           "\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"bri\":%u,\"speed\":%u,"
           "\"gate\":%d,\"en3v3\":%d,\"strikes\":%lu,\"blocked\":%lu,\"failsafes\":%lu,"
           "\"last\":\"%s\",\"rssi\":%d,\"mac\":\"%s\","
           "\"pf\":%d,\"chg\":%d,\"bv\":%.3f,\"ma\":%.0f,\"soc\":%u,\"sv\":%.2f,"
           "\"sma\":%.0f,\"sgood\":%d}",
           FW_VERSION, gPulseMs, gIntervalMs, gAuto ? 1 : 0, gArmed ? 1 : 0,
           gRailOn ? 1 : 0, gFlashSync ? 1 : 0, gGamma ? 1 : 0, gAnim, gOrder, gLedPin,
           gR, gG, gB, gW, gBri, gSpeed,
           gGateOn ? 1 : 0, en3v3Level(), (unsigned long)gStrikes,
           (unsigned long)gBlocked, (unsigned long)gFailsafes, gLast, (int)WiFi.RSSI(),
           WiFi.macAddress().c_str(),
           gPfReady ? 1 : 0, gChargeOn ? 1 : 0, gBatV, gBatMa, gSoc, gSupV,
           gSupMa, gSupGood ? 1 : 0);
  server.send(200, "application/json", buf);
}

void setupWifi() {
#if HAVE_SECRETS
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname("ledsol");
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
    Serial.print("LED+Sol Bench STA at http://");
    Serial.println(WiFi.localIP());
    if (apOk) {
      Serial.print("LED+Sol Bench AP '" AP_SSID "' -> http://");
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
  // Gate low FIRST -- the coil supply is VBAT (always hot with a cell attached),
  // so the gate must be pinned low before anything else runs.
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW);

  Serial.begin(115200);
  delay(200);
  Serial.println("LED+Solenoid Bench " FW_VERSION);

  const esp_timer_create_args_t targs = {
      .callback = &pulseEnd, .arg = nullptr, .dispatch_method = ESP_TIMER_TASK,
      .name = "sol_pulse", .skip_unhandled_events = true};
  esp_timer_create(&targs, &gPulseTimer);

  Result pf = Result::Failure;
  for (int i = 0; i < 4 && pf != Result::Ok; i++) {
    pf = Board.init(2000, Mainboard::BatteryType::Generic_LFP);
    if (pf != Result::Ok) delay(250);
  }
  if (pf == Result::Ok) {
    gPfReady = true;
    Board.enableBatteryCharging(false); // chargeTick() enables it once the gauge warms up
    Board.setSupplyMaintainVoltage(BENCH_MAINTAIN_V);
    Board.enableVSQT(true); // VEML7700 on the STEMMA-QT port (lux A/B, 2026-07-11)
    // Loads are VBAT-fed: keep the 3V3 header rail OFF as a wiring check. If the
    // LED or coil goes dark, it is on the rail, not VBAT.
    Result r3 = Result::Failure;
    for (int i = 0; i < 5 && r3 != Result::Ok; i++) {
      r3 = Board.enable3V3(false);
      if (r3 != Result::Ok) delay(50);
    }
    Serial.printf("PowerFeather SDK Ok: VSQT off, charging OFF (until gauge check); "
                  "3V3 rail OFF (VBAT bench) %s, GPIO4 pad reads %d\n",
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
    Serial.println("WARNING: Board.init failed -- VBAT loads unaffected; charger UNCONFIGURED "
                   "(do NOT attach a cell while on USB)");
    Wire1.begin(47, 48, 100000);
  }
  Wire1.setClock(100000); // shared charger/gauge bus: 100 kHz, NEVER faster
  delay(50); // VSQT settle before the VEML probe
  gVemlPresent = vemlWriteConf();
  Serial.printf("VEML7700 on SQT: %s\n", gVemlPresent ? "present (gain 1/8, IT 100 ms)"
                                                      : "MISSING (re-probed on /lux)");
  gMsaOk = msaInit();
  Serial.printf("MSA311 on SQT: %s\n", gMsaOk ? "present (16g, 1 kHz -- impact meter)"
                                              : "MISSING (re-probed on /probe_strike)");

  strip.begin();
  strip.setBrightness(255);
  strip.clear();
  strip.show();
  renderStaticLed(); // warm amber at boot: proves data -> LED on the VBAT feed

  strike(gPulseMs, "boot"); // boot click: proves the whole gate->coil path

  setupWifi();
  if (MDNS.begin("ledsol")) { // http://ledsol.local/
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://ledsol.local/");
  } else {
    Serial.println("mDNS start failed (use the IP)");
  }
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/strike", handleStrike);
  server.on("/raw", handleRaw);
  server.on("/lux", handleLux);
  server.on("/probe_strike", handleProbeStrike);
  server.on("/gndprobe", handleGndProbe);
  server.on("/burst", handleBurst);
  server.on("/stop", []() {
    stopAll();
    gAnim = 0;
    gFlashUntil = 0;
    strip.clear();
    strip.show();
    server.send(200, "text/plain", "ok");
  });
  server.on("/set", handleSet);
  server.on("/state", handleState);
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
          stopAll(); // gate low + no queued strikes while flash writes stall loop()
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
  Serial.printf("LED+Sol Bench ready: LED GPIO%d (x%d), gate GPIO%d, pulse %u ms\n",
                LED_PIN, LED_COUNT, SOLENOID_PIN, gPulseMs);
}

void loop() {
  server.handleClient();
  failsafeTick();
  burstTick();
  autoTick();
  ledTick();
  batteryTick();
  chargeTick();
  solarGuardTick();
}
