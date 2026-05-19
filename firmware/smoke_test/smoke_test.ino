#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
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

#define SMOKE_VERSION "smoke-2026-05-18.2"

#if defined(ARDUINO_ADAFRUIT_FEATHER_ESP32C6)
#define RES_BOARD_NAME "adafruit_feather_esp32c6"
#define RES_HAS_IS31 1
#define RES_HAS_NEOPIXEL 0
#elif defined(ARDUINO_FEATHERS2NEO)
#define RES_BOARD_NAME "um_feathers2neo"
#define RES_HAS_IS31 0
#define RES_HAS_NEOPIXEL 1
#define RES_PIXEL_PIN NEOPIXEL_MATRIX_DATA
#define RES_PIXEL_POWER_PIN NEOPIXEL_MATRIX_PWR
#define RES_PIXEL_COUNT 25
#define RES_PIXEL_CENTER 12
#elif defined(ARDUINO_M5STACK_ATOM)
#if defined(RES_ATOM_GROVE_NEOHEX)
#define RES_BOARD_NAME "m5stack_atom_neohex"
#define RES_HAS_IS31 0
#define RES_HAS_NEOPIXEL 1
#define RES_PIXEL_PIN 26
#define RES_PIXEL_COUNT 37
#define RES_PIXEL_CENTER 18
#define RES_PIXEL_LAYOUT_HEX37 1
#else
#define RES_BOARD_NAME "m5stack_atom"
#define RES_HAS_IS31 0
#define RES_HAS_NEOPIXEL 1
#define RES_PIXEL_PIN 27
#define RES_PIXEL_COUNT 25
#define RES_PIXEL_CENTER 12
#endif
#else
#error "Unsupported board. Use adafruit_feather_esp32c6, um_feathers2neo, or m5stack_atom."
#endif

#ifndef RES_HAS_IS31
#define RES_HAS_IS31 0
#endif

#ifndef RES_HAS_NEOPIXEL
#define RES_HAS_NEOPIXEL 0
#endif

#if RES_HAS_IS31
Adafruit_IS31FL3741_QT_buffered matrix;
#endif

#if RES_HAS_NEOPIXEL
Adafruit_NeoPixel pixels(RES_PIXEL_COUNT, RES_PIXEL_PIN, NEO_GRB + NEO_KHZ800);
#endif

WebServer server(80);
bool otaActive = false;
bool otaRoutesConfigured = false;
bool is31Ready = false;
uint32_t lastHeartbeatMs = 0;
String shortId;
String otaMode = "off";
char activeMeasurementMode = '0';

#if RES_HAS_NEOPIXEL
#if defined(RES_PIXEL_LAYOUT_HEX37)
// NeoHEX appears column-indexed by hex columns of 4,5,6,7,6,5,4 LEDs.
// These are the center pixel plus its first ring, not a contiguous index run.
const uint8_t neoCropPixels[] = {11, 12, 17, 18, 19, 24, 25};
#else
const uint8_t neoCropPixels[] = {6, 7, 8, 11, 12, 13, 16, 17, 18};
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
    return "center_dim_warm_white";
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
    setIs31Drive(0x28, 0x0C);
    if (is31Ready) {
      matrix.fill(0);
      matrix.drawPixel(6, 4, matrix.color565(32, 28, 24));
      matrix.show();
    }
#endif
#if RES_HAS_NEOPIXEL
    clearNeoPixelsFullScale();
    pixels.setPixelColor(RES_PIXEL_CENTER, pixels.Color(16, 14, 10));
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
    pixels.setPixelColor(RES_PIXEL_CENTER - 1, pixels.Color(20, 0, 0));
    pixels.setPixelColor(RES_PIXEL_CENTER, pixels.Color(0, 18, 0));
    pixels.setPixelColor(RES_PIXEL_CENTER + 1, pixels.Color(0, 0, 20));
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

