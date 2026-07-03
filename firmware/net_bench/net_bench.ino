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
#include <Wire.h>
// Env sensors (MPP sweep): auto-probed on the STEMMA bus (Wire1) at boot --
// one image serves sensored and bare boards alike. See envInit()/envTick().
#include <Adafruit_TSL2591.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_NeoPixel.h>

#define NET_BENCH_VERSION "net-bench-2026-06-30.7" // charger-status telemetry
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
#include "../powerfeather_solar_guard.h"
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
// ---- WiFi scan-report (field 2.4 GHz coverage mapping over ESP-NOW) ---------
// A field PEER built with -DNB_SCAN_REPORT periodically WiFi-scans (never
// associates) and broadcasts each visible AP as an NB_SCANAP packet. A board
// built with -DNB_SERIAL_BRIDGE (a master) relays everything it hears to USB
// serial (so a desk-tethered bridge logs the fleet WITHOUT a laptop in the
// field) -- the answer to the WiFi range diagnostic / item (b) of the solar-
// telemetry plan. Scan-only: no association, so the radio stays pinned to
// NB_CHANNEL for ESP-NOW (no Eero-channel coupling). See README.
#ifndef NB_SCAN_S
#define NB_SCAN_S 20 // field-peer scan-report interval (s)
#endif
#ifndef NB_SCAN_MAX
#define NB_SCAN_MAX 12 // max APs reported per scan batch (bounds the ESP-NOW burst)
#endif
// ---- sleep-cycle (field-representative duty-cycled load measurement) --------
// -DNB_SLEEP_CYCLE makes a peer wake -> send a telemetry heartbeat -> listen
// briefly for ENTER_MAINT (preserve no-touch OTA) -> deep-sleep NB_SLEEP_S. On
// battery-only this measures the duty-cycled average draw (the overnight/idle
// budget) -- vs the always-on ~168 mA that flattens a 2 Ah cell in ~12 h. Run it
// multi-hour/overnight: avg current = SOC/coulomb drop over time (gauge is too
// coarse for a precise per-wake sleep current -> use an external meter for that).
#ifndef NB_SLEEP_S
#define NB_SLEEP_S 30 // deep-sleep interval between wakes (s)
#endif
#ifndef NB_WAKE_LISTEN_MS
#define NB_WAKE_LISTEN_MS 400 // per-wake window to catch ENTER_MAINT before re-sleeping (ms)
#endif

// Remote low-battery park command. A bare serial 'S' sleeps peers for this long.
#ifndef NB_REMOTE_SLEEP_S
#define NB_REMOTE_SLEEP_S 21600 // 6 hours; enough to wait for sun without losing recoverability
#endif
#ifndef NB_TARGET_SLEEP_DEFAULT_S
#define NB_TARGET_SLEEP_DEFAULT_S 3600 // selected-peer "solar nap" default
#endif
#ifndef NB_DRAWDOWN_PIXEL_PIN
#define NB_DRAWDOWN_PIXEL_PIN 10 // GPIO10 / A0: direct-GPIO HEX bench harness
#endif
#ifndef NB_DRAWDOWN_PIXEL_COUNT
#define NB_DRAWDOWN_PIXEL_COUNT 37
#endif
#ifndef NB_DRAWDOWN_BRIGHTNESS
#define NB_DRAWDOWN_BRIGHTNESS 128 // measured about 0.75 A battery-side on the current HEX rig
#endif
#ifndef NB_DRAWDOWN_R
#define NB_DRAWDOWN_R 255
#endif
#ifndef NB_DRAWDOWN_G
#define NB_DRAWDOWN_G 255
#endif
#ifndef NB_DRAWDOWN_B
#define NB_DRAWDOWN_B 255
#endif
#ifndef NB_DRAWDOWN_DEFAULT_MAH
#define NB_DRAWDOWN_DEFAULT_MAH 3500 // full 7.2 Ah -> roughly half-full/hungry
#endif
#ifndef NB_DRAWDOWN_SOFT_FLOOR_MV
#define NB_DRAWDOWN_SOFT_FLOOR_MV 3180 // loaded LFP, after the initial sag window
#endif
#ifndef NB_DRAWDOWN_HARD_FLOOR_MV
#define NB_DRAWDOWN_HARD_FLOOR_MV 3050 // stay well above precharge/trickle territory
#endif
#ifndef NB_DRAWDOWN_MIN_RUN_S
#define NB_DRAWDOWN_MIN_RUN_S 1800 // ignore early load sag for the soft floor
#endif
#ifndef NB_DRAWDOWN_SLEEP_S
#define NB_DRAWDOWN_SLEEP_S 43200 // 12 h: preserve the hungry cell for tomorrow morning
#endif
// ---- field-cycle (production-ish solar day / radio-night lifecycle) --------
// -DNB_FIELD_CYCLE makes a peer run an autonomous state machine:
// charge/sleep on external supply, wait-full, draw down in dark with the normal
// ESP-NOW radio load, then protect-sleep at a low LFP voltage. Timer wakes keep
// it recoverable; a sustained U can still catch an awake window for OTA.
#ifndef NB_FIELD_CHARGE_SLEEP_S
#define NB_FIELD_CHARGE_SLEEP_S 300 // charge-mode telemetry cadence while sleeping
#endif
#ifndef NB_FIELD_WAIT_SLEEP_S
#define NB_FIELD_WAIT_SLEEP_S 300 // full-but-daylight check cadence
#endif
#ifndef NB_FIELD_PROTECT_SLEEP_S
#define NB_FIELD_PROTECT_SLEEP_S 900 // low-battery protect recheck cadence
#endif
#ifndef NB_FIELD_WAKE_LISTEN_MS
#define NB_FIELD_WAKE_LISTEN_MS 8000 // timer-wake OTA/telemetry window before re-sleep
#endif
#ifndef NB_FIELD_COLD_LISTEN_MS
#define NB_FIELD_COLD_LISTEN_MS 30000 // cold boot catch window before first sleep
#endif
#ifndef NB_FIELD_FULL_MV
#define NB_FIELD_FULL_MV 3550 // LFP top knee / effectively full candidate
#endif
#ifndef NB_FIELD_FULL_TAPER_MA
#define NB_FIELD_FULL_TAPER_MA 120 // net battery charge current at/under this near top = full-ish
#endif
#ifndef NB_FIELD_LOW_MV
#define NB_FIELD_LOW_MV NB_DRAWDOWN_SOFT_FLOOR_MV
#endif
#ifndef NB_FIELD_CRITICAL_MV
#define NB_FIELD_CRITICAL_MV NB_DRAWDOWN_HARD_FLOOR_MV
#endif
#ifndef NB_FIELD_DRAWDOWN_MIN_S
#define NB_FIELD_DRAWDOWN_MIN_S NB_DRAWDOWN_MIN_RUN_S
#endif
#ifndef NB_FIELD_SUN_SUPPLY_MV
#define NB_FIELD_SUN_SUPPLY_MV 4000 // supply/panel voltage that counts as present
#endif
#ifndef NB_FIELD_SUN_CHARGE_MA
#define NB_FIELD_SUN_CHARGE_MA 20 // input current into board that counts as useful sun/supply
#endif
#ifndef NB_FIELD_DRAWDOWN_HZ
#define NB_FIELD_DRAWDOWN_HZ 1 // production-ish always-awake radio draw cadence
#endif
#ifndef NB_MAINTAIN_MIN_V10
#define NB_MAINTAIN_MIN_V10 40 // PowerFeather SDK supports 4.0 V minimum
#endif
#ifndef NB_MAINTAIN_MAX_V10
#define NB_MAINTAIN_MAX_V10 168 // PowerFeather SDK supports 16.8 V maximum
#endif
#ifndef NB_MAINT_MIN_LFP_MV
#define NB_MAINT_MIN_LFP_MV 3200 // below this, defer OTA until the LFP has charged
#endif
#ifndef NB_MAINT_MIN_3V7_MV
#define NB_MAINT_MIN_3V7_MV 3600 // low Li-ion cells have browned out on WiFi entry
#endif
#ifndef NB_MAINT_SUPPLY_OVERRIDE_MA
#define NB_MAINT_SUPPLY_OVERRIDE_MA 250 // strong external/solar input can override low Vbat
#endif
#ifndef NB_MAINT_POWER_ENFORCE
#define NB_MAINT_POWER_ENFORCE 0 // default: report low-power maintenance risk; do not block OTA tests
#endif
#ifndef NB_MAINT_REFUSE_REPORT_MS
#define NB_MAINT_REFUSE_REPORT_MS 5000 // throttle warning prints/HBs during sustained U
#endif
#ifndef NB_CAPACITY_MIN_MAH
#define NB_CAPACITY_MIN_MAH 100
#endif
#ifndef NB_CAPACITY_MAX_MAH
#define NB_CAPACITY_MAX_MAH 30000
#endif
#ifndef NB_CHARGE_MIN_MA
#define NB_CHARGE_MIN_MA 40
#endif
#ifndef NB_CHARGE_MAX_MA
#define NB_CHARGE_MAX_MA 2000
#endif
// NB_START_MAINT, NB_WDT_HANGTEST, NB_AUTOSLEEP, NB_SCAN_REPORT, NB_SERIAL_BRIDGE,
// NB_MAINT_AP, NB_SLEEP_CYCLE, NB_FIELD_CYCLE are presence-only flags.
#ifndef NB_MAINT_AP_PASS
#define NB_MAINT_AP_PASS "resonance"
#endif
#ifndef NB_MAINT_AP_PREFIX
#define NB_MAINT_AP_PREFIX "ResonanceMaint"
#endif
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

#if defined(NB_SERIAL_BRIDGE)
static const bool SERIAL_BRIDGE = true; // master relays bridge lines to USB serial
#else
static const bool SERIAL_BRIDGE = false;
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
enum MaintStatus : uint8_t {
  MAINT_STATUS_IDLE = 0,
  MAINT_STATUS_ACTIVE = 1,
  MAINT_STATUS_POWER_WARN = 2,
  MAINT_STATUS_START_FAILED = 3,
  MAINT_STATUS_TIMEOUT = 4,
  MAINT_STATUS_RESUMED = 5,
};
enum FieldPhase : uint8_t {
  FC_OFF = 0,
  FC_BOOT = 1,
  FC_CHARGE = 2,
  FC_WAIT_DARK = 3,
  FC_DRAWDOWN = 4,
  FC_PROTECT = 5,
};
enum FieldReason : uint8_t {
  FC_REASON_NONE = 0,
  FC_REASON_BOOT = 1,
  FC_REASON_SUPPLY = 2,
  FC_REASON_FULL = 3,
  FC_REASON_DARK = 4,
  FC_REASON_LOW = 5,
  FC_REASON_CRITICAL = 6,
  FC_REASON_SUNRISE = 7,
};
volatile NetMode gMode = MODE_COMMS;
volatile bool gResumePending = false; // /resume sets this; loop() does the real enterComms()
bool otaActive = false;
uint32_t maintEnteredMs = 0;
uint8_t gMaintStatus = MAINT_STATUS_IDLE;
uint32_t gLastMaintRefuseMs = 0;

#define NB_FIELD_RTC_MAGIC 0xC7
RTC_DATA_ATTR uint8_t rtcFieldMagic = 0;
RTC_DATA_ATTR uint8_t rtcFieldPhase = FC_OFF;
RTC_DATA_ATTR uint8_t rtcFieldReason = FC_REASON_NONE;
RTC_DATA_ATTR uint16_t rtcFieldCycle = 0;
RTC_DATA_ATTR uint32_t rtcFieldPhaseElapsedS = 0;
RTC_DATA_ATTR uint32_t rtcFieldChargeMas = 0;
RTC_DATA_ATTR uint32_t rtcFieldDischargeMas = 0;
RTC_DATA_ATTR uint16_t rtcFieldMinMv = 0;
RTC_DATA_ATTR uint16_t rtcFieldMaxMv = 0;
uint32_t fieldLastIntegrateMs = 0;

uint32_t txSeq = 0;        // our outgoing sequence number (monotonic)
uint32_t sendOk = 0, sendFail = 0; // ESP-NOW send-callback tallies
uint8_t gRateHz = IS_MASTER ? NB_FRAME_HZ : NB_HB_HZ; // runtime-settable (master '+'/'-')

// cached battery + supply (panel) telemetry (refreshed ~1 Hz; the ESP-NOW callback
// must not touch the SDK). Supply = the solar/USB input side, the missing half for
// battery/panel sizing -- supply_v x supply_ma = panel harvest; battery_ma = net.
float cbV = 0, cbMa = 0;
int cbSoc = -1;
float csV = 0, csMa = 0; // supply (panel) voltage / current
bool csGood = false;     // charger considers the supply valid (checkSupplyGood)
uint16_t gBatteryCapacityMah = (uint16_t)RES_PF_BATTERY_CAPACITY_MAH;
uint16_t gChargeMa = (uint16_t)(RES_PF_MAX_CHARGE_MA + 0.5f);
float gMaintainV = (float)RES_PF_MAINTAIN_V;
bool gBenchConfigLoaded = false;
uint16_t gBqVindpmMv = 0xFFFF, gBqIchgMa = 0xFFFF, gBqVregMv = 0xFFFF;
uint8_t gBqReg16 = 0xFF, gBqReg18 = 0xFF, gBqStat0 = 0xFF, gBqStat1 = 0xFF;
uint8_t gBqFault0 = 0xFF, gBqFlag0 = 0xFF, gBqFlag1 = 0xFF, gBqFaultFlag0 = 0xFF;
uint8_t gBqPart = 0xFF;

