// Resonance power-bench firmware for the ESP32-S3 PowerFeather V2.
//
// Forked from firmware/smoke_test/smoke_test.ino. Adds PowerFeather-SDK power
// telemetry (battery + supply via MAX17260 / BQ25628E) and a JSON /telemetry
// endpoint so a host poller can log power data over WiFi across the three test
// axes: battery, LED option, and solar panel.
//
// LED option is chosen at build time (default NONE for telemetry-only bring-up):
//   -DRES_PF_LED_NEOHEX     M5Stack NeoHEX 37px WS2812 on GPIO16 (D6)
//   -DRES_PF_LED_RGBW1       single high-power SK6812 RGBW pixel on GPIO16 (D6)
//   -DRES_PF_LED_IS31        IS31FL3741 13x9 over STEMMA-QT (Wire1, GPIO47/48)
//
// Build with build.sh (it always sets -DPOWERFEATHER_BOARD_V2=1, REQUIRED for the
// V2 MAX17260 fuel gauge -- a bare `arduino-cli compile` will hit the #error guard):
//   ./build.sh --led is31 --cap 4400 --port /dev/ttyACM0    # USB flash
//   ./build.sh --led is31 --cap 4400 --ota 192.168.4.185   # wireless flash, no USB

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <Update.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_IS31FL3741.h>

#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_system.h"

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define RES_HAS_WIFI_SECRETS 1
#else
#define RES_HAS_WIFI_SECRETS 0
#endif

#ifndef RES_WIFI_AUTO_CONNECT
#define RES_WIFI_AUTO_CONNECT 0
#endif

// Battery-friendly WiFi (modem sleep + reduced TX power) to reduce TX current
// bursts that can brown out VSYS on battery. Off by default. See build.sh --wifi-lowpower.
#ifndef RES_WIFI_LOWPOWER
#define RES_WIFI_LOWPOWER 0
#endif

// Battery-stress test mode: radio fully OFF, no web server. Blinks the LED-panel
// center pixel (+ onboard LED) as a 1 Hz heartbeat with a triple-flash on each
// boot, so a reboot is visible without any connection. Isolates whether
// battery-only resets are caused by WiFi current spikes (stable here => yes) or
// the board's battery path (still resets here => board-level). See build.sh --batt-stress.
#ifndef RES_BATT_STRESS
#define RES_BATT_STRESS 0
#endif

// When set (with RES_BATT_STRESS), the heartbeat blinks the FULL LED array at max
// brightness instead of one center pixel -- a large repeated current step to test
// whether an LED-driven transient (not just WiFi) browns out VSYS on battery.
#ifndef RES_BATT_STRESS_FULL
#define RES_BATT_STRESS_FULL 0
#endif

// Load-generator mode for brownout characterization: WiFi station (no HTTP
// server), emits a 1/s UDP heartbeat carrying uptime (so a host listener detects
// resets/outages remotely). Sub-flags add a constant full-grid LED load and/or
// heavy sustained UDP TX, so we can sweep {light/heavy WiFi} x {LED off/on} on a
// fixed battery. See build.sh --loadgen / --loadgen-led / --tx-heavy.
#ifndef RES_LOADGEN
#define RES_LOADGEN 0
#endif
#ifndef RES_LOADGEN_LED
#define RES_LOADGEN_LED 0
#endif
#ifndef RES_LOADGEN_TXHEAVY
#define RES_LOADGEN_TXHEAVY 0
#endif

#define POWER_BENCH_VERSION "power-bench-2026-06-03.5"

// ---------------------------------------------------------------------------
// Board / LED-option detection
// ---------------------------------------------------------------------------
#if defined(ARDUINO_ESP32S3_POWERFEATHER)
#define RES_BOARD_NAME "powerfeather_v2"
#define RES_HAS_PF_TELEMETRY 1

#if defined(RES_PF_LED_NEOHEX)
#define RES_HAS_NEOPIXEL 1
#define RES_PIXEL_PIN 16 // D6 (GPIO16), free IO
#define RES_PIXEL_COUNT 37
#define RES_PIXEL_CENTER 18
#define RES_PIXEL_LAYOUT_HEX37 1
#define RES_LED_OPTION "neohex37"
#elif defined(RES_PF_LED_RGBW1)
#define RES_HAS_NEOPIXEL 1
#define RES_PIXEL_PIN 16 // D6 (GPIO16), free IO
#define RES_PIXEL_COUNT 1
#define RES_PIXEL_CENTER 0
#define RES_PIXEL_TYPE_RGBW 1
#define RES_LED_OPTION "rgbw_single"
#elif defined(RES_PF_LED_IS31)
#define RES_HAS_IS31 1
#define RES_LED_OPTION "is31_13x9"
#else
#define RES_LED_OPTION "none"
#endif

#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32C6)
#define RES_BOARD_NAME "adafruit_feather_esp32c6"
#define RES_HAS_IS31 1
#define RES_LED_OPTION "is31_13x9"
#elif defined(ARDUINO_FEATHERS2NEO)
#define RES_BOARD_NAME "um_feathers2neo"
#define RES_HAS_NEOPIXEL 1
#define RES_PIXEL_PIN NEOPIXEL_MATRIX_DATA
#define RES_PIXEL_POWER_PIN NEOPIXEL_MATRIX_PWR
#define RES_PIXEL_COUNT 25
#define RES_PIXEL_CENTER 12
#define RES_LED_OPTION "neopixel5x5"
#else
#error "Unsupported board. This sketch targets esp32s3_powerfeather (and the smoke-test COTS boards)."
#endif

