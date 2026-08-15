// Chandelier addressable-pixel chain diagnostic for PowerFeather V2.
//
// One homogeneous RGBW or RGB chain on GPIO10/A0 and the switchable 3V3
// header rail. The automatic sequence distinguishes data/order faults from
// rail droop while a conservative aggregate-current model reserves part of the
// shared 1 A rail for the PowerFeather itself.

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PowerFeather.h>
#include "driver/rtc_io.h"
#include "esp_system.h"

using namespace PowerFeather;

#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 (build.sh does this)."
#endif

#define CHAIN_VERSION "chandelier-chain-2026-08-11.5"
#define DATA_PIN 10
#define USER_BUTTON_PIN 0
#define EN_3V3_PIN GPIO_NUM_4

#ifndef CHAIN_MAX_PIXELS
#define CHAIN_MAX_PIXELS 24
#endif
#ifndef CHAIN_START_PIXELS
#define CHAIN_START_PIXELS 4
#endif
#ifndef CHAIN_BUDGET_MA
#define CHAIN_BUDGET_MA 800
#endif
#ifndef CHAIN_START_BRIGHTNESS
#define CHAIN_START_BRIGHTNESS 64
#endif
#ifndef CHAIN_PIXEL_RGBW
#define CHAIN_PIXEL_RGBW 1
#endif
#ifndef CHAIN_BATTERY_MAH
#define CHAIN_BATTERY_MAH 6000
#endif

#if CHAIN_PIXEL_RGBW
#define CHAIN_NEO_TYPE (NEO_RGBW + NEO_KHZ800)
// Measured production 4 W RGBW module at full RGBW on the 3V3 rail.
#define CHAIN_FULL_PIXEL_MA 290
// Measured W-only draw is about 63 mA. Use 70 mA for the limiter so a
// dedicated-white fill can reach full scale without weakening the cap on the
// substantially heavier RGBW stress pattern.
#define CHAIN_WHITE_PIXEL_MA 70
#define CHAIN_TYPE_NAME "RGBW"
#else
#define CHAIN_NEO_TYPE (NEO_GRB + NEO_KHZ800)
// Measured lensed 3 W RGB module at full R+G+B on the sagged 3V3 rail:
// 256.5 mA (n=1, three cycles). Round up to the nearest milliamp.
#define CHAIN_FULL_PIXEL_MA 257
#define CHAIN_WHITE_PIXEL_MA CHAIN_FULL_PIXEL_MA
#define CHAIN_TYPE_NAME "RGB"
#endif

static Adafruit_NeoPixel gStrip(CHAIN_MAX_PIXELS, DATA_PIN, CHAIN_NEO_TYPE);
static bool gPfReady = false;
static bool gRailReady = false;
static bool gCharging = false;
static bool gChargeDecisionDone = false;
static uint8_t gPixels = CHAIN_START_PIXELS;
// Reboot-safe low baseline. Increase deliberately over serial after the chain
// proves stable; a brownout must never reboot back into the stress ceiling.
static uint8_t gRequestedBrightness = CHAIN_START_BRIGHTNESS;
static bool gAuto = true;
static uint8_t gManualPattern = 0;
static uint32_t gPhase = 0;
static uint32_t gLastFrameMs = 0;
static float gBatteryV = 0.0f, gBatteryMa = 0.0f;
static float gSupplyV = 0.0f, gSupplyMa = 0.0f;
static bool gSupplyGood = false;
static char gId[7] = "000000";

enum Pattern : uint8_t {
  PATTERN_RAINBOW = 0,
  PATTERN_RED = 1,
  PATTERN_GREEN = 2,
  PATTERN_BLUE = 3,
  PATTERN_WHITE = 4,
  PATTERN_BARS = 5,
  PATTERN_CHASE = 6,
  PATTERN_STRESS = 7,
};

static const char *patternName(uint8_t p) {
  switch (p) {
  case PATTERN_RAINBOW: return "spatial-rainbow";
  case PATTERN_RED: return "red-fill";
  case PATTERN_GREEN: return "green-fill";
  case PATTERN_BLUE: return "blue-fill";
  case PATTERN_WHITE:
#if CHAIN_PIXEL_RGBW
    return "white-die-fill";
#else
    return "rgb-white-fill";
#endif
  case PATTERN_BARS: return "indexed-rgbw-bars";
  case PATTERN_CHASE: return "white-chase";
  case PATTERN_STRESS: return "full-rgbw-stress";
  default: return "unknown";
  }
}

