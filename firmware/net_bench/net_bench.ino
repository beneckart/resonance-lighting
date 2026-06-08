// Resonance net-bench -- ESP-NOW networking feasibility bench for PowerFeather V2
// (ESP32-S3). Throwaway-friendly sketch to de-risk basing ~100 fixtures on this
// board: validates ESP-NOW broadcast comms (master-multicast + peer mesh), per-link
// packet-delivery-ratio / RSSI, a maintenance-mode WiFi OTA cycle, power telemetry,
// a watchdog, and battery stability. See docs/tests/NETWORKING_FEASIBILITY_5NODE_*.
//
// Forked from firmware/power_bench (reuses telemetry, OTA /update, autosleep guard).
//
// ROLES (build flag): master broadcasts SHOW_FRAME commands + WiFi-STA-joins the
// bench AP and bridges per-peer stats to the host over UDP:54321; peers run pure
// ESP-NOW on battery and broadcast HEARTBEAT (id, seq, battery, dl-PDR/RSSI).
//
// HARD ADR CONSTRAINTS: OTA is STANDARD WiFi only (ADR 0010) -- ESP-NOW carries only
// small control/state/metadata, never firmware bytes. USB/pogo stays recovery path.
//
// Build/flash (see build.sh):
//   ./build.sh --role master --channel 6 --port /dev/ttyACM0
//   ./build.sh --role peer   --channel 6 --port /dev/ttyACM1
// The bench AP MUST be on the same fixed channel as --channel (see README).

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <WiFiUdp.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_sleep.h"
#include "esp_task_wdt.h"
#include <Preferences.h>

#define NET_BENCH_VERSION "net-bench-2026-06-07.3"
#define RES_BOARD_NAME "powerfeather_v2"
#define NB_LED_PIN 46 // PowerFeather onboard user LED (battery-level indicator)

// ---- WiFi secrets (gitignored; copied from ../power_bench by build.sh) ------
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define RES_HAS_WIFI_SECRETS 1
#else
#define RES_HAS_WIFI_SECRETS 0
#endif

// ---- PowerFeather SDK (telemetry) ------------------------------------------
#include <PowerFeather.h>
using namespace PowerFeather;
#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 so the SDK uses the V2 MAX17260 fuel gauge. See firmware/net_bench/build.sh."
#endif
#ifndef RES_PF_BATTERY_CAPACITY_MAH
#define RES_PF_BATTERY_CAPACITY_MAH 2000
#endif
#ifndef RES_PF_BATTERY_TYPE
#define RES_PF_BATTERY_TYPE Mainboard::BatteryType::Generic_3V7
#endif
#ifndef RES_PF_ENABLE_CHARGING
#define RES_PF_ENABLE_CHARGING 1
#endif
#ifndef RES_PF_MAX_CHARGE_MA
#define RES_PF_MAX_CHARGE_MA 1000.0f
#endif
#ifndef RES_PF_MAINTAIN_V
#define RES_PF_MAINTAIN_V 4.6f
#endif

// ---- net-bench config (override with -D at build time) ---------------------
#if !defined(NB_ROLE_MASTER) && !defined(NB_ROLE_PEER)
#define NB_ROLE_PEER 1
#endif
#ifndef NB_CHANNEL
#define NB_CHANNEL 6 // locked WiFi/ESP-NOW channel; MUST equal the bench AP channel
#endif
#ifndef NB_FRAME_HZ
#define NB_FRAME_HZ 10 // master SHOW_FRAME rate (0 = master is a pure bridge/mesh node)
#endif
#ifndef NB_HB_HZ
#define NB_HB_HZ 2 // peer HEARTBEAT rate
#endif
#ifndef NB_JITTER_PCT
#define NB_JITTER_PCT 30 // +/- jitter (% of period) on periodic sends (anti-collision)
#endif
#ifndef NB_WDT_S
#define NB_WDT_S 8 // task watchdog timeout (s)
#endif
#ifndef NB_MAINT_TIMEOUT_S
#define NB_MAINT_TIMEOUT_S 600 // peer auto-resume from maintenance if no OTA (s)
#endif
#ifndef NB_BRIDGE_HZ
#define NB_BRIDGE_HZ 1 // master -> host UDP stats rate
#endif
#ifndef NB_MAX_TRACKED
#define NB_MAX_TRACKED 16 // per-source tracking table size (5-board bench + headroom)
#endif
#ifndef NB_WIFI_LOWPOWER
#define NB_WIFI_LOWPOWER 0
#endif
// NB_START_MAINT, NB_WDT_HANGTEST, NB_AUTOSLEEP are presence-only flags.
#ifndef NB_BUDGET_MAH
#define NB_BUDGET_MAH 1000
#endif
#ifndef NB_WAKE_S
#define NB_WAKE_S 900
#endif
#define NB_LOOP_LIMIT 25   // NVS reboot-loop breaker threshold
#define NB_HEALTHY_MS 120000 // a boot is "healthy" after surviving this long