#ifndef RES_HAS_IS31
#define RES_HAS_IS31 0
#endif
#ifndef RES_HAS_NEOPIXEL
#define RES_HAS_NEOPIXEL 0
#endif
#ifndef RES_HAS_PF_TELEMETRY
#define RES_HAS_PF_TELEMETRY 0
#endif
#ifndef RES_LED_OPTION
#define RES_LED_OPTION "unknown"
#endif

#if RES_HAS_PF_TELEMETRY
#include <PowerFeather.h>
using namespace PowerFeather;

// The PowerFeather SDK selects the fuel gauge at COMPILE TIME: MAX17260 (V2) only
// if POWERFEATHER_BOARD_V2 / CONFIG_ESP32S3_POWERFEATHER_V2 is defined, else the V1
// LC709204F. In an Arduino build neither is set by default, so the gauge silently
// falls back to V1 and all SOC/health/cycles reads fail (probe on the wrong IC).
// This must be a GLOBAL build flag (compiler.cpp.extra_flags) so the SDK library
// translation units see it -- a #define here would not reach them. Use build.sh.
#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 so the SDK uses the V2 MAX17260 fuel gauge. See firmware/power_bench/build.sh / README."
#endif

// --- PowerFeather power configuration (override with -D at build time) ------
// Li-ion cell on hand. Switch RES_PF_BATTERY_TYPE to Generic_LFP for LiFePO4.
#ifndef RES_PF_BATTERY_CAPACITY_MAH
#define RES_PF_BATTERY_CAPACITY_MAH 2000
#endif
#ifndef RES_PF_BATTERY_TYPE
#define RES_PF_BATTERY_TYPE Mainboard::BatteryType::Generic_3V7
#endif
// init() leaves charging DISABLED by default; enable it so a USB/solar supply
// tops up the cell. Keep <= 1C for the cell capacity.
#ifndef RES_PF_ENABLE_CHARGING
#define RES_PF_ENABLE_CHARGING 1
#endif
// Charge-current ceiling (mA). The charger (BQ25628E) accepts 40-2000 mA and
// self-limits to what the supply can give (input regulation at MAINTAIN_V), so
// this is a cap, not a guarantee. 1000 mA is <= 0.5C for cells >= 2000 mAh and
// gentle on reused cells; lower it (--charge-ma) for smaller cells (<= 1C).
#ifndef RES_PF_MAX_CHARGE_MA
#define RES_PF_MAX_CHARGE_MA 1000.0f
#endif
// Supply maintain voltage (charger VINDPM). 4.6 V default; set to panel MPP for
// solar runs. Valid range 4.6-16.8 V.
#ifndef RES_PF_MAINTAIN_V
#define RES_PF_MAINTAIN_V 4.6f
#endif
#endif // RES_HAS_PF_TELEMETRY

#if RES_HAS_IS31
Adafruit_IS31FL3741_QT_buffered matrix;
#endif

#if RES_HAS_NEOPIXEL
#ifdef RES_PIXEL_TYPE_RGBW
Adafruit_NeoPixel pixels(RES_PIXEL_COUNT, RES_PIXEL_PIN, NEO_GRBW + NEO_KHZ800);
#else
Adafruit_NeoPixel pixels(RES_PIXEL_COUNT, RES_PIXEL_PIN, NEO_GRB + NEO_KHZ800);
#endif
#endif

WebServer server(80);
bool otaActive = false;
bool otaRoutesConfigured = false;
bool is31Ready = false;
uint32_t lastHeartbeatMs = 0;
String shortId;
String otaMode = "off";
char activeMeasurementMode = '0';

#if RES_HAS_PF_TELEMETRY
bool pfReady = false;
int pfInitResult = -1;
#endif

#if RES_HAS_NEOPIXEL
#if defined(RES_PIXEL_LAYOUT_HEX37)
// NeoHEX center pixel plus its first ring.
const uint8_t neoCropPixels[] = {11, 12, 17, 18, 19, 24, 25};
#elif RES_PIXEL_COUNT >= 19
const uint8_t neoCropPixels[] = {6, 7, 8, 11, 12, 13, 16, 17, 18};
#else
const uint8_t neoCropPixels[] = {0};
#endif
#endif

const char *measurementModeName(char mode);
bool applyMeasurementMode(char mode);
void stopOtaAndWifi();

const char *resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
  case ESP_RST_POWERON:
    return "poweron";
  case ESP_RST_EXT:
    return "external";
  case ESP_RST_SW:
    return "software";
  case ESP_RST_PANIC:
    return "panic";
  case ESP_RST_INT_WDT:
    return "interrupt_watchdog";
  case ESP_RST_TASK_WDT:
    return "task_watchdog";
  case ESP_RST_WDT:
    return "other_watchdog";
  case ESP_RST_DEEPSLEEP:
    return "deepsleep";
  case ESP_RST_BROWNOUT:
    return "brownout";
  case ESP_RST_SDIO:
    return "sdio";
  case ESP_RST_USB:
    return "usb";
  case ESP_RST_JTAG:
    return "jtag";
  case ESP_RST_EFUSE:
    return "efuse";
  case ESP_RST_PWR_GLITCH:
    return "power_glitch";
  case ESP_RST_CPU_LOCKUP:
    return "cpu_lockup";
  default:
    return "unknown";
  }
}