// ---- env sensors (MPP sweep: light + panel temp, no host tether) ------------
Adafruit_TSL2591 gTsl(2591);          // lux/irradiance proxy (STEMMA, 0x29)
Adafruit_SHT31 gSht(&Wire1);          // panel-back temp + RH (STEMMA, 0x44)
bool gEnvTsl = false, gEnvSht = false, gEnvBtemp = false;
uint32_t gEnvLuxX10 = 0xFFFFFFFF;     // 0xFFFFFFFE = saturated (full sun > range)
uint16_t gEnvCh0 = 0, gEnvCh1 = 0;
int16_t gEnvPtempX10 = INT16_MIN;
uint8_t gEnvRh = 255;
int16_t gEnvBtempX10 = INT16_MIN;

// ---- onboard SEN0291/INA219 meters (panel + battery leads, on Wire1) --------
// Address convention (uses the two UNAMBIGUOUS DIP settings -- no need to know
// which switch is A0): both OFF = 0x40 = panel/VDC lead; both ON = 0x45 =
// battery lead. Override with -DNB_INA_PANEL_ADDR / -DNB_INA_BATT_ADDR.
// NOTE: 0x44 is reserved by the SHT31; 0x41 needs a single-switch guess.
#ifndef NB_INA_PANEL_ADDR
#define NB_INA_PANEL_ADDR 0x40
#endif
#ifndef NB_INA_BATT_ADDR
#define NB_INA_BATT_ADDR 0x45
#endif
#define NB_INA_RSHUNT_OHMS 0.01f
#define NB_INA_CONFIG 0x27FF // BRNG 32V, PG /1 (+-4A @ 10mR), 128-avg, continuous
bool gInaPanel = false, gInaBatt = false;
int16_t gInaPvMv = INT16_MIN, gInaPaMa = INT16_MIN, gInaBvMv = INT16_MIN, gInaBaMa = INT16_MIN;
// (the ina* helper FUNCTIONS live below the struct definitions -- defining any
// function this early moves the Arduino auto-prototype insertion point above
// the structs and breaks every later type reference)

Adafruit_NeoPixel drawdownPixels(NB_DRAWDOWN_PIXEL_COUNT, NB_DRAWDOWN_PIXEL_PIN,
                                 NEO_GRB + NEO_KHZ800);
bool gDrawdownPixelsBegun = false;
bool gDrawdownActive = false;
uint16_t gDrawdownBudgetMah = NB_DRAWDOWN_DEFAULT_MAH;
float gDrawdownMah = 0.0f;
uint32_t gDrawdownStartMs = 0;
uint32_t gDrawdownLastMs = 0;
uint32_t gDrawdownLastLogMs = 0;

uint32_t identifyUntil = 0; // millis deadline: blink the locate pattern instead of battery

// field scan-report state (set on a -DNB_SCAN_REPORT peer). gScanRunning is read
// unconditionally by the send loop (suppress ESP-NOW TX while the radio is hopping).
bool gScanRunning = false;
uint8_t gScanId = 0;
uint32_t gNextScanMs = 0;

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
  NB_IDENTIFY = 6,    // master -> target (or all): blink the locate pattern to find a board
  NB_SCANAP = 7,      // field peer -> bridge: one scanned 2.4 GHz AP (coverage mapping)
  NB_SET_MAINTAIN = 8,// master -> all: set charger VINDPM/maintain V (panel MPP sweep / MPPT)
  NB_SET_CAPACITY = 9,// master -> all: persist battery capacity (mAh), reboot to apply
  NB_SET_CHARGE_MA = 10,// master -> all: persist/apply charger current cap (mA)
  NB_SLEEP_FOR = 11,  // master -> all: cut rails and timed deep-sleep (seconds)
  NB_DRAWDOWN = 12,   // master -> target: HEX drawdown load, then guarded timed sleep
  NB_TARGET_SLEEP_FOR = 13, // master -> target: cut rails and timed deep-sleep
  NB_TARGET_CAPACITY = 14,  // master -> target: persist capacity (mAh), reboot to apply
  NB_TARGET_CHARGE_MA = 15, // master -> target: persist/apply charger current cap (mA)
  NB_TARGET_ENTER_MAINT = 16, // master -> target: enter shared-WiFi maintenance/OTA
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
  // --- APPEND-ONLY below (supply/panel side) ---------------------------------
  // New fields go at the END. Old parsers length-check (it.len >= sizeof) and
  // ignore the tail, so NB_PROTO_VER stays 1 (no flag-day; a pre-supply master
  // still reads the battery fields of a supply-capable peer). NEVER reorder/insert.
  int16_t supply_mv; // panel/USB input voltage (mV)
  int16_t supply_ma; // panel/USB input current INTO the board (mA)
  uint8_t supply_good; // charger considers the supply valid (0/1)
  // --- APPEND-ONLY tail 2 (env sensors on the peer's STEMMA bus, for the MPP
  // sweep: no-tether outdoor light + panel-temp logging). Same rules as above.
  uint32_t lux_x10;   // TSL2591 lux*10; 0xFFFFFFFF = no sensor, 0xFFFFFFFE = SATURATED
  uint16_t light_ch0; // TSL2591 raw full-spectrum count (GAIN_LOW / 100 ms, fixed)
  uint16_t light_ch1; // TSL2591 raw IR count
  int16_t ptemp_cx10; // SHT31 temp x10 C (tape it to the panel BACK); INT16_MIN = absent
  uint8_t prh_pct;    // SHT31 RH %; 255 = absent
  int16_t btemp_cx10; // battery NTC via charger TS x10 C (needs -DNB_BATT_NTC); INT16_MIN = absent
  // --- APPEND-ONLY tail 3 (SEN0291/INA219 meters on the peer's OWN Wire1:
  // ground-truth panel + battery power for the MPP sweep, no Metro tether.
  // The peer is awake during sweeps, so self-metering works; sleep/reset tests
  // still need the separate Metro monitor). INT16_MIN = channel absent.
  int16_t ina_pv_mv; // panel-lead INA bus mV (0x40 default)
  int16_t ina_pa_ma; // panel-lead INA current mA (wired so INTO the board = +)
  int16_t ina_bv_mv; // battery-lead INA bus mV (0x41 default)
  int16_t ina_ba_ma; // battery-lead INA current mA
  // --- APPEND-ONLY tail 4 (runtime bench config). Lets the bridge/dashboard
  // verify which NVS-backed gauge/charge settings this peer booted with.
  uint16_t cfg_cap_mah;
  uint16_t cfg_charge_ma;
  // --- APPEND-ONLY tail 5 (targeted battery drawdown helper). This is bench-only
  // telemetry for preparing a hungry LFP and estimating delivered capacity.
  uint16_t drawdown_mah_x10;
  uint16_t drawdown_budget_mah;
  uint8_t drawdown_active;
  // --- APPEND-ONLY tail 6 (dashboard/debug identity). Fixed-size to keep the
  // heartbeat binary and parser simple; NUL-terminated when sent by this firmware.
  char fw_rev[24];
  // --- APPEND-ONLY tail 7 (maintenance health). Lets the bridge see power warnings
  // and maintenance-start failures instead of inferring them from silence.
  uint8_t maint_status;
  // --- APPEND-ONLY tail 8 (field-cycle lifecycle). Production-ish solar day /
  // radio-night state, persisted through timer deep sleep in RTC memory.
  uint8_t field_phase;
  uint8_t field_reason;
  uint16_t field_cycle;
  uint16_t field_elapsed_s;
  uint16_t field_charge_mah;
  uint16_t field_discharge_mah;
  uint16_t field_min_mv;
  uint16_t field_max_mv;
  // --- APPEND-ONLY tail 9 (BQ25628E charger truth). Raw status/control bytes
  // avoid over-decoding in the firmware while still exposing CHG_EN/HIZ,
  // BATFET, VBUS/charge state, and fault flags to the dashboard/log.
  uint16_t bq_vindpm_mv; // programmed/effective VINDPM via SDK, 0xFFFF = unknown
  uint16_t bq_ichg_ma;   // charge-current limit via SDK, 0xFFFF = unknown
  uint16_t bq_vreg_mv;   // charge-voltage/CV limit via SDK, 0xFFFF = unknown
  uint8_t bq_reg16;      // Charger_Control_0: CHG_EN, EN_HIZ, watchdog
  uint8_t bq_reg18;      // Charger_Control_2: BATFET_CTRL, WVBUS
  uint8_t bq_stat0;      // Charger_Status_0
  uint8_t bq_stat1;      // Charger_Status_1: VBUS_STAT, CHG_STAT
  uint8_t bq_fault0;     // FAULT_Status_0
  uint8_t bq_flag0;      // Charger_Flag_0
  uint8_t bq_flag1;      // Charger_Flag_1
  uint8_t bq_fault_flag0;// FAULT_Flag_0
  uint8_t bq_part;       // Part_Information
};
struct __attribute__((packed)) NbCmd { // ENTER_MAINT / RESUME / SET_RATE
  NbHeader h;
  uint8_t arg; // SET_RATE: Hz
};
struct __attribute__((packed)) NbSetU16 {
  NbHeader h;
  uint16_t value;
};
struct __attribute__((packed)) NbTargetU16 {
  NbHeader h;
  uint8_t target_id[3]; // 00:00:00 = all; otherwise node short ID
  uint16_t value;
};
struct __attribute__((packed)) NbTargetCmd {
  NbHeader h;
  uint8_t target_id[3]; // 00:00:00 = all; otherwise node short ID
  uint8_t arg;
};
struct __attribute__((packed)) NbIdentify { // locate a board (target 00:00:00 = all)
  NbHeader h;
  uint8_t target_id[3];
  uint8_t secs;
};
struct __attribute__((packed)) NbScanAp { // one AP from a field scan batch (45 bytes)
  NbHeader h;
  uint8_t scan_id;  // increments per scan batch (groups a batch on the host)
  uint8_t idx;      // rank within the batch (0 = strongest), sorted by RSSI desc
  uint8_t count;    // total APs found this scan (idx<count; >NB_SCAN_MAX => truncated)
  uint8_t bssid[6]; // which radio (e.g. which Eero node)
  int8_t ap_rssi;   // RSSI of this AP as seen from the FIELD board (the coverage metric)
  uint8_t channel;
  uint8_t enc;      // wifi_auth_mode_t
  char ssid[20];    // truncated SSID (identifies home network vs neighbors)
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
  int16_t supply_mv, supply_ma; // panel side (0 if peer pre-dates supply telemetry)
  uint8_t supply_good;
  bool has_env; // peer sent the env-sensor tail
  uint32_t lux_x10;
  uint16_t light_ch0, light_ch1;
  int16_t ptemp_cx10;
  uint8_t prh_pct;
  int16_t btemp_cx10;
  bool has_ina; // peer sent the onboard-INA tail
  int16_t ina_pv_mv, ina_pa_ma, ina_bv_mv, ina_ba_ma;
  bool has_cfg; // peer sent the runtime-config tail
  uint16_t cfg_cap_mah, cfg_charge_ma;
  bool has_drawdown; // peer sent the targeted drawdown tail
  uint16_t drawdown_mah_x10, drawdown_budget_mah;
  uint8_t drawdown_active;
  bool has_fw;
  char fw_rev[24];
  bool has_maint_status;
  uint8_t maint_status;
  bool has_field_cycle;
  uint8_t field_phase, field_reason;
  uint16_t field_cycle, field_elapsed_s, field_charge_mah, field_discharge_mah;
  uint16_t field_min_mv, field_max_mv;
  bool has_bq;
  uint16_t bq_vindpm_mv, bq_ichg_ma, bq_vreg_mv;
  uint8_t bq_reg16, bq_reg18, bq_stat0, bq_stat1, bq_fault0, bq_flag0, bq_flag1;
  uint8_t bq_fault_flag0, bq_part;
};
NbPeerStat peers[NB_MAX_TRACKED];

// ESP-NOW rx -> loop queue (callback enqueues only; loop processes)
struct RxItem {
  uint8_t mac[6];
  int8_t rssi;
  uint8_t len;
  uint8_t data[128];
};
QueueHandle_t rxQueue;
static_assert(sizeof(NbHeartbeat) <= sizeof(RxItem::data),
              "heartbeat outgrew the rx buffer -- bump RxItem::data");
#define NB_HAS_HB_FIELD(len, field) \
  ((len) >= (int)(offsetof(NbHeartbeat, field) + sizeof(((NbHeartbeat *)0)->field)))

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

uint16_t checkedStoredUInt(Preferences &pf, const char *key, uint16_t fallback,
                           uint16_t minVal, uint16_t maxVal) {
  uint32_t raw = pf.getUInt(key, fallback);
  if (raw < minVal || raw > maxVal) return fallback;
  return (uint16_t)raw;
}

void loadBenchConfig() {
  if (gBenchConfigLoaded) return;
  gBenchConfigLoaded = true;
  uint16_t defaultChargeMa = (uint16_t)(RES_PF_MAX_CHARGE_MA + 0.5f);
  Preferences pf;
  pf.begin("netbench", true);
  gBatteryCapacityMah = checkedStoredUInt(pf, "cap_mah", (uint16_t)RES_PF_BATTERY_CAPACITY_MAH,
                                          NB_CAPACITY_MIN_MAH, NB_CAPACITY_MAX_MAH);
  gChargeMa = checkedStoredUInt(pf, "chg_ma", defaultChargeMa, NB_CHARGE_MIN_MA, NB_CHARGE_MAX_MA);
  pf.end();
  Serial.printf("  bench config: cap=%u mAh charge=%u mA%s\n",
                gBatteryCapacityMah, gChargeMa,
                (gBatteryCapacityMah == (uint16_t)RES_PF_BATTERY_CAPACITY_MAH &&
                 gChargeMa == defaultChargeMa) ? " (build defaults)" : " (NVS override)");
}