#if defined(NB_ROLE_MASTER)
static const bool IS_MASTER = true;
#else
static const bool IS_MASTER = false;
#endif

// ---- Globals ---------------------------------------------------------------
WebServer server(80);
WiFiUDP bridgeUdp;
bool pfReady = false;
String shortId;
uint8_t myMac[6] = {0};
uint8_t myId[3] = {0};
static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
String otaMode = "off";

enum NetMode { MODE_COMMS = 0, MODE_MAINT = 1 };
volatile NetMode gMode = MODE_COMMS;
volatile bool gResumePending = false; // /resume sets this; loop() does the real enterComms()
bool otaActive = false;
uint32_t maintEnteredMs = 0;

uint32_t txSeq = 0;        // our outgoing sequence number (monotonic)
uint32_t sendOk = 0, sendFail = 0; // ESP-NOW send-callback tallies
uint8_t gRateHz = IS_MASTER ? NB_FRAME_HZ : NB_HB_HZ; // runtime-settable (master '+'/'-')

// cached battery (refreshed ~1 Hz; ESP-NOW callback must not read the SDK)
float cbV = 0, cbMa = 0;
int cbSoc = -1;

// downlink (master SHOW_FRAME) tracking on peers
uint32_t masterLastSeq = 0;
uint32_t masterRx = 0, masterGaps = 0;
int8_t masterRssi = 0;
bool masterSeen = false;

// ---- ESP-NOW packet formats (packed, versioned, little-endian) -------------
#define NB_PROTO_VER 1
enum NbType : uint8_t {
  NB_HEARTBEAT = 1,   // peer -> all: state + telemetry summary
  NB_SHOWFRAME = 2,   // master -> all: show command
  NB_ENTER_MAINT = 3, // master -> all: ADR-0010 metadata, "enter maintenance"
  NB_RESUME = 4,      // master -> all: leave maintenance
  NB_SET_RATE = 5,    // master -> all: set heartbeat/frame rate (Hz) for sweeps
};

struct __attribute__((packed)) NbHeader {
  uint8_t ver;
  uint8_t type;
  uint8_t src_id[3];
  uint32_t seq;
  uint32_t uptime_ms;
};
struct __attribute__((packed)) NbShowFrame {
  NbHeader h;
  uint16_t phase;
  uint8_t hue;
  uint8_t flags;
};
struct __attribute__((packed)) NbHeartbeat {
  NbHeader h;
  int16_t batt_mv;
  int16_t batt_ma;
  uint8_t soc_pct;
  uint8_t reset_reason; // (uint8_t)esp_reset_reason()
  uint8_t ca_state;
  uint8_t mode;
  uint16_t dl_pdr_x1000; // master-multicast PDR as seen by this peer
  int8_t dl_rssi;        // RSSI of master frames at this peer
};
struct __attribute__((packed)) NbCmd { // ENTER_MAINT / RESUME / SET_RATE
  NbHeader h;
  uint8_t arg; // SET_RATE: Hz
};

// ---- per-source receive tracking (PDR) -------------------------------------
struct NbPeerStat {
  bool used;
  uint8_t id[3];
  uint8_t last_type;
  uint32_t last_seq;
  uint32_t recv;
  uint32_t gaps;
  int8_t rssi;
  uint32_t last_heard_ms;
  uint32_t up; // peer's reported uptime_ms (for host reboot detection)
  // last heartbeat snapshot (for the bridge)
  int16_t batt_mv, batt_ma;
  uint8_t soc, rr, ca, pmode;
  uint16_t dl_pdr_x1000;
  int8_t dl_rssi;
};
NbPeerStat peers[NB_MAX_TRACKED];