String macString() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

String compactIdFromMac() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[7];
  snprintf(buf, sizeof(buf), "%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(buf);
}

void setupBoardPower() {
#if defined(NEOPIXEL_I2C_POWER)
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, HIGH);
#endif
#if RES_HAS_NEOPIXEL && defined(RES_PIXEL_POWER_PIN)
  pinMode(RES_PIXEL_POWER_PIN, OUTPUT);
  digitalWrite(RES_PIXEL_POWER_PIN, HIGH);
#endif
}

void clearLeds() {
#if RES_HAS_IS31
  if (is31Ready) {
    matrix.fill(0);
    matrix.show();
  }
#endif
#if RES_HAS_NEOPIXEL
  pixels.clear();
  pixels.show();
#endif
}

void setIs31Drive(uint8_t ledScaling, uint8_t globalCurrent) {
#if RES_HAS_IS31
  if (is31Ready) {
    matrix.setLEDscaling(ledScaling);
    matrix.setGlobalCurrent(globalCurrent);
  }
#else
  (void)ledScaling;
  (void)globalCurrent;
#endif
}

void fillIs31(uint16_t color) {
#if RES_HAS_IS31
  if (is31Ready) {
    matrix.fill(color);
    matrix.show();
  }
#else
  (void)color;
#endif
}

void clearNeoPixelsFullScale() {
#if RES_HAS_NEOPIXEL
  pixels.setBrightness(255);
  pixels.clear();
#endif
}

void showNeoPixels() {
#if RES_HAS_NEOPIXEL
  pixels.show();
#endif
}

const char *measurementModeName(char mode) {
  switch (mode) {
  case '0':
    return "off_wifi_state_unchanged";
  case 'q':
    return "quiet_baseline_wifi_off_leds_off";
  case '1':
    return "center_max_white";
  case '2':
    return "three_pixel_rgb_fringe";
  case '3':
    return "center_3x3_dim_warm_white";
  case '4':
    return "full_array_very_low_white";
  case '5':
    return "full_array_capped_white_brief";
  default:
    return "unknown";
  }
}

bool isMeasurementMode(char mode) {
  return mode == '0' || mode == 'q' || mode == '1' || mode == '2' ||
         mode == '3' || mode == '4' || mode == '5';
}

void printMeasurementMode(char mode) {
  Serial.printf("measurement_mode: %c %s\n", mode, measurementModeName(mode));
#if RES_HAS_IS31
  if (is31Ready) {
    Serial.printf("  is31_global_current=%u\n", matrix.getGlobalCurrent());
  }
#endif
#if RES_HAS_NEOPIXEL
  Serial.printf("  neopixel_brightness=%u/255\n", pixels.getBrightness());
  Serial.printf("  neopixel_pin=%d count=%d center=%d\n", RES_PIXEL_PIN,
                RES_PIXEL_COUNT, RES_PIXEL_CENTER);
#endif
  Serial.printf("  wifi=%s ota=%s\n",
                WiFi.status() == WL_CONNECTED ? "connected" : "not_connected",
                otaActive ? "on" : "off");
}

bool applyMeasurementMode(char mode) {
  if (!isMeasurementMode(mode)) {
    return false;
  }

  if (mode == 'q') {
    stopOtaAndWifi();
    clearLeds();
    activeMeasurementMode = mode;
    printMeasurementMode(mode);
    return true;
  }

  activeMeasurementMode = mode;

  switch (mode) {
  case '0':
    clearLeds();
    break;

  case '1':
#if RES_HAS_IS31
    setIs31Drive(0xFF, 0xFF);
    if (is31Ready) {
      matrix.fill(0);
      matrix.drawPixel(6, 4, matrix.color565(255, 255, 255));
      matrix.show();
    }
#endif
#if RES_HAS_NEOPIXEL
    clearNeoPixelsFullScale();
#ifdef RES_PIXEL_TYPE_RGBW
    pixels.setPixelColor(RES_PIXEL_CENTER, pixels.Color(0, 0, 0, 255));
#else
    pixels.setPixelColor(RES_PIXEL_CENTER, pixels.Color(255, 255, 255));
#endif
    showNeoPixels();
#endif
    break;

  case '2':
#if RES_HAS_IS31
    setIs31Drive(0x28, 0x10);
    if (is31Ready) {
      matrix.fill(0);
      matrix.drawPixel(5, 4, matrix.color565(32, 0, 0));
      matrix.drawPixel(6, 4, matrix.color565(0, 32, 0));
      matrix.drawPixel(7, 4, matrix.color565(0, 0, 32));
      matrix.show();
    }
#endif
#if RES_HAS_NEOPIXEL
    clearNeoPixelsFullScale();
#if RES_PIXEL_COUNT >= 3
    pixels.setPixelColor(RES_PIXEL_CENTER - 1, pixels.Color(20, 0, 0));
    pixels.setPixelColor(RES_PIXEL_CENTER, pixels.Color(0, 18, 0));
    pixels.setPixelColor(RES_PIXEL_CENTER + 1, pixels.Color(0, 0, 20));
#else
    pixels.setPixelColor(RES_PIXEL_CENTER, pixels.Color(0, 18, 0));
#endif
    showNeoPixels();
#endif
    break;

  case '3':
#if RES_HAS_IS31
    setIs31Drive(0x20, 0x0C);
    if (is31Ready) {
      matrix.fill(0);
      for (int y = 3; y <= 5; y++) {
        for (int x = 5; x <= 7; x++) {
          matrix.drawPixel(x, y, matrix.color565(16, 16, 8));
        }
      }
      matrix.show();
    }
#endif
#if RES_HAS_NEOPIXEL
    clearNeoPixelsFullScale();
    for (uint8_t i = 0; i < sizeof(neoCropPixels); i++) {
      pixels.setPixelColor(neoCropPixels[i], pixels.Color(6, 5, 4));
    }
    showNeoPixels();
#endif
    break;

  case '4':
#if RES_HAS_IS31
    setIs31Drive(0x18, 0x08);
    if (is31Ready) {
      fillIs31(matrix.color565(8, 8, 8));
    }
#endif
#if RES_HAS_NEOPIXEL
    clearNeoPixelsFullScale();
    for (uint16_t i = 0; i < RES_PIXEL_COUNT; i++) {
      pixels.setPixelColor(i, pixels.Color(2, 2, 2));
    }
    showNeoPixels();
#endif
    break;

  case '5':
#if RES_HAS_IS31
    setIs31Drive(0x30, 0x10);
    if (is31Ready) {
      fillIs31(matrix.color565(24, 24, 24));
    }
#endif
#if RES_HAS_NEOPIXEL
    clearNeoPixelsFullScale();
    for (uint16_t i = 0; i < RES_PIXEL_COUNT; i++) {
      pixels.setPixelColor(i, pixels.Color(10, 10, 10));
    }
    showNeoPixels();
#endif
    break;
  }

  printMeasurementMode(mode);
  return true;
}