bool persistCapacity(uint16_t mah) {
  if (mah < NB_CAPACITY_MIN_MAH || mah > NB_CAPACITY_MAX_MAH) {
    Serial.printf("capacity %u rejected (range %u..%u mAh)\n",
                  mah, (unsigned)NB_CAPACITY_MIN_MAH, (unsigned)NB_CAPACITY_MAX_MAH);
    return false;
  }
  Preferences pf;
  pf.begin("netbench", false);
  pf.putUInt("cap_mah", mah);
  pf.end();
  gBatteryCapacityMah = mah;
  Serial.printf("battery capacity stored -> %u mAh; rebooting to apply gauge model\n", mah);
  Serial.flush();
  delay(150);
  esp_restart();
  return true;
}

bool persistAndApplyChargeMa(uint16_t ma) {
  if (ma < NB_CHARGE_MIN_MA || ma > NB_CHARGE_MAX_MA) {
    Serial.printf("charge current %u rejected (range %u..%u mA)\n",
                  ma, (unsigned)NB_CHARGE_MIN_MA, (unsigned)NB_CHARGE_MAX_MA);
    return false;
  }
  Preferences pf;
  pf.begin("netbench", false);
  pf.putUInt("chg_ma", ma);
  pf.end();
  gChargeMa = ma;
  if (pfReady) Board.setBatteryChargingMaxCurrent((float)ma);
  Serial.printf("charge current cap stored/applied -> %u mA\n", ma);
  return true;
}

void setupPowerFeather() {
  Serial.println("PowerFeather SDK init:");
  loadBenchConfig();
  Result r = Result::Failure;
  for (int a = 1; a <= 4; a++) {
    r = Board.init(gBatteryCapacityMah, RES_PF_BATTERY_TYPE);
    if (r == Result::Ok) break;
    Serial.printf("  Board.init attempt %d -> %d, retrying\n", a, (int)r);
    delay(250);
  }
  pfReady = (r == Result::Ok);
  Serial.printf("  Board.init(cap=%u, %s) -> %s\n", (unsigned)gBatteryCapacityMah,
                batteryTypeName(), pfReady ? "Ok" : "ERR");
  if (!pfReady) return;
  gMaintainV = (float)RES_PF_MAINTAIN_V;
  Board.setSupplyMaintainVoltage(gMaintainV);
#if RES_PF_ENABLE_CHARGING
  Board.setBatteryChargingMaxCurrent((float)gChargeMa);
  Board.enableBatteryCharging(true);
#else
  Board.enableBatteryCharging(false);
#endif
  pfSolarGuardInit("net_bench", gMaintainV, RES_PF_ENABLE_CHARGING != 0);
}

// ---- onboard INA helpers (placed after the structs; see note at the globals) -
static bool inaReadReg(uint8_t addr, uint8_t reg, uint16_t &val) {
  Wire1.beginTransmission(addr);
  Wire1.write(reg);
  if (Wire1.endTransmission(false) != 0) return false;
  if (Wire1.requestFrom((int)addr, 2) != 2) return false;
  val = ((uint16_t)Wire1.read() << 8) | Wire1.read();
  return true;
}
static bool inaWriteReg(uint8_t addr, uint8_t reg, uint16_t val) {
  Wire1.beginTransmission(addr);
  Wire1.write(reg);
  Wire1.write(val >> 8);
  Wire1.write(val & 0xFF);
  return Wire1.endTransmission() == 0;
}
static bool inaProbe(uint8_t addr) {
  uint16_t v;
  if (!inaReadReg(addr, 0x00, v)) return false;
  return inaWriteReg(addr, 0x00, NB_INA_CONFIG);
}
static bool inaRead(uint8_t addr, int16_t &bus_mv, int16_t &ma) {
  uint16_t rs, rb;
  if (!(inaReadReg(addr, 0x01, rs) && inaReadReg(addr, 0x02, rb))) return false;
  float shunt_mv = (int16_t)rs * 0.01f;            // 10 uV LSB
  bus_mv = (int16_t)((rb >> 3) * 4);               // 4 mV LSB
  ma = (int16_t)(shunt_mv / NB_INA_RSHUNT_OHMS);   // mV / ohm = mA
  return true;
}

static uint16_t bqMvOrUnknown(bool ok, float volts) {
  if (!ok || volts < 0.0f) return 0xFFFF;
  return (uint16_t)min(65535UL, (unsigned long)(volts * 1000.0f + 0.5f));
}

static uint16_t bqMaOrUnknown(bool ok, float ma) {
  if (!ok || ma < 0.0f) return 0xFFFF;
  return (uint16_t)min(65535UL, (unsigned long)(ma + 0.5f));
}

void readChargerStatus() {
  gBqVindpmMv = gBqIchgMa = gBqVregMv = 0xFFFF;
  gBqReg16 = gBqReg18 = gBqStat0 = gBqStat1 = 0xFF;
  gBqFault0 = gBqFlag0 = gBqFlag1 = gBqFaultFlag0 = gBqPart = 0xFF;
  if (!pfReady) return;

  float v = 0.0f;
  gBqVindpmMv = bqMvOrUnknown(Board.getCharger().getVINDPM(v), v);
  gBqIchgMa = bqMaOrUnknown(Board.getCharger().getChargeCurrentLimit(v), v);
  gBqVregMv = bqMvOrUnknown(Board.getCharger().getChargeVoltageLimit(v), v);

  uint8_t b = 0;
  if (pfSolarGuardRead8(PF_SOLAR_GUARD_REG_CHG_CTRL0, b)) gBqReg16 = b;
  if (pfSolarGuardRead8(0x18, b)) gBqReg18 = b;
  if (pfSolarGuardRead8(0x1D, b)) gBqStat0 = b;
  if (pfSolarGuardRead8(0x1E, b)) gBqStat1 = b;
  if (pfSolarGuardRead8(0x1F, b)) gBqFault0 = b;
  if (pfSolarGuardRead8(0x20, b)) gBqFlag0 = b;
  if (pfSolarGuardRead8(0x21, b)) gBqFlag1 = b;
  if (pfSolarGuardRead8(0x22, b)) gBqFaultFlag0 = b;
  if (pfSolarGuardRead8(0x38, b)) gBqPart = b;
}

// Env sensors: probe the STEMMA bus once at boot. Board.init() already started
// Wire1 (47/48) and enabled VSQT, so a sensored board just works; a bare board
// probes-and-misses and sends absent-sentinels. Battery NTC is OPT-IN
// (-DNB_BATT_NTC): enabling the charger's TS input with no thermistor attached
// makes the BQ apply JEITA limits to a floating pin -> can suspend charging.
void envInit() {
  Wire1.begin(47, 48, 100000); // idempotent; keep the SDK's 100 kHz
  // Wiring check: list every ACK on the STEMMA bus (the power chips live here too)
  Serial.print("Wire1 scan:");
  for (uint8_t a = 0x08; a <= 0x77; a++) {
    Wire1.beginTransmission(a);
    if (Wire1.endTransmission() == 0) Serial.printf(" 0x%02X", a);
  }
  Serial.println();
  gEnvTsl = gTsl.begin(&Wire1);
  if (gEnvTsl) {
    gTsl.setGain(TSL2591_GAIN_LOW);                 // full-sun headroom
    gTsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // max count 36863
  }
  gEnvSht = gSht.begin(0x44);
  gInaPanel = inaProbe(NB_INA_PANEL_ADDR);
  gInaBatt = inaProbe(NB_INA_BATT_ADDR);
#ifdef NB_BATT_NTC
  if (pfReady && Board.enableBatteryTempSense(true) == Result::Ok) {
    float t;
    gEnvBtemp = (Board.getBatteryTemperature(t) == Result::Ok);
  }
#endif
  Serial.printf("env sensors: TSL2591=%d SHT31=%d batt-NTC=%d INA-panel(0x%02X)=%d INA-batt(0x%02X)=%d\n",
                gEnvTsl, gEnvSht, gEnvBtemp, NB_INA_PANEL_ADDR, gInaPanel, NB_INA_BATT_ADDR, gInaBatt);
}

// Refresh the env cache at ~1 Hz. The TSL2591 read blocks ~120 ms (integration),
// so heartbeats at high rates reuse the cache instead of reading inline.
void envTick() {
  static uint32_t nextMs = 0;
  if ((!gEnvTsl && !gEnvSht && !gEnvBtemp && !gInaPanel && !gInaBatt) || millis() < nextMs) return;
  nextMs = millis() + 1000;
  if (gInaPanel && !inaRead(NB_INA_PANEL_ADDR, gInaPvMv, gInaPaMa)) {
    gInaPvMv = INT16_MIN; gInaPaMa = INT16_MIN;
  }
  if (gInaBatt && !inaRead(NB_INA_BATT_ADDR, gInaBvMv, gInaBaMa)) {
    gInaBvMv = INT16_MIN; gInaBaMa = INT16_MIN;
  }
  if (gEnvTsl) {
    uint32_t lum = gTsl.getFullLuminosity();
    gEnvCh0 = (uint16_t)(lum & 0xFFFF);
    gEnvCh1 = (uint16_t)(lum >> 16);
    float lux = gTsl.calculateLux(gEnvCh0, gEnvCh1);
    if (gEnvCh0 >= 36800 || gEnvCh1 >= 36800 || lux < 0.0f)
      gEnvLuxX10 = 0xFFFFFFFE; // saturated: add a diffuser (relative use survives it)
    else
      gEnvLuxX10 = (uint32_t)(lux * 10.0f);
  }
  if (gEnvSht) {
    float t = gSht.readTemperature(), h = gSht.readHumidity();
    gEnvPtempX10 = isnan(t) ? INT16_MIN : (int16_t)(t * 10.0f);
    gEnvRh = isnan(h) ? 255 : (uint8_t)(h + 0.5f);
  }
  if (gEnvBtemp) {
    float t;
    if (Board.getBatteryTemperature(t) == Result::Ok) gEnvBtempX10 = (int16_t)(t * 10.0f);
  }
}

// Locate pattern "..-" (dot dot dash + gap), unmistakable vs the battery blinks.
struct LedStep { uint16_t ms; bool on; };
const LedStep IDENT_PAT[] = {
    {150, true}, {150, false}, {150, true}, {150, false}, {450, true}, {700, false}};
const uint8_t IDENT_N = 6;

// Onboard LED indicator. Identify (locate) overrides battery for its window:
//   identify: "..-" | >50% solid | 25-50% 1 Hz | 10-24% 2 Hz | <10% 4 Hz | no info off
// VOLTAGE CROSS-CHECK: the MAX17260 SOC can read wildly wrong after a DesignCap change
// or before a learn cycle (we saw a full 4.19 V cell report 1%). So floor the displayed
// level by what the (loaded, Li-ion) terminal voltage allows -- a healthy cell never
// shows "critical". Thresholds are Generic_3V7; LFP would need its own (flat curve).
void updateStatusLed() {
  uint32_t now = millis();
  if (now < identifyUntil) { // locate beacon
    static uint32_t t0 = 0;
    static uint8_t i = 0;
    if (now - t0 >= IDENT_PAT[i].ms) { i = (i + 1) % IDENT_N; t0 = now; }
    digitalWrite(NB_LED_PIN, IDENT_PAT[i].on ? HIGH : LOW);
    return;
  }
  static uint32_t last = 0;
  static bool on = false;
  int soc = cbSoc;
  float v = cbV;
  if (soc < 0 && v < 2.5f) { digitalWrite(NB_LED_PIN, LOW); return; } // no battery info -> off
  int vfloor = (v >= 3.95f) ? 99 : (v >= 3.70f) ? 40 : (v >= 3.50f) ? 20 : (v >= 3.35f) ? 12 : 0;
  int eff = (soc < 0) ? vfloor : (soc > vfloor ? soc : vfloor); // healthier of gauge vs voltage
  if (eff > 50) { digitalWrite(NB_LED_PIN, HIGH); return; }      // solid
  uint16_t interval = (eff >= 25) ? 500 : (eff >= 10) ? 250 : 125; // 1/2/4 Hz
  if (now - last >= interval) { last = now; on = !on; digitalWrite(NB_LED_PIN, on ? HIGH : LOW); }
}