// ESP-NOW rx -> loop queue (callback enqueues only; loop processes)
struct RxItem {
  uint8_t mac[6];
  int8_t rssi;
  uint8_t len;
  uint8_t data[64];
};
QueueHandle_t rxQueue;

// ---- reused helpers (from power_bench) -------------------------------------
const char *resetReasonName(esp_reset_reason_t r) {
  switch (r) {
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
const char *batteryTypeName() {
  switch (RES_PF_BATTERY_TYPE) {
  case Mainboard::BatteryType::Generic_3V7: return "Generic_3V7";
  case Mainboard::BatteryType::Generic_LFP: return "Generic_LFP";
  default: return "other";
  }
}

void setupPowerFeather() {
  Serial.println("PowerFeather SDK init:");
  Result r = Result::Failure;
  for (int a = 1; a <= 4; a++) {
    r = Board.init((uint16_t)RES_PF_BATTERY_CAPACITY_MAH, RES_PF_BATTERY_TYPE);
    if (r == Result::Ok) break;
    Serial.printf("  Board.init attempt %d -> %d, retrying\n", a, (int)r);
    delay(250);
  }
  pfReady = (r == Result::Ok);
  Serial.printf("  Board.init(cap=%u, %s) -> %s\n", (unsigned)RES_PF_BATTERY_CAPACITY_MAH,
                batteryTypeName(), pfReady ? "Ok" : "ERR");
  if (!pfReady) return;
  Board.setSupplyMaintainVoltage((float)RES_PF_MAINTAIN_V);
#if RES_PF_ENABLE_CHARGING
  Board.setBatteryChargingMaxCurrent((float)RES_PF_MAX_CHARGE_MA);
  Board.enableBatteryCharging(true);
#else
  Board.enableBatteryCharging(false);
#endif
}

// Onboard LED battery-level indicator (uses cached SOC, refreshed ~1 Hz):
//   >50% solid on | 25-50% 1 Hz | 10-24% 2 Hz | <10% 4 Hz | no reading = off
void updateStatusLed() {
  static uint32_t last = 0;
  static bool on = false;
  int soc = cbSoc;
  if (soc < 0) { digitalWrite(NB_LED_PIN, LOW); return; } // unknown -> off
  if (soc > 50) { digitalWrite(NB_LED_PIN, HIGH); return; } // solid
  uint16_t interval = (soc >= 25) ? 500 : (soc >= 10) ? 250 : 125; // 1/2/4 Hz
  uint32_t now = millis();
  if (now - last >= interval) { last = now; on = !on; digitalWrite(NB_LED_PIN, on ? HIGH : LOW); }
}

void readBattery() {
  if (!pfReady) return;
  float v;
  if (Board.getBatteryVoltage(v) == Result::Ok) cbV = v;
  if (Board.getBatteryCurrent(v) == Result::Ok) cbMa = v;
  uint8_t s;
  if (Board.getBatteryCharge(s) == Result::Ok) cbSoc = s;
}

String telemetryJson() {
  String j = "{";
  j += "\"board\":\"" RES_BOARD_NAME "\"";
  j += ",\"fw\":\"" NET_BENCH_VERSION "\"";
  j += ",\"fixture_id\":\"" + shortId + "\"";
  j += ",\"role\":\"";
  j += IS_MASTER ? "master" : "peer";
  j += "\"";
  j += ",\"mode\":";
  j += (int)gMode;
  j += ",\"uptime_ms\":" + String((unsigned long)millis());
  j += ",\"heap_free\":" + String(ESP.getFreeHeap());
  j += ",\"reset_reason\":\"" + String(resetReasonName(esp_reset_reason())) + "\"";
  j += ",\"pf_ready\":";
  j += pfReady ? "true" : "false";
  j += ",\"battery_type\":\"" + String(batteryTypeName()) + "\"";
  if (pfReady) {
    char b[24];
    float v;
    if (Board.getBatteryVoltage(v) == Result::Ok) { snprintf(b, sizeof(b), "%.3f", v); j += ",\"battery_v\":" + String(b); }
    if (Board.getBatteryCurrent(v) == Result::Ok) { snprintf(b, sizeof(b), "%.1f", v); j += ",\"battery_ma\":" + String(b); }
    uint8_t s;
    if (Board.getBatteryCharge(s) == Result::Ok) j += ",\"soc_pct\":" + String(s);
    if (Board.getSupplyVoltage(v) == Result::Ok) { snprintf(b, sizeof(b), "%.3f", v); j += ",\"supply_v\":" + String(b); }
  }
  j += "}";
  return j;
}

// ---- OTA web routes (ADR-0010 standard WiFi OTA) ---------------------------
bool otaRoutesConfigured = false;
void configureOtaRoutes() {
  if (otaRoutesConfigured) return;
  server.on("/", HTTP_GET, []() { server.send(200, "text/plain", "net-bench " NET_BENCH_VERSION "\n"); });
  server.on("/telemetry", HTTP_GET, []() { server.send(200, "application/json", telemetryJson()); });
  server.on("/resume", HTTP_GET, []() { server.send(200, "text/plain", "resuming\n"); gResumePending = true; });
  server.on(
      "/update", HTTP_POST,
      []() {
        bool ok = !Update.hasError();
        server.send(ok ? 200 : 500, "text/plain", ok ? "Update complete. Rebooting.\n" : "Update failed.\n");
        delay(500);
        if (ok) ESP.restart();
      },
      []() {
        HTTPUpload &up = server.upload();
        if (up.status == UPLOAD_FILE_START) {
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
        } else if (up.status == UPLOAD_FILE_WRITE) {
          if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
        } else if (up.status == UPLOAD_FILE_END) {
          if (Update.end(true)) Serial.printf("OTA done: %u bytes\n", up.totalSize);
          else Update.printError(Serial);
        }
      });
  otaRoutesConfigured = true;
}

bool startWifiOta() {
#if RES_HAS_WIFI_SECRETS
  WiFi.mode(WIFI_STA);
#if NB_WIFI_LOWPOWER
  WiFi.setSleep(true);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
#else
  WiFi.setSleep(false);
#endif
  WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) { delay(250); Serial.print("."); }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) { Serial.println("WiFi join failed"); return false; }
  configureOtaRoutes();
  server.begin();
  otaActive = true;
  otaMode = "wifi";
  Serial.print("maintenance WiFi up, ip="); Serial.print(WiFi.localIP());
  Serial.printf(" ch=%d\n", WiFi.channel());
  return true;
#else
  Serial.println("no wifi_secrets.h -> cannot OTA");
  return false;
#endif
}