static uint16_t modeledPixelMa(uint8_t pattern) {
#if CHAIN_PIXEL_RGBW
  if (pattern == PATTERN_WHITE) return CHAIN_WHITE_PIXEL_MA;
#else
  (void)pattern;
#endif
  return CHAIN_FULL_PIXEL_MA;
}

static uint8_t brightnessCap(uint8_t pattern) {
  uint32_t denom =
      (uint32_t)max((uint8_t)1, gPixels) * modeledPixelMa(pattern);
  uint32_t cap = (uint32_t)CHAIN_BUDGET_MA * 255UL / denom;
  return (uint8_t)min(255UL, max(1UL, cap));
}

static uint8_t appliedBrightness(uint8_t pattern) {
  return min(gRequestedBrightness, brightnessCap(pattern));
}

static uint16_t estimatedWorstMa(uint8_t pattern) {
  return (uint16_t)((uint32_t)gPixels * modeledPixelMa(pattern) *
                    appliedBrightness(pattern) / 255UL);
}

static bool enableLedRail() {
  if (!gPfReady) return false;
  rtc_gpio_hold_dis(EN_3V3_PIN);
  for (int attempt = 1; attempt <= 4; attempt++) {
    Result r = Board.enable3V3(true);
    int level = rtc_gpio_get_level(EN_3V3_PIN);
    if (r == Result::Ok && level == 1) return true;
    Serial.printf("3V3 rail attempt %d -> sdk=%d gpio4=%d\n",
                  attempt, (int)r, level);
    delay(15);
  }
  return false;
}

static void disableLedRail() {
  gStrip.clear();
  gStrip.show(); // latch all-off before removing pixel power
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  if (gPfReady) Board.enable3V3(false);
  gRailReady = false;
  Serial.println("LED rail OFF: safe to change the chain");
}

static void restoreLedRail() {
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  gRailReady = enableLedRail();
  delay(25);
  gStrip.clear();
  gStrip.show();
  Serial.printf("LED rail %s\n", gRailReady ? "ON" : "FAILED");
}

static void refreshPower() {
  if (!gPfReady) return;
  float v = 0.0f;
  if (Board.getBatteryVoltage(v) == Result::Ok) gBatteryV = v;
  if (Board.getBatteryCurrent(v) == Result::Ok) gBatteryMa = v;
  if (Board.getSupplyVoltage(v) == Result::Ok) gSupplyV = v;
  if (Board.getSupplyCurrent(v) == Result::Ok) gSupplyMa = v;
  bool good = false;
  if (Board.checkSupplyGood(good) == Result::Ok) gSupplyGood = good;

  if (!gChargeDecisionDone && millis() >= 6000 && gBatteryV > 0.1f) {
    gChargeDecisionDone = true;
    if (gBatteryV > 2.5f && gBatteryV < 4.4f) {
      Board.setBatteryChargingMaxCurrent(2000.0f);
      gCharging = Board.enableBatteryCharging(true) == Result::Ok;
      Serial.printf("battery %.3f V present -> charging %s\n",
                    gBatteryV, gCharging ? "ON" : "FAILED");
    } else {
      Serial.printf("battery %.3f V implausible -> charging stays OFF\n", gBatteryV);
    }
  }
}

static uint8_t autoPattern(uint32_t now) {
  // 34-second visual ladder: motion/data, each channel, ordering, chase, then
  // the maximum modeled aggregate load.
  uint32_t s = (now / 1000UL) % 34UL;
  if (s < 10) return PATTERN_RAINBOW;
  if (s < 13) return PATTERN_RED;
  if (s < 16) return PATTERN_GREEN;
  if (s < 19) return PATTERN_BLUE;
  if (s < 22) return PATTERN_WHITE;
  if (s < 27) return PATTERN_BARS;
  if (s < 31) return PATTERN_CHASE;
  return PATTERN_STRESS;
}