void readBattery() {
  if (!pfReady) return;
  float v;
  if (Board.getBatteryVoltage(v) == Result::Ok) cbV = v;
  if (Board.getBatteryCurrent(v) == Result::Ok) cbMa = v;
  uint8_t s;
  if (Board.getBatteryCharge(s) == Result::Ok) cbSoc = s;
  if (Board.getSupplyVoltage(v) == Result::Ok) csV = v;
  if (Board.getSupplyCurrent(v) == Result::Ok) csMa = v;
  bool g;
  if (Board.checkSupplyGood(g) == Result::Ok) csGood = g;
  readChargerStatus();
  pfSolarGuardTick("net_bench", csV, csMa, csGood, gMaintainV, RES_PF_ENABLE_CHARGING != 0);
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
  j += ",\"drawdown_active\":";
  j += gDrawdownActive ? "true" : "false";
  j += ",\"drawdown_mah\":";
  j += String(gDrawdownMah, 1);
  j += ",\"drawdown_budget_mah\":";
  j += String(gDrawdownBudgetMah);
  j += ",\"heap_free\":" + String(ESP.getFreeHeap());
  j += ",\"reset_reason\":\"" + String(resetReasonName(esp_reset_reason())) + "\"";
  j += ",\"pf_ready\":";
  j += pfReady ? "true" : "false";
  j += ",\"maint_status\":";
  j += String(gMaintStatus);
  j += ",\"field_phase\":";
  j += String(rtcFieldPhase);
  j += ",\"field_reason\":";
  j += String(rtcFieldReason);
  j += ",\"field_cycle\":";
  j += String(rtcFieldCycle);
  j += ",\"field_elapsed_s\":";
  j += String((unsigned long)rtcFieldPhaseElapsedS);
  j += ",\"field_charge_mah\":";
  j += String((unsigned long)(rtcFieldChargeMas / 3600UL));
  j += ",\"field_discharge_mah\":";
  j += String((unsigned long)(rtcFieldDischargeMas / 3600UL));
  j += ",\"field_min_mv\":";
  j += String(rtcFieldMinMv);
  j += ",\"field_max_mv\":";
  j += String(rtcFieldMaxMv);
  j += ",\"battery_type\":\"" + String(batteryTypeName()) + "\"";
  if (pfReady) {
    char b[24];
    float v;
    if (Board.getBatteryVoltage(v) == Result::Ok) { snprintf(b, sizeof(b), "%.3f", v); j += ",\"battery_v\":" + String(b); }
    if (Board.getBatteryCurrent(v) == Result::Ok) { snprintf(b, sizeof(b), "%.1f", v); j += ",\"battery_ma\":" + String(b); }
    uint8_t s;
    if (Board.getBatteryCharge(s) == Result::Ok) j += ",\"soc_pct\":" + String(s);
    if (Board.getSupplyVoltage(v) == Result::Ok) { snprintf(b, sizeof(b), "%.3f", v); j += ",\"supply_v\":" + String(b); }
    if (Board.getSupplyCurrent(v) == Result::Ok) { snprintf(b, sizeof(b), "%.1f", v); j += ",\"supply_ma\":" + String(b); }
    bool g;
    if (Board.checkSupplyGood(g) == Result::Ok) { j += ",\"supply_good\":"; j += g ? "true" : "false"; }
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
        esp_task_wdt_reset();
        bool ok = !Update.hasError();
        server.send(ok ? 200 : 500, "text/plain", ok ? "Update complete. Rebooting.\n" : "Update failed.\n");
        delay(500);
        if (ok) ESP.restart();
      },
      []() {
        esp_task_wdt_reset();
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
#if defined(NB_MAINT_AP)
  esp_task_wdt_reset();
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  String ssid = String(NB_MAINT_AP_PREFIX) + "-" + shortId;
  bool ok = WiFi.softAP(ssid.c_str(), NB_MAINT_AP_PASS, NB_CHANNEL);
  esp_task_wdt_reset();
  if (!ok) {
    Serial.println("maintenance AP start failed");
    return false;
  }
  configureOtaRoutes();
  server.begin();
  otaActive = true;
  otaMode = "ap";
  Serial.print("maintenance AP '"); Serial.print(ssid);
  Serial.print("' pass='" NB_MAINT_AP_PASS "' ip="); Serial.print(WiFi.softAPIP());
  Serial.printf(" ch=%d\n", NB_CHANNEL);
  return true;
#else
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
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    esp_task_wdt_reset();
    delay(250);
    Serial.print(".");
  }
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
#endif
}

void stopOtaAndWifi() {
  if (otaActive) server.stop();
  WiFi.softAPdisconnect(true);
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
  if (len < (int)sizeof(NbHeader) || len > (int)sizeof(RxItem::data)) return;
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
  static uint32_t hbSeq = 0;
  NbHeartbeat hb;
  fillHeader(&hb.h, NB_HEARTBEAT);
  // Heartbeats carry their OWN contiguous seq: the master derives uplink PDR from
  // it, and the shared txSeq is also consumed by scan-AP sends -- without this, a
  // scan batch's N packets would read as N phantom heartbeat gaps. (Suppressed
  // heartbeats during a scan simply don't advance hbSeq, so they aren't gaps.)
  hb.h.seq = hbSeq++;
  hb.batt_mv = (int16_t)(cbV * 1000.0f);
  hb.batt_ma = (int16_t)cbMa;
  hb.soc_pct = (cbSoc < 0) ? 255 : (uint8_t)cbSoc;
  hb.reset_reason = (uint8_t)esp_reset_reason();
  hb.ca_state = caState;
  hb.mode = (uint8_t)gMode;
  uint32_t tot = masterRx + masterGaps;
  hb.dl_pdr_x1000 = tot ? (uint16_t)((uint64_t)masterRx * 1000 / tot) : 0;
  hb.dl_rssi = masterRssi;
  hb.supply_mv = (int16_t)(csV * 1000.0f);
  hb.supply_ma = (int16_t)csMa;
  hb.supply_good = csGood ? 1 : 0;
  hb.lux_x10 = gEnvTsl ? gEnvLuxX10 : 0xFFFFFFFF;
  hb.light_ch0 = gEnvCh0;
  hb.light_ch1 = gEnvCh1;
  hb.ptemp_cx10 = gEnvPtempX10;
  hb.prh_pct = gEnvRh;
  hb.btemp_cx10 = gEnvBtempX10;
  hb.ina_pv_mv = gInaPvMv;
  hb.ina_pa_ma = gInaPaMa;
  hb.ina_bv_mv = gInaBvMv;
  hb.ina_ba_ma = gInaBaMa;
  hb.cfg_cap_mah = gBatteryCapacityMah;
  hb.cfg_charge_ma = gChargeMa;
  hb.drawdown_mah_x10 = (uint16_t)min(65535UL, (unsigned long)(gDrawdownMah * 10.0f + 0.5f));
  hb.drawdown_budget_mah = gDrawdownBudgetMah;
  hb.drawdown_active = gDrawdownActive ? 1 : 0;
  memset(hb.fw_rev, 0, sizeof(hb.fw_rev));
  strncpy(hb.fw_rev, NET_BENCH_VERSION, sizeof(hb.fw_rev) - 1);
  hb.maint_status = gMaintStatus;
  hb.field_phase = rtcFieldPhase;
  hb.field_reason = rtcFieldReason;
  hb.field_cycle = rtcFieldCycle;
  hb.field_elapsed_s = (uint16_t)min(65535UL, rtcFieldPhaseElapsedS);
  hb.field_charge_mah = (uint16_t)min(65535UL, rtcFieldChargeMas / 3600UL);
  hb.field_discharge_mah = (uint16_t)min(65535UL, rtcFieldDischargeMas / 3600UL);
  hb.field_min_mv = rtcFieldMinMv;
  hb.field_max_mv = rtcFieldMaxMv;
  hb.bq_vindpm_mv = gBqVindpmMv;
  hb.bq_ichg_ma = gBqIchgMa;
  hb.bq_vreg_mv = gBqVregMv;
  hb.bq_reg16 = gBqReg16;
  hb.bq_reg18 = gBqReg18;
  hb.bq_stat0 = gBqStat0;
  hb.bq_stat1 = gBqStat1;
  hb.bq_fault0 = gBqFault0;
  hb.bq_flag0 = gBqFlag0;
  hb.bq_flag1 = gBqFlag1;
  hb.bq_fault_flag0 = gBqFaultFlag0;
  hb.bq_part = gBqPart;
  esp_now_send(BCAST, (uint8_t *)&hb, sizeof(hb));
}
void sendCmd(uint8_t type, uint8_t arg) {
  NbCmd c;
  fillHeader(&c.h, type);
  c.arg = arg;
  for (int i = 0; i < 4; i++) { esp_now_send(BCAST, (uint8_t *)&c, sizeof(c)); delay(5); } // repeat (unacked)
}
void sendSetU16(uint8_t type, uint16_t value) {
  NbSetU16 c;
  fillHeader(&c.h, type);
  c.value = value;
  for (int i = 0; i < 4; i++) { esp_now_send(BCAST, (uint8_t *)&c, sizeof(c)); delay(5); }
}
void sendTargetU16(uint8_t type, const uint8_t target[3], uint16_t value) {
  NbTargetU16 c;
  fillHeader(&c.h, type);
  memcpy(c.target_id, target, 3);
  c.value = value;
  for (int i = 0; i < 6; i++) { esp_now_send(BCAST, (uint8_t *)&c, sizeof(c)); delay(8); }
}
void sendTargetCmd(uint8_t type, const uint8_t target[3], uint8_t arg) {
  NbTargetCmd c;
  fillHeader(&c.h, type);
  memcpy(c.target_id, target, 3);
  c.arg = arg;
  for (int i = 0; i < 6; i++) { esp_now_send(BCAST, (uint8_t *)&c, sizeof(c)); delay(8); }
}
bool targetMatchesMe(const uint8_t target[3]) {
  bool all = (target[0] == 0 && target[1] == 0 && target[2] == 0);
  return all || memcmp(target, myId, 3) == 0;
}
uint16_t maintenanceBatteryFloorMv() {
  return (RES_PF_BATTERY_TYPE == Mainboard::BatteryType::Generic_LFP)
             ? NB_MAINT_MIN_LFP_MV
             : NB_MAINT_MIN_3V7_MV;
}
bool maintenancePowerOk() {
  if (IS_MASTER || !pfReady) return true;

  readBattery();
  esp_task_wdt_reset();
  int battMv = (int)(cbV * 1000.0f + 0.5f);
  uint16_t floorMv = maintenanceBatteryFloorMv();
  bool batteryKnown = battMv > 500;
  bool batteryOk = !batteryKnown || battMv >= (int)floorMv;
  bool supplyOk = csGood && csMa >= (float)NB_MAINT_SUPPLY_OVERRIDE_MA;
  if (batteryOk || supplyOk) return true;

  gMaintStatus = MAINT_STATUS_POWER_WARN;
  uint32_t now = millis();
  if (!gLastMaintRefuseMs || now - gLastMaintRefuseMs >= NB_MAINT_REFUSE_REPORT_MS) {
    gLastMaintRefuseMs = now;
    Serial.printf("maintenance power warning: bv=%dmV floor=%umV sv=%.3fV sma=%.0fmA sgood=%d enforce=%d\n",
                  battMv, (unsigned)floorMv, csV, csMa, csGood ? 1 : 0,
                  NB_MAINT_POWER_ENFORCE);
    sendHeartbeat(0);
  }
#if NB_MAINT_POWER_ENFORCE
  return false;
#else
  return true;
#endif
}
void enterTimedDeepSleep(uint16_t seconds, const char *why) {
  if (seconds == 0) seconds = 1;
  digitalWrite(NB_LED_PIN, LOW);
  if (pfReady) {
    Board.enable3V3(false);
    Board.enableVSQT(false);
  }
  Serial.printf("deep sleep (%s), timer wake %us\n", why, (unsigned)seconds);
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
  esp_deep_sleep_start();
}
void drawdownPixelsBegin() {
  if (gDrawdownPixelsBegun) return;
  drawdownPixels.begin();
  drawdownPixels.setBrightness(255);
  drawdownPixels.clear();
  drawdownPixels.show();
  gDrawdownPixelsBegun = true;
}
void drawdownPixelsOff() {
  drawdownPixelsBegin();
  drawdownPixels.setBrightness(255);
  drawdownPixels.clear();
  drawdownPixels.show();
  pinMode(NB_DRAWDOWN_PIXEL_PIN, OUTPUT);
  digitalWrite(NB_DRAWDOWN_PIXEL_PIN, LOW);
}
void drawdownPixelsLoadOn() {
  if (pfReady) Board.enable3V3(true);
  delay(20);
  drawdownPixelsBegin();
  drawdownPixels.setBrightness(NB_DRAWDOWN_BRIGHTNESS);
  uint32_t c = drawdownPixels.Color(NB_DRAWDOWN_R, NB_DRAWDOWN_G, NB_DRAWDOWN_B);
  for (uint16_t i = 0; i < NB_DRAWDOWN_PIXEL_COUNT; i++) drawdownPixels.setPixelColor(i, c);
  drawdownPixels.show();
}
void drawdownCancel(const char *why) {
  if (!gDrawdownActive) return;
  gDrawdownActive = false;
  drawdownPixelsOff();
  Serial.printf("drawdown canceled (%s), %.1f mAh delivered\n", why, gDrawdownMah);
}
void drawdownStopAndSleep(const char *why) {
  if (!gDrawdownActive) return;
  readBattery();
  Serial.printf("drawdown stop: %s; delivered %.1f / %u mAh, bv=%.3f ima=%.0f -> sleep %us\n",
                why, gDrawdownMah, gDrawdownBudgetMah, cbV, cbMa,
                (unsigned)NB_DRAWDOWN_SLEEP_S);
  for (int i = 0; i < 3; i++) { sendHeartbeat(0); delay(40); }
  gDrawdownActive = false;
  drawdownPixelsOff();
  delay(40);
  enterTimedDeepSleep((uint16_t)min((unsigned long)NB_DRAWDOWN_SLEEP_S, 65535UL), why);
}
void drawdownStart(uint16_t budgetMah) {
  if (budgetMah == 0) {
    drawdownCancel("remote-zero");
    return;
  }
  readBattery();
  if (cbV > 0 && cbV * 1000.0f <= NB_DRAWDOWN_HARD_FLOOR_MV) {
    Serial.printf("drawdown rejected: bv %.3f <= hard floor %.3f\n",
                  cbV, NB_DRAWDOWN_HARD_FLOOR_MV / 1000.0f);
    drawdownPixelsOff();
    return;
  }
  gDrawdownBudgetMah = budgetMah;
  gDrawdownMah = 0.0f;
  gDrawdownStartMs = millis();
  gDrawdownLastMs = gDrawdownStartMs;
  gDrawdownLastLogMs = 0;
  gDrawdownActive = true;
  drawdownPixelsLoadOn();
  Serial.printf("drawdown start: budget=%u mAh load=HEX %upx rgb=(%u,%u,%u) bri=%u soft=%.3f hard=%.3f\n",
                gDrawdownBudgetMah, NB_DRAWDOWN_PIXEL_COUNT, NB_DRAWDOWN_R,
                NB_DRAWDOWN_G, NB_DRAWDOWN_B, NB_DRAWDOWN_BRIGHTNESS,
                NB_DRAWDOWN_SOFT_FLOOR_MV / 1000.0f, NB_DRAWDOWN_HARD_FLOOR_MV / 1000.0f);
}
void drawdownTick() {
  if (!gDrawdownActive) return;
  uint32_t now = millis();
  if (gDrawdownLastMs == 0) gDrawdownLastMs = now;
  uint32_t dtMs = now - gDrawdownLastMs;
  if (dtMs >= 250) {
    float dischargeMa = cbMa < 0 ? -cbMa : 0.0f;
    gDrawdownMah += dischargeMa * (float)dtMs / 3600000.0f;
    gDrawdownLastMs = now;
  }
  uint32_t elapsedS = (now - gDrawdownStartMs) / 1000;
  if (now - gDrawdownLastLogMs > 30000) {
    gDrawdownLastLogMs = now;
    Serial.printf("drawdown: %.1f/%u mAh elapsed=%lus bv=%.3f ima=%.0f soc=%d\n",
                  gDrawdownMah, gDrawdownBudgetMah, (unsigned long)elapsedS,
                  cbV, cbMa, cbSoc);
  }
  if (cbV > 0 && cbV * 1000.0f <= NB_DRAWDOWN_HARD_FLOOR_MV) {
    drawdownStopAndSleep("hard-floor");
  } else if (elapsedS >= NB_DRAWDOWN_MIN_RUN_S &&
             cbV > 0 && cbV * 1000.0f <= NB_DRAWDOWN_SOFT_FLOOR_MV) {
    drawdownStopAndSleep("soft-floor");
  } else if (gDrawdownMah >= gDrawdownBudgetMah) {
    drawdownStopAndSleep("budget");
  }
}