void scanBus(TwoWire &bus, const char *label) {
  Serial.printf("I2C scan (%s):\n", label);
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    bus.beginTransmission(addr);
    uint8_t error = bus.endTransmission();
    if (error == 0) {
      Serial.printf("  0x%02X", addr);
      if (addr == IS3741_ADDR_DEFAULT) {
        Serial.print("  IS31FL3741-default");
      }
      Serial.println();
      found++;
    }
  }
  if (!found) {
    Serial.println("  no devices found");
  }
}

void runI2cScan() {
  Serial.println();
  scanBus(Wire, "default Wire");
#if RES_HAS_PF_TELEMETRY
  // On PowerFeather V2 the STEMMA-QT module + charger/gauge live on Wire1 (47/48).
  scanBus(Wire1, "Wire1 STEMMA-QT 47/48");
#endif
}

void setupIs31() {
#if RES_HAS_IS31
  Serial.println("IS31FL3741 setup:");
#if RES_HAS_PF_TELEMETRY
  // PowerFeather V2: IS31 is on the STEMMA-QT bus = Wire1 (GPIO47/48), shared
  // with the SDK charger/gauge. Board.init() already started Wire1 and enabled
  // VSQT. Keep the bus at the SDK's 100 kHz so power-mgmt comms stay stable.
  TwoWire *ledWire = &Wire1;
  Wire1.begin(47, 48, 100000);
#else
  TwoWire *ledWire = &Wire;
#endif
  if (!matrix.begin(IS3741_ADDR_DEFAULT, ledWire)) {
    Serial.println("  not found at 0x30");
    is31Ready = false;
    return;
  }

#if !RES_HAS_PF_TELEMETRY
  Wire.setClock(400000);
#endif
  matrix.setLEDscaling(0x20);
  matrix.setGlobalCurrent(0x10);
  matrix.enable(true);
  matrix.setRotation(0);
  matrix.fill(0);
  matrix.show();
  is31Ready = true;
  Serial.printf("  found, global_current=%u, size=%dx%d\n",
                matrix.getGlobalCurrent(), matrix.width(), matrix.height());
#endif
}

void setupNeoPixels() {
#if RES_HAS_NEOPIXEL
  pixels.begin();
  pixels.setBrightness(255);
  pixels.clear();
  pixels.show();
  Serial.printf("NeoPixel setup: pin=%d count=%d brightness=255/255\n",
                RES_PIXEL_PIN, RES_PIXEL_COUNT);
#endif
}

// ---------------------------------------------------------------------------
// PowerFeather telemetry
// ---------------------------------------------------------------------------
#if RES_HAS_PF_TELEMETRY
const char *batteryTypeName() {
  switch (RES_PF_BATTERY_TYPE) {
  case Mainboard::BatteryType::Generic_3V7:
    return "Generic_3V7";
  case Mainboard::BatteryType::ICR18650_26H:
    return "ICR18650_26H";
  case Mainboard::BatteryType::UR18650ZY:
    return "UR18650ZY";
  case Mainboard::BatteryType::Generic_LFP:
    return "Generic_LFP";
  default:
    return "unknown";
  }
}

