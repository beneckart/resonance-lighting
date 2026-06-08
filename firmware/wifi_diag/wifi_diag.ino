// wifi_diag -- PowerFeather V2 (ESP32-S3) WiFi range / roaming diagnostic.
//
// Purpose (see docs/tests/SOLAR_TELEMETRY_RANGE_PLAN_2026-06-08.md, item (b)):
// confirm WHY the ESP32 falls off the house Eero mesh in the yard while 5/6 GHz
// devices stay happy. The ESP32-S3 is 2.4 GHz only and (hypothesis) clings to one
// Eero BSSID instead of roaming to a nearer node as RSSI collapses. This sketch
// makes that visible: it stays associated, streams RSSI/BSSID/channel/TX-power
// continuously, and periodically scans ALL visible 2.4 GHz APs -- flagging when a
// *stronger* same-SSID node was available but not chosen (the roaming smoking gun).
//
// It is a SERIAL/USB tool: you carry a tethered laptop on the range walk and read
// the stream over the native-USB serial monitor (115200). No web server, no OTA --
// flash over USB, then watch serial. Because it streams every WD_ASSOC_S seconds,
// opening the monitor late still catches the data (unlike a boot-only banner -- see
// POWERFEATHER_NOTES "Native USB-CDC").
//
// Build/flash:  ./build.sh --port /dev/ttyACM0     (USB flash, then open serial)
//               ./build.sh                          (compile only)
// Tuning flags (build.sh): --assoc-s <sec> --scan-s <sec> --tx-low
//
// Line formats (prefixed for easy grep/logging):
//   wd-assoc  t=<ms> up=<0|1> rssi=<dBm> bssid=<mac> ch=<n> ssid=<name> tx=<dBm>
//   wd-scan   t=<ms> n=<count>            (start of a scan block)
//   wd-ap     i=<idx> rssi=<dBm> bssid=<mac> ch=<n> enc=<n> ssid=<name> assoc=<0|1> best=<0|1>
//   wd-roam   t=<ms> better=<0|1> assoc_rssi=<dBm> best_rssi=<dBm> best_bssid=<mac> margin_db=<n>
//   wd-event  <drop|reconnect> t=<ms> [after_ms=<n>]

#include <WiFi.h>
#include "esp_wifi.h"

// ---- WiFi credentials (shared with power_bench / net_bench) -----------------
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define RES_HAS_WIFI_SECRETS 1
#else
#define RES_HAS_WIFI_SECRETS 0
#endif

#ifndef RES_WIFI_SSID
#define RES_WIFI_SSID "your-network"
#endif
#ifndef RES_WIFI_PASSWORD
#define RES_WIFI_PASSWORD "your-password"
#endif

// ---- Tunables (overridable via build.sh -D flags) ---------------------------
#ifndef WD_ASSOC_S
#define WD_ASSOC_S 2      // association status line cadence (seconds)
#endif
#ifndef WD_SCAN_S
#define WD_SCAN_S 15      // full 2.4 GHz scan cadence (seconds)
#endif
#ifndef WD_TX_LOW
#define WD_TX_LOW 0       // 0 = force MAX TX power (apples-to-apples); 1 = 8.5 dBm
#endif
// A same-SSID AP must beat the associated one by at least this margin before we
// call it a missed-roam (avoids flapping on noise near equal RSSI).
#ifndef WD_ROAM_MARGIN_DB
#define WD_ROAM_MARGIN_DB 8
#endif

static const uint32_t ASSOC_MS = (uint32_t)WD_ASSOC_S * 1000UL;
static const uint32_t SCAN_MS = (uint32_t)WD_SCAN_S * 1000UL;

static uint32_t lastAssocMs = 0;
static uint32_t lastScanMs = 0;
static bool wasConnected = false;
static uint32_t dropMs = 0;

// ---- helpers ----------------------------------------------------------------
static void macToStr(const uint8_t *m, char *out /*>=18*/) {
  snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4],
           m[5]);
}

// WiFi.getTxPower() / scan RSSI are in real dBm; TX power enum is 0.25 dBm units.
static float txPowerDbm() { return (float)WiFi.getTxPower() / 4.0f; }

static void applyTxPower() {
#if WD_TX_LOW
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
#else
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // max for the S3 PCB antenna
#endif
}

static void connectWifi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);          // diagnostic: no modem sleep, steady radio
  WiFi.setAutoReconnect(true);   // we WANT to see whether/how fast it recovers
  applyTxPower();
  Serial.printf("Connecting to WiFi SSID: %s\n", RES_WIFI_SSID);
  WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);
  const uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    applyTxPower(); // re-assert after association
    Serial.print("Connected. ip=");
    Serial.print(WiFi.localIP());
    Serial.printf(" tx=%.2f dBm\n", txPowerDbm());
  } else {
    Serial.println("Initial connect timed out -- will keep retrying "
                   "(auto-reconnect on); walk anyway, watch wd-event lines.");
  }
}