void stopOtaAndWifi() {
  if (otaActive) server.stop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  otaActive = false;
  otaMode = "off";
}

// ---- watchdog (net-new: closes the field-reliability TODO) -----------------
void setupWatchdog() {
  esp_task_wdt_config_t cfg = {};
  cfg.timeout_ms = (uint32_t)NB_WDT_S * 1000;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic = true;
  esp_err_t e = esp_task_wdt_init(&cfg);
  if (e == ESP_ERR_INVALID_STATE) esp_task_wdt_reconfigure(&cfg); // core already inited it
  esp_task_wdt_add(NULL); // subscribe the loop task
  Serial.printf("watchdog: %ds, panic+reset on hang\n", NB_WDT_S);
}

// ---- per-source table ------------------------------------------------------
NbPeerStat *findPeer(const uint8_t id[3], bool create) {
  for (int i = 0; i < NB_MAX_TRACKED; i++)
    if (peers[i].used && memcmp(peers[i].id, id, 3) == 0) return &peers[i];
  if (!create) return nullptr;
  for (int i = 0; i < NB_MAX_TRACKED; i++)
    if (!peers[i].used) {
      memset(&peers[i], 0, sizeof(NbPeerStat));
      peers[i].used = true;
      memcpy(peers[i].id, id, 3);
      return &peers[i];
    }
  return nullptr;
}