#ifdef NB_FIELD_CYCLE
void fieldCycleMarkSample() {
  if (cbV < 0.5f) return;
  uint16_t mv = (uint16_t)min(65535UL, (unsigned long)(cbV * 1000.0f + 0.5f));
  if (rtcFieldMinMv == 0 || mv < rtcFieldMinMv) rtcFieldMinMv = mv;
  if (mv > rtcFieldMaxMv) rtcFieldMaxMv = mv;
}
void fieldCycleIntegrateSeconds(uint32_t seconds) {
  if (seconds == 0) return;
  fieldCycleMarkSample();
  rtcFieldPhaseElapsedS = min(65535UL, rtcFieldPhaseElapsedS + seconds);
  float ma = cbMa;
  if (ma > 0.5f) {
    uint64_t add = (uint64_t)(ma + 0.5f) * seconds;
    uint64_t next = (uint64_t)rtcFieldChargeMas + add;
    rtcFieldChargeMas = (uint32_t)min(next, (uint64_t)65535UL * 3600UL);
  } else if (ma < -0.5f) {
    uint64_t add = (uint64_t)((-ma) + 0.5f) * seconds;
    uint64_t next = (uint64_t)rtcFieldDischargeMas + add;
    rtcFieldDischargeMas = (uint32_t)min(next, (uint64_t)65535UL * 3600UL);
  }
}
void fieldCycleIntegrateActive() {
  uint32_t now = millis();
  if (fieldLastIntegrateMs == 0) {
    fieldLastIntegrateMs = now;
    fieldCycleMarkSample();
    return;
  }
  uint32_t dt = now - fieldLastIntegrateMs;
  if (dt < 1000) return;
  fieldLastIntegrateMs = now;
  fieldCycleIntegrateSeconds(dt / 1000);
}
void fieldCycleResetCounters() {
  rtcFieldPhaseElapsedS = 0;
  rtcFieldChargeMas = 0;
  rtcFieldDischargeMas = 0;
  rtcFieldMinMv = 0;
  rtcFieldMaxMv = 0;
  fieldCycleMarkSample();
}
void fieldCycleSetPhase(uint8_t phase, uint8_t reason) {
  if (rtcFieldPhase == phase) {
    rtcFieldReason = reason;
    return;
  }
  rtcFieldPhase = phase;
  rtcFieldReason = reason;
  rtcFieldPhaseElapsedS = 0;
  fieldLastIntegrateMs = millis();
  Serial.printf("field-cycle phase -> %u reason=%u cycle=%u bv=%.3f ima=%.0f sv=%.3f sma=%.0f sgood=%d\n",
                rtcFieldPhase, rtcFieldReason, rtcFieldCycle, cbV, cbMa, csV, csMa,
                csGood ? 1 : 0);
}
void fieldCycleStartNewCycle(uint8_t reason) {
  if (rtcFieldCycle < 65535) rtcFieldCycle++;
  fieldCycleResetCounters();
  fieldCycleSetPhase(FC_CHARGE, reason);
}
bool fieldCycleSupplyPresent() {
  return csGood || csMa >= (float)NB_FIELD_SUN_CHARGE_MA ||
         csV >= (float)NB_FIELD_SUN_SUPPLY_MV / 1000.0f;
}
bool fieldCycleFullEnough() {
  int mv = (int)(cbV * 1000.0f + 0.5f);
  return fieldCycleSupplyPresent() && mv >= NB_FIELD_FULL_MV &&
         cbMa >= 0.0f && cbMa <= (float)NB_FIELD_FULL_TAPER_MA;
}
bool fieldCycleLow() {
  int mv = (int)(cbV * 1000.0f + 0.5f);
  return cbV > 0.5f && mv <= NB_FIELD_LOW_MV;
}
bool fieldCycleCritical() {
  int mv = (int)(cbV * 1000.0f + 0.5f);
  return cbV > 0.5f && mv <= NB_FIELD_CRITICAL_MV;
}
bool fieldCycleInWakeWindow() {
  uint32_t window = (esp_reset_reason() == ESP_RST_POWERON) ? NB_FIELD_COLD_LISTEN_MS : NB_FIELD_WAKE_LISTEN_MS;
  return millis() < window;
}
void fieldCycleSleep(uint16_t seconds, const char *why, uint8_t reason, bool integrateSleepCurrent) {
  rtcFieldReason = reason;
  if (integrateSleepCurrent) fieldCycleIntegrateSeconds(seconds);
  drawdownPixelsOff();
  for (int i = 0; i < 3; i++) {
    sendHeartbeat(0);
    delay(40);
    esp_task_wdt_reset();
  }
  enterTimedDeepSleep(seconds, why);
}
void fieldCycleInit() {
  if (IS_MASTER) return;
  if (rtcFieldMagic != NB_FIELD_RTC_MAGIC || rtcFieldCycle == 0) {
    rtcFieldMagic = NB_FIELD_RTC_MAGIC;
    rtcFieldPhase = FC_BOOT;
    rtcFieldReason = FC_REASON_BOOT;
    rtcFieldCycle = 1;
    fieldCycleResetCounters();
  }
  bool supply = fieldCycleSupplyPresent();
  if (supply) {
    if (rtcFieldPhase == FC_OFF || rtcFieldPhase == FC_BOOT || rtcFieldPhase == FC_PROTECT)
      fieldCycleSetPhase(FC_CHARGE, FC_REASON_SUPPLY);
  } else if (fieldCycleLow()) {
    fieldCycleSetPhase(FC_PROTECT, fieldCycleCritical() ? FC_REASON_CRITICAL : FC_REASON_LOW);
  } else if (rtcFieldPhase == FC_OFF || rtcFieldPhase == FC_BOOT || rtcFieldPhase == FC_CHARGE || rtcFieldPhase == FC_WAIT_DARK) {
    fieldCycleSetPhase(FC_DRAWDOWN, FC_REASON_DARK);
  }
}
void fieldCycleTick() {
  if (IS_MASTER || !pfReady || gMode != MODE_COMMS) return;

  readBattery();
  fieldCycleIntegrateActive();
  bool supply = fieldCycleSupplyPresent();
  bool critical = fieldCycleCritical();
  bool low = fieldCycleLow();

  if (supply && (rtcFieldPhase == FC_PROTECT || rtcFieldPhase == FC_DRAWDOWN)) {
    fieldCycleStartNewCycle(FC_REASON_SUNRISE);
  } else if (supply && (rtcFieldPhase == FC_OFF || rtcFieldPhase == FC_BOOT)) {
    fieldCycleSetPhase(FC_CHARGE, FC_REASON_SUPPLY);
  }

  if (!supply && critical) {
    fieldCycleSetPhase(FC_PROTECT, FC_REASON_CRITICAL);
  }

  switch (rtcFieldPhase) {
  case FC_CHARGE:
    if (!supply) {
      if (low) fieldCycleSetPhase(FC_PROTECT, critical ? FC_REASON_CRITICAL : FC_REASON_LOW);
      else fieldCycleSetPhase(FC_DRAWDOWN, FC_REASON_DARK);
      break;
    }
    if (fieldCycleFullEnough()) {
      fieldCycleSetPhase(FC_WAIT_DARK, FC_REASON_FULL);
      break;
    }
    if (!fieldCycleInWakeWindow())
      fieldCycleSleep(NB_FIELD_CHARGE_SLEEP_S, "field-charge", FC_REASON_SUPPLY, true);
    break;

  case FC_WAIT_DARK:
    if (!supply) {
      if (low) fieldCycleSetPhase(FC_PROTECT, critical ? FC_REASON_CRITICAL : FC_REASON_LOW);
      else fieldCycleSetPhase(FC_DRAWDOWN, FC_REASON_DARK);
      break;
    }
    if (!fieldCycleInWakeWindow())
      fieldCycleSleep(NB_FIELD_WAIT_SLEEP_S, "field-wait-dark", FC_REASON_FULL, true);
    break;

  case FC_DRAWDOWN:
    if (supply) {
      fieldCycleStartNewCycle(FC_REASON_SUNRISE);
      break;
    }
    if (critical || (low && rtcFieldPhaseElapsedS >= NB_FIELD_DRAWDOWN_MIN_S)) {
      fieldCycleSetPhase(FC_PROTECT, critical ? FC_REASON_CRITICAL : FC_REASON_LOW);
      fieldCycleSleep(NB_FIELD_PROTECT_SLEEP_S, "field-protect", rtcFieldReason, false);
    }
    break;

  case FC_PROTECT:
    if (supply) {
      fieldCycleStartNewCycle(FC_REASON_SUPPLY);
      break;
    }
    if (!fieldCycleInWakeWindow())
      fieldCycleSleep(NB_FIELD_PROTECT_SLEEP_S, "field-protect", rtcFieldReason, false);
    break;

  default:
    if (supply) fieldCycleSetPhase(FC_CHARGE, FC_REASON_SUPPLY);
    else if (low) fieldCycleSetPhase(FC_PROTECT, critical ? FC_REASON_CRITICAL : FC_REASON_LOW);
    else fieldCycleSetPhase(FC_DRAWDOWN, FC_REASON_DARK);
    break;
  }
}
#endif

void sendIdentify(const uint8_t target[3], uint8_t secs) {
  NbIdentify m;
  fillHeader(&m.h, NB_IDENTIFY);
  memcpy(m.target_id, target, 3);
  m.secs = secs;
  for (int i = 0; i < 4; i++) { esp_now_send(BCAST, (uint8_t *)&m, sizeof(m)); delay(5); }
}
// Broadcast one scanned AP (rank within a batch). scanIdx indexes the live scan
// results (valid until WiFi.scanDelete()).
void sendScanAp(uint8_t scanId, uint8_t rank, uint8_t count, int scanIdx) {
  NbScanAp p;
  fillHeader(&p.h, NB_SCANAP);
  p.scan_id = scanId;
  p.idx = rank;
  p.count = count;
  uint8_t *b = WiFi.BSSID(scanIdx);
  if (b) memcpy(p.bssid, b, 6); else memset(p.bssid, 0, 6);
  p.ap_rssi = (int8_t)WiFi.RSSI(scanIdx);
  p.channel = (uint8_t)WiFi.channel(scanIdx);
  p.enc = (uint8_t)WiFi.encryptionType(scanIdx);
  memset(p.ssid, 0, sizeof(p.ssid));
  strncpy(p.ssid, WiFi.SSID(scanIdx).c_str(), sizeof(p.ssid) - 1);
  esp_now_send(BCAST, (uint8_t *)&p, sizeof(p));
}

