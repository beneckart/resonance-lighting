#include "maintenance.h"

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Update.h>
#include "esp_task_wdt.h"

#include "board_power.h"
#include "espnow_link.h"
#include "identity.h"
#include "loads.h"
#include "motion_trace.h"
#include "sensors/sensor_bus.h"
#include "telemetry.h"
#include "../core/wifi_credential_plan.h"

// WiFi secrets (gitignored; copied from ../net_bench by build.sh).
#if __has_include("../../wifi_secrets.h")
#include "../../wifi_secrets.h"
#define RES_HAS_WIFI_SECRETS 1
#else
#define RES_HAS_WIFI_SECRETS 0
#endif

#if RES_HAS_WIFI_SECRETS
#if !defined(RES_WIFI_SSID) || !defined(RES_WIFI_PASSWORD)
#error "wifi_secrets.h must define RES_WIFI_SSID and RES_WIFI_PASSWORD"
#endif
#if defined(RES_WIFI_SSID_2) != defined(RES_WIFI_PASSWORD_2)
#error "define both RES_WIFI_SSID_2 and RES_WIFI_PASSWORD_2, or neither"
#endif

struct MaintenanceWifiCredential {
  const char *ssid;
  const char *password;
};

static const MaintenanceWifiCredential kMaintenanceWifiCredentials[] = {
    {RES_WIFI_SSID, RES_WIFI_PASSWORD},
#if defined(RES_WIFI_SSID_2)
    {RES_WIFI_SSID_2, RES_WIFI_PASSWORD_2},
#endif
};
static constexpr size_t kMaintenanceWifiCredentialCount =
    sizeof(kMaintenanceWifiCredentials) / sizeof(kMaintenanceWifiCredentials[0]);
static_assert(kMaintenanceWifiCredentialCount <= 2,
              "maintenance credential scratch arrays support at most two networks");
#endif

static WebServer gServer(80);
static NetMode gMode = MODE_COMMS;
static uint8_t gMaintStatus = MAINT_STATUS_IDLE;
static bool gOtaActive = false;
static bool gOtaRoutesConfigured = false;
static volatile bool gResumePending = false;
static uint32_t gMaintEnteredMs = 0;
static uint32_t gLastMaintRefuseMs = 0;
static uint32_t gNextCommsRetryMs = 0;
static uint32_t gCommsInitAttempts = 0;
static uint32_t gCommsInitFailures = 0;

#define RES_COMMS_RETRY_MS 1000

static bool parseRtcUtc(const String &text, uint32_t &utcS) {
  if (text.length() != 10) return false;
  uint64_t value = 0;
  for (size_t i = 0; i < text.length(); ++i) {
    char ch = text[i];
    if (ch < '0' || ch > '9') return false;
    value = value * 10U + (uint8_t)(ch - '0');
  }
  if (value < 1735689600ULL || value >= 2082758400ULL) return false;
  utcS = (uint32_t)value;
  return true;
}

static void handleRtcCommission() {
  if (!gServer.hasArg("fixture_id") || !gServer.hasArg("utc_s") ||
      !gServer.hasArg("confirm")) {
    gServer.send(400, "application/json", "{\"ok\":false,\"error\":\"missing fields\"}\n");
    return;
  }
  if (!gServer.arg("fixture_id").equalsIgnoreCase(gShortId)) {
    gServer.send(409, "application/json", "{\"ok\":false,\"error\":\"fixture identity mismatch\"}\n");
    return;
  }
  if (gServer.arg("confirm") != "SET_RTC_UTC") {
    gServer.send(400, "application/json", "{\"ok\":false,\"error\":\"confirmation refused\"}\n");
    return;
  }
  uint32_t requestedUtcS = 0;
  if (!parseRtcUtc(gServer.arg("utc_s"), requestedUtcS)) {
    gServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid UTC\"}\n");
    return;
  }
  if (!sensorBusWriteRtcUtc(requestedUtcS)) {
    gServer.send(500, "application/json", "{\"ok\":false,\"error\":\"RTC write failed\"}\n");
    return;
  }
  uint32_t readbackUtcS = 0;
  if (!sensorBusReadRtcUtc(readbackUtcS) ||
      (readbackUtcS > requestedUtcS ? readbackUtcS - requestedUtcS
                                   : requestedUtcS - readbackUtcS) > 2U) {
    gServer.send(500, "application/json", "{\"ok\":false,\"error\":\"RTC readback failed\"}\n");
    return;
  }
  String body = "{\"ok\":true,\"fixture_id\":\"" + gShortId +
                "\",\"rtc_utc_s\":" + String((unsigned long)readbackUtcS) +
                "}\n";
  gServer.send(200, "application/json", body);
}