void runI2cScan() {
  Serial.println();
  Serial.println("I2C scan:");
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
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

void setupIs31() {
#if RES_HAS_IS31
  Serial.println("IS31FL3741 setup:");
  if (!matrix.begin(IS3741_ADDR_DEFAULT, &Wire)) {
    Serial.println("  not found at 0x30");
    is31Ready = false;
    return;
  }

  Wire.setClock(400000);
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
  Serial.printf("NeoPixel matrix setup: pin=%d count=%d brightness=255/255\n",
                RES_PIXEL_PIN, RES_PIXEL_COUNT);
#endif
}

void runLedTest() {
  Serial.println();
  Serial.println("LED test: conservative center / 3-pixel pattern");
#if RES_HAS_IS31
  if (!is31Ready) {
    Serial.println("  IS31FL3741 not ready");
  } else {
    matrix.fill(0);
    matrix.drawPixel(matrix.width() / 2, matrix.height() / 2,
                     matrix.color565(32, 28, 20));
    matrix.show();
    delay(900);

    matrix.fill(0);
    matrix.drawPixel((matrix.width() / 2) - 1, matrix.height() / 2,
                     matrix.color565(32, 0, 0));
    matrix.drawPixel(matrix.width() / 2, matrix.height() / 2,
                     matrix.color565(0, 28, 0));
    matrix.drawPixel((matrix.width() / 2) + 1, matrix.height() / 2,
                     matrix.color565(0, 0, 32));
    matrix.show();
    delay(900);

    matrix.fill(0);
    matrix.drawPixel(matrix.width() / 2, matrix.height() / 2,
                     matrix.color565(10, 8, 6));
    matrix.show();
    Serial.println("  IS31FL3741 visible center pixel left on dimly");
  }
#endif

#if RES_HAS_NEOPIXEL
  pixels.clear();
  pixels.setPixelColor(RES_PIXEL_CENTER, pixels.Color(32, 28, 20));
  pixels.show();
  delay(900);

  pixels.clear();
  pixels.setPixelColor(RES_PIXEL_CENTER - 1, pixels.Color(32, 0, 0));
  pixels.setPixelColor(RES_PIXEL_CENTER, pixels.Color(0, 28, 0));
  pixels.setPixelColor(RES_PIXEL_CENTER + 1, pixels.Color(0, 0, 32));
  pixels.show();
  delay(900);

  pixels.clear();
  pixels.setPixelColor(RES_PIXEL_CENTER, pixels.Color(10, 8, 6));
  pixels.show();
  Serial.println("  built-in matrix visible center pixel left on dimly");
#endif
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
  Serial.println("=== Resonance COTS smoke test ===");
  Serial.printf("version: %s\n", SMOKE_VERSION);
  Serial.printf("board: %s\n", RES_BOARD_NAME);
  Serial.printf("chip: %s rev %u, cores=%u\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getChipCores());
  Serial.printf("flash: %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("mac: %s\n", macString().c_str());
  Serial.printf("fixture_id: %s\n", shortId.c_str());
  Serial.printf("reset_reason: %s (%d)\n",
                resetReasonName(esp_reset_reason()),
                static_cast<int>(esp_reset_reason()));
  Serial.printf("heap_free: %u\n", ESP.getFreeHeap());
  Serial.printf("i2c_sda: %d\n", SDA);
  Serial.printf("i2c_scl: %d\n", SCL);
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
  html += F("<title>Resonance Smoke OTA</title></head><body>");
  html += F("<h1>Resonance Smoke OTA</h1>");
  html += F("<p>Board: ");
  html += RES_BOARD_NAME;
  html += F("<br>Fixture: ");
  html += shortId;
  html += F("<br>Version: ");
  html += SMOKE_VERSION;
  html += F("<br>Mode: ");
  html += activeMeasurementMode;
  html += F(" ");
  html += measurementModeName(activeMeasurementMode);
  html += F("</p><p>LED measurement modes: ");
  html += F("<a href='/mode?m=0'>0 off</a> ");
  html += F("<a href='/mode?m=1'>1 center</a> ");
  html += F("<a href='/mode?m=2'>2 RGB</a> ");
  html += F("<a href='/mode?m=3'>3 3x3</a> ");
  html += F("<a href='/mode?m=4'>4 full low</a> ");
  html += F("<a href='/mode?m=5'>5 capped brief</a> ");
  html += F("<a href='/mode?m=q'>q quiet</a></p>");
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

  clearLeds();
  activeMeasurementMode = '0';
  String ssid = "resonance-smoke-" + shortId;
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

  clearLeds();
  activeMeasurementMode = '0';
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
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
  Serial.println("WiFi OTA web updater started");
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
  Serial.println("  i    I2C scan");
  Serial.println("  l    LED test");
  Serial.println("  c/0  clear LEDs, keep current WiFi/OTA state");
  Serial.println("  q    quiet baseline: stop OTA/WiFi and clear LEDs");
  Serial.println("  1    center dim warm white");
  Serial.println("  2    3-pixel RGB fringe");
  Serial.println("  3    center 3x3 dim warm white");
  Serial.println("  4    full array very low white");
  Serial.println("  5    full array capped white, brief only");
  Serial.println("  w    connect to configured WiFi and start web OTA updater");
  Serial.println("  o    start temporary AP web OTA updater");
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
    case 'i':
      runI2cScan();
      break;
    case 'l':
      runLedTest();
      applyMeasurementMode('1');
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

void setup() {
  setupBoardPower();
  Serial.begin(115200);
  delay(1500);

  shortId = compactIdFromMac();
  Wire.begin();

  Serial.println();
  Serial.println("Booting Resonance smoke firmware");
  setupNeoPixels();
  runI2cScan();
  setupIs31();
  printReport();
  applyMeasurementMode('1');
  printHelp();

#if RES_WIFI_AUTO_CONNECT
  startWifiOta();
#endif
}

void loop() {
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