// ---- mode transitions ------------------------------------------------------
void enterComms();
bool enterMaintenance() {
  if (!maintenancePowerOk()) return false;
  Serial.println("-> MAINTENANCE (WiFi OTA)");
  espNowDeinit();
  gMode = MODE_MAINT;
  maintEnteredMs = millis();
  if (!startWifiOta()) {
    gMaintStatus = MAINT_STATUS_START_FAILED;
    Serial.println("maintenance startup failed -> resume comms");
    enterComms();
    return false;
  }
  gMaintStatus = MAINT_STATUS_ACTIVE;
  return true;
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
  if (IS_MASTER && SERIAL_BRIDGE) {
    // Desk serial bridge: do NOT join WiFi. Stay STA-unassociated so the radio
    // is freely pinned to NB_CHANNEL (no Eero-channel coupling); relay to USB.
    WiFi.disconnect();
    Serial.println("serial-bridge master: WiFi OFF, pure ESP-NOW on NB_CHANNEL -> USB");
  } else if (IS_MASTER) {
#if RES_HAS_WIFI_SECRETS
    // Master joins the bench AP (on NB_CHANNEL) to bridge stats + serve /telemetry.
    WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
      esp_task_wdt_reset();
      delay(250);
      Serial.print(".");
    }
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

// ---- master -> host bridge (UDP and/or USB serial) -------------------------
// A WiFi-STA master broadcasts to UDP:54321 (existing host tooling); a
// -DNB_SERIAL_BRIDGE master (no WiFi needed) also prints to USB serial so a
// desk-tethered board logs the field fleet with no laptop in the yard. Both
// emit the SAME nb-* line format, so net_bench_log.py works from either source.
void emitBridge(const char *line, int n) {
  if (SERIAL_BRIDGE) Serial.write((const uint8_t *)line, n);
  if (WiFi.status() == WL_CONNECTED) {
    bridgeUdp.beginPacket(IPAddress(255, 255, 255, 255), 54321);
    bridgeUdp.write((uint8_t *)line, n);
    bridgeUdp.endPacket();
  }
}

void bridgeStats() {
  if (!IS_MASTER) return;
  char line[512]; // base peer line + optional env/INA/config/drawdown/fw tails
  // self line (report the LOCKED channel; WiFi.channel() reads 0 when unassociated)
  int n = snprintf(line, sizeof(line),
                   "nb-master id=%02X%02X%02X ch=%d frames=%lu sendok=%lu sendfail=%lu up=%lu bv=%.3f fw=%s\n",
                   myId[0], myId[1], myId[2], NB_CHANNEL, (unsigned long)txSeq,
                   (unsigned long)sendOk, (unsigned long)sendFail, (unsigned long)millis(), cbV,
                   NET_BENCH_VERSION);
  emitBridge(line, n);
  // per-peer lines
  for (int i = 0; i < NB_MAX_TRACKED; i++) {
    NbPeerStat *p = &peers[i];
    if (!p->used) continue;
    uint32_t tot = p->recv + p->gaps;
    float pdr = tot ? (float)p->recv / (float)tot : 0.0f;
    n = snprintf(line, sizeof(line),
                 "nb-peer id=%02X%02X%02X seq=%lu rx=%lu gaps=%lu pdr=%.4f rssi=%d bv=%.3f ima=%d soc=%d rr=%s ca=%d mode=%d dlpdr=%.3f dlrssi=%d up=%lu age=%lu sv=%.3f sma=%d sgood=%d",
                 p->id[0], p->id[1], p->id[2], (unsigned long)p->last_seq, (unsigned long)p->recv,
                 (unsigned long)p->gaps, pdr, p->rssi, p->batt_mv / 1000.0f, p->batt_ma,
                 (p->soc == 255 ? -1 : p->soc), resetReasonName((esp_reset_reason_t)p->rr), p->ca,
                 p->pmode, p->dl_pdr_x1000 / 1000.0f, p->dl_rssi, (unsigned long)p->up,
                 (unsigned long)(millis() - p->last_heard_ms),
                 p->supply_mv / 1000.0f, p->supply_ma, p->supply_good);
    if (p->has_env && n < (int)sizeof(line)) { // env tail: lux/ptc/btc "nan" = absent, lux "sat" = saturated
      char luxs[16];
      if (p->lux_x10 == 0xFFFFFFFF) snprintf(luxs, sizeof(luxs), "nan");
      else if (p->lux_x10 == 0xFFFFFFFE) snprintf(luxs, sizeof(luxs), "sat");
      else snprintf(luxs, sizeof(luxs), "%.1f", p->lux_x10 / 10.0f);
      char ptcs[12], btcs[12];
      if (p->ptemp_cx10 == INT16_MIN) snprintf(ptcs, sizeof(ptcs), "nan");
      else snprintf(ptcs, sizeof(ptcs), "%.1f", p->ptemp_cx10 / 10.0f);
      if (p->btemp_cx10 == INT16_MIN) snprintf(btcs, sizeof(btcs), "nan");
      else snprintf(btcs, sizeof(btcs), "%.1f", p->btemp_cx10 / 10.0f);
      n += snprintf(line + n, sizeof(line) - n, " lux=%s ch0=%u ch1=%u ptc=%s prh=%d btc=%s",
                    luxs, p->light_ch0, p->light_ch1, ptcs,
                    (p->prh_pct == 255 ? -1 : (int)p->prh_pct), btcs);
    }
    if (p->has_ina && n < (int)sizeof(line)) { // onboard INA tail: -32768 = channel absent
      n += snprintf(line + n, sizeof(line) - n, " ipv=%d ipa=%d ibv=%d iba=%d",
                    p->ina_pv_mv, p->ina_pa_ma, p->ina_bv_mv, p->ina_ba_ma);
    }
    if (p->has_cfg && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n, " cap=%u chg=%u",
                    p->cfg_cap_mah, p->cfg_charge_ma);
    }
    if (p->has_drawdown && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n, " dd=%.1f ddb=%u dda=%u",
                    p->drawdown_mah_x10 / 10.0f, p->drawdown_budget_mah,
                    p->drawdown_active);
    }
    if (p->has_fw && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n, " fw=%s", p->fw_rev);
    }
    if (p->has_maint_status && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n, " mt=%u", p->maint_status);
    }
    if (p->has_field_cycle && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n,
                    " fc=%u fcr=%u fcc=%u fce=%u fcchg=%u fcdis=%u fcmin=%u fcmax=%u",
                    p->field_phase, p->field_reason, p->field_cycle, p->field_elapsed_s,
                    p->field_charge_mah, p->field_discharge_mah,
                    p->field_min_mv, p->field_max_mv);
    }
    if (p->has_bq && n < (int)sizeof(line)) {
      n += snprintf(line + n, sizeof(line) - n,
                    " bqv=%u bqichg=%u bqvreg=%u bq16=%02X bq18=%02X bq1d=%02X bq1e=%02X bq1f=%02X bq20=%02X bq21=%02X bq22=%02X bq38=%02X",
                    p->bq_vindpm_mv, p->bq_ichg_ma, p->bq_vreg_mv,
                    p->bq_reg16, p->bq_reg18, p->bq_stat0, p->bq_stat1, p->bq_fault0,
                    p->bq_flag0, p->bq_flag1, p->bq_fault_flag0, p->bq_part);
    }
    if (n < (int)sizeof(line) - 1) { line[n++] = '\n'; line[n] = '\0'; }
    emitBridge(line, n);
  }
}