void accountSeq(NbPeerStat *p, uint32_t seq) {
  if (p->recv == 0) { p->last_seq = seq; p->recv = 1; return; }
  if (seq > p->last_seq) {
    p->gaps += (seq - p->last_seq - 1);
    p->recv += 1;
    p->last_seq = seq;
  } else {
    // out-of-order or sender reboot (seq reset): treat as fresh start
    if (seq < p->last_seq && p->last_seq - seq > 100) { p->last_seq = seq; p->recv = 1; p->gaps = 0; }
    else p->recv += 1;
  }
}

// ---- ESP-NOW callbacks -----------------------------------------------------
void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < (int)sizeof(NbHeader) || len > 64) return;
  RxItem it;
  memcpy(it.mac, info->src_addr, 6);
  it.rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  it.len = (uint8_t)len;
  memcpy(it.data, data, len);
  BaseType_t hpw = pdFALSE;
  xQueueSendFromISR(rxQueue, &it, &hpw); // recv cb is WiFi-task ctx; ISR-safe send is fine
}

void onEspNowSend(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) sendOk++; else sendFail++;
}

bool espNowInit() {
  esp_wifi_set_channel(NB_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) { Serial.println("esp_now_init FAILED"); return false; }
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BCAST, 6);
  peer.channel = NB_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false; // UNENCRYPTED broadcast = the 100-node-scalable pattern
  esp_now_add_peer(&peer);
  Serial.printf("esp-now up, ch=%d, broadcast peer registered\n", NB_CHANNEL);
  return true;
}
void espNowDeinit() {
  esp_now_unregister_recv_cb();
  esp_now_deinit();
}

// fill a header for an outgoing packet
void fillHeader(NbHeader *h, uint8_t type) {
  h->ver = NB_PROTO_VER;
  h->type = type;
  memcpy(h->src_id, myId, 3);
  h->seq = txSeq++;
  h->uptime_ms = millis();
}
void sendShowFrame() {
  NbShowFrame f;
  fillHeader(&f.h, NB_SHOWFRAME);
  f.phase = (uint16_t)((millis() / 50) & 0xFFFF);
  f.hue = (uint8_t)(f.phase & 0xFF);
  f.flags = 0;
  esp_now_send(BCAST, (uint8_t *)&f, sizeof(f));
}
void sendHeartbeat(uint8_t caState) {
  NbHeartbeat hb;
  fillHeader(&hb.h, NB_HEARTBEAT);
  hb.batt_mv = (int16_t)(cbV * 1000.0f);
  hb.batt_ma = (int16_t)cbMa;
  hb.soc_pct = (cbSoc < 0) ? 255 : (uint8_t)cbSoc;
  hb.reset_reason = (uint8_t)esp_reset_reason();
  hb.ca_state = caState;
  hb.mode = (uint8_t)gMode;
  uint32_t tot = masterRx + masterGaps;
  hb.dl_pdr_x1000 = tot ? (uint16_t)((uint64_t)masterRx * 1000 / tot) : 0;
  hb.dl_rssi = masterRssi;
  esp_now_send(BCAST, (uint8_t *)&hb, sizeof(hb));
}
void sendCmd(uint8_t type, uint8_t arg) {
  NbCmd c;
  fillHeader(&c.h, type);
  c.arg = arg;
  for (int i = 0; i < 4; i++) { esp_now_send(BCAST, (uint8_t *)&c, sizeof(c)); delay(5); } // repeat (unacked)
}

// ---- mode transitions ------------------------------------------------------
void enterComms();
void enterMaintenance() {
  Serial.println("-> MAINTENANCE (WiFi OTA)");
  espNowDeinit();
  gMode = MODE_MAINT;
  maintEnteredMs = millis();
  startWifiOta();
}
void enterComms() {
  Serial.println("-> COMMS (ESP-NOW)");
  if (otaActive) stopOtaAndWifi();
  gMode = MODE_COMMS;
  WiFi.mode(WIFI_STA);
#if NB_WIFI_LOWPOWER
  WiFi.setSleep(true);
#else
  WiFi.setSleep(false);
#endif
  if (IS_MASTER) {
#if RES_HAS_WIFI_SECRETS
    // Master joins the bench AP (on NB_CHANNEL) to bridge stats + serve /telemetry.
    WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) { delay(250); Serial.print("."); }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("master WiFi-STA ip="); Serial.print(WiFi.localIP());
      Serial.printf(" ch=%d\n", WiFi.channel());
      if (WiFi.channel() != NB_CHANNEL)
        Serial.printf("*** WARNING: AP channel %d != NB_CHANNEL %d -> ESP-NOW WILL FAIL. Set the AP to channel %d.\n",
                      WiFi.channel(), NB_CHANNEL, NB_CHANNEL);
      configureOtaRoutes();
      server.begin();
      bridgeUdp.begin(54321);
    } else {
      Serial.println("master AP join failed; bridging disabled (ESP-NOW still on NB_CHANNEL)");
    }