void setupPowerFeather() {
  Serial.println("PowerFeather SDK init:");
  // The MAX17260 model init can need a moment after a flash-triggered reset;
  // retry a few times so a post-upload boot reliably comes up ready.
  Result r = Result::Failure;
  for (int attempt = 1; attempt <= 4; attempt++) {
    r = Board.init((uint16_t)RES_PF_BATTERY_CAPACITY_MAH, RES_PF_BATTERY_TYPE);
    if (r == Result::Ok) {
      break;
    }
    Serial.printf("  Board.init attempt %d -> %d, retrying\n", attempt,
                  static_cast<int>(r));
    delay(250);
  }
  pfInitResult = static_cast<int>(r);
  pfReady = (r == Result::Ok);
  Serial.printf("  Board.init(capacity=%u, type=%s) -> %s (%d)\n",
                (unsigned)RES_PF_BATTERY_CAPACITY_MAH, batteryTypeName(),
                pfReady ? "Ok" : "ERR", pfInitResult);
  if (!pfReady) {
    Serial.println("  telemetry unavailable; check battery/board");
    return;
  }

  Result mr = Board.setSupplyMaintainVoltage((float)RES_PF_MAINTAIN_V);
  Serial.printf("  setSupplyMaintainVoltage(%.2f) -> %d\n", (float)RES_PF_MAINTAIN_V,
                static_cast<int>(mr));

#if RES_PF_ENABLE_CHARGING
  Result cc = Board.setBatteryChargingMaxCurrent((float)RES_PF_MAX_CHARGE_MA);
  Result ce = Board.enableBatteryCharging(true);
  Serial.printf("  charging enabled, max=%.0f mA -> setCurrent %d, enable %d\n",
                (float)RES_PF_MAX_CHARGE_MA, static_cast<int>(cc),
                static_cast<int>(ce));
#else
  Board.enableBatteryCharging(false);
  Serial.println("  charging disabled");
#endif
}

// JSON helpers: append a numeric field, or null + record the field name in errs.
void addNumF(String &j, String &errs, const char *key, float v, bool ok) {
  j += ",\"";
  j += key;
  j += "\":";
  if (ok) {
    char b[24];
    snprintf(b, sizeof(b), "%.3f", v);
    j += b;
  } else {
    j += "null";
    if (errs.length()) errs += ",";
    errs += "\"";
    errs += key;
    errs += "\"";
  }
}

void addNumI(String &j, String &errs, const char *key, long v, bool ok) {
  j += ",\"";
  j += key;
  j += "\":";
  if (ok) {
    j += String(v);
  } else {
    j += "null";
    if (errs.length()) errs += ",";
    errs += "\"";
    errs += key;
    errs += "\"";
  }
}
#endif // RES_HAS_PF_TELEMETRY

String telemetryJson() {
  String j = "{";
  j += "\"board\":\"" RES_BOARD_NAME "\"";
  j += ",\"fw\":\"" POWER_BENCH_VERSION "\"";
  j += ",\"fixture_id\":\"" + shortId + "\"";
  j += ",\"led_option\":\"" RES_LED_OPTION "\"";
  j += ",\"led_mode\":\"";
  j += activeMeasurementMode;
  j += "\"";
  j += ",\"uptime_ms\":" + String((unsigned long)millis());
  j += ",\"heap_free\":" + String(ESP.getFreeHeap());
  j += ",\"reset_reason\":\"" + String(resetReasonName(esp_reset_reason())) + "\"";

#if RES_HAS_PF_TELEMETRY
  j += ",\"pf_ready\":";
  j += (pfReady ? "true" : "false");
  j += ",\"battery_type\":\"" + String(batteryTypeName()) + "\"";
  String errs = "";
  if (pfReady) {
    {
      float v;
      Result rr = Board.getBatteryVoltage(v);
      addNumF(j, errs, "battery_v", v, rr == Result::Ok);
    }
    {
      float v;
      Result rr = Board.getBatteryCurrent(v);
      addNumF(j, errs, "battery_ma", v, rr == Result::Ok);
    }
    {
      uint8_t v;
      bool ok = Board.getBatteryCharge(v) == Result::Ok;
      addNumI(j, errs, "soc_pct", v, ok);
    }
    {
      uint8_t v;
      bool ok = Board.getBatteryHealth(v) == Result::Ok;
      addNumI(j, errs, "health_pct", v, ok);
    }
    {
      uint16_t v;
      bool ok = Board.getBatteryCycles(v) == Result::Ok;
      addNumI(j, errs, "cycles", v, ok);
    }
    {
      int v;
      bool ok = Board.getBatteryTimeLeft(v) == Result::Ok;
      addNumI(j, errs, "time_left_min", v, ok);
    }
    {
      float v;
      Result rr = Board.getSupplyVoltage(v);
      addNumF(j, errs, "supply_v", v, rr == Result::Ok);
    }
    {
      float v;
      Result rr = Board.getSupplyCurrent(v);
      addNumF(j, errs, "supply_ma", v, rr == Result::Ok);
    }
    {
      bool good = false;
      bool ok = Board.checkSupplyGood(good) == Result::Ok;
      j += ",\"supply_good\":";
      j += ok ? (good ? "true" : "false") : "null";
      if (!ok) {
        if (errs.length()) errs += ",";
        errs += "\"supply_good\"";
      }
    }
  }
  j += ",\"telemetry_errors\":[" + errs + "]";
#endif

  j += "}";
  return j;
}

void printTelemetry() {
  Serial.println(telemetryJson());
}

void printOtaPartitionInfo() {
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *boot = esp_ota_get_boot_partition();
  Serial.print("OTA running partition: ");
  Serial.println(running ? running->label : "unknown");
  Serial.print("OTA boot partition: ");
  Serial.println(boot ? boot->label : "unknown");
}