// Field scan-report: a -DNB_SCAN_REPORT peer async-scans 2.4 GHz (never
// associates), then broadcasts up to NB_SCAN_MAX APs (strongest first) as
// NB_SCANAP packets. The radio is re-pinned to NB_CHANNEL after each scan so
// ESP-NOW keeps reaching the bridge. gScanRunning suppresses TX while hopping.
void scanReportTick() {
  uint32_t now = millis();
  if (!gScanRunning) {
    if (now < gNextScanMs) return;
    WiFi.scanNetworks(/*async*/ true, /*show_hidden*/ false);
    gScanRunning = true;
    return;
  }
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return;
  esp_wifi_set_channel(NB_CHANNEL, WIFI_SECOND_CHAN_NONE); // re-pin for ESP-NOW
  if (n > 0) {
    uint8_t count = (n > 255) ? 255 : (uint8_t)n;
    uint8_t sendN = count < NB_SCAN_MAX ? count : NB_SCAN_MAX;
    // rank the strongest sendN APs by RSSI (selection over a small list)
    uint8_t order[NB_SCAN_MAX];
    bool taken[64] = {false}; // n is small; guard the index
    int cap = (n < 64) ? n : 64;
    for (uint8_t k = 0; k < sendN && k < cap; k++) {
      int best = -1;
      for (int j = 0; j < cap; j++)
        if (!taken[j] && (best < 0 || WiFi.RSSI(j) > WiFi.RSSI(best))) best = j;
      if (best < 0) break;
      taken[best] = true;
      order[k] = (uint8_t)best;
    }
    for (uint8_t k = 0; k < sendN && k < cap; k++) {
      sendScanAp(gScanId, k, count, order[k]);
      delay(4); // pace the burst (unacked broadcast)
    }
    Serial.printf("scan id=%u found=%d sent=%u\n", gScanId, n, sendN);
    gScanId++;
  } else {
    Serial.printf("scan id=%u failed (%d)\n", gScanId, n);
  }
  WiFi.scanDelete();
  gScanRunning = false;
  gNextScanMs = now + (uint32_t)NB_SCAN_S * 1000;
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
    } else if (h->type == NB_HEARTBEAT && it.len >= (int)offsetof(NbHeartbeat, supply_mv)) {
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
        if (NB_HAS_HB_FIELD(it.len, supply_good)) { // supply-capable peer (append-only tail 1)
          p->supply_mv = hb->supply_mv; p->supply_ma = hb->supply_ma; p->supply_good = hb->supply_good;
        } else {
          p->supply_mv = 0; p->supply_ma = 0; p->supply_good = 0;
        }
        if (NB_HAS_HB_FIELD(it.len, btemp_cx10)) { // env-capable peer (append-only tail 2)
          p->has_env = true;
          p->lux_x10 = hb->lux_x10; p->light_ch0 = hb->light_ch0; p->light_ch1 = hb->light_ch1;
          p->ptemp_cx10 = hb->ptemp_cx10; p->prh_pct = hb->prh_pct; p->btemp_cx10 = hb->btemp_cx10;
        } else {
          p->has_env = false;
        }
        if (NB_HAS_HB_FIELD(it.len, ina_ba_ma)) { // onboard-INA peer (append-only tail 3)
          p->has_ina = true;
          p->ina_pv_mv = hb->ina_pv_mv; p->ina_pa_ma = hb->ina_pa_ma;
          p->ina_bv_mv = hb->ina_bv_mv; p->ina_ba_ma = hb->ina_ba_ma;
        } else {
          p->has_ina = false;
        }
        if (NB_HAS_HB_FIELD(it.len, cfg_charge_ma)) { // runtime-config tail 4
          p->has_cfg = true;
          p->cfg_cap_mah = hb->cfg_cap_mah; p->cfg_charge_ma = hb->cfg_charge_ma;
        } else {
          p->has_cfg = false;
        }
        if (NB_HAS_HB_FIELD(it.len, drawdown_active)) { // targeted drawdown tail 5
          p->has_drawdown = true;
          p->drawdown_mah_x10 = hb->drawdown_mah_x10;
          p->drawdown_budget_mah = hb->drawdown_budget_mah;
          p->drawdown_active = hb->drawdown_active;
        } else {
          p->has_drawdown = false;
        }
        if (NB_HAS_HB_FIELD(it.len, fw_rev)) { // firmware revision tail 6
          p->has_fw = true;
          memcpy(p->fw_rev, hb->fw_rev, sizeof(p->fw_rev));
          p->fw_rev[sizeof(p->fw_rev) - 1] = '\0';
        } else {
          p->has_fw = false;
          p->fw_rev[0] = '\0';
        }
        if (NB_HAS_HB_FIELD(it.len, maint_status)) { // maintenance status tail 7
          p->has_maint_status = true;
          p->maint_status = hb->maint_status;
        } else {
          p->has_maint_status = false;
          p->maint_status = MAINT_STATUS_IDLE;
        }
        if (NB_HAS_HB_FIELD(it.len, field_max_mv)) { // field-cycle lifecycle tail 8
          p->has_field_cycle = true;
          p->field_phase = hb->field_phase;
          p->field_reason = hb->field_reason;
          p->field_cycle = hb->field_cycle;
          p->field_elapsed_s = hb->field_elapsed_s;
          p->field_charge_mah = hb->field_charge_mah;
          p->field_discharge_mah = hb->field_discharge_mah;
          p->field_min_mv = hb->field_min_mv;
          p->field_max_mv = hb->field_max_mv;
        } else {
          p->has_field_cycle = false;
        }
        if (NB_HAS_HB_FIELD(it.len, bq_part)) { // BQ25628E charger truth tail 9
          p->has_bq = true;
          p->bq_vindpm_mv = hb->bq_vindpm_mv;
          p->bq_ichg_ma = hb->bq_ichg_ma;
          p->bq_vreg_mv = hb->bq_vreg_mv;
          p->bq_reg16 = hb->bq_reg16;
          p->bq_reg18 = hb->bq_reg18;
          p->bq_stat0 = hb->bq_stat0;
          p->bq_stat1 = hb->bq_stat1;
          p->bq_fault0 = hb->bq_fault0;
          p->bq_flag0 = hb->bq_flag0;
          p->bq_flag1 = hb->bq_flag1;
          p->bq_fault_flag0 = hb->bq_fault_flag0;
          p->bq_part = hb->bq_part;
        } else {
          p->has_bq = false;
        }
      }
    } else if (h->type == NB_ENTER_MAINT) {
      if (gMode == MODE_COMMS) enterMaintenance();
    } else if (h->type == NB_TARGET_ENTER_MAINT && it.len >= (int)sizeof(NbTargetCmd)) {
      NbTargetCmd *m = (NbTargetCmd *)it.data;
      if (targetMatchesMe(m->target_id) && gMode == MODE_COMMS) enterMaintenance();
    } else if (h->type == NB_RESUME) {
      if (gMode == MODE_COMMS) { /* already comms */ }
    } else if (h->type == NB_SET_RATE && it.len >= (int)sizeof(NbCmd)) {
      uint8_t hz = ((NbCmd *)it.data)->arg;
      if (hz >= 1 && hz <= 100) { gRateHz = hz; Serial.printf("rate set -> %u Hz\n", hz); }
    } else if (h->type == NB_SET_MAINTAIN && it.len >= (int)sizeof(NbCmd)) {
      // Live-set the charger VINDPM/maintain (panel MPP). arg = volts x10.
      // Lets the master sweep the setpoint or hill-climb it (P&O MPPT) with no reflash.
      uint8_t v10 = ((NbCmd *)it.data)->arg;
      if (pfReady && v10 >= NB_MAINTAIN_MIN_V10 && v10 <= NB_MAINTAIN_MAX_V10) {
        gMaintainV = (float)v10 / 10.0f;
        Board.setSupplyMaintainVoltage(gMaintainV);
        Serial.printf("VINDPM/maintain set -> %.1f V\n", gMaintainV);
      }
    } else if (h->type == NB_SET_CAPACITY && it.len >= (int)sizeof(NbSetU16)) {
      // Capacity changes affect the gauge model used by Board.init(), so store and reboot.
      persistCapacity(((NbSetU16 *)it.data)->value);
    } else if (h->type == NB_SET_CHARGE_MA && it.len >= (int)sizeof(NbSetU16)) {
      // Charge-current cap is independent from gauge capacity and can apply immediately.
      persistAndApplyChargeMa(((NbSetU16 *)it.data)->value);
    } else if (h->type == NB_SLEEP_FOR && it.len >= (int)sizeof(NbSetU16)) {
      enterTimedDeepSleep(((NbSetU16 *)it.data)->value, "remote");
    } else if (h->type == NB_DRAWDOWN && it.len >= (int)sizeof(NbTargetU16)) {
      NbTargetU16 *m = (NbTargetU16 *)it.data;
      if (targetMatchesMe(m->target_id)) drawdownStart(m->value);
    } else if (h->type == NB_TARGET_SLEEP_FOR && it.len >= (int)sizeof(NbTargetU16)) {
      NbTargetU16 *m = (NbTargetU16 *)it.data;
      if (targetMatchesMe(m->target_id)) enterTimedDeepSleep(m->value, "target-remote");
    } else if (h->type == NB_TARGET_CAPACITY && it.len >= (int)sizeof(NbTargetU16)) {
      NbTargetU16 *m = (NbTargetU16 *)it.data;
      if (targetMatchesMe(m->target_id)) persistCapacity(m->value);
    } else if (h->type == NB_TARGET_CHARGE_MA && it.len >= (int)sizeof(NbTargetU16)) {
      NbTargetU16 *m = (NbTargetU16 *)it.data;
      if (targetMatchesMe(m->target_id)) persistAndApplyChargeMa(m->value);
    } else if (h->type == NB_IDENTIFY && it.len >= (int)sizeof(NbIdentify)) {
      NbIdentify *m = (NbIdentify *)it.data;
      if (targetMatchesMe(m->target_id)) {
        identifyUntil = millis() + (uint32_t)m->secs * 1000;
        Serial.printf("IDENTIFY me (%us)\n", m->secs);
      }
    } else if (h->type == NB_SCANAP && it.len >= (int)sizeof(NbScanAp) && IS_MASTER) {
      // bridge: relay a field-scanned AP to the host (ssid LAST -- may contain spaces)
      NbScanAp *sp = (NbScanAp *)it.data;
      char ssid[21];
      memcpy(ssid, sp->ssid, 20);
      ssid[20] = 0;
      char line[160];
      int n = snprintf(line, sizeof(line),
                       "nb-scanap from=%02X%02X%02X scan=%u idx=%u count=%u "
                       "bssid=%02x:%02x:%02x:%02x:%02x:%02x ap_rssi=%d ch=%u enc=%u linkrssi=%d ssid=%s\n",
                       h->src_id[0], h->src_id[1], h->src_id[2], sp->scan_id, sp->idx, sp->count,
                       sp->bssid[0], sp->bssid[1], sp->bssid[2], sp->bssid[3], sp->bssid[4],
                       sp->bssid[5], sp->ap_rssi, sp->channel, sp->enc, it.rssi, ssid);
      emitBridge(line, n);
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

int readSerialUint(uint32_t windowMs = 70, int maxValue = 100000) {
  int value = -1;
  uint32_t digitDeadline = millis() + windowMs;
  while ((int32_t)(digitDeadline - millis()) > 0) {
    if (!Serial.available()) { delay(1); continue; }
    int p = Serial.peek();
    if (p < '0' || p > '9') break;
    Serial.read();
    value = (value < 0 ? 0 : value * 10) + (p - '0');
    if (value > maxValue) break;
  }
  return value;
}
int hexNibble(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + c - 'a';
  if (c >= 'A' && c <= 'F') return 10 + c - 'A';
  return -1;
}
bool readSerialHexId(uint8_t out[3], uint32_t windowMs = 120) {
  uint8_t nibbles[6];
  uint8_t n = 0;
  uint32_t deadline = millis() + windowMs;
  while (n < 6 && (int32_t)(deadline - millis()) > 0) {
    if (!Serial.available()) { delay(1); continue; }
    int h = hexNibble(Serial.peek());
    if (h < 0) break;
    Serial.read();
    nibbles[n++] = (uint8_t)h;
  }
  if (n != 6) return false;
  out[0] = (nibbles[0] << 4) | nibbles[1];
  out[1] = (nibbles[2] << 4) | nibbles[3];
  out[2] = (nibbles[4] << 4) | nibbles[5];
  return true;
}
void consumeOptionalSeparator() {
  uint32_t deadline = millis() + 40;
  while ((int32_t)(deadline - millis()) > 0) {
    if (!Serial.available()) { delay(1); continue; }
    int p = Serial.peek();
    if (p == ':' || p == ',' || p == '=') Serial.read();
    return;
  }
}
size_t readSerialArg(char *out, size_t outLen, uint32_t windowMs = 90) {
  size_t n = 0;
  bool saw = false;
  uint32_t deadline = millis() + windowMs;
  while ((int32_t)(deadline - millis()) > 0) {
    if (!Serial.available()) { delay(1); continue; }
    int p = Serial.peek();
    if (p == '\r' || p == '\n' || p == ' ' || p == '\t') {
      Serial.read();
      if (saw) break;
      continue;
    }
    Serial.read();
    saw = true;
    if (n + 1 < outLen) out[n++] = (char)p;
    deadline = millis() + 20;
  }
  if (outLen > 0) out[n] = 0;
  return n;
}
bool parseHexIdText(const char *s, uint8_t out[3]) {
  if (strlen(s) != 6) return false;
  uint8_t nibbles[6];
  for (int i = 0; i < 6; i++) {
    int h = hexNibble(s[i]);
    if (h < 0) return false;
    nibbles[i] = (uint8_t)h;
  }
  out[0] = (nibbles[0] << 4) | nibbles[1];
  out[1] = (nibbles[2] << 4) | nibbles[3];
  out[2] = (nibbles[4] << 4) | nibbles[5];
  return true;
}
bool parseUint16Text(const char *s, uint16_t minValue, uint16_t maxValue, uint16_t *out) {
  if (!s || !*s) return false;
  uint32_t value = 0;
  for (const char *p = s; *p; p++) {
    if (*p < '0' || *p > '9') return false;
    value = value * 10 + (uint32_t)(*p - '0');
    if (value > maxValue) return false;
  }
  if (value < minValue) return false;
  *out = (uint16_t)value;
  return true;
}
bool parseTargetU16Arg(const char *arg, uint8_t target[3], bool *haveTarget,
                       uint16_t minValue, uint16_t maxValue, uint16_t *value) {
  *haveTarget = false;
  const char *sep = strchr(arg, ':');
  if (!sep) sep = strchr(arg, ',');
  if (!sep) sep = strchr(arg, '=');
  if (!sep) return parseUint16Text(arg, minValue, maxValue, value);

  char id[7];
  size_t idLen = (size_t)(sep - arg);
  if (idLen != 6) return false;
  memcpy(id, arg, 6);
  id[6] = 0;
  if (!parseHexIdText(id, target)) return false;
  if (!parseUint16Text(sep + 1, minValue, maxValue, value)) return false;
  *haveTarget = true;
  return true;
}
int peerCount(uint8_t first[3]) {
  int count = 0;
  for (int i = 0; i < NB_MAX_TRACKED; i++) {
    if (!peers[i].used) continue;
    if (count == 0 && first) memcpy(first, peers[i].id, 3);
    count++;
  }
  return count;
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
  case 'U': // master: SUSTAINED ENTER_MAINT (~35s) to catch a SLEEPING (sleep-cycle) peer's
            // brief wake window -- the fleet wake-for-maintenance primitive. Master stays in
            // comms (keeps bridging); only the peer joins WiFi. Sustain must exceed the sleep period.
    if (IS_MASTER) {
      uint8_t target[3] = {0, 0, 0};
      bool haveTarget = readSerialHexId(target, 120);
      if (haveTarget)
        Serial.printf("sustained TARGET_ENTER_MAINT %02X%02X%02X 35s (catching wake)...\n",
                      target[0], target[1], target[2]);
      else
        Serial.println("sustained ENTER_MAINT 35s (catching a sleeping peer's wake)...");
      uint32_t endMs = millis() + 35000;
      while ((int32_t)(endMs - millis()) > 0) {
        if (haveTarget) {
          NbTargetCmd tc;
          fillHeader(&tc.h, NB_TARGET_ENTER_MAINT);
          memcpy(tc.target_id, target, 3);
          tc.arg = 0;
          esp_now_send(BCAST, (uint8_t *)&tc, sizeof(tc));
        } else {
          NbCmd c; fillHeader(&c.h, NB_ENTER_MAINT); c.arg = 0;
          esp_now_send(BCAST, (uint8_t *)&c, sizeof(c));
        }
        esp_task_wdt_reset();
        delay(100);
      }
      Serial.println("sustained ENTER_MAINT done -> sweep for the peer + OTA");
    }
    break;
  case 'c': // master: resume
    if (IS_MASTER && gMode == MODE_COMMS) { Serial.println("broadcast RESUME"); sendCmd(NB_RESUME, 0); }
    else if (gMode == MODE_MAINT) { gMaintStatus = MAINT_STATUS_RESUMED; enterComms(); }
    break;
  case '+':
    if (IS_MASTER) { if (rateIdx < 5) rateIdx++; gRateHz = rates[rateIdx]; sendCmd(NB_SET_RATE, gRateHz); Serial.printf("rate -> %u Hz\n", gRateHz); }
    break;
  case '-':
    if (IS_MASTER) { if (rateIdx > 0) rateIdx--; gRateHz = rates[rateIdx]; sendCmd(NB_SET_RATE, gRateHz); Serial.printf("rate -> %u Hz\n", gRateHz); }
    break;
  case 'R': { // direct heartbeat/frame rate: R1, R2, R5...R100
    int hz = readSerialUint(80, 100);
    if (hz < 0) {
      Serial.printf("rate=%u Hz\n", gRateHz);
      break;
    }
    if (hz < 1 || hz > 100) {
      Serial.printf("SET_RATE %d rejected (range 1..100 Hz)\n", hz);
      break;
    }
    gRateHz = (uint8_t)hz;
    for (uint8_t i = 0; i < (uint8_t)(sizeof(rates)); i++) {
      if (rates[i] <= gRateHz) rateIdx = i;
    }
    if (IS_MASTER) {
      sendCmd(NB_SET_RATE, gRateHz);
      Serial.printf("broadcast SET_RATE %u Hz\n", gRateHz);
    } else {
      Serial.printf("rate set -> %u Hz\n", gRateHz);
    }
    break;
  }
  case 'C': { // capacity mAh: C6000 broadcasts; C9E5AB8:6000 targets one peer.
    char arg[24];
    uint8_t target[3] = {0, 0, 0};
    bool haveTarget = false;
    uint16_t mah = 0;
    if (readSerialArg(arg, sizeof(arg), 100) == 0) {
      Serial.printf("config: cap=%u mAh charge=%u mA\n", gBatteryCapacityMah, gChargeMa);
      break;
    }
    if (!parseTargetU16Arg(arg, target, &haveTarget,
                           NB_CAPACITY_MIN_MAH, NB_CAPACITY_MAX_MAH, &mah)) {
      Serial.printf("SET_CAPACITY rejected: use C<mah> or C<id>:<mah> (range %u..%u mAh)\n",
                    (unsigned)NB_CAPACITY_MIN_MAH, (unsigned)NB_CAPACITY_MAX_MAH);
      break;
    }
    if (IS_MASTER) {
      if (haveTarget) {
        sendTargetU16(NB_TARGET_CAPACITY, target, mah);
        Serial.printf("target SET_CAPACITY %02X%02X%02X %u mAh (peer reboots to apply)\n",
                      target[0], target[1], target[2], (unsigned)mah);
      } else {
        sendSetU16(NB_SET_CAPACITY, mah);
        Serial.printf("broadcast SET_CAPACITY %u mAh (peers reboot to apply)\n", (unsigned)mah);
      }
    } else {
      if (haveTarget && !targetMatchesMe(target)) break;
      persistCapacity(mah);
    }
    break;
  }
  case 'G': { // charge cap mA: G1500 broadcasts; G9E5AB8:1500 targets one peer.
    char arg[24];
    uint8_t target[3] = {0, 0, 0};
    bool haveTarget = false;
    uint16_t ma = 0;
    if (readSerialArg(arg, sizeof(arg), 100) == 0) {
      Serial.printf("config: cap=%u mAh charge=%u mA\n", gBatteryCapacityMah, gChargeMa);
      break;
    }
    if (!parseTargetU16Arg(arg, target, &haveTarget,
                           NB_CHARGE_MIN_MA, NB_CHARGE_MAX_MA, &ma)) {
      Serial.printf("SET_CHARGE_MA rejected: use G<mA> or G<id>:<mA> (range %u..%u mA)\n",
                    (unsigned)NB_CHARGE_MIN_MA, (unsigned)NB_CHARGE_MAX_MA);
      break;
    }
    if (IS_MASTER) {
      if (haveTarget) {
        sendTargetU16(NB_TARGET_CHARGE_MA, target, ma);
        Serial.printf("target SET_CHARGE_MA %02X%02X%02X %u mA\n",
                      target[0], target[1], target[2], (unsigned)ma);
      } else {
        sendSetU16(NB_SET_CHARGE_MA, ma);
        Serial.printf("broadcast SET_CHARGE_MA %u mA\n", (unsigned)ma);
      }
    } else {
      if (haveTarget && !targetMatchesMe(target)) break;
      persistAndApplyChargeMa(ma);
    }
    break;
  }
  case 'S': { // timed deep sleep: bare S uses NB_REMOTE_SLEEP_S; S900 sleeps 15 min.
    int seconds = readSerialUint(80, 65535);
    uint16_t sleepS = (uint16_t)(seconds < 0 ? NB_REMOTE_SLEEP_S : seconds);
    if (sleepS == 0) {
      Serial.println("SLEEP rejected (seconds must be 1..65535)");
      break;
    }
    if (IS_MASTER) {
      sendSetU16(NB_SLEEP_FOR, sleepS);
      Serial.printf("broadcast SLEEP_FOR %us\n", (unsigned)sleepS);
    } else {
      enterTimedDeepSleep(sleepS, "serial");
    }
    break;
  }
  case 'P': { // targeted timed park: P9E5AB8:3600, default 1 h if seconds omitted.
    uint8_t target[3] = {0, 0, 0};
    bool haveTarget = readSerialHexId(target, 100);
    if (!haveTarget) {
      Serial.println("PARK rejected: use P<id>[:seconds]");
      break;
    }
    consumeOptionalSeparator();
    int seconds = readSerialUint(80, 65535);
    uint16_t sleepS = (uint16_t)(seconds < 0 ? NB_TARGET_SLEEP_DEFAULT_S : seconds);
    if (sleepS == 0) {
      Serial.println("PARK rejected (seconds must be 1..65535)");
      break;
    }
    if (IS_MASTER) {
      sendTargetU16(NB_TARGET_SLEEP_FOR, target, sleepS);
      Serial.printf("target PARK %02X%02X%02X sleep=%us\n",
                    target[0], target[1], target[2], (unsigned)sleepS);
    } else if (memcmp(target, myId, 3) == 0) {
      enterTimedDeepSleep(sleepS, "serial-target");
    }
    break;
  }
  case 'D': { // targeted HEX drawdown: D9E5AF0:3500, D9E5AF0, or D when exactly one peer
    uint8_t target[3] = {0, 0, 0};
    bool haveTarget = readSerialHexId(target, 100);
    if (haveTarget) consumeOptionalSeparator();
    int budget = readSerialUint(80, NB_CAPACITY_MAX_MAH);
    if (budget < 0) budget = NB_DRAWDOWN_DEFAULT_MAH;
    if (budget > NB_CAPACITY_MAX_MAH) {
      Serial.printf("DRAWDOWN %d rejected (max %u mAh)\n", budget, (unsigned)NB_CAPACITY_MAX_MAH);
      break;
    }
    if (IS_MASTER) {
      if (!haveTarget) {
        uint8_t only[3] = {0, 0, 0};
        int n = peerCount(only);
        if (n != 1) {
          Serial.printf("DRAWDOWN rejected: %d peers known; use D<id>[:mah]\n", n);
          break;
        }
        memcpy(target, only, 3);
      }
      sendTargetU16(NB_DRAWDOWN, target, (uint16_t)budget);
      Serial.printf("target DRAWDOWN %02X%02X%02X budget=%d mAh\n",
                    target[0], target[1], target[2], budget);
    } else {
      if (haveTarget && memcmp(target, myId, 3) != 0) break;
      drawdownStart((uint16_t)budget);
    }
    break;
  }
#ifdef NB_WDT_HANGTEST
  case 'x':
    Serial.println("HANGTEST: stop feeding watchdog, busy-loop (expect WDT reset)...");
    Serial.flush();
    while (1) { /* deliberately never esp_task_wdt_reset() */ }
    break;
#endif
  case 'i': { // master: identify (locate) the next peer in the table -> it blinks "..-"
    if (!IS_MASTER) break;
    static int ii = 0;
    int found = -1;
    for (int k = 0; k < NB_MAX_TRACKED; k++) { int j = (ii + k) % NB_MAX_TRACKED; if (peers[j].used) { found = j; break; } }
    if (found < 0) { Serial.println("no peers to identify"); break; }
    ii = (found + 1) % NB_MAX_TRACKED;
    sendIdentify(peers[found].id, 8);
    Serial.printf("identifying %02X%02X%02X for 8s (watch for the ..- blink)\n",
                  peers[found].id[0], peers[found].id[1], peers[found].id[2]);
    break;
  }
  case 'I': { // master: identify ALL peers at once
    if (!IS_MASTER) break;
    uint8_t all[3] = {0, 0, 0};
    sendIdentify(all, 8);
    Serial.println("identify ALL peers 8s");
    break;
  }
  case 'm': { // master: VINDPM/maintain setpoint (panel MPP sweep over the fleet).
              // Bare 'm' cycles the preset list; 'm<v10>' (e.g. m48 -> 4.8 V) sets an
              // explicit point so a host script can sweep with anchors/repeats.
    if (!IS_MASTER) break;
    static const uint8_t mv10[] = {55, 52, 50, 48, 46, 44}; // 5.5..4.4 V
    static uint8_t mi = 0;
    int explicitV10 = -1;
    uint32_t digitDeadline = millis() + 50; // digits (if any) arrive with the 'm'
    while ((int32_t)(digitDeadline - millis()) > 0) {
      if (!Serial.available()) { delay(1); continue; }
      int p = Serial.peek();
      if (p < '0' || p > '9') break;
      Serial.read();
      explicitV10 = (explicitV10 < 0 ? 0 : explicitV10 * 10) + (p - '0');
      if (explicitV10 > NB_MAINTAIN_MAX_V10) break;
    }
    uint8_t v10;
    if (explicitV10 >= 0) {
      if (explicitV10 < NB_MAINTAIN_MIN_V10 || explicitV10 > NB_MAINTAIN_MAX_V10) {
        Serial.printf("SET_MAINTAIN %d rejected (range %d..%d = %.1f..%.1f V)\n",
                      explicitV10, NB_MAINTAIN_MIN_V10, NB_MAINTAIN_MAX_V10,
                      (float)NB_MAINTAIN_MIN_V10 / 10.0f,
                      (float)NB_MAINTAIN_MAX_V10 / 10.0f);
        break;
      }
      v10 = (uint8_t)explicitV10;
    } else {
      v10 = mv10[mi];
      mi = (mi + 1) % (uint8_t)(sizeof(mv10));
    }
    sendCmd(NB_SET_MAINTAIN, v10);
    Serial.printf("broadcast SET_MAINTAIN %.1f V\n", (float)v10 / 10.0f);
    break;
  }
  case 'r':
    Serial.printf("role=%s mode=%d ch=%d rate=%uHz cap=%u charge=%umA txseq=%lu sendok=%lu fail=%lu peers=", IS_MASTER ? "master" : "peer",
                  (int)gMode, NB_CHANNEL, gRateHz, gBatteryCapacityMah, gChargeMa,
                  (unsigned long)txSeq, (unsigned long)sendOk, (unsigned long)sendFail);
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

// ---- sleep-cycle duty (deep-sleep load measurement) ------------------------
#ifdef NB_SLEEP_CYCLE
// Run at the end of setup() (ESP-NOW already up). Reports telemetry, gives a short
// window to be commanded into maintenance (no-touch OTA), then deep-sleeps. Deep
// sleep = full reboot, so setup() re-runs on each timer wake -> this repeats. The
// per-wake radio-on time (boot + init + send + listen) IS the active duty; tune the
// average via NB_SLEEP_S. Returns (without sleeping) only if maintenance is requested.
void sleepCycleStep() {
  readBattery();
  for (int i = 0; i < 3; i++) { sendHeartbeat(0); delay(8); } // a few (lossy bcast) HBs
  bool cold = (esp_reset_reason() == ESP_RST_POWERON);
  uint32_t listenMs = cold ? 30000 : NB_WAKE_LISTEN_MS; // long catch window on cold boot
  Serial.printf("sleep-cycle: awake, listen %lums for ENTER_MAINT...\n", (unsigned long)listenMs);
  uint32_t t0 = millis();
  while (millis() - t0 < listenMs) {
    processRx(); // ENTER_MAINT -> enterMaintenance() flips gMode to MODE_MAINT
    if (gMode == MODE_MAINT) {
      Serial.println("sleep-cycle: maintenance requested -> staying awake for OTA");
      return; // fall through to loop(), which serves /update
    }
    esp_task_wdt_reset();
    delay(5);
  }
  digitalWrite(NB_LED_PIN, LOW);
  // Cut the switchable rails for minimum deep-sleep current. They're I2C-latched in the
  // power-management domain, so they persist through ESP32 deep sleep (no gpio_hold); on V2
  // the gauge/charger stay alive with VSQT off, so telemetry resumes on the next wake (when
  // Board.init re-enables them). This is the rails-OFF arm of the idle-current A/B test.
  if (pfReady) { Board.enable3V3(false); Board.enableVSQT(false); }
  Serial.printf("sleep-cycle: rails cut (3V3+VSQT off), deep sleep %ds\n", NB_SLEEP_S);
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)NB_SLEEP_S * 1000000ULL);
  esp_deep_sleep_start(); // wakes into a fresh boot -> setup() again
}
#endif