#endif
  } else {
    WiFi.disconnect(); // STA up but unassociated -> sit on NB_CHANNEL, pure ESP-NOW
  }
  espNowInit();
}

// ---- master -> host UDP bridge ---------------------------------------------
void bridgeStats() {
  if (!IS_MASTER || WiFi.status() != WL_CONNECTED) return;
  char line[256];
  // self line
  int n = snprintf(line, sizeof(line),
                   "nb-master id=%02X%02X%02X ch=%d frames=%lu sendok=%lu sendfail=%lu up=%lu bv=%.3f\n",
                   myId[0], myId[1], myId[2], WiFi.channel(), (unsigned long)txSeq,
                   (unsigned long)sendOk, (unsigned long)sendFail, (unsigned long)millis(), cbV);
  bridgeUdp.beginPacket(IPAddress(255, 255, 255, 255), 54321);
  bridgeUdp.write((uint8_t *)line, n);
  bridgeUdp.endPacket();
  // per-peer lines
  for (int i = 0; i < NB_MAX_TRACKED; i++) {
    NbPeerStat *p = &peers[i];
    if (!p->used) continue;
    uint32_t tot = p->recv + p->gaps;
    float pdr = tot ? (float)p->recv / (float)tot : 0.0f;
    n = snprintf(line, sizeof(line),
                 "nb-peer id=%02X%02X%02X seq=%lu rx=%lu gaps=%lu pdr=%.4f rssi=%d bv=%.3f ima=%d soc=%d rr=%s ca=%d mode=%d dlpdr=%.3f dlrssi=%d up=%lu age=%lu\n",
                 p->id[0], p->id[1], p->id[2], (unsigned long)p->last_seq, (unsigned long)p->recv,
                 (unsigned long)p->gaps, pdr, p->rssi, p->batt_mv / 1000.0f, p->batt_ma,
                 (p->soc == 255 ? -1 : p->soc), resetReasonName((esp_reset_reason_t)p->rr), p->ca,
                 p->pmode, p->dl_pdr_x1000 / 1000.0f, p->dl_rssi, (unsigned long)p->up,
                 (unsigned long)(millis() - p->last_heard_ms));
    bridgeUdp.beginPacket(IPAddress(255, 255, 255, 255), 54321);
    bridgeUdp.write((uint8_t *)line, n);
    bridgeUdp.endPacket();
  }
}

// ---- rx processing (loop context) ------------------------------------------
void processRx() {
  RxItem it;
  while (xQueueReceive(rxQueue, &it, 0) == pdTRUE) {
    if (it.len < (int)sizeof(NbHeader)) continue;
    NbHeader *h = (NbHeader *)it.data;
    if (h->ver != NB_PROTO_VER) continue;
    if (memcmp(h->src_id, myId, 3) == 0) continue; // ignore our own (shouldn't happen)

    if (h->type == NB_SHOWFRAME) {
      // peers measure downlink (master multicast) PDR
      if (!masterSeen || (h->seq < masterLastSeq && masterLastSeq - h->seq > 100)) {
        masterSeen = true; masterLastSeq = h->seq; masterRx = 1; masterGaps = 0;
      } else if (h->seq > masterLastSeq) {
        masterGaps += (h->seq - masterLastSeq - 1); masterRx++; masterLastSeq = h->seq;
      } else masterRx++;
      masterRssi = it.rssi;
    } else if (h->type == NB_HEARTBEAT && it.len >= (int)sizeof(NbHeartbeat)) {
      NbPeerStat *p = findPeer(h->src_id, true);
      if (p) {
        accountSeq(p, h->seq);
        p->last_type = h->type;
        p->rssi = it.rssi;
        p->last_heard_ms = millis();
        p->up = h->uptime_ms;
        NbHeartbeat *hb = (NbHeartbeat *)it.data;
        p->batt_mv = hb->batt_mv; p->batt_ma = hb->batt_ma; p->soc = hb->soc_pct;
        p->rr = hb->reset_reason; p->ca = hb->ca_state; p->pmode = hb->mode;
        p->dl_pdr_x1000 = hb->dl_pdr_x1000; p->dl_rssi = hb->dl_rssi;
      }
    } else if (h->type == NB_ENTER_MAINT) {
      if (gMode == MODE_COMMS) enterMaintenance();
    } else if (h->type == NB_RESUME) {
      if (gMode == MODE_COMMS) { /* already comms */ }
    } else if (h->type == NB_SET_RATE && it.len >= (int)sizeof(NbCmd)) {
      uint8_t hz = ((NbCmd *)it.data)->arg;
      if (hz >= 1 && hz <= 100) { gRateHz = hz; Serial.printf("rate set -> %u Hz\n", hz); }
    }
  }
}