void printWifiInfo() {
  Serial.print("wifi_secrets_compiled: ");
  Serial.println(RES_HAS_WIFI_SECRETS ? "yes" : "no");
  Serial.print("wifi_status: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "connected" : "not_connected");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("wifi_ssid: ");
    Serial.println(WiFi.SSID());
    Serial.print("wifi_ip: ");
    Serial.println(WiFi.localIP());
  }
  Serial.print("ota_mode: ");
  Serial.println(otaMode);
  Serial.print("measurement_mode: ");
  Serial.print(activeMeasurementMode);
  Serial.print(" ");
  Serial.println(measurementModeName(activeMeasurementMode));
}

void printReport() {
  Serial.println();
  Serial.println("=== Resonance power-bench ===");
  Serial.printf("version: %s\n", POWER_BENCH_VERSION);
  Serial.printf("board: %s\n", RES_BOARD_NAME);
  Serial.printf("led_option: %s\n", RES_LED_OPTION);
  Serial.printf("chip: %s rev %u, cores=%u\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getChipCores());
  Serial.printf("flash: %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("mac: %s\n", macString().c_str());
  Serial.printf("fixture_id: %s\n", shortId.c_str());
  Serial.printf("reset_reason: %s (%d)\n", resetReasonName(esp_reset_reason()),
                static_cast<int>(esp_reset_reason()));
  Serial.printf("heap_free: %u\n", ESP.getFreeHeap());
  Serial.printf("i2c_sda: %d\n", SDA);
  Serial.printf("i2c_scl: %d\n", SCL);
#if RES_HAS_PF_TELEMETRY
  Serial.printf("pf_ready: %s (init=%d)\n", pfReady ? "yes" : "no", pfInitResult);
  Serial.printf("battery_type: %s\n", batteryTypeName());
#endif
#if RES_HAS_IS31
  Serial.printf("is31_ready: %s\n", is31Ready ? "yes" : "no");
#endif
#if RES_HAS_NEOPIXEL
  Serial.printf("neopixel_pin: %d\n", RES_PIXEL_PIN);
  Serial.printf("neopixel_count: %d\n", RES_PIXEL_COUNT);
#endif
  Serial.printf("ota_web_active: %s\n", otaActive ? "yes" : "no");
  printOtaPartitionInfo();
  printWifiInfo();
}

String otaFormHtml() {
  String html;
  html += F("<!doctype html><html><head><meta name='viewport' "
            "content='width=device-width,initial-scale=1'>");
  html += F("<title>Resonance Power Bench</title></head><body>");
  html += F("<h1>Resonance Power Bench</h1>");
  html += F("<p>Board: ");
  html += RES_BOARD_NAME;
  html += F("<br>LED: ");
  html += RES_LED_OPTION;
  html += F("<br>Fixture: ");
  html += shortId;
  html += F("<br>Version: ");
  html += POWER_BENCH_VERSION;
  html += F("<br>Mode: ");
  html += activeMeasurementMode;
  html += F(" ");
  html += measurementModeName(activeMeasurementMode);
  html += F("</p><p>LED modes: ");
  html += F("<a href='/mode?m=0'>0 off</a> ");
  html += F("<a href='/mode?m=1'>1 center max</a> ");
  html += F("<a href='/mode?m=2'>2 RGB</a> ");
  html += F("<a href='/mode?m=3'>3 3x3</a> ");
  html += F("<a href='/mode?m=4'>4 full low</a> ");
  html += F("<a href='/mode?m=5'>5 capped brief</a> ");
  html += F("<a href='/mode?m=q'>q quiet</a></p>");
  html += F("<p><a href='/telemetry'>/telemetry</a></p>");
  html += F("<form method='POST' action='/update' "
            "enctype='multipart/form-data'>");
  html += F("<input type='file' name='firmware'>");
  html += F("<input type='submit' value='Update'>");
  html += F("</form></body></html>");
  return html;
}

void configureOtaRoutes() {
  if (otaRoutesConfigured) {
    return;
  }

  server.on("/", HTTP_GET, []() { server.send(200, "text/html", otaFormHtml()); });

  server.on("/telemetry", HTTP_GET,
            []() { server.send(200, "application/json", telemetryJson()); });

  server.on("/mode", HTTP_GET, []() {
    if (!server.hasArg("m") || server.arg("m").length() != 1) {
      server.send(400, "text/plain", "Missing mode. Use /mode?m=0,1,2,3,4,5,q\n");
      return;
    }

    char mode = server.arg("m")[0];
    if (!isMeasurementMode(mode)) {
      server.send(400, "text/plain", "Unknown measurement mode\n");
      return;
    }

    String reply = "Mode ";
    reply += mode;
    reply += " ";
    reply += measurementModeName(mode);
    reply += "\n";
    server.send(200, "text/plain", reply);

    if (mode == 'q') {
      delay(250);
    }
    applyMeasurementMode(mode);
  });

  server.on(
      "/update", HTTP_POST,
      []() {
        bool ok = !Update.hasError();
        server.send(ok ? 200 : 500, "text/plain",
                    ok ? "Update complete. Rebooting.\n" : "Update failed.\n");
        delay(500);
        if (ok) {
          ESP.restart();
        }
      },
      []() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.printf("OTA upload start: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) {
            Serial.printf("OTA upload done: %u bytes\n", upload.totalSize);
          } else {
            Update.printError(Serial);
          }
        }
      });

  otaRoutesConfigured = true;
}