// ---- setup / loop ----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  // Native USB-CDC settle so the cold-boot banner isn't lost. SKIP it on a deep-sleep
  // wake (sleep-cycle): it dominates the wake's active time and the field board has no
  // host attached -- trimming it makes the duty-cycled load representative.
  if (esp_reset_reason() != ESP_RST_DEEPSLEEP) delay(1500);
  Serial.println();
  Serial.println("=== Resonance net-bench " NET_BENCH_VERSION " ===");
  Serial.printf("role=%s channel=%d frame_hz=%d hb_hz=%d\n", IS_MASTER ? "master" : "peer",
                NB_CHANNEL, NB_FRAME_HZ, NB_HB_HZ);
  if (SERIAL_BRIDGE) Serial.println("mode: SERIAL BRIDGE (no WiFi; relaying nb-* to USB serial)");
#ifdef NB_SCAN_REPORT
  Serial.printf("mode: SCAN-REPORT (2.4 GHz scan every %ds, <=%d APs over ESP-NOW)\n",
                NB_SCAN_S, NB_SCAN_MAX);
#endif
#ifdef NB_SLEEP_CYCLE
  Serial.printf("mode: SLEEP-CYCLE (deep-sleep %ds between telemetry wakes; %dms maint-listen)\n",
                NB_SLEEP_S, NB_WAKE_LISTEN_MS);
#endif
#ifdef NB_FIELD_CYCLE
  Serial.printf("mode: FIELD-CYCLE (charge sleep %ds, wait sleep %ds, protect sleep %ds, low/hard %.3f/%.3fV)\n",
                NB_FIELD_CHARGE_SLEEP_S, NB_FIELD_WAIT_SLEEP_S, NB_FIELD_PROTECT_SLEEP_S,
                NB_FIELD_LOW_MV / 1000.0f, NB_FIELD_CRITICAL_MV / 1000.0f);
#endif
#ifdef NB_MAINT_AP
  Serial.println("mode: MAINT-AP (maintenance starts a temporary self AP for /update)");
#endif

  esp_read_mac(myMac, ESP_MAC_WIFI_STA);
  myId[0] = myMac[3]; myId[1] = myMac[4]; myId[2] = myMac[5];
  char idb[7]; snprintf(idb, sizeof(idb), "%02X%02X%02X", myId[0], myId[1], myId[2]);
  shortId = String(idb);
  Serial.printf("node id=%s mac=%02X:%02X:%02X:%02X:%02X:%02X\n", idb, myMac[0], myMac[1],
                myMac[2], myMac[3], myMac[4], myMac[5]);

  setupPowerFeather();
  if (pfReady) {
    Board.enable3V3(true);  // clear any latched SK6812 frame from a previous image
    Board.enableVSQT(true); // restore STEMMA/INA rail after a rails-off deep sleep
    delay(150);             // give external sensors time to power up before probing
  }
  envInit();
  envTick(); // prime the cache so even a sleep-cycle wake's heartbeat carries env data
  delay(20);
  drawdownPixelsBegin();
#ifdef NB_FIELD_CYCLE
  if (!IS_MASTER) {
    gRateHz = NB_FIELD_DRAWDOWN_HZ;
    readBattery();
    fieldCycleInit();
  }
#endif
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

#ifdef NB_SLEEP_CYCLE
  sleepCycleStep(); // wake->report->(maint?)->deep-sleep; only returns to run loop() for OTA
#endif
}

void loop() {
  esp_task_wdt_reset();
  handleSerial();

  static uint32_t lastBat = 0;
  uint32_t now = millis();
  if (now - lastBat > 1000) { lastBat = now; readBattery(); }
  envTick();
  drawdownTick();
  updateStatusLed();

#ifdef NB_AUTOSLEEP
  autosleepHealthyClear();
#endif

  if (gMode == MODE_MAINT) {
    server.handleClient();
    // /resume HTTP hit -> do a real comms transition (re-init ESP-NOW), not just a flag flip
    if (gResumePending) {
      gResumePending = false;
      gMaintStatus = MAINT_STATUS_RESUMED;
      Serial.println("/resume -> comms");
      enterComms();
      return;
    }
    // peers auto-resume if no OTA happens within the window (prevents stranding)
    if (!IS_MASTER && millis() - maintEnteredMs > (uint32_t)NB_MAINT_TIMEOUT_S * 1000) {
      Serial.println("maintenance timeout -> resume comms");
      gMaintStatus = MAINT_STATUS_TIMEOUT;
      enterComms();
    }
    return;
  }

  // COMMS mode
  processRx();
  if (IS_MASTER && WiFi.status() == WL_CONNECTED) server.handleClient();

#ifdef NB_FIELD_CYCLE
  if (!IS_MASTER) fieldCycleTick();
#endif

#ifdef NB_SCAN_REPORT
  if (!IS_MASTER) scanReportTick(); // field peer: 2.4 GHz coverage over ESP-NOW
#endif

  // periodic jittered sends (suppressed while a scan has the radio hopping)
  static uint32_t nextSend = 0;
  if (now >= nextSend && !gScanRunning) {
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