static void setPixel(uint16_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
#if CHAIN_PIXEL_RGBW
  gStrip.setPixelColor(i, gStrip.Color(r, g, b, w));
#else
  (void)w;
  gStrip.setPixelColor(i, gStrip.Color(r, g, b));
#endif
}

static void renderPattern(uint8_t pattern, uint32_t now) {
  uint8_t v = appliedBrightness(pattern);
  gStrip.clear();
  switch (pattern) {
  case PATTERN_RAINBOW:
    for (uint8_t i = 0; i < gPixels; i++) {
      uint16_t hue = (uint16_t)(gPhase + (uint32_t)i * 65536UL / gPixels);
      gStrip.setPixelColor(i, gStrip.ColorHSV(hue, 255, v));
    }
    gPhase += 320;
    break;
  case PATTERN_RED:
    for (uint8_t i = 0; i < gPixels; i++) setPixel(i, v, 0, 0, 0);
    break;
  case PATTERN_GREEN:
    for (uint8_t i = 0; i < gPixels; i++) setPixel(i, 0, v, 0, 0);
    break;
  case PATTERN_BLUE:
    for (uint8_t i = 0; i < gPixels; i++) setPixel(i, 0, 0, v, 0);
    break;
  case PATTERN_WHITE:
#if CHAIN_PIXEL_RGBW
    for (uint8_t i = 0; i < gPixels; i++) setPixel(i, 0, 0, 0, v);
#else
    for (uint8_t i = 0; i < gPixels; i++) setPixel(i, v, v, v, 0);
#endif
    break;
  case PATTERN_BARS:
    for (uint8_t i = 0; i < gPixels; i++) {
      switch (i & 3) {
      case 0: setPixel(i, v, 0, 0, 0); break;
      case 1: setPixel(i, 0, v, 0, 0); break;
      case 2: setPixel(i, 0, 0, v, 0); break;
      default:
#if CHAIN_PIXEL_RGBW
        setPixel(i, 0, 0, 0, v);
#else
        setPixel(i, v, v, v, 0);
#endif
        break;
      }
    }
    break;
  case PATTERN_CHASE: {
    uint8_t head = (uint8_t)((now / 280UL) % gPixels);
    for (uint8_t i = 0; i < gPixels; i++)
      setPixel(i, i == head ? v : 0, i == head ? v : 0,
               i == head ? v : 0, i == head ? v : 0);
    break;
  }
  case PATTERN_STRESS:
    for (uint8_t i = 0; i < gPixels; i++)
      setPixel(i, v, v, v, v);
    break;
  }
  gStrip.show();
}

static void printTelemetry() {
  uint8_t p = gAuto ? autoPattern(millis()) : gManualPattern;
  Serial.printf(
      "{\"fw\":\"%s\",\"fixture_id\":\"%s\",\"pixel_type\":\"%s\","
      "\"pixels\":%u,\"pattern\":\"%s\",\"auto\":%s,"
      "\"brightness_requested\":%u,\"brightness_cap\":%u,"
      "\"brightness_applied\":%u,\"estimated_worst_ma\":%u,"
      "\"budget_ma\":%u,\"rail_ok\":%s,\"pf_ready\":%s,"
      "\"battery_present\":%s,\"charging_enabled\":%s,"
      "\"battery_v\":%.3f,\"battery_ma\":%.1f,"
      "\"supply_v\":%.3f,\"supply_ma\":%.1f,\"supply_good\":%s,"
      "\"reset_reason\":%d,\"uptime_ms\":%lu}\n",
      CHAIN_VERSION, gId, CHAIN_TYPE_NAME, (unsigned)gPixels, patternName(p),
      gAuto ? "true" : "false", (unsigned)gRequestedBrightness,
      (unsigned)brightnessCap(p), (unsigned)appliedBrightness(p),
      (unsigned)estimatedWorstMa(p), (unsigned)CHAIN_BUDGET_MA,
      gRailReady ? "true" : "false", gPfReady ? "true" : "false",
      (gBatteryV > 2.5f && gBatteryV < 4.4f) ? "true" : "false",
      gCharging ? "true" : "false", gBatteryV, gBatteryMa,
      gSupplyV, gSupplyMa, gSupplyGood ? "true" : "false",
      (int)esp_reset_reason(), (unsigned long)millis());
}