void startOtaAp() {
  if (otaActive) {
    Serial.println("OTA web server already active");
    return;
  }

  String ssid = "resonance-bench-" + shortId;
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(ssid.c_str());
  if (!ok) {
    Serial.println("Failed to start OTA AP");
    return;
  }

  configureOtaRoutes();
  server.begin();
  otaActive = true;
  otaMode = "ap";
  Serial.println();
  Serial.println("OTA maintenance AP started");
  Serial.printf("  ssid: %s\n", ssid.c_str());
  Serial.println("  url:  http://192.168.4.1/");
}

bool startWifiOta() {
#if RES_HAS_WIFI_SECRETS
  if (otaActive) {
    Serial.println("OTA web server already active");
    return true;
  }

  WiFi.mode(WIFI_STA);
#if RES_WIFI_LOWPOWER
  // Battery-friendly: modem sleep (radio naps between DTIM beacons) + reduced TX
  // power to flatten the WiFi-TX current bursts that can brown out VSYS on
  // battery. Adds a little HTTP latency; fine for telemetry polling.
  WiFi.setSleep(true);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  Serial.println("WiFi low-power mode: modem sleep on, TX power 8.5 dBm");
#else
  WiFi.setSleep(false);
#endif
  Serial.println();
  Serial.printf("Connecting to WiFi SSID: %s\n", RES_WIFI_SSID);
  WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);

  const uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 20000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return false;
  }

  configureOtaRoutes();
  server.begin();
  otaActive = true;
  otaMode = "wifi";
  Serial.println("WiFi web server started (OTA + /telemetry)");
  Serial.print("  ip:  ");
  Serial.println(WiFi.localIP());
  Serial.print("  url: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  return true;
#else
  Serial.println("No wifi_secrets.h compiled in; cannot start station OTA");
  return false;
#endif
}

void stopOtaAndWifi() {
  if (otaActive) {
    server.stop();
  }
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  otaActive = false;
  otaMode = "off";
  Serial.println("OTA server stopped and WiFi turned off");
}

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  h/?  help");
  Serial.println("  r    print report");
  Serial.println("  t    print telemetry JSON");
  Serial.println("  i    I2C scan (default Wire)");
  Serial.println("  c/0  clear LEDs, keep current WiFi/OTA state");
  Serial.println("  q    quiet baseline: stop OTA/WiFi and clear LEDs");
  Serial.println("  1-5  LED measurement modes");
  Serial.println("  w    connect to configured WiFi and start web server");
  Serial.println("  o    start temporary AP web server");
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n' || c == ' ') {
      continue;
    }
    switch (c) {
    case 'h':
    case '?':
      printHelp();
      break;
    case 'r':
      printReport();
      break;
    case 't':
      printTelemetry();
      break;
    case 'i':
      runI2cScan();
      break;
    case 'c':
    case '0':
      applyMeasurementMode('0');
      break;
    case 'q':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
      applyMeasurementMode(c);
      break;
    case 'w':
      startWifiOta();
      break;
    case 'o':
      startOtaAp();
      break;
    default:
      Serial.printf("Unknown command: %c\n", c);
      printHelp();
      break;
    }
  }
}

#if RES_BATT_STRESS
// Heartbeat indicator for the WiFi-off battery-stress test: LED-panel center
// pixel (primary, as requested) plus the onboard user LED (GPIO46) as backup.
void heartbeatLed(bool on) {
#if RES_HAS_IS31
  if (is31Ready) {
#if RES_BATT_STRESS_FULL
    setIs31Drive(0xFF, 0xFF); // max scaling + global current
    matrix.fill(on ? matrix.color565(255, 255, 255) : 0); // whole 13x9 grid
    matrix.show();
#else
    matrix.fill(0);
    if (on) {
      matrix.drawPixel(6, 4, matrix.color565(255, 255, 255));
    }
    matrix.show();
#endif
  }
#endif
#if RES_HAS_NEOPIXEL
  clearNeoPixelsFullScale();
#if RES_BATT_STRESS_FULL
  if (on) {
    for (uint16_t i = 0; i < RES_PIXEL_COUNT; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 255, 255));
    }
  }
#else
  if (on) {
    pixels.setPixelColor(RES_PIXEL_CENTER, pixels.Color(255, 255, 255));
  }
#endif
  pixels.show();
#endif
  digitalWrite(46, on ? HIGH : LOW); // PowerFeather onboard user LED
}
#endif

#if RES_LOADGEN
WiFiUDP loadUdp;
Preferences lgPrefs;
const uint32_t LG_PHASE_MS = 300000UL; // 5 minutes per phase
uint32_t lgStep = 0;     // persisted in NVS; phase = lgStep % 4
int lgPhase = 0;
uint32_t lgPhaseStart = 0;

void lgApplyLed() {
#if RES_HAS_IS31
  if (is31Ready) {
    bool ledOn = (lgPhase == 1 || lgPhase == 3);
    setIs31Drive(0xFF, 0xFF);
    matrix.fill(ledOn ? matrix.color565(255, 255, 255) : 0);
    matrix.show();
  }
#endif
}

// Enter the phase given by lgStep, and PRE-COMMIT lgStep+1 to NVS. Because the
// resets are full power-loss (poweron) -- which wipes RTC RAM -- the next phase
// is persisted in flash, so a phase that crashes the board is not retried on the
// next boot (the sweep advances past it instead of getting stuck).
void lgEnterPhase() {
  lgPhase = lgStep % 4;
  lgPrefs.putUInt("step", lgStep + 1);
  lgPhaseStart = millis();
  bool ledOn = (lgPhase == 1 || lgPhase == 3);
  bool heavy = (lgPhase == 2 || lgPhase == 3);
  Serial.printf("LOADGEN phase %d (step %lu): tx=%s led=%s\n", lgPhase,
                (unsigned long)lgStep, heavy ? "heavy" : "light",
                ledOn ? "on" : "off");
  lgApplyLed();
}