// trivial single-hop CA: next state = parity of fresh neighbors' states
uint8_t caTick() {
  uint32_t now = millis();
  int sum = 0;
  for (int i = 0; i < NB_MAX_TRACKED; i++)
    if (peers[i].used && now - peers[i].last_heard_ms < 3000) sum += peers[i].ca;
  return (uint8_t)(sum & 1);
}

// ---- serial commands -------------------------------------------------------
void handleSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  static const uint8_t rates[] = {1, 2, 5, 10, 20, 50};
  static int rateIdx = 3;
  switch (c) {
  case 't': Serial.println(telemetryJson()); break;
  case 'u': // master: announce maintenance window then enter
    if (IS_MASTER) { Serial.println("broadcast ENTER_MAINT"); sendCmd(NB_ENTER_MAINT, 0); delay(50); enterMaintenance(); }
    break;
  case 'c': // master: resume
    if (IS_MASTER && gMode == MODE_COMMS) { Serial.println("broadcast RESUME"); sendCmd(NB_RESUME, 0); }
    else if (gMode == MODE_MAINT) enterComms();
    break;
  case '+':
    if (IS_MASTER) { if (rateIdx < 5) rateIdx++; gRateHz = rates[rateIdx]; sendCmd(NB_SET_RATE, gRateHz); Serial.printf("rate -> %u Hz\n", gRateHz); }
    break;
  case '-':
    if (IS_MASTER) { if (rateIdx > 0) rateIdx--; gRateHz = rates[rateIdx]; sendCmd(NB_SET_RATE, gRateHz); Serial.printf("rate -> %u Hz\n", gRateHz); }
    break;
#ifdef NB_WDT_HANGTEST
  case 'x':
    Serial.println("HANGTEST: stop feeding watchdog, busy-loop (expect WDT reset)...");
    Serial.flush();
    while (1) { /* deliberately never esp_task_wdt_reset() */ }
    break;
#endif
  case 'r':
    Serial.printf("role=%s mode=%d ch=%d rate=%uHz txseq=%lu sendok=%lu fail=%lu peers=", IS_MASTER ? "master" : "peer",
                  (int)gMode, NB_CHANNEL, gRateHz, (unsigned long)txSeq, (unsigned long)sendOk, (unsigned long)sendFail);
    { int np = 0; for (int i = 0; i < NB_MAX_TRACKED; i++) if (peers[i].used) np++; Serial.println(np); }
    break;
  default: break;
  }
}