static void printHelp() {
  Serial.println("commands: t telemetry | n<N> pixels | b<0..255> brightness");
  Serial.println("          a toggle auto | m<0..7> manual pattern | + / - brightness");
  Serial.println("          o rail OFF for chain changes | p rail ON after chain changes");
  Serial.println("patterns: 0 rainbow 1 R 2 G 3 B 4 W 5 bars 6 chase 7 stress");
}

static void handleCommand(const char *cmd) {
  if (!cmd || !cmd[0]) return;
  if (cmd[0] == 't') {
    printTelemetry();
  } else if (cmd[0] == '?') {
    printHelp();
  } else if (cmd[0] == 'o') {
    disableLedRail();
  } else if (cmd[0] == 'p') {
    restoreLedRail();
  } else if (cmd[0] == 'a') {
    gAuto = !gAuto;
  } else if (cmd[0] == 'n') {
    int n = atoi(cmd + 1);
    if (n >= 1 && n <= CHAIN_MAX_PIXELS) gPixels = (uint8_t)n;
  } else if (cmd[0] == 'b') {
    int b = atoi(cmd + 1);
    if (b >= 0 && b <= 255) gRequestedBrightness = (uint8_t)b;
  } else if (cmd[0] == 'm') {
    int m = atoi(cmd + 1);
    if (m >= 0 && m <= PATTERN_STRESS) {
      gManualPattern = (uint8_t)m;
      gAuto = false;
    }
  } else if (cmd[0] == '+') {
    gRequestedBrightness = (uint8_t)min(255, (int)gRequestedBrightness + 16);
  } else if (cmd[0] == '-') {
    gRequestedBrightness = (uint8_t)max(0, (int)gRequestedBrightness - 16);
  }
  printTelemetry();
}

static void serialTick() {
  static char line[24];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      line[len] = 0;
      handleCommand(line);
      len = 0;
    } else if (len + 1 < sizeof(line)) {
      line[len++] = c;
    }
  }
}

void setup() {
  // Data low before Board.init, whose cold-start default may raise EN_3V3.
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  Serial.begin(115200);
  delay(800);

  uint64_t mac = ESP.getEfuseMac();
  snprintf(gId, sizeof(gId), "%02X%02X%02X", (uint8_t)(mac >> 24),
           (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
  Serial.printf("\n=== %s id=%s ===\n", CHAIN_VERSION, gId);

  Result r = Result::Failure;
  for (int attempt = 1; attempt <= 4; attempt++) {
    r = Board.init(CHAIN_BATTERY_MAH, Mainboard::BatteryType::Generic_LFP);
    if (r == Result::Ok) break;
    Serial.printf("Board.init attempt %d -> %d\n", attempt, (int)r);
    delay(250);
  }
  gPfReady = r == Result::Ok;
  if (gPfReady) {
    Board.setSupplyMaintainVoltage(4.6f);
    Board.setBatteryChargingMaxCurrent(2000.0f);
    Board.enableBatteryCharging(false); // deferred until a plausible cell read
  }

  gStrip.begin();
  gStrip.setBrightness(255);
  gStrip.clear();
  gStrip.show();
  gRailReady = enableLedRail();
  delay(25);
  gStrip.clear();
  gStrip.show();

  pinMode(USER_BUTTON_PIN, INPUT_PULLUP);
  refreshPower();
  printHelp();
  printTelemetry();
}

void loop() {
  serialTick();
  uint32_t now = millis();
  static uint32_t nextPowerMs = 0;
  static uint32_t nextTelemetryMs = 0;
  if (now >= nextPowerMs) {
    nextPowerMs = now + 1000;
    refreshPower();
  }
  if (now >= nextTelemetryMs) {
    nextTelemetryMs = now + 5000;
    printTelemetry();
  }
  if (gRailReady && now - gLastFrameMs >= 50) {
    gLastFrameMs = now;
    uint8_t p = gAuto ? autoPattern(now) : gManualPattern;
    renderPattern(p, now);
  }
}
