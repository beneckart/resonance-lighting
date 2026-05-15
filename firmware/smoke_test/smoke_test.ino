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

#define SMOKE_VERSION "smoke-2026-05-15.2"

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
#define RES_BOARD_NAME "m5stack_atom"
#define RES_HAS_IS31 0
#define RES_HAS_NEOPIXEL 1
#define RES_PIXEL_PIN 27
#define RES_PIXEL_COUNT 25
#define RES_PIXEL_CENTER 12
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
bool is31Ready = false;
uint32_t lastHeartbeatMs = 0;
String shortId;

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
  pixels.setBrightness(8);
  pixels.clear();
  pixels.show();
  Serial.printf("NeoPixel matrix setup: pin=%d count=%d brightness=8/255\n",
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
  Serial.printf("ota_ap_active: %s\n", otaActive ? "yes" : "no");
  printOtaPartitionInfo();
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
  html += F("</p><form method='POST' action='/update' "
            "enctype='multipart/form-data'>");
  html += F("<input type='file' name='firmware'>");
  html += F("<input type='submit' value='Update'>");
  html += F("</form></body></html>");
  return html;
}

void startOtaAp() {
  if (otaActive) {
    Serial.println("OTA AP already active");
    return;
  }

  clearLeds();
  String ssid = "resonance-smoke-" + shortId;
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(ssid.c_str());
  if (!ok) {
    Serial.println("Failed to start OTA AP");
    return;
  }

  server.on("/", HTTP_GET, []() { server.send(200, "text/html", otaFormHtml()); });

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

  server.begin();
  otaActive = true;
  Serial.println();
  Serial.println("OTA maintenance AP started");
  Serial.printf("  ssid: %s\n", ssid.c_str());
  Serial.println("  url:  http://192.168.4.1/");
}

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  h/?  help");
  Serial.println("  r    print report");
  Serial.println("  i    I2C scan");
  Serial.println("  l    LED test");
  Serial.println("  c    clear LEDs");
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
      break;
    case 'c':
      clearLeds();
      Serial.println("LEDs cleared");
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
  runLedTest();
  printHelp();
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