NetMode maintMode() { return gMode; }
uint8_t maintStatus() { return gMaintStatus; }
bool maintenanceReady() {
  return gMode == MODE_MAINT && gMaintStatus == MAINT_STATUS_ACTIVE &&
         gOtaActive && WiFi.status() == WL_CONNECTED;
}
uint32_t commsInitAttempts() { return gCommsInitAttempts; }
uint32_t commsInitFailures() { return gCommsInitFailures; }

static void configureOtaRoutes() {
  if (gOtaRoutesConfigured) return;
  gServer.on("/", HTTP_GET, []() { gServer.send(200, "text/plain", telemetryBanner()); });
  gServer.on("/telemetry", HTTP_GET, []() { gServer.send(200, "application/json", telemetryJson()); });
#if defined(RES_MSA_TRACE_TARGET)
  gServer.on("/motion-trace", HTTP_GET,
             []() { motionTraceHandleHttp(gServer); });
#endif
  gServer.on("/rtc", HTTP_POST, handleRtcCommission);
  gServer.on("/resume", HTTP_GET, []() {
    gServer.send(200, "text/plain", "resuming\n");
    gResumePending = true;
  });
  gServer.on(
      "/update", HTTP_POST,
      []() {
        esp_task_wdt_reset();
        bool ok = !Update.hasError();
        gServer.send(ok ? 200 : 500, "text/plain",
                     ok ? "Update complete. Rebooting.\n" : "Update failed.\n");
        delay(500);
        if (ok) ESP.restart();
      },
      []() {
        esp_task_wdt_reset();
        HTTPUpload &up = gServer.upload();
        if (up.status == UPLOAD_FILE_START) {
          allLoadsOff("OTA");
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
        } else if (up.status == UPLOAD_FILE_WRITE) {
          if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
        } else if (up.status == UPLOAD_FILE_END) {
          if (Update.end(true)) Serial.printf("OTA done: %u bytes\n", up.totalSize);
          else Update.printError(Serial);
        }
      });
  gOtaRoutesConfigured = true;
}

static bool startWifiOta() {
#if RES_HAS_WIFI_SECRETS
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  int16_t bestRssi[2] = {WIFI_CREDENTIAL_UNSEEN_RSSI,
                         WIFI_CREDENTIAL_UNSEEN_RSSI};
  esp_task_wdt_reset();
  int networkCount = WiFi.scanNetworks(false, true);
  esp_task_wdt_reset();
  if (networkCount >= 0) {
    for (int i = 0; i < networkCount; ++i) {
      String seenSsid = WiFi.SSID(i);
      int32_t seenRssi = WiFi.RSSI(i);
      if (seenRssi < -32767) seenRssi = -32767;
      if (seenRssi > 32767) seenRssi = 32767;
      for (size_t credential = 0;
           credential < kMaintenanceWifiCredentialCount; ++credential) {
        if (seenSsid == kMaintenanceWifiCredentials[credential].ssid &&
            seenRssi > bestRssi[credential]) {
          bestRssi[credential] = (int16_t)seenRssi;
        }
      }
    }
    WiFi.scanDelete();
  } else {
    Serial.printf("WiFi scan failed: %d; using configured order\n", networkCount);
  }

  uint8_t order[2] = {0, 1};
  wifiCredentialOrder(bestRssi, kMaintenanceWifiCredentialCount, order);
  const uint32_t totalJoinBudgetMs =
      kMaintenanceWifiCredentialCount > 1 ? 30000UL : 20000UL;
  uint32_t joinStartedMs = millis();
  uint8_t connectedCredential = 0xFF;
  for (size_t position = 0;
       position < kMaintenanceWifiCredentialCount && WiFi.status() != WL_CONNECTED;
       ++position) {
    uint32_t elapsedMs = millis() - joinStartedMs;
    uint32_t remainingMs = elapsedMs < totalJoinBudgetMs
                               ? totalJoinBudgetMs - elapsedMs
                               : 0;
    size_t attemptsLeft = kMaintenanceWifiCredentialCount - position;
    uint32_t attemptBudgetMs =
        attemptsLeft ? remainingMs / (uint32_t)attemptsLeft : 0;
    if (attemptBudgetMs == 0) break;

    uint8_t credential = order[position];
    WiFi.disconnect(false, false);
    delay(100);
    Serial.printf("WiFi join profile=%u/%u scan_rssi=%d timeout=%lu ms\n",
                  (unsigned)(credential + 1),
                  (unsigned)kMaintenanceWifiCredentialCount,
                  (int)bestRssi[credential], (unsigned long)attemptBudgetMs);
    WiFi.begin(kMaintenanceWifiCredentials[credential].ssid,
               kMaintenanceWifiCredentials[credential].password);
    uint32_t attemptStartedMs = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - attemptStartedMs < attemptBudgetMs) {
      esp_task_wdt_reset();
      delay(250);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      connectedCredential = credential;
    } else {
      Serial.printf("WiFi join profile=%u failed\n", (unsigned)(credential + 1));
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(false, false);
    Serial.println("all configured WiFi joins failed");
    return false;
  }
  Serial.printf("maintenance WiFi profile=%u/%u\n",
                (unsigned)(connectedCredential + 1),
                (unsigned)kMaintenanceWifiCredentialCount);
  configureOtaRoutes();
  gServer.begin();
  gOtaActive = true;
  // Byte-exact banner: fleet_usb_bringup.py regexes "maintenance WiFi up,\s*ip=".
  Serial.print("maintenance WiFi up, ip=");
  Serial.print(WiFi.localIP());
  Serial.printf(" ch=%d\n", WiFi.channel());
  return true;
#else
  Serial.println("no wifi_secrets.h -> cannot OTA");
  return false;
#endif
}