static void printAssoc() {
  const bool up = (WiFi.status() == WL_CONNECTED);
  char bssid[18] = "--";
  int ch = 0, rssi = 0;
  String ssid = "--";
  if (up) {
    macToStr(WiFi.BSSID(), bssid);
    ch = WiFi.channel();
    rssi = WiFi.RSSI();
    ssid = WiFi.SSID();
  }
  Serial.printf("wd-assoc t=%lu up=%d rssi=%d bssid=%s ch=%d ssid=%s tx=%.2f\n",
                (unsigned long)millis(), up ? 1 : 0, rssi, bssid, ch,
                ssid.c_str(), txPowerDbm());
}

// Full 2.4 GHz scan. Flags the AP we're associated to and the strongest same-SSID
// AP; if a same-SSID node beats our association by >= the margin, that's a missed
// roam -- the diagnostic we're after.
static void runScan() {
  const bool up = (WiFi.status() == WL_CONNECTED);
  uint8_t assocBssid[6] = {0};
  int assocRssi = -127;
  if (up) {
    memcpy(assocBssid, WiFi.BSSID(), 6);
    assocRssi = WiFi.RSSI();
  }
  const String wantSsid = up ? WiFi.SSID() : String(RES_WIFI_SSID);

  const int n = WiFi.scanNetworks(/*async*/ false, /*hidden*/ false);
  Serial.printf("wd-scan t=%lu n=%d\n", (unsigned long)millis(), n < 0 ? 0 : n);

  int bestSameSsidRssi = -127;
  uint8_t bestSameSsidBssid[6] = {0};
  bool haveBest = false;

  for (int i = 0; i < n; i++) {
    uint8_t *bm = WiFi.BSSID(i);
    char bssid[18];
    macToStr(bm, bssid);
    const int rssi = WiFi.RSSI(i);
    const String ssid = WiFi.SSID(i);
    const bool isAssoc = up && (memcmp(bm, assocBssid, 6) == 0);
    const bool sameSsid = (ssid == wantSsid);
    if (sameSsid && rssi > bestSameSsidRssi) {
      bestSameSsidRssi = rssi;
      memcpy(bestSameSsidBssid, bm, 6);
      haveBest = true;
    }
    Serial.printf("wd-ap i=%d rssi=%d bssid=%s ch=%d enc=%d ssid=%s assoc=%d "
                  "best=0\n",
                  i, rssi, bssid, WiFi.channel(i), (int)WiFi.encryptionType(i),
                  ssid.c_str(), isAssoc ? 1 : 0);
  }

  if (up && haveBest) {
    const bool bestIsAssoc = (memcmp(bestSameSsidBssid, assocBssid, 6) == 0);
    const int margin = bestSameSsidRssi - assocRssi;
    const bool better = !bestIsAssoc && (margin >= WD_ROAM_MARGIN_DB);
    char bestStr[18];
    macToStr(bestSameSsidBssid, bestStr);
    Serial.printf("wd-roam t=%lu better=%d assoc_rssi=%d best_rssi=%d "
                  "best_bssid=%s margin_db=%d\n",
                  (unsigned long)millis(), better ? 1 : 0, assocRssi,
                  bestSameSsidRssi, bestStr, margin);
    if (better) {
      Serial.println("  ^ stronger same-SSID AP available but NOT roamed to "
                     "(missed-roam: the diagnostic).");
    }
  }
  WiFi.scanDelete();
}

static void trackEvents() {
  const bool up = (WiFi.status() == WL_CONNECTED);
  if (wasConnected && !up) {
    dropMs = millis();
    Serial.printf("wd-event drop t=%lu\n", (unsigned long)dropMs);
  } else if (!wasConnected && up) {
    const uint32_t now = millis();
    Serial.printf("wd-event reconnect t=%lu after_ms=%lu\n",
                  (unsigned long)now,
                  (unsigned long)(dropMs ? now - dropMs : 0));
    applyTxPower(); // re-assert after a reassociation
  }
  wasConnected = up;
}

void setup() {
  Serial.begin(115200);
  delay(1500); // let native USB-CDC enumerate
  Serial.println();
  Serial.println("=== wifi_diag (PowerFeather V2 / ESP32-S3) ===");
  Serial.printf("build: assoc=%ds scan=%ds tx=%s roam_margin=%ddB secrets=%d\n",
                WD_ASSOC_S, WD_SCAN_S, WD_TX_LOW ? "8.5dBm" : "MAX",
                WD_ROAM_MARGIN_DB, RES_HAS_WIFI_SECRETS);
  Serial.println("NOTE: ESP32-S3 is 2.4 GHz only -- scans show ONLY the 2.4 GHz "
                 "landscape (that's the point; 5/6 GHz is invisible to this radio).");
  Serial.println("Streams wd-assoc every interval; opening serial late still "
                 "catches data. Watch wd-roam/wd-event on the walk.");
  Serial.println();
  connectWifi();
  // Fire one of each immediately so the operator sees output without waiting.
  lastAssocMs = millis() - ASSOC_MS;
  lastScanMs = millis() - SCAN_MS;
}

void loop() {
  trackEvents();
  const uint32_t now = millis();
  if (now - lastAssocMs >= ASSOC_MS) {
    lastAssocMs = now;
    printAssoc();
  }
  if (now - lastScanMs >= SCAN_MS) {
    lastScanMs = now;
    runScan();
  }
  delay(50);
}