void loadgenSetup() {
  pinMode(46, OUTPUT);
  WiFi.mode(WIFI_STA);
#if RES_WIFI_LOWPOWER
  WiFi.setSleep(true);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
#else
  WiFi.setSleep(false);
#endif
#if RES_HAS_WIFI_SECRETS
  WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(200);
  }
#endif
  loadUdp.begin(54320);
  lgPrefs.begin("loadgen", false);
  lgStep = lgPrefs.getUInt("step", 0); // resumes/advances across power-loss reboots
  Serial.printf("LOADGEN auto-sweep: wifi=%s ip=%s\n",
                WiFi.status() == WL_CONNECTED ? "connected" : "FAILED",
                WiFi.localIP().toString().c_str());
  Serial.println("  5-min phases: 0=light/LED-off 1=light/LED-on 2=heavy/LED-off 3=heavy/LED-on");
  Serial.println("  (NVS-persisted: a phase that reboots the board is skipped, not retried)");
  lgEnterPhase();
}

void loadgenLoop() {
  static uint32_t lastBeat = 0, lastTx = 0, lastBv = 0;
  static bool on = false;
  static float bv = 0.0f;
  uint32_t now = millis();

  if (now - lgPhaseStart >= LG_PHASE_MS) { // advance on timer (phase survived)
    lgStep++;
    lgEnterPhase();
  }
  bool ledOn = (lgPhase == 1 || lgPhase == 3);
  bool heavy = (lgPhase == 2 || lgPhase == 3);
  int phase = lgPhase;

  if (now - lastBeat >= 250) { // onboard LED ~2 Hz liveness
    lastBeat = now;
    on = !on;
    digitalWrite(46, on);
  }

#if RES_HAS_PF_TELEMETRY
  if (now - lastBv >= 1000) { // cache battery voltage for the heartbeat payload
    lastBv = now;
    float v;
    if (Board.getBatteryVoltage(v) == Result::Ok) {
      bv = v;
    }
  }
#endif

  bool connected = (WiFi.status() == WL_CONNECTED);
  uint32_t interval = heavy ? 5 : 500; // heavy ~200/s, light ~2/s
  if (connected && now - lastTx >= interval) {
    lastTx = now;
    static char pkt[512];
    int n = snprintf(pkt, sizeof(pkt),
                     "pf-load ph=%d led=%d heavy=%d up=%lu bv=%.3f ", phase,
                     (int)ledOn, (int)heavy, (unsigned long)now, bv);
    int len = n;
    if (heavy) { // pad to 512 B for heavier sustained TX
      for (int i = n; i < (int)sizeof(pkt); i++) {
        pkt[i] = 'x';
      }
      len = sizeof(pkt);
    }
    loadUdp.beginPacket("255.255.255.255", 54321);
    loadUdp.write((const uint8_t *)pkt, len);
    loadUdp.endPacket();
  }
}
#endif // RES_LOADGEN

void setup() {
  setupBoardPower();
  Serial.begin(115200);
  delay(1500);

  shortId = compactIdFromMac();

  Serial.println();
  Serial.println("Booting Resonance power-bench firmware");

#if RES_HAS_PF_TELEMETRY
  // Board.init sets EN high, 3V3 + VSQT enabled. Run before LED/I2C bring-up.
  setupPowerFeather();
#endif

  Wire.begin();
  setupNeoPixels();
  runI2cScan();
  setupIs31();
  printReport();
  applyMeasurementMode('0');
  printHelp();

#if RES_BATT_STRESS
  // Radio fully off — this is the whole point of the test.
  WiFi.mode(WIFI_OFF);
  pinMode(46, OUTPUT);
  Serial.println("BATTERY-STRESS MODE: WiFi off, blinking center pixel @1Hz");
  // boot signature: 3 fast flashes so a reboot is visually obvious
  for (int i = 0; i < 3; i++) {
    heartbeatLed(true);
    delay(120);
    heartbeatLed(false);
    delay(120);
  }
#elif RES_LOADGEN
  loadgenSetup();
#elif RES_WIFI_AUTO_CONNECT
  startWifiOta();
#endif
}

void loop() {
#if RES_LOADGEN
  loadgenLoop();
  return;
#endif
#if RES_BATT_STRESS
  // 1 Hz heartbeat on the LED-panel center pixel; no WiFi, no web server.
  static uint32_t hbLast = 0;
  static bool hbOn = false;
  uint32_t hbNow = millis();
  if (hbNow - hbLast >= 500) {
    hbLast = hbNow;
    hbOn = !hbOn;
    heartbeatLed(hbOn);
  }
  return;
#endif

  handleSerial();
  if (otaActive) {
    server.handleClient();
  }

  uint32_t now = millis();
  if (now - lastHeartbeatMs > 10000) {
    lastHeartbeatMs = now;
    Serial.printf("heartbeat ms=%lu heap=%u ota=%s\n",
                  static_cast<unsigned long>(now), ESP.getFreeHeap(),
                  otaActive ? "on" : "off");
  }
}