// ---- autosleep reboot-loop breaker (ported) --------------------------------
#ifdef NB_AUTOSLEEP
bool nbSupplyPresent() {
  if (!pfReady) return false;
  float v = 0;
  if (Board.getSupplyVoltage(v) == Result::Ok) return v > 4.0f;
  return false;
}
void nbDeepSleep(const char *why) {
  Serial.printf("deep sleep (%s), timer wake %ds\n", why, NB_WAKE_S);
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)NB_WAKE_S * 1000000ULL);
  esp_deep_sleep_start();
}
void autosleepBootCheck() {
  esp_reset_reason_t rr = esp_reset_reason();
  bool supply = nbSupplyPresent();
  Preferences pf;
  pf.begin("netbench", false);
  if (supply) {
    pf.putUInt("boots", 0); pf.end();
    Serial.println("autosleep: supply present -> normal run");
  } else {
    if (rr == ESP_RST_DEEPSLEEP) { pf.end(); nbDeepSleep("battery-resleep"); }
    uint32_t boots = (rr == ESP_RST_POWERON) ? pf.getUInt("boots", 0) + 1 : 1;
    pf.putUInt("boots", boots); pf.end();
    Serial.printf("autosleep: boot #%u on battery\n", boots);
    if (boots >= NB_LOOP_LIMIT) nbDeepSleep("loop-break");
  }
}
void autosleepHealthyClear() {
  static bool done = false;
  if (done || millis() < NB_HEALTHY_MS) return;
  Preferences pf; pf.begin("netbench", false); pf.putUInt("boots", 0); pf.end();
  done = true;
  Serial.println("autosleep: boot healthy -> counter cleared");
}
#endif

// ---- setup / loop ----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1500); // native USB-CDC: give the host time to attach before banner
  Serial.println();
  Serial.println("=== Resonance net-bench " NET_BENCH_VERSION " ===");
  Serial.printf("role=%s channel=%d frame_hz=%d hb_hz=%d\n", IS_MASTER ? "master" : "peer",
                NB_CHANNEL, NB_FRAME_HZ, NB_HB_HZ);

  esp_read_mac(myMac, ESP_MAC_WIFI_STA);
  myId[0] = myMac[3]; myId[1] = myMac[4]; myId[2] = myMac[5];
  char idb[7]; snprintf(idb, sizeof(idb), "%02X%02X%02X", myId[0], myId[1], myId[2]);
  shortId = String(idb);
  Serial.printf("node id=%s mac=%02X:%02X:%02X:%02X:%02X:%02X\n", idb, myMac[0], myMac[1],
                myMac[2], myMac[3], myMac[4], myMac[5]);

  setupPowerFeather();
#ifdef NB_AUTOSLEEP
  autosleepBootCheck();
#endif
  setupWatchdog();

  pinMode(NB_LED_PIN, OUTPUT);
  digitalWrite(NB_LED_PIN, LOW);
  rxQueue = xQueueCreate(32, sizeof(RxItem));
  memset(peers, 0, sizeof(peers));

#ifdef NB_START_MAINT
  enterMaintenance();
#else
  enterComms();
#endif
}

void loop() {
  esp_task_wdt_reset();
  handleSerial();

  static uint32_t lastBat = 0;
  uint32_t now = millis();
  if (now - lastBat > 1000) { lastBat = now; readBattery(); }
  updateStatusLed();

#ifdef NB_AUTOSLEEP
  autosleepHealthyClear();
#endif

  if (gMode == MODE_MAINT) {
    server.handleClient();
    // /resume HTTP hit -> do a real comms transition (re-init ESP-NOW), not just a flag flip
    if (gResumePending) {
      gResumePending = false;
      Serial.println("/resume -> comms");
      enterComms();
      return;
    }
    // peers auto-resume if no OTA happens within the window (prevents stranding)
    if (!IS_MASTER && millis() - maintEnteredMs > (uint32_t)NB_MAINT_TIMEOUT_S * 1000) {
      Serial.println("maintenance timeout -> resume comms");
      enterComms();
    }
    return;
  }

  // COMMS mode
  processRx();
  if (IS_MASTER && WiFi.status() == WL_CONNECTED) server.handleClient();

  // periodic jittered sends
  static uint32_t nextSend = 0;
  if (now >= nextSend) {
    uint8_t hz = gRateHz < 1 ? 1 : gRateHz;
    uint32_t period = 1000 / hz;
    if (IS_MASTER) { if (NB_FRAME_HZ > 0) sendShowFrame(); }
    else sendHeartbeat(caTick());
    uint32_t jit = (period * NB_JITTER_PCT) / 100;
    nextSend = now + period + (jit ? (esp_random() % (2 * jit)) - jit : 0);
  }

  // master bridge to host
  static uint32_t nextBridge = 0;
  if (IS_MASTER && now >= nextBridge) { bridgeStats(); nextBridge = now + (1000 / NB_BRIDGE_HZ); }
}