static void stopOtaAndWifi() {
  if (gOtaActive) gServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  gOtaActive = false;
}

static uint16_t maintenanceBatteryFloorMv() {
  // Chemistry is build-time LFP in production; keep the 3V7 branch for the odd
  // bench build so the floor stays honest.
  return (strcmp(batteryTypeName(), "Generic_LFP") == 0) ? RES_MAINT_MIN_LFP_MV
                                                         : RES_MAINT_MIN_3V7_MV;
}

bool maintenancePowerOk() {
  if (!pfIsReady()) return true;

  readBatteryNow();
  esp_task_wdt_reset();
  int battMv = (int)(batteryVolts() * 1000.0f + 0.5f);
  uint16_t floorMv = maintenanceBatteryFloorMv();
  bool batteryKnown = battMv > 500;
  bool batteryOk = !batteryKnown || battMv >= (int)floorMv;
  bool supplyOk = supplyGood() && supplyMa() >= (float)RES_MAINT_SUPPLY_OVERRIDE_MA;
  if (batteryOk || supplyOk) return true;

  gMaintStatus = MAINT_STATUS_POWER_WARN;
  uint32_t now = millis();
  if (!gLastMaintRefuseMs || now - gLastMaintRefuseMs >= RES_MAINT_REFUSE_REPORT_MS) {
    gLastMaintRefuseMs = now;
    Serial.printf("maintenance power warning: bv=%dmV floor=%umV sv=%.3fV sma=%.0fmA sgood=%d enforce=%d\n",
                  battMv, (unsigned)floorMv, supplyVolts(), supplyMa(),
                  supplyGood() ? 1 : 0, RES_MAINT_POWER_ENFORCE);
  }
#if RES_MAINT_POWER_ENFORCE
  return false;
#else
  return true;
#endif
}

bool enterMaintenance() {
  if (gMode == MODE_MAINT) return true;
  if (!maintenancePowerOk()) return false;
  Serial.println("-> MAINTENANCE (WiFi OTA)");
  allLoadsOff("maintenance");
  espNowDeinit();
  gMode = MODE_MAINT;
  gMaintEnteredMs = millis();
  if (!startWifiOta()) {
    gMaintStatus = MAINT_STATUS_START_FAILED;
    Serial.println("maintenance startup failed -> resume comms");
    enterComms();
    return false;
  }
  gMaintStatus = MAINT_STATUS_ACTIVE;
  return true;
}

static bool startEspNowComms() {
  ++gCommsInitAttempts;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect(); // STA up but unassociated -> sit on the channel, pure ESP-NOW
  if (espNowInit()) {
    gNextCommsRetryMs = 0;
    return true;
  }
  ++gCommsInitFailures;
  gNextCommsRetryMs = millis() + RES_COMMS_RETRY_MS;
  Serial.printf("ESP-NOW comms unavailable; retry %lu in %u ms\n",
                (unsigned long)(gCommsInitFailures + 1),
                (unsigned)RES_COMMS_RETRY_MS);
  return false;
}

void enterComms() {
  Serial.println("-> COMMS (ESP-NOW)");
  if (gOtaActive) stopOtaAndWifi();
  gMode = MODE_COMMS;
  startEspNowComms();
}

void commsRecoveryTick() {
  if (gMode != MODE_COMMS || espNowUp()) return;
  uint32_t now = millis();
  if (gNextCommsRetryMs && (int32_t)(now - gNextCommsRetryMs) < 0) return;
  Serial.println("retrying ESP-NOW comms");
  startEspNowComms();
}

void maintenanceTick() {
  if (gMode != MODE_MAINT) return;
  gServer.handleClient();
  // /resume HTTP hit -> a real comms transition (re-init ESP-NOW), not a flag flip
  if (gResumePending) {
    gResumePending = false;
    gMaintStatus = MAINT_STATUS_RESUMED;
    Serial.println("/resume -> comms");
    enterComms();
    return;
  }
  // auto-resume if no OTA happens within the window (prevents stranding)
  if (millis() - gMaintEnteredMs > (uint32_t)RES_MAINT_TIMEOUT_S * 1000) {
    Serial.println("maintenance timeout -> resume comms");
    gMaintStatus = MAINT_STATUS_TIMEOUT;
    enterComms();
  }
}
