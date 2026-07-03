// Resonance presence bench -- 4-sensor side-by-side presence-sensing comparison rig.
// Research phase for the interactivity ask (docs/research/PRESENCE_SENSING_
// INTERACTIVITY_2026-06-12.md): compare a thermal array, two multizone ToF imagers,
// and a 60 GHz radar live, wirelessly, with a baseline/delta detection view -- to
// judge feasibility, latency, and multi-zone / multi-object behavior (especially
// self-occlusion by the bamboo splay when hung under the solar overhang, pointing
// straight down).
//
// Sensor chain (one shared Qwiic/STEMMA-QT line, co-facing on a rigid board):
//   0x33 MLX90640   32x24 IR thermal array (Adafruit lib)
//   0x29 VL53L5CX   8x8 multizone ToF, 2 targets/zone (VENDORED lib in src/vl53l5cx/
//                   with VL53L5CX_NB_TARGET_PER_ZONE=2 -- see VENDORED.md)
//   0x41 TMF8821    3x3/4x4 multizone ToF, up to 2 objects/zone (SparkFun lib)
//   0x52 XM125      Acconeer A121 radar; runs EITHER the I2C presence-detector OR
//                   distance-detector app firmware -- this sketch probes both and
//                   reports which is loaded (app=none is a graceful state).
//
// Targets:
//   PowerFeather V2 (primary): sensors on the STEMMA-QT / VSQT rail = Wire1
//     (GPIO47/48). The PowerFeather SDK's charger/gauge share that Wire1 object;
//     we retune it to PB_I2C_HZ (400 kHz -- BQ25628E and MAX17260 are 400 kHz
//     parts; a deliberate, soak-tested exception to POWERFEATHER_NOTES' 100 kHz
//     guidance, because the MLX frame + VL53 init blob cannot live at 100 kHz).
//     Battery charging is left OFF (bench board, usually no cell -- see the
//     "charging into a missing battery" gotcha in POWERFEATHER_NOTES).
//   Metro ESP32-S3 (variant, -DPB_BOARD_METRO=1): plain Wire on the Qwiic port,
//     no PowerFeather SDK.
//
// Architecture: ALL I2C (sensor init + reads + battery telemetry round-robin) runs
// in ONE FreeRTOS task pinned to core 0 -- the sensor libraries block (MLX getFrame
// waits out both subpages; TMF startMeasuring waits a report period), and a single
// I2C owner needs no bus locking. loop()/HTTP on core 1 serves CACHED frames only,
// so the dashboard never stalls behind a sensor read. Handlers and the task share
// data under gDataMux.
//
// Build/flash (USB):   ./build.sh --port /dev/ttyACM0
// OTA after that:      ./build.sh --ota <ip>       (or presencebench.local)
// Dashboard:           http://presencebench.local/
// Host logger:         ops/bench/presence_logger.py

#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

#include <Adafruit_MLX90640.h>
#include "src/vl53l5cx/SparkFun_VL53L5CX_Library.h"
#include <SparkFun_TMF882X_Library.h>
#include <SparkFun_Qwiic_XM125_Arduino_Library.h>
#include <SparkFun_VL53L1X.h>

#define PRESENCE_BENCH_VERSION "presence-bench-2026-07-02.26"

// ---- Compile-time knobs (override via build.sh -> compiler.cpp.extra_flags) ----
#ifndef PB_ENABLE_MLX
#define PB_ENABLE_MLX 1
#endif
#ifndef PB_ENABLE_VL53
#define PB_ENABLE_VL53 1
#endif
#ifndef PB_ENABLE_TMF
#define PB_ENABLE_TMF 1
#endif
#ifndef PB_ENABLE_XM
#define PB_ENABLE_XM 1
#endif
// 5th sensor: TOF400C / VL53L1X single-zone ToF (the ~$3 original production
// candidate). It boots at 0x29 = the VL53L5CX's address, so its XSHUT is held LOW
// on a GPIO until the L5CX has been RELOCATED to PB_VL53_RELOC_ADDR; then the L1X
// is released and owns 0x29. Wire: TOF400C XSHUT -> A0/GPIO10 (jumper).
#ifndef PB_ENABLE_L1X
#define PB_ENABLE_L1X 1
#endif
#ifndef PB_L1X_XSHUT_PIN
#define PB_L1X_XSHUT_PIN 10 // A0 on the PowerFeather
#endif
#ifndef PB_VL53_RELOC_ADDR
#define PB_VL53_RELOC_ADDR 0x2A
#endif
// Software relocation of the VL53L5CX is DISABLED by default: the address change
// reproducibly leaves the chip a zombie (ACKs at the new address, registers read
// 0, DCI times out until power cycle) -- a known ST community issue; the official
// multi-sensor recipe needs the LPn pin, which the breakout doesn't wire. The
// 0x29 collision is instead resolved by a TCA9548A Qwiic mux when present.
#ifndef PB_VL53_RELOCATE
#define PB_VL53_RELOCATE 0
#endif
// TCA9548A I2C mux (SparkFun Qwiic Mux, addr 0x70): auto-detected at boot. The
// two 0x29 residents each live behind their own port -- select-before-use, one
// channel open at a time, no address changes ever. Without the mux, the L1X
// stays XSHUT-gated (collision) and only the VL53L5CX runs.
#ifndef PB_MUX_ADDR
#define PB_MUX_ADDR 0x70
#endif
#ifndef PB_MUX_VL5_CH
#define PB_MUX_VL5_CH 0 // VL53L5CX on mux port 0
#endif
#ifndef PB_MUX_L1X_CH
#define PB_MUX_L1X_CH 1 // TOF400C / VL53L1X on mux port 1
#endif
#ifndef PB_I2C_HZ
#define PB_I2C_HZ 400000
#endif

#ifdef PB_BOARD_METRO
#define PB_WIRE Wire
#else
#define PB_WIRE Wire1
#include <PowerFeather.h>
using namespace PowerFeather;
#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 (build.sh passes it) so the SDK targets the V2."
#endif
#endif

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif
#define AP_SSID "PresenceBench"
#define AP_PASS "resonance"
// Keep the SoftAP alongside the STA? A SoftAP cannot modem-sleep -- it beacons at
// full TX power every ~100 ms forever, a spike diet that battery-only VSYS has
// NEVER been soak-tested against (June's stable battery runs were all STA-only).
// Default OFF as of the 2026-07-02 reboot hunt; AP still comes up as the no-secrets
// fallback either way.
#ifndef PB_STA_AP
#define PB_STA_AP 0
#endif

WebServer server(80);
SemaphoreHandle_t gDataMux;

// ---- Sensor bookkeeping -----------------------------------------------------
enum PbState : uint8_t { ST_OFF = 0, ST_PENDING, ST_INIT, ST_OK, ST_MISSING, ST_ERROR };
static const char *ST_NAME[] = {"off", "pending", "init", "ok", "missing", "error"};
enum PbSensor : uint8_t { SEN_MLX = 0, SEN_VL53, SEN_TMF, SEN_XM, SEN_L1X, SEN_COUNT };
static const char *SEN_NAME[] = {"mlx", "vl53", "tmf", "xm", "l1x"};
static const uint8_t SEN_ADDR[] = {0x33, 0x29, 0x41, 0x52, 0x29};
static uint8_t gVl5Addr = 0x29; // VL53L5CX current address (relocated after init)
static uint8_t senAddr(uint8_t s) { return (s == SEN_VL53) ? gVl5Addr : SEN_ADDR[s]; }

struct PbStat {
  volatile uint8_t st = ST_PENDING;
  uint32_t seq = 0;
  uint32_t errs = 0;
  uint8_t consec = 0;   // consecutive failures -> ST_ERROR at 5
  float hz = 0;         // EMA of achieved read rate
  uint32_t lastOkMs = 0;
  uint32_t lastTickMs = 0;
  uint8_t initFails = 0;      // consecutive failed init attempts -> backoff
  uint32_t lastInitEndMs = 0; // when the last init attempt finished
};
PbStat gStat[SEN_COUNT];

// ---- Boot/status log ring (served at /api/log -- USB serial on the S3 native CDC
// is easy to miss after resets, and this bench is meant to run untethered) --------
static char gLog[48][100];
static uint8_t gLogHead = 0, gLogCount = 0;
static void logf(const char *fmt, ...) {
  char line[100];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line, sizeof(line), fmt, ap);
  va_end(ap);
  Serial.println(line);
  snprintf(gLog[gLogHead], sizeof(gLog[0]), "%8lu %s", (unsigned long)millis(), line);
  gLogHead = (gLogHead + 1) % 48;
  if (gLogCount < 48) gLogCount++;
}

static void statOk(PbSensor s) {
  PbStat &st = gStat[s];
  uint32_t now = millis();
  if (st.lastOkMs) {
    float inst = 1000.0f / (float)max((uint32_t)1, now - st.lastOkMs);
    st.hz = st.hz * 0.8f + inst * 0.2f;
  }
  st.lastOkMs = now;
  st.seq++;
  st.consec = 0;
  st.st = ST_OK;
}

static void statErr(PbSensor s) {
  PbStat &st = gStat[s];
  st.errs++;
  if (++st.consec >= 5) st.st = ST_ERROR; // reprobe path picks it up
}

// ---- MLX90640 state ----------------------------------------------------------
#if PB_ENABLE_MLX
Adafruit_MLX90640 mlx;
static float mlxFrame[768];        // task working buffer
int16_t gMlxT[768];                // published centi-degC
int16_t gMlxMin = 0, gMlxMax = 0, gMlxTa = 0;
volatile uint8_t gMlxRate = 4;     // subpages/s: 1,2,4,8 (2 subpages = 1 frame)
volatile bool gMlxRateReq = false;

// Raw status-register poll (0x8000 bit 3 = new subpage ready) so the library's
// blocking wait inside getFrame starts already-satisfied for the first subpage.
static bool mlxDataReady() {
  PB_WIRE.beginTransmission(SEN_ADDR[SEN_MLX]);
  PB_WIRE.write((uint8_t)0x80);
  PB_WIRE.write((uint8_t)0x00);
  if (PB_WIRE.endTransmission(false) != 0) return false;
  if (PB_WIRE.requestFrom((int)SEN_ADDR[SEN_MLX], 2) != 2) return false;
  uint16_t v = ((uint16_t)PB_WIRE.read() << 8) | PB_WIRE.read();
  return (v & 0x0008) != 0;
}

static mlx90640_refreshrate_t mlxRateEnum(uint8_t r) {
  switch (r) {
    case 1: return MLX90640_1_HZ;
    case 2: return MLX90640_2_HZ;
    case 8: return MLX90640_8_HZ;
    default: return MLX90640_4_HZ;
  }
}
#endif

// ---- VL53L5CX state ----------------------------------------------------------
#if PB_ENABLE_VL53
SparkFun_VL53L5CX vl53;
VL53L5CX_ResultsData vlData;
volatile uint8_t gVlRes = 8;       // 4 or 8
volatile uint8_t gVlHz = 15;       // device ranging frequency
volatile bool gVlCfgReq = false;
// Published, fixed stride-64 layout: index t*64+z; -1 / 255 = no target.
uint8_t gVlNb[64];
int16_t gVlD[128];
uint8_t gVlSt[128];
#endif

// ---- TMF8821 state -----------------------------------------------------------
#if PB_ENABLE_TMF
SparkFun_TMF882X tmf;
static struct tmf882x_msg_meas_results tmfRes;
volatile uint8_t gTmfMap = 1;      // spad_map_id: 1=3x3 29deg, 6=3x3 44x48deg, 7=4x4 44x48 (time-mux)
volatile uint16_t gTmfPeriod = 100;
struct PbTmfHit { uint8_t ch; uint8_t sub; uint16_t mm; uint8_t conf; };
PbTmfHit gTmf[40];
uint8_t gTmfN = 0;
#endif

// ---- XM125 state ---------------------------------------------------------------
#if PB_ENABLE_XM
SparkFunXM125Presence xmP;
SparkFunXM125Distance xmD;
volatile uint8_t gXmApp = 0;       // 0 none, 1 presence, 2 distance
uint32_t gXmPres = 0, gXmSticky = 0, gXmIntra = 0, gXmInter = 0, gXmDist = 0;
uint8_t gXmNPeaks = 0;
uint32_t gXmPeakMm[10];
int32_t gXmPeakStr[10];
static const uint32_t XM_RANGE_START_MM = 200, XM_RANGE_END_MM = 5000;
#endif

// ---- VL53L1X (TOF400C) state ----------------------------------------------------
#if PB_ENABLE_L1X
SFEVL53L1X l1x;
uint16_t gL1xMm = 0;
uint8_t gL1xStatus = 255;
uint16_t gL1xSig = 0;
#endif

// ---- Battery cache (PowerFeather only; led_studio round-robin idiom) ----------
bool gPfReady = false;
float gBatV = 0, gBatMa = 0, gSupV = 0;
uint8_t gSoc = 0;
bool gSupGood = false;
uint32_t gBoots = 0; // NVS boot counter -- catches silent reboots (brownout hunt)
int gRstReason = 0;

// ---- Cross-core requests (HTTP handler -> sensor task) ------------------------
volatile bool gScanReq = false;
volatile uint32_t gScanSeq = 0;
static char gScanBuf[1024] = "[]";
volatile uint8_t gReinitReq = 0;   // bitmask by PbSensor

// ---- JSON cursor helper --------------------------------------------------------
static size_t jcat(char *buf, size_t pos, size_t cap, const char *fmt, ...) {
  if (pos >= cap) return pos;
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf + pos, cap - pos, fmt, ap);
  va_end(ap);
  return (n < 0) ? pos : min(pos + (size_t)n, cap - 1);
}

// ---- TCA9548A mux (sensor task only; select-before-use) -------------------------
bool gMuxPresent = false;
static int8_t gMuxSel = -1; // currently open channel, -1 = none/unknown

static bool muxSelect(int8_t ch) { // ch -1 = all channels closed
  if (!gMuxPresent) return true;
  if (ch == gMuxSel) return true;
  PB_WIRE.beginTransmission(PB_MUX_ADDR);
  PB_WIRE.write(ch < 0 ? 0 : (uint8_t)(1 << ch));
  if (PB_WIRE.endTransmission() != 0) return false;
  gMuxSel = ch;
  return true;
}

// Channel a sensor needs open, or -1 for main-bus devices.
static int8_t senMuxCh(uint8_t s) {
  if (!gMuxPresent) return -1;
  if (s == SEN_VL53) return PB_MUX_VL5_CH;
  if (s == SEN_L1X) return PB_MUX_L1X_CH;
  return -1; // main bus; leave whatever channel is open (no 0x29 conflict)
}

// ---- I2C probe / scan (sensor task only) ---------------------------------------
// Read-probe (1-byte requestFrom), NOT a zero-length write: on arduino-esp32 3.x
// empty-write probes intermittently phantom-ACK large swaths of the address space
// (seen 2026-07-02: scan returned ~40 bogus devices on a healthy bus). A 1-byte
// read is harmless for every chip on this bench (register-based, no pop-on-read
// FIFOs) and NACKs reliably on empty addresses.
static bool ackProbe(uint8_t addr) {
  if (PB_WIRE.requestFrom((int)addr, 1) != 1) return false;
  PB_WIRE.read();
  return true;
}

static const char *knownAddr(uint8_t a) {
  switch (a) {
    case 0x29: return "VL53L1X (or un-relocated VL53L5CX)";
    case PB_VL53_RELOC_ADDR: return "VL53L5CX (relocated)";
    case 0x33: return "MLX90640";
    case 0x36: return "MAX17260 fuel gauge";
    case 0x41: return "TMF882X (NB: also INA219 alt addr -- never mix chains)";
    case 0x52: return "XM125";
    case 0x6A: return "BQ25628E charger";
    case PB_MUX_ADDR: return "TCA9548A mux";
    default: return "";
  }
}

static void doScan() {
  size_t p = 0;
  p = jcat(gScanBuf, p, sizeof(gScanBuf), "[");
  int n = 0;
  for (uint8_t a = 0x08; a <= 0x77; a++) {
    if (!ackProbe(a)) continue;
    p = jcat(gScanBuf, p, sizeof(gScanBuf), "%s{\"addr\":\"0x%02X\",\"name\":\"%s\"}",
             n ? "," : "", a, knownAddr(a));
    n++;
  }
  jcat(gScanBuf, p, sizeof(gScanBuf), "]");
  gScanSeq++;
}

// =================================================================================
// Sensor init + tick functions (ALL run in the sensor task on core 0)
// =================================================================================

#if PB_ENABLE_MLX
static bool mlxInit() {
  if (!ackProbe(SEN_ADDR[SEN_MLX])) return false;
  if (!mlx.begin(SEN_ADDR[SEN_MLX], &PB_WIRE)) return false; // EEPROM dump + params
  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(mlxRateEnum(gMlxRate));
  return true;
}

static void mlxTick() {
  PbStat &st = gStat[SEN_MLX];
  if (st.st != ST_OK) return; // ERROR recovers via reprobe->reinit ONLY: ticking a
                              // dead/mis-addressed device can read garbage from
                              // WHATEVER sits at that address and fake-recover
                              // (seen: un-begun L1X 'ok' off the L5CX at 0x29)
  if (gMlxRateReq) {
    gMlxRateReq = false;
    mlx.setRefreshRate(mlxRateEnum(gMlxRate));
  }
  uint32_t now = millis();
  if (now - st.lastTickMs < 100) return;
  st.lastTickMs = now;
  if (!mlxDataReady()) return;    // cheap gate; getFrame's first wait exits at once
  if (mlx.getFrame(mlxFrame) != 0) { statErr(SEN_MLX); return; }
  int16_t mn = 32767, mx = -32768;
  xSemaphoreTake(gDataMux, portMAX_DELAY);
  for (int i = 0; i < 768; i++) {
    float c = mlxFrame[i] * 100.0f;
    int16_t v = (int16_t)constrain(c, -32000.0f, 32000.0f);
    gMlxT[i] = v;
    if (v < mn) mn = v;
    if (v > mx) mx = v;
  }
  gMlxMin = mn;
  gMlxMax = mx;
  gMlxTa = (int16_t)(mlx.getTa(false) * 100.0f);
  statOk(SEN_MLX);
  xSemaphoreGive(gDataMux);
}
#endif

#if PB_ENABLE_VL53
static bool gVlRanging = false;

static bool vl53Apply() {
  uint8_t res = (gVlRes == 4) ? 4 : 8;
  uint8_t maxHz = (res == 8) ? 15 : 60;
  uint8_t hz = constrain((int)gVlHz, 1, (int)maxHz);
  gVlHz = hz;
  // Only stop if actually ranging: stop_ranging on a fresh device waits on an
  // MCU-stop bit that never asserts (see the vendored-loop note in VENDORED.md).
  if (gVlRanging) {
    vl53.stopRanging();
    gVlRanging = false;
  }
  if (!vl53.setResolution(res * res)) { logf("[vl53] setResolution FAILED"); return false; }
  if (!vl53.setRangingFrequency(hz)) { logf("[vl53] setRangingFrequency FAILED"); return false; }
  gVlRanging = vl53.startRanging();
  if (!gVlRanging) logf("[vl53] startRanging FAILED");
  return gVlRanging;
}

static void vl53OnError(SF_VL53L5CX_ERROR_TYPE code, uint32_t value) {
  logf("[vl53] error code=%d value=%lu", (int)code, (unsigned long)value);
}

// Relocate the L5CX with raw register writes BEFORE begin(): changing the address
// of a fully-initialized device left it a zombie (ACKs at the new address, all
// registers read 0, DCI times out -- 2026-07-02). The address register is
// low-level comms plumbing that works at POR, and begin() then runs entirely at
// the new address.
static bool vl5RawRelocate(uint8_t from, uint8_t to) {
  PB_WIRE.beginTransmission(from);
  PB_WIRE.write(0x7F); PB_WIRE.write(0xFF); PB_WIRE.write((uint8_t)0x00);
  if (PB_WIRE.endTransmission() != 0) return false;
  PB_WIRE.beginTransmission(from);
  PB_WIRE.write((uint8_t)0x00); PB_WIRE.write((uint8_t)0x04);
  PB_WIRE.write(to);
  if (PB_WIRE.endTransmission() != 0) return false;
  vTaskDelay(pdMS_TO_TICKS(10));
  uint8_t id[2];
  vl5ReadIdAt(to, id); // verify identity at the new address (restores page 2)
  if (id[0] != 0xF0) {
    vTaskDelay(pdMS_TO_TICKS(100));
    vl5ReadIdAt(to, id);
  }
  logf("[vl53] raw relocate 0x%02X -> 0x%02X: id %02X%02X", from, to, id[0], id[1]);
  return id[0] == 0xF0;
}

static bool vl53Init() {
  muxSelect(senMuxCh(SEN_VL53));
  // Locate the L5CX by IDENTITY, not bare ACK: it may sit at the relocated
  // address (warm reinit without a sensor power cycle) or at 0x29 (fresh POR,
  // L1X XSHUT-gated). 0x29 answering with anything but F0/02 means contention
  // or the wrong chip.
  uint8_t id2a[2], id29[2];
  vl5ReadIdAt(PB_VL53_RELOC_ADDR, id2a);
  vl5ReadIdAt(0x29, id29);
  logf("[vl53] id probe: 0x%02X=%02X%02X 0x29=%02X%02X", PB_VL53_RELOC_ADDR, id2a[0],
       id2a[1], id29[0], id29[1]);
  uint8_t addr;
  if (id2a[0] == 0xF0) addr = PB_VL53_RELOC_ADDR;
  else if (id29[0] == 0xF0) addr = 0x29;
  else return false; // no healthy L5CX visible; the diag will characterize 0x29
#if PB_ENABLE_L1X && PB_VL53_RELOCATE
  if (addr == 0x29) { // vacate 0x29 BEFORE begin (see vl5RawRelocate)
    if (vl5RawRelocate(0x29, PB_VL53_RELOC_ADDR)) addr = PB_VL53_RELOC_ADDR;
    else logf("[vl53] raw relocation failed -- continuing at 0x29, l1x stays gated");
  }
#endif
  gVl5Addr = addr;
  vl53.setErrorCallback(vl53OnError);
  // begin() uploads the ~84 KB ULD firmware blob -- several seconds, once.
  logf("[vl53] begin at 0x%02X (blob upload)...", addr);
  uint32_t t0 = millis();
  bool ok = vl53.begin(addr, PB_WIRE);
  logf("[vl53] begin -> %d in %lu ms", ok ? 1 : 0, (unsigned long)(millis() - t0));
  if (!ok) return false;
  vl53.setWireMaxPacketSize(124); // ESP32 Wire buffer is 128; speeds result reads
  return vl53Apply();
}

static void vl53Tick() {
  PbStat &st = gStat[SEN_VL53];
  if (st.st != ST_OK) return; // ERROR recovers via reprobe->reinit ONLY: ticking a
                              // dead/mis-addressed device can read garbage from
                              // WHATEVER sits at that address and fake-recover
                              // (seen: un-begun L1X 'ok' off the L5CX at 0x29)
  if (gVlCfgReq) {
    gVlCfgReq = false;
    if (!vl53Apply()) { statErr(SEN_VL53); return; }
  }
  uint32_t now = millis();
  if (now - st.lastTickMs < 100) return;
  st.lastTickMs = now;
  muxSelect(senMuxCh(SEN_VL53));
  // Self-heal: ranging silently stalled -- either never produced a frame after
  // init (stale pre-reflash sensor state; VSQT used to stay up across reboots) or
  // froze mid-session (seen when a colliding 0x29 device was hot-plugged during
  // the first soak). isDataReady() returns false without erroring in both cases,
  // so staleness is the only signal. Backoff bounds the retries.
  bool bootSilent = (st.seq == 0 && now - st.lastInitEndMs > 8000);
  bool midStall = (st.seq > 0 && st.lastOkMs && now - st.lastOkMs > 10000);
  if (bootSilent || midStall) {
    logf("[vl53] ranging stalled (%s) -> self-heal reinit", bootSilent ? "since boot" : "mid-session");
    st.initFails++;
    gReinitReq |= (1 << SEN_VL53);
    return;
  }
  if (!vl53.isDataReady()) return;
  if (!vl53.getRangingData(&vlData)) { statErr(SEN_VL53); return; }
  uint8_t zones = (gVlRes == 4) ? 16 : 64;
  xSemaphoreTake(gDataMux, portMAX_DELAY);
  for (uint8_t z = 0; z < 64; z++) {
    uint8_t nt = (z < zones) ? vlData.nb_target_detected[z] : 0;
    gVlNb[z] = nt;
    for (uint8_t t = 0; t < 2; t++) {
      uint16_t src = z * VL53L5CX_NB_TARGET_PER_ZONE + t;
      bool has = (z < zones) && (t < nt);
      gVlD[t * 64 + z] = has ? vlData.distance_mm[src] : -1;
      gVlSt[t * 64 + z] = has ? vlData.target_status[src] : 255;
    }
  }
  statOk(SEN_VL53);
  xSemaphoreGive(gDataMux);
}
#endif

#if PB_ENABLE_TMF
static bool tmfInit() {
  if (!ackProbe(SEN_ADDR[SEN_TMF])) return false;
  if (!tmf.begin(PB_WIRE, SEN_ADDR[SEN_TMF])) return false; // uploads tof_bin_image
  struct tmf882x_mode_app_config cfg;
  if (!tmf.getTMF882XConfig(cfg)) return false;
  cfg.report_period_ms = gTmfPeriod;
  cfg.spad_map_id = gTmfMap;
  return tmf.setTMF882XConfig(cfg);
}

static void tmfTick() {
  PbStat &st = gStat[SEN_TMF];
  if (st.st != ST_OK) return; // ERROR recovers via reprobe->reinit ONLY: ticking a
                              // dead/mis-addressed device can read garbage from
                              // WHATEVER sits at that address and fake-recover
                              // (seen: un-begun L1X 'ok' off the L5CX at 0x29)
  uint32_t now = millis();
  if (now - st.lastTickMs < 300) return;
  st.lastTickMs = now;
  if (!tmf.startMeasuring(tmfRes, 700)) { statErr(SEN_TMF); return; } // single-shot
  xSemaphoreTake(gDataMux, portMAX_DELAY);
  gTmfN = min((uint32_t)40, tmfRes.num_results);
  for (uint8_t i = 0; i < gTmfN; i++) {
    gTmf[i].ch = tmfRes.results[i].channel;
    gTmf[i].sub = tmfRes.results[i].sub_capture;
    gTmf[i].mm = (uint16_t)min((uint32_t)65535, tmfRes.results[i].distance_mm);
    gTmf[i].conf = (uint8_t)min((uint32_t)255, tmfRes.results[i].confidence);
  }
  statOk(SEN_TMF);
  xSemaphoreGive(gDataMux);
}
#endif

#if PB_ENABLE_XM
// Probe order: presence app first (liveness = measure counter advancing), then a
// module reset and the distance app. Wrong-app register writes land on undefined
// virtual registers, so the losing path just errors out -- harmless.
static bool xmInit() {
  gXmApp = 0;
  if (!ackProbe(SEN_ADDR[SEN_XM])) return false;
  if (xmP.begin(SFE_XM125_I2C_ADDRESS, PB_WIRE)) {
    if (xmP.detectorStart(XM_RANGE_START_MM, XM_RANGE_END_MM) == 0) {
      uint32_t c0 = 0, c1 = 0;
      xmP.getMeasureCounter(c0);
      vTaskDelay(pdMS_TO_TICKS(600));
      xmP.getMeasureCounter(c1);
      if (c1 > c0) { gXmApp = 1; return true; }
    }
    xmP.setCommand(SFE_XM125_PRESENCE_RESET_MODULE); // fall through to distance
    vTaskDelay(pdMS_TO_TICKS(1500));
  }
  if (xmD.begin(SFE_XM125_I2C_ADDRESS, PB_WIRE)) {
    if (xmD.distanceSetup(XM_RANGE_START_MM, XM_RANGE_END_MM) == 0) {
      if (xmD.detectorReadingSetup() == 0) { gXmApp = 2; return true; }
    }
  }
  return false;
}

static void xmTick() {
  PbStat &st = gStat[SEN_XM];
  if (st.st != ST_OK) return; // ERROR recovers via reprobe->reinit ONLY: ticking a
                              // dead/mis-addressed device can read garbage from
                              // WHATEVER sits at that address and fake-recover
                              // (seen: un-begun L1X 'ok' off the L5CX at 0x29)
  uint32_t now = millis();
  uint32_t period = (gXmApp == 1) ? 150 : 300;
  if (now - st.lastTickMs < period) return;
  st.lastTickMs = now;
  if (gXmApp == 1) {
    uint32_t pres = 0, sticky = 0, intra = 0, inter = 0, dist = 0;
    bool ok = xmP.getDetectorPresenceDetected(pres) == 0;
    ok = ok && xmP.getDetectorPresenceStickyDetected(sticky) == 0;
    ok = ok && xmP.getIntraPresenceScore(intra) == 0;
    ok = ok && xmP.getInterPresenceScore(inter) == 0;
    ok = ok && xmP.getDistance(dist) == 0;
    if (!ok) { statErr(SEN_XM); return; }
    xSemaphoreTake(gDataMux, portMAX_DELAY);
    gXmPres = pres; gXmSticky = sticky; gXmIntra = intra; gXmInter = inter; gXmDist = dist;
    statOk(SEN_XM);
    xSemaphoreGive(gDataMux);
  } else if (gXmApp == 2) {
    if (xmD.detectorReadingSetup() != 0) { statErr(SEN_XM); return; } // trigger + wait
    uint32_t np = 0;
    if (xmD.getNumberDistances(np) != 0) { statErr(SEN_XM); return; }
    np = min(np, (uint32_t)10);
    uint32_t mm[10];
    int32_t str[10];
    for (uint8_t i = 0; i < np; i++) {
      if (xmD.getPeakDistance(i, mm[i]) != 0) mm[i] = 0;
      if (xmD.getPeakStrength(i, str[i]) != 0) str[i] = 0;
    }
    xSemaphoreTake(gDataMux, portMAX_DELAY);
    gXmNPeaks = np;
    for (uint8_t i = 0; i < np; i++) { gXmPeakMm[i] = mm[i]; gXmPeakStr[i] = str[i]; }
    statOk(SEN_XM);
    xSemaphoreGive(gDataMux);
  }
}
#endif

#if PB_ENABLE_L1X
// Release/park the TOF400C. Release tries open-drain style first (INPUT: the
// module's onboard pull-up boots it) and falls back to driving HIGH -- covers
// boards with and without a pull-up, and Ben's "a little janky" jumper.
static void l1xGate(bool release) {
  if (release) {
    pinMode(PB_L1X_XSHUT_PIN, INPUT);
  } else {
    pinMode(PB_L1X_XSHUT_PIN, OUTPUT);
    digitalWrite(PB_L1X_XSHUT_PIN, LOW);
  }
}

// Read the VL53L5CX device-id bytes (page 0x7fff=0, regs 0x00/0x01) at an
// arbitrary address. A healthy solo L5CX answers F0/02. Also the basis of the
// XSHUT jumper diagnostic: if the bytes at 0x29 are identical with the L1X gated
// LOW vs driven HIGH, the XSHUT jumper is not conducting. Identity reads beat
// bare ACK probes here -- 0x29 can host either chip (or a bus fight), and bare
// requestFrom probes proved unreliable in this exact sequence (see LOG).
static void vl5ReadIdAt(uint8_t addr, uint8_t *out) {
  out[0] = out[1] = 0xEE;
  PB_WIRE.beginTransmission(addr);
  PB_WIRE.write(0x7F); PB_WIRE.write(0xFF); PB_WIRE.write((uint8_t)0x00);
  PB_WIRE.endTransmission();
  PB_WIRE.beginTransmission(addr);
  PB_WIRE.write((uint8_t)0x00); PB_WIRE.write((uint8_t)0x00);
  if (PB_WIRE.endTransmission(false) == 0 && PB_WIRE.requestFrom((int)addr, 2) == 2) {
    out[0] = PB_WIRE.read();
    out[1] = PB_WIRE.read();
  }
  PB_WIRE.beginTransmission(addr); // restore page 2
  PB_WIRE.write(0x7F); PB_WIRE.write(0xFF); PB_WIRE.write((uint8_t)0x02);
  PB_WIRE.endTransmission();
}
static void l1xReadId29(uint8_t *out) { vl5ReadIdAt(0x29, out); }

static void l1xJumperDiag() {
  uint8_t lo[2], hi[2];
  l1xGate(false);
  vTaskDelay(pdMS_TO_TICKS(60));
  l1xReadId29(lo);
  pinMode(PB_L1X_XSHUT_PIN, OUTPUT);
  digitalWrite(PB_L1X_XSHUT_PIN, HIGH);
  vTaskDelay(pdMS_TO_TICKS(60));
  l1xReadId29(hi);
  l1xGate(false); // leave gated
  bool same = (lo[0] == hi[0]) && (lo[1] == hi[1]);
  logf("[l1x] XSHUT diag: id@0x29 gated=%02X%02X released=%02X%02X -> %s", lo[0], lo[1],
       hi[0], hi[1], same ? "NO CHANGE: jumper likely NOT conducting" : "changes: jumper works");
}

static bool l1xInit() {
  // Without a mux, never release the L1X onto 0x29 while the VL53L5CX owns it.
  if (!gMuxPresent && PB_ENABLE_VL53 && gVl5Addr == 0x29 &&
      gStat[SEN_VL53].st != ST_MISSING && gStat[SEN_VL53].st != ST_OFF) {
    logf("[l1x] blocked: VL53L5CX owns 0x29 and no mux present (wire the TCA9548A)");
    return false;
  }
  muxSelect(senMuxCh(SEN_L1X));
  l1xGate(true); // INPUT: let the board pull-up boot it
  vTaskDelay(pdMS_TO_TICKS(60));
  if (!ackProbe(0x29)) { // no pull-up on the board? drive XSHUT high
    pinMode(PB_L1X_XSHUT_PIN, OUTPUT);
    digitalWrite(PB_L1X_XSHUT_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(60));
  }
  if (!ackProbe(0x29)) return false; // check the XSHUT jumper
  if (l1x.begin(PB_WIRE)) return false; // NB: returns 0 on SUCCESS (wraps VL53L1X_ERROR)
  l1x.setDistanceModeLong();       // 4 m
  l1x.setTimingBudgetInMs(100);
  l1x.setIntermeasurementPeriod(120);
  l1x.startRanging();
  return true;
}

static void l1xTick() {
  PbStat &st = gStat[SEN_L1X];
  if (st.st != ST_OK) return; // ERROR recovers via reprobe->reinit ONLY: ticking a
                              // dead/mis-addressed device can read garbage from
                              // WHATEVER sits at that address and fake-recover
                              // (seen: un-begun L1X 'ok' off the L5CX at 0x29)
  uint32_t now = millis();
  if (now - st.lastTickMs < 150) return;
  st.lastTickMs = now;
  muxSelect(senMuxCh(SEN_L1X));
  if (st.seq > 0 && st.lastOkMs && now - st.lastOkMs > 10000) {
    logf("[l1x] stalled mid-session -> self-heal reinit");
    st.initFails++;
    gReinitReq |= (1 << SEN_L1X);
    return;
  }
  if (!l1x.checkForDataReady()) return;
  uint16_t mm = l1x.getDistance();
  uint8_t rs = l1x.getRangeStatus();
  uint16_t sig = l1x.getSignalRate();
  l1x.clearInterrupt(); // arm the next measurement
  xSemaphoreTake(gDataMux, portMAX_DELAY);
  gL1xMm = mm;
  gL1xStatus = rs;
  gL1xSig = sig;
  statOk(SEN_L1X);
  xSemaphoreGive(gDataMux);
}
#endif

// ---- Battery round-robin (one SDK field per 800 ms; PowerFeather only) ---------
static void batteryTick() {
#ifndef PB_BOARD_METRO
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
#endif
}

// Brownout mitigation, one-shot (Ben saw all Qwiic LEDs blink off = a reboot; the
// board was also observed discharging its cell at ~290 mA WITH USB attached, so a
// sagging cell = brownout). Gentle charging keeps it topped. Guarded: only with a
// plausible warmed-up gauge reading (charging into a missing battery brownout-
// loops -- POWERFEATHER_NOTES), and the LFP profile's 3.65 V ceiling is a safe
// UNDERcharge even if the cell is actually Li-ion. Runs in the sensor task once
// the battery round-robin has produced a real voltage.
static void chargeTick() {
#ifndef PB_BOARD_METRO
  static bool done = false;
  if (done || !gPfReady || millis() < 6000) return;
  if (gBatV < 0.1f) { // gauge not warmed / no cell -- give up after a minute
    if (millis() > 60000) {
      done = true;
      logf("no battery reading after 60 s -> charging stays OFF");
    }
    return;
  }
  done = true;
  if (gBatV > 2.5f && gBatV < 4.4f) {
    Board.setBatteryChargingMaxCurrent(500);
    Board.enableBatteryCharging(true);
    logf("battery %.2fV present -> gentle charging ON (500 mA, 3.65V LFP ceiling)", gBatV);
  } else {
    logf("battery %.2fV implausible -> charging stays OFF", gBatV);
  }
#endif
}

// Pre-death breadcrumb: stash uptime + battery voltage to NVS every 10 s so the
// NEXT boot can report what the run looked like just before it died (see the
// boot block). GATED (PB_BREADCRUMB=0 to disable): periodic NVS commits are
// flash program/erase ops -- current spikes with both cores cache-stalled --
// and "brownout during flash writes on marginal supply" is a classic ESP32
// failure mode. June's loadgen instrumentation deliberately ran with "no NVS
// flash write" for artifact reasons. Ironic suspect #1 in the reboot hunt this
// instrument was built for.
#ifndef PB_BREADCRUMB
#define PB_BREADCRUMB 1
#endif
// Bisect knob: PB_NO_TASK=1 skips creating the core-0 sensor task entirely --
// no I2C, no SDK reads, no charging call after setup. Isolates the task (the
// biggest structural difference vs the historically-stable loop()-idiom
// firmwares, and it shares core 0 with the WiFi stack).
#ifndef PB_NO_TASK
#define PB_NO_TASK 0
#endif
// Sub-bisect knob: task runs but makes NO PowerFeather SDK calls (no battery
// round-robin, no charge-enable) -- splits "SDK I2C from core 0" from the
// task's probe/scheduling machinery.
#ifndef PB_TASK_NO_SDK
#define PB_TASK_NO_SDK 0
#endif
static void breadcrumbTick() {
#if !PB_BREADCRUMB
  return;
#endif
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  if (now - lastMs < 10000) return;
  lastMs = now;
  Preferences p;
  p.begin("pb", false);
  p.putUInt("lastup", now / 1000);
  p.putUInt("lastbv", (uint32_t)(gBatV * 100.0f));
  p.end();
}

// ---- Staged init: one pending sensor per pass, cheap-first ---------------------
static bool senEnabled(uint8_t s) {
  switch (s) {
    case SEN_MLX: return PB_ENABLE_MLX;
    case SEN_VL53: return PB_ENABLE_VL53;
    case SEN_TMF: return PB_ENABLE_TMF;
    case SEN_XM: return PB_ENABLE_XM;
    case SEN_L1X: return PB_ENABLE_L1X;
  }
  return false;
}

static bool senInit(uint8_t s) {
  switch (s) {
#if PB_ENABLE_MLX
    case SEN_MLX: return mlxInit();
#endif
#if PB_ENABLE_VL53
    case SEN_VL53: return vl53Init();
#endif
#if PB_ENABLE_TMF
    case SEN_TMF: return tmfInit();
#endif
#if PB_ENABLE_XM
    case SEN_XM: return xmInit();
#endif
#if PB_ENABLE_L1X
    case SEN_L1X: return l1xInit();
#endif
  }
  return false;
}

static void initTick() {
  // Handle explicit reinit requests first.
  if (gReinitReq) {
#if PB_ENABLE_L1X
    // Re-initializing the VL53L5CX means the relocation dance reruns: gate the
    // L1X off 0x29 for the duration and re-init it afterward too.
    if (gReinitReq & (1 << SEN_VL53)) {
      l1xGate(false);
      gReinitReq |= (1 << SEN_L1X);
    }
#endif
    for (uint8_t s = 0; s < SEN_COUNT; s++)
      if (gReinitReq & (1 << s)) gStat[s].st = senEnabled(s) ? ST_PENDING : ST_OFF;
    gReinitReq = 0;
  }
  // Cheap-first so a slow/failing VL53 init (its blob upload takes seconds, and a
  // FAILING attempt takes minutes -- the ULD does not early-exit between steps)
  // never delays the others. The L1X goes LAST: it may only be released onto 0x29
  // once the VL53L5CX has settled (relocated / missing / off).
  static const uint8_t ORDER[] = {SEN_XM, SEN_TMF, SEN_MLX, SEN_VL53, SEN_L1X};
  for (uint8_t i = 0; i < SEN_COUNT; i++) {
    uint8_t s = ORDER[i];
    PbStat &st = gStat[s];
    if (st.st != ST_PENDING) continue;
    if (!senEnabled(s)) { st.st = ST_OFF; continue; }
#if PB_ENABLE_L1X
    if (s == SEN_L1X && !gMuxPresent && senEnabled(SEN_VL53) &&
        (gStat[SEN_VL53].st == ST_PENDING || gStat[SEN_VL53].st == ST_INIT))
      continue; // no mux: wait for the L5CX to settle; not an attempt, no backoff
#endif
    // Exponential backoff on failed inits (5 s * 2^fails, cap 120 s) -- a failing
    // VL53 init attempt costs minutes of blocked bus, so do not hammer it.
    if (st.initFails) {
      uint32_t backoff = min((uint32_t)120000, (uint32_t)5000 << min(st.initFails, (uint8_t)5));
      if (millis() - st.lastInitEndMs < backoff) continue;
    }
    st.st = ST_INIT;
    logf("[init] %s (0x%02X) attempt %u...", SEN_NAME[s], senAddr(s), st.initFails + 1);
    bool ok = senInit(s);
    muxSelect(senMuxCh(s));
    st.st = ok ? ST_OK : (ackProbe(senAddr(s)) ? ST_ERROR : ST_MISSING);
#if PB_ENABLE_L1X && PB_ENABLE_VL53
    // A failing VL53L5CX init at 0x29 with the L1X supposedly gated is the
    // address-collision signature -- run the jumper diagnostic so /api/log says
    // whether the XSHUT wire is actually conducting.
    if (s == SEN_VL53 && !ok && gVl5Addr == 0x29) l1xJumperDiag();
#endif
    st.initFails = ok ? 0 : st.initFails + 1;
    st.lastInitEndMs = millis();
    st.consec = 0;
    st.lastOkMs = 0;
    st.hz = 0;
    logf("[init] %s -> %s", SEN_NAME[s], ST_NAME[st.st]);
    return; // one init per pass
  }
}

static void reprobeTick() {
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  if (now - lastMs < 5000) return;
  lastMs = now;
  for (uint8_t s = 0; s < SEN_COUNT; s++) {
    if (gStat[s].st == ST_MISSING) {
      muxSelect(senMuxCh(s));
      if (ackProbe(senAddr(s))) gStat[s].st = ST_PENDING;
    } else if (gStat[s].st == ST_ERROR) {
      gStat[s].st = ST_PENDING; // full re-init
    }
  }
}

// ---- The sensor task ------------------------------------------------------------
static void sensorTask(void *) {
  for (;;) {
    if (gScanReq) { gScanReq = false; doScan(); }
    initTick();
#if PB_ENABLE_MLX
    mlxTick();
#endif
#if PB_ENABLE_VL53
    vl53Tick();
#endif
#if PB_ENABLE_TMF
    tmfTick();
#endif
#if PB_ENABLE_XM
    xmTick();
#endif
#if PB_ENABLE_L1X
    l1xTick();
#endif
#if !PB_TASK_NO_SDK
    batteryTick();
    chargeTick();
#endif
    breadcrumbTick();
    reprobeTick();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// =================================================================================
// HTTP handlers (core 1) -- serve CACHES only, never touch I2C directly
// =================================================================================
static char gFrameBuf[16384];

static void handleFrame() {
  size_t p = 0;
  const size_t cap = sizeof(gFrameBuf);
  char *b = gFrameBuf;
  xSemaphoreTake(gDataMux, portMAX_DELAY);
  p = jcat(b, p, cap, "{\"t\":%lu", (unsigned long)millis());

#if PB_ENABLE_MLX
  {
    PbStat &st = gStat[SEN_MLX];
    p = jcat(b, p, cap, ",\"mlx\":{\"ok\":%d,\"seq\":%lu,\"hz\":%.1f,\"tmin\":%d,\"tmax\":%d,\"ta\":%d,\"t\":[",
             st.st == ST_OK ? 1 : 0, (unsigned long)st.seq, st.hz, gMlxMin, gMlxMax, gMlxTa);
    for (int i = 0; i < 768; i++) p = jcat(b, p, cap, "%s%d", i ? "," : "", gMlxT[i]);
    p = jcat(b, p, cap, "]}");
  }
#else
  p = jcat(b, p, cap, ",\"mlx\":{\"ok\":0}");
#endif

#if PB_ENABLE_VL53
  {
    PbStat &st = gStat[SEN_VL53];
    p = jcat(b, p, cap, ",\"vl53\":{\"ok\":%d,\"seq\":%lu,\"hz\":%.1f,\"res\":%u,\"nt\":[",
             st.st == ST_OK ? 1 : 0, (unsigned long)st.seq, st.hz, (unsigned)gVlRes);
    for (int z = 0; z < 64; z++) p = jcat(b, p, cap, "%s%u", z ? "," : "", gVlNb[z]);
    p = jcat(b, p, cap, "],\"d\":[");
    for (int i = 0; i < 128; i++) p = jcat(b, p, cap, "%s%d", i ? "," : "", gVlD[i]);
    p = jcat(b, p, cap, "],\"st\":[");
    for (int i = 0; i < 128; i++) p = jcat(b, p, cap, "%s%u", i ? "," : "", gVlSt[i]);
    p = jcat(b, p, cap, "]}");
  }
#else
  p = jcat(b, p, cap, ",\"vl53\":{\"ok\":0}");
#endif

#if PB_ENABLE_TMF
  {
    PbStat &st = gStat[SEN_TMF];
    p = jcat(b, p, cap, ",\"tmf\":{\"ok\":%d,\"seq\":%lu,\"hz\":%.1f,\"map\":%u,\"n\":%u,\"r\":[",
             st.st == ST_OK ? 1 : 0, (unsigned long)st.seq, st.hz, (unsigned)gTmfMap, gTmfN);
    for (uint8_t i = 0; i < gTmfN; i++)
      p = jcat(b, p, cap, "%s[%u,%u,%u,%u]", i ? "," : "", gTmf[i].ch, gTmf[i].sub,
               gTmf[i].mm, gTmf[i].conf);
    p = jcat(b, p, cap, "]}");
  }
#else
  p = jcat(b, p, cap, ",\"tmf\":{\"ok\":0}");
#endif

#if PB_ENABLE_XM
  {
    PbStat &st = gStat[SEN_XM];
    if (gXmApp == 1) {
      p = jcat(b, p, cap,
               ",\"xm\":{\"ok\":%d,\"app\":1,\"seq\":%lu,\"hz\":%.1f,\"pres\":%lu,\"sticky\":%lu,"
               "\"intra\":%lu,\"inter\":%lu,\"mm\":%lu}",
               st.st == ST_OK ? 1 : 0, (unsigned long)st.seq, st.hz, (unsigned long)gXmPres,
               (unsigned long)gXmSticky, (unsigned long)gXmIntra, (unsigned long)gXmInter,
               (unsigned long)gXmDist);
    } else if (gXmApp == 2) {
      p = jcat(b, p, cap, ",\"xm\":{\"ok\":%d,\"app\":2,\"seq\":%lu,\"hz\":%.1f,\"np\":%u,\"pk\":[",
               st.st == ST_OK ? 1 : 0, (unsigned long)st.seq, st.hz, gXmNPeaks);
      for (uint8_t i = 0; i < gXmNPeaks; i++)
        p = jcat(b, p, cap, "%s[%lu,%ld]", i ? "," : "", (unsigned long)gXmPeakMm[i],
                 (long)gXmPeakStr[i]);
      p = jcat(b, p, cap, "]}");
    } else {
      p = jcat(b, p, cap, ",\"xm\":{\"ok\":0,\"app\":0}");
    }
  }
#else
  p = jcat(b, p, cap, ",\"xm\":{\"ok\":0,\"app\":0}");
#endif

#if PB_ENABLE_L1X
  {
    PbStat &st = gStat[SEN_L1X];
    p = jcat(b, p, cap, ",\"l1x\":{\"ok\":%d,\"seq\":%lu,\"hz\":%.1f,\"mm\":%u,\"st\":%u,\"sig\":%u}",
             st.st == ST_OK ? 1 : 0, (unsigned long)st.seq, st.hz, gL1xMm, gL1xStatus, gL1xSig);
  }
#else
  p = jcat(b, p, cap, ",\"l1x\":{\"ok\":0}");
#endif

  p = jcat(b, p, cap, "}");
  xSemaphoreGive(gDataMux);
  server.setContentLength(p);
  server.send(200, "application/json", "");
  server.sendContent(b, p);
}

static void handleState() {
  char buf[1024];
  size_t p = 0;
  p = jcat(buf, p, sizeof(buf),
           "{\"v\":\"%s\",\"up_s\":%lu,\"heap\":%lu,\"i2c_hz\":%d,\"rssi\":%d,"
           "\"boots\":%lu,\"rst\":%d,\"mux\":%d",
           PRESENCE_BENCH_VERSION, (unsigned long)(millis() / 1000),
           (unsigned long)ESP.getFreeHeap(), (int)PB_I2C_HZ, (int)WiFi.RSSI(),
           (unsigned long)gBoots, gRstReason, gMuxPresent ? 1 : 0);
  for (uint8_t s = 0; s < SEN_COUNT; s++)
    p = jcat(buf, p, sizeof(buf), ",\"%s\":{\"st\":\"%s\",\"hz\":%.1f,\"seq\":%lu,\"err\":%lu}",
             SEN_NAME[s], ST_NAME[gStat[s].st], gStat[s].hz, (unsigned long)gStat[s].seq,
             (unsigned long)gStat[s].errs);
#if PB_ENABLE_XM
  p = jcat(buf, p, sizeof(buf), ",\"xm_app\":%u", (unsigned)gXmApp);
#endif
#if PB_ENABLE_VL53
  p = jcat(buf, p, sizeof(buf), ",\"vl_res\":%u,\"vl_hz\":%u", (unsigned)gVlRes, (unsigned)gVlHz);
#endif
#if PB_ENABLE_TMF
  p = jcat(buf, p, sizeof(buf), ",\"tmf_map\":%u,\"tmf_period\":%u", (unsigned)gTmfMap,
           (unsigned)gTmfPeriod);
#endif
#if PB_ENABLE_MLX
  p = jcat(buf, p, sizeof(buf), ",\"mlx_hz\":%u", (unsigned)gMlxRate);
#endif
  p = jcat(buf, p, sizeof(buf), ",\"pf\":%d,\"bv\":%.3f,\"ma\":%.0f,\"soc\":%u,\"sv\":%.2f,\"sgood\":%d}",
           gPfReady ? 1 : 0, gBatV, gBatMa, gSoc, gSupV, gSupGood ? 1 : 0);
  server.send(200, "application/json", buf);
}

static void handleScan() {
  uint32_t seq0 = gScanSeq;
  gScanReq = true;
  uint32_t t0 = millis();
  while (gScanSeq == seq0 && millis() - t0 < 3000) delay(10);
  server.send(200, "application/json", gScanSeq != seq0 ? gScanBuf : "[]");
}

static void handleSet() {
#if PB_ENABLE_MLX
  if (server.hasArg("mlx_hz")) {
    int v = server.arg("mlx_hz").toInt();
    if (v == 1 || v == 2 || v == 4 || v == 8) { gMlxRate = v; gMlxRateReq = true; }
  }
#endif
#if PB_ENABLE_VL53
  if (server.hasArg("vl_res")) {
    int v = server.arg("vl_res").toInt();
    if (v == 4 || v == 8) {
      gVlRes = v;
      gVlHz = (v == 4) ? 30 : 15;
      gVlCfgReq = true;
    }
  }
  if (server.hasArg("vl_hz")) {
    gVlHz = constrain(server.arg("vl_hz").toInt(), 1, 60);
    gVlCfgReq = true;
  }
#endif
#if PB_ENABLE_TMF
  if (server.hasArg("tmf_map")) {
    int v = server.arg("tmf_map").toInt();
    if (v == 1 || v == 2 || v == 6 || v == 7) {
      gTmfMap = v;
      gReinitReq |= (1 << SEN_TMF); // full re-init applies the config
    }
  }
  if (server.hasArg("tmf_period")) {
    gTmfPeriod = constrain(server.arg("tmf_period").toInt(), 50, 1000);
    gReinitReq |= (1 << SEN_TMF);
  }
#endif
  if (server.hasArg("reinit")) {
    String w = server.arg("reinit");
    for (uint8_t s = 0; s < SEN_COUNT; s++)
      if (w == "all" || w == SEN_NAME[s]) {
        gReinitReq |= (1 << s);
#if PB_ENABLE_XM
        if (s == SEN_XM) gXmApp = 0; // re-probe both apps
#endif
      }
  }
  for (uint8_t s = 0; s < SEN_COUNT; s++) {
    String k = String("en_") + SEN_NAME[s];
    if (server.hasArg(k)) {
      if (server.arg(k).toInt() == 0) gStat[s].st = ST_OFF;
      else if (gStat[s].st == ST_OFF) gStat[s].st = ST_PENDING;
    }
  }
  server.send(200, "text/plain", "ok");
}

// ---- Dashboard page (PROGMEM; ASCII only) ---------------------------------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>Presence Bench</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:12px;max-width:1100px;margin:auto}
 h2{margin:.2em 0} h3{margin:.4em 0 .2em;font-size:15px;color:#ccc}
 .tiles{display:flex;gap:8px;flex-wrap:wrap;margin:10px 0}
 .tile{flex:1 1 120px;min-width:120px;padding:10px;border-radius:10px;background:#222;text-align:center}
 .tile .nm{font-size:13px;color:#aaa}.tile .stx{font-size:20px;font-weight:700;margin:2px 0}
 .tile .sub{font-size:11px;color:#999;font-family:monospace}
 .tile.present{background:#0a7;color:#fff}.tile.present .nm,.tile.present .sub{color:#dff}
 .tile.nobase{background:#a70}.tile.off{background:#333;opacity:.6}
 .panels{display:flex;gap:12px;flex-wrap:wrap}
 .panel{flex:1 1 320px;min-width:300px;background:#1a1a1a;border-radius:10px;padding:10px}
 canvas{background:#000;border-radius:6px;display:block;max-width:100%}
 .meta{font-family:monospace;font-size:12px;color:#8f8;margin:4px 0;white-space:pre-wrap}
 .row{margin:8px 0}
 label{font-size:12px;color:#aaa;margin-right:4px}
 input[type=number]{width:64px;background:#222;color:#eee;border:1px solid #444;border-radius:4px;padding:3px}
 select{background:#222;color:#eee;border:1px solid #444;border-radius:4px;padding:3px}
 button{padding:8px 12px;font-size:13px;border:0;border-radius:8px;background:#333;color:#eee;margin:2px}
 button.on{background:#0a7;color:#fff}
 #log{font-family:monospace;font-size:12px;background:#000;padding:8px;border-radius:6px;max-height:180px;overflow-y:auto;color:#6f6}
 #stat,#bat{font-family:monospace;font-size:12px;color:#aaa}
 .ctl{background:#1a1a1a;border-radius:10px;padding:10px;margin:12px 0}
</style></head><body>
<h2>Presence Bench <span id=fw style="font-size:12px;color:#888"></span></h2>
<div id=stat>connecting...</div><div id=bat></div>

<div class=tiles>
 <div class=tile id=tile_mlx><div class=nm>MLX90640 thermal</div><div class=stx>--</div><div class=sub></div></div>
 <div class=tile id=tile_vl53><div class=nm>VL53L5CX 8x8 ToF</div><div class=stx>--</div><div class=sub></div></div>
 <div class=tile id=tile_tmf><div class=nm>TMF8821 zones</div><div class=stx>--</div><div class=sub></div></div>
 <div class=tile id=tile_xm><div class=nm>XM125 radar</div><div class=stx>--</div><div class=sub></div></div>
 <div class=tile id=tile_l1x><div class=nm>VL53L1X 1-zone</div><div class=stx>--</div><div class=sub></div></div>
</div>

<div class=ctl>
 <button id=bcap onclick="capBase()">Capture baseline</button>
 <button onclick="clearBase()">Clear baseline</button>
 <button onclick="dlBase()">Download baseline</button>
 <button id=bdelta onclick="toggleDelta()">Delta view: off</button>
 <button onclick="scanI2C()">Scan I2C</button>
 <span id=scanout class=meta></span>
 <div class=row>
  <label>MLX dT (0.01C)</label><input type=number id=th_mlxDc value=150>
  <label>min px</label><input type=number id=th_mlxPx value=6>
  <label>ToF dmm</label><input type=number id=th_tofMm value=300>
  <label>min zones</label><input type=number id=th_tofZones value=2>
  <label>XM score</label><input type=number id=th_xmScore value=300>
  <label>occl mm</label><input type=number id=th_occMm value=800>
 </div>
 <div class=row>
  <label>MLX pages/s</label><select id=k_mlx onchange="kset('mlx_hz',this.value)"><option>1</option><option>2</option><option selected>4</option><option>8</option></select>
  <label>VL53 res</label><select id=k_vlres onchange="kset('vl_res',this.value)"><option value=8 selected>8x8 (15Hz)</option><option value=4>4x4 (30Hz)</option></select>
  <label>TMF map</label><select id=k_tmfmap onchange="kset('tmf_map',this.value)"><option value=1 selected>3x3 29deg</option><option value=2>3x3 29x44</option><option value=6>3x3 44x48</option><option value=7>4x4 44x48 (mux)</option></select>
  <label>TMF ms</label><select id=k_tmfp onchange="kset('tmf_period',this.value)"><option selected>100</option><option>250</option><option>500</option></select>
  <label>enable</label>
  <button id=en_mlx class=on onclick="toggleEn('mlx')">MLX</button>
  <button id=en_vl53 class=on onclick="toggleEn('vl53')">VL53</button>
  <button id=en_tmf class=on onclick="toggleEn('tmf')">TMF</button>
  <button id=en_xm class=on onclick="toggleEn('xm')">XM</button>
  <button id=en_l1x class=on onclick="toggleEn('l1x')">L1X</button>
  <button onclick="fetch('/api/set?reinit=all')">Reinit all</button>
 </div>
</div>

<div class=panels>
 <div class=panel><h3>MLX90640 thermal (32x24)</h3>
  <canvas id=c_mlx width=320 height=240></canvas>
  <div class=meta id=m_mlx></div>
  <label><input type=checkbox id=mlx_auto checked> autoscale</label>
  <label>min</label><input type=number id=mlx_min value=1800><label>max</label><input type=number id=mlx_max value=3200>
 </div>
 <div class=panel><h3>Depth cloud (VL53, oblique + wobble)</h3>
  <canvas id=c_3d width=340 height=300></canvas>
  <div class=meta id=m_3d></div>
  <label><input type=checkbox id=d3_trails checked> trails</label>
  <label><input type=checkbox id=d3_wobble checked> wobble</label>
 </div>
 <div class=panel><h3>TMF8821 depth billboards</h3>
  <canvas id=c_t3d width=340 height=300></canvas>
  <div class=meta id=m_t3d></div>
 </div>
</div>
<details style="margin:12px 0"><summary style="cursor:pointer;color:#aaa;font-size:14px;padding:6px 0">2D diagnostic panels (tap to expand)</summary>
<div class=panels>
 <div class=panel><h3>VL53L5CX multizone (tap a zone)</h3>
  <canvas id=c_vl width=256 height=256 onclick="vlTap(event)"></canvas>
  <div class=meta id=m_vl></div>
  <label><input type=checkbox id=vl_fh onchange="saveFlip()"> flip H</label>
  <label><input type=checkbox id=vl_fv onchange="saveFlip()"> flip V</label>
 </div>
 <div class=panel><h3>TMF8821 zones (2 objects/zone)</h3>
  <canvas id=c_tmf width=240 height=240></canvas>
  <div class=meta id=m_tmf></div>
 </div>
 <div class=panel><h3>XM125 radar</h3>
  <canvas id=c_xm width=320 height=90></canvas>
  <div class=meta id=m_xm></div>
 </div>
 <div class=panel><h3>VL53L1X single-zone (the $3 candidate)</h3>
  <canvas id=c_l1x width=320 height=90></canvas>
  <div class=meta id=m_l1x></div>
 </div>
</div>
</details>

<h3>Detection events</h3>
<div id=log></div>

<script>
let F=null,S=null,base=null,capN=0,capBuf=null,delta=false,sel=-1,inflight=false;
let hist={intra:[],inter:[],l1x:[]};
let det={mlx:0,vl53:0,tmf:0,xm:0,l1x:0},detSince={mlx:0,vl53:0,tmf:0,xm:0,l1x:0};
let en={mlx:1,vl53:1,tmf:1,xm:1,l1x:1};
let t0=Date.now();
function th(id){return +document.getElementById('th_'+id).value;}
function el(id){return document.getElementById(id);}

// ---- baseline ----
function capBase(){capN=20;capBuf={mlx:[],vlN:[],vlF:[],vlNt:[],tmf:[],xmPk:[],l1x:[]};el('bcap').textContent='capturing...';}
function median(a){if(!a.length)return -1;let s=[...a].sort((x,y)=>x-y);return s[Math.floor(s.length/2)];}
function finishBase(){
 let b={mlx:null,vlN:[],vlF:[],vlNt:[],tmf:{},xmNear:-1,l1x:-1};
 if(capBuf.mlx.length){b.mlx=[];for(let i=0;i<768;i++)b.mlx[i]=median(capBuf.mlx.map(f=>f[i]));}
 if(capBuf.vlN.length){for(let z=0;z<64;z++){b.vlN[z]=median(capBuf.vlN.map(f=>f[z]).filter(v=>v>0));
  b.vlF[z]=median(capBuf.vlF.map(f=>f[z]).filter(v=>v>0));b.vlNt[z]=median(capBuf.vlNt.map(f=>f[z]));}}
 if(capBuf.tmf.length){let zs={};capBuf.tmf.forEach(m=>{for(let z in m){(zs[z]=zs[z]||[]).push(m[z]);}});
  for(let z in zs)b.tmf[z]=median(zs[z]);}
 if(capBuf.xmPk.length){let n=capBuf.xmPk.filter(v=>v>0);b.xmNear=n.length?median(n):-1;}
 if(capBuf.l1x.length){let n=capBuf.l1x.filter(v=>v>0);b.l1x=n.length?median(n):-1;}
 base=b;localStorage.setItem('pb_base',JSON.stringify(b));el('bcap').textContent='Capture baseline';
 logEv('baseline captured ('+(b.mlx?'mlx ':'')+(b.vlN.length?'vl53 ':'')+(Object.keys(b.tmf).length?'tmf ':'')+(b.xmNear>0?'xm ':'')+(b.l1x>0?'l1x':'')+')');
}
function clearBase(){base=null;localStorage.removeItem('pb_base');logEv('baseline cleared');}
function dlBase(){if(!base)return;let a=document.createElement('a');
 a.href=URL.createObjectURL(new Blob([JSON.stringify(base)],{type:'application/json'}));
 a.download='presence_baseline.json';a.click();}
try{let s=localStorage.getItem('pb_base');if(s)base=JSON.parse(s);}catch(e){}
function saveFlip(){localStorage.setItem('pb_flip',JSON.stringify({h:el('vl_fh').checked,v:el('vl_fv').checked}));}
try{let f=JSON.parse(localStorage.getItem('pb_flip')||'{}');el('vl_fh').checked=!!f.h;el('vl_fv').checked=!!f.v;}catch(e){}
function vlXY(z,res){ // zone -> display col/row honoring the flip toggles
 let col=z%res,row=Math.floor(z/res);
 if(el('vl_fh').checked)col=res-1-col;
 if(el('vl_fv').checked)row=res-1-row;
 return [col,row];}
function toggleDelta(){delta=!delta;el('bdelta').textContent='Delta view: '+(delta?'ON':'off');el('bdelta').className=delta?'on':'';}

// ---- helpers over the frame ----
function vlZone(f,z){ // {near,far,nt} from valid targets (status 5 or 9)
 let out={near:-1,far:-1,nt:f.vl53.nt[z]};
 for(let t=0;t<2;t++){let d=f.vl53.d[t*64+z],st=f.vl53.st[t*64+z];
  if(d>0&&(st==5||st==9)){if(out.near<0||d<out.near)out.near=d;if(d>out.far)out.far=d;}}
 return out;}
function tmfZones(f){ // {zoneIdx: farthest mm} zoneIdx = sub*9+ch-1
 let m={};if(!f.tmf.ok)return m;
 f.tmf.r.forEach(r=>{let z=r[1]*9+(r[0]-1);if(r[2]>0&&(!(z in m)||r[2]>m[z]))m[z]=r[2];});
 return m;}

// ---- detection ----
function runDetect(){
 let now=(Date.now()-t0)/1000;
 // MLX: N pixels hotter than baseline by dT
 if(F.mlx.ok&&base&&base.mlx){let hot=0;for(let i=0;i<768;i++)if(F.mlx.t[i]-base.mlx[i]>th('mlxDc'))hot++;
  setDet('mlx',hot>=th('mlxPx'),now,'hot='+hot);}else setDet('mlx',null,now,'');
 // VL53: zones whose FAR target stepped closer than baseline-far by dmm
 if(F.vl53.ok&&base&&base.vlN.length){let nz=0,res=F.vl53.res,zone=res*res;
  for(let z=0;z<zone;z++){let v=vlZone(F,z);if(v.far>0&&base.vlF[z]>0&&base.vlF[z]-v.far>th('tofMm'))nz++;}
  setDet('vl53',nz>=th('tofZones'),now,'zones='+nz);}else setDet('vl53',null,now,'');
 // TMF: any zone's farthest target stepped closer by dmm
 if(F.tmf.ok&&base&&Object.keys(base.tmf).length){let nz=0,m=tmfZones(F);
  for(let z in m)if(base.tmf[z]>0&&base.tmf[z]-m[z]>th('tofMm'))nz++;
  setDet('tmf',nz>=1,now,'zones='+nz);}else setDet('tmf',null,now,'');
 // XM: presence app = module flag or score; distance app = new near peak
 if(F.xm.ok&&F.xm.app==1){setDet('xm',F.xm.pres==1||F.xm.intra>th('xmScore')||F.xm.inter>th('xmScore'),now,
  'i='+F.xm.intra+' e='+F.xm.inter);}
 else if(F.xm.ok&&F.xm.app==2&&base&&base.xmNear>0){let near=-1;
  F.xm.pk.forEach(p=>{if(p[0]>0&&(near<0||p[0]<near))near=p[0];});
  setDet('xm',near>0&&base.xmNear-near>th('tofMm'),now,'near='+near);}
 else setDet('xm',null,now,'');
 // L1X: single-zone distance stepped closer than baseline by dmm (status 0 = valid)
 if(F.l1x.ok&&base&&base.l1x>0){let v=F.l1x.st==0&&F.l1x.mm>0;
  setDet('l1x',v&&base.l1x-F.l1x.mm>th('tofMm'),now,F.l1x.mm+'mm');}
 else setDet('l1x',null,now,F.l1x.ok?F.l1x.mm+'mm':'');
}
function setDet(k,on,now,info){
 let tl=el('tile_'+k),stx=tl.querySelector('.stx'),sub=tl.querySelector('.sub');
 let st=S?S[k]:null;
 if(!en[k]||(st&&st.st=='off')){tl.className='tile off';stx.textContent='OFF';sub.textContent='';return;}
 if(st&&st.st!='ok'){tl.className='tile off';stx.textContent=st.st.toUpperCase();sub.textContent='err='+st.err;return;}
 if(on===null){tl.className='tile nobase';stx.textContent='NO BASE';
  sub.textContent=(st?st.hz.toFixed(1)+' Hz err='+st.err:'')+' '+info;return;}
 if(on&&!det[k]){detSince[k]=now;logEv('+'+now.toFixed(1)+'s '+k.toUpperCase()+' PRESENT');}
 if(!on&&det[k])logEv('+'+now.toFixed(1)+'s '+k.toUpperCase()+' clear');
 det[k]=on;
 tl.className='tile'+(on?' present':'');stx.textContent=on?'PRESENT':'absent';
 sub.textContent=(st?st.hz.toFixed(1)+' Hz err='+st.err:'')+' '+info;
}
function logEv(s){let d=el('log');d.innerHTML='<div>'+s+'</div>'+d.innerHTML;
 while(d.childNodes.length>50)d.removeChild(d.lastChild);}

// ---- rendering ----
function heat(v){ // 0..1 -> [r,g,b] blue-cyan-green-yellow-red
 v=Math.max(0,Math.min(1,v));let s=[[0,0,128],[0,120,255],[0,220,120],[255,230,0],[255,40,0]];
 let x=v*(s.length-1),i=Math.min(s.length-2,Math.floor(x)),f=x-i;
 return s[i].map((c,k)=>Math.round(c+(s[i+1][k]-c)*f));}
function drawMlx(){
 let c=el('c_mlx'),ctx=c.getContext('2d');if(!F.mlx.ok){ctx.clearRect(0,0,320,240);return;}
 let lo,hi;
 if(delta&&base&&base.mlx){lo=-300;hi=300;}
 else if(el('mlx_auto').checked){lo=F.mlx.tmin;hi=Math.max(F.mlx.tmax,lo+50);}
 else{lo=+el('mlx_min').value;hi=+el('mlx_max').value;}
 for(let y=0;y<24;y++)for(let x=0;x<32;x++){
  let i=y*32+x,v=F.mlx.t[i];if(delta&&base&&base.mlx)v=v-base.mlx[i];
  let rgb=heat((v-lo)/(hi-lo));ctx.fillStyle='rgb('+rgb+')';ctx.fillRect(x*10,y*10,10,10);}
 el('m_mlx').textContent=(delta?'DELTA ':'')+'min='+(F.mlx.tmin/100).toFixed(1)+'C max='+(F.mlx.tmax/100).toFixed(1)+
  'C ta='+(F.mlx.ta/100).toFixed(1)+'C  scale=['+(lo/100).toFixed(1)+','+(hi/100).toFixed(1)+']';
}
function drawVl(){
 let c=el('c_vl'),ctx=c.getContext('2d');ctx.clearRect(0,0,256,256);if(!F.vl53.ok)return;
 let res=F.vl53.res,cs=256/res,usable=0,occ=0;
 for(let z=0;z<res*res;z++){
  let v=vlZone(F,z),[dc,dr]=vlXY(z,res),x=dc*cs,y=dr*cs;
  let isOcc=base&&base.vlN[z]>0&&base.vlN[z]<th('occMm');
  if(base){if(isOcc)occ++;else if(base.vlF[z]>0)usable++;}
  let val=delta&&base&&base.vlF[z]>0&&v.far>0?(base.vlF[z]-v.far)/2000:(v.near>0?1-v.near/4000:-1);
  ctx.fillStyle=val<0?'#222':'rgb('+heat(val)+')';ctx.fillRect(x,y,cs-1,cs-1);
  if(isOcc){ctx.strokeStyle='rgba(255,255,255,.5)';ctx.beginPath();
   ctx.moveTo(x,y+cs-1);ctx.lineTo(x+cs-1,y);ctx.stroke();}
  if(v.nt>1){ctx.fillStyle='#fff';ctx.font='10px monospace';ctx.fillText(v.nt,x+2,y+10);}
  if(z==sel){ctx.strokeStyle='#0f0';ctx.strokeRect(x+1,y+1,cs-3,cs-3);}}
 let m='res='+res+'x'+res;
 if(base)m+='  usable zones: '+usable+'/'+(res*res)+' (occluded '+occ+')';
 if(sel>=0&&sel<res*res){let d0=F.vl53.d[sel],s0=F.vl53.st[sel],d1=F.vl53.d[64+sel],s1=F.vl53.st[64+sel];
  m+='\nzone '+sel+': T0 '+d0+'mm st'+s0+' | T1 '+(d1>0?d1+'mm st'+s1:'--')+
   (base&&base.vlN[sel]>0?'  base near/far '+base.vlN[sel]+'/'+base.vlF[sel]:'');}
 el('m_vl').textContent=m;
}
function vlTap(e){let r=e.target.getBoundingClientRect(),res=F&&F.vl53.ok?F.vl53.res:8;
 let cs=r.width/res,col=Math.floor((e.clientX-r.left)/cs),row=Math.floor((e.clientY-r.top)/cs);
 if(el('vl_fh').checked)col=res-1-col; // flips are involutive: same map inverts
 if(el('vl_fv').checked)row=res-1-row;
 sel=row*res+col;}
function drawTmf(){
 let c=el('c_tmf'),ctx=c.getContext('2d');ctx.clearRect(0,0,240,240);if(!F.tmf.ok)return;
 let n=(F.tmf.map==7||F.tmf.map==4||F.tmf.map==5)?4:3,cs=240/n;
 // group hits per zone
 let zs={};F.tmf.r.forEach(r=>{let z=r[1]*9+(r[0]-1);(zs[z]=zs[z]||[]).push(r);});
 for(let z=0;z<n*n;z++){let x=(z%n)*cs,y=Math.floor(z/n)*cs;
  let hits=zs[z]||[];let near=hits.length?Math.min(...hits.map(h=>h[2])):-1;
  ctx.fillStyle=near>0?'rgb('+heat(1-near/4000)+')':'#222';ctx.fillRect(x,y,cs-2,cs-2);
  let isOcc=base&&base.tmf[z]>0&&base.tmf[z]<th('occMm');
  if(isOcc){ctx.strokeStyle='rgba(255,255,255,.5)';ctx.beginPath();ctx.moveTo(x,y+cs-2);ctx.lineTo(x+cs-2,y);ctx.stroke();}
  ctx.fillStyle='#fff';ctx.font='10px monospace';
  hits.slice(0,2).forEach((h,i)=>{ctx.fillText(h[2]+'mm c'+h[3],x+3,y+14+i*12);});}
 el('m_tmf').textContent='map='+F.tmf.map+' results='+F.tmf.n+(F.tmf.map==7?' (4x4 time-mux: zone=sub*9+ch-1)':'');
}
function drawXm(){
 let c=el('c_xm'),ctx=c.getContext('2d');ctx.clearRect(0,0,320,90);
 let m='';
 if(!F.xm.ok||F.xm.app==0){el('m_xm').textContent='no detector app responded (see README)';return;}
 if(F.xm.app==1){
  hist.intra.push(F.xm.intra);hist.inter.push(F.xm.inter);
  if(hist.intra.length>160){hist.intra.shift();hist.inter.shift();}
  let mx=Math.max(1000,...hist.intra,...hist.inter);
  [['intra','#0f8',hist.intra],['inter','#f80',hist.inter]].forEach(([nm,col,h])=>{
   ctx.strokeStyle=col;ctx.beginPath();
   h.forEach((v,i)=>{let x=i*2,y=88-(v/mx)*84;i?ctx.lineTo(x,y):ctx.moveTo(x,y);});ctx.stroke();});
  m='PRESENCE app  pres='+F.xm.pres+' sticky='+F.xm.sticky+' dist='+F.xm.mm+'mm\nintra='+F.xm.intra+' inter='+F.xm.inter+' (green/orange, max '+mx+')';
 }else{
  ctx.strokeStyle='#444';ctx.beginPath();ctx.moveTo(0,60);ctx.lineTo(320,60);ctx.stroke();
  F.xm.pk.forEach(p=>{let x=p[0]/5000*320,r=Math.max(3,Math.min(12,Math.log10(Math.max(1,Math.abs(p[1])))*3));
   ctx.fillStyle='#0cf';ctx.beginPath();ctx.arc(x,60,r,0,7);ctx.fill();
   ctx.fillStyle='#fff';ctx.font='10px monospace';ctx.fillText(p[0],x-10,45);});
  m='DISTANCE app  peaks='+F.xm.np+'  [mm,strength]: '+JSON.stringify(F.xm.pk)+(base&&base.xmNear>0?'\nbase near='+base.xmNear+'mm':'');
 }
 el('m_xm').textContent=m;
}
// Depth cloud: every valid VL53 target is a 3D point (zone angle x depth through
// the ~45deg FoV). Oblique projection; a slow +-25deg wobble about the sensor
// axis adds motion parallax so the structure reads as 3D. T0 cyan, T1 orange;
// fading trails leave comet streaks on motion.
// Webcam-style 3D views: optical axis points INTO the page; perspective
// projection with the orbit pivot at 2 m (a demo subject 2 m away stays
// centered). Drag to orbit (kills the wobble), wheel to zoom. The VL53 cloud
// and the TMF billboards share the same camera.
let trail3d=[],lastSeq3d=-1,vYaw=0,vPitch=0.15,vZoom=1,drag3d=null;
function attachOrbit(c){
 c.addEventListener('mousedown',e=>{drag3d=[e.clientX,e.clientY];el('d3_wobble').checked=false;e.preventDefault();});
 c.addEventListener('wheel',e=>{e.preventDefault();
  vZoom=Math.max(0.3,Math.min(4,vZoom*(e.deltaY<0?1.12:0.89)));},{passive:false});}
attachOrbit(el('c_3d'));attachOrbit(el('c_t3d'));
window.addEventListener('mousemove',e=>{if(!drag3d)return;
 vYaw+=(e.clientX-drag3d[0])*0.01;
 vPitch=Math.max(-1.3,Math.min(1.3,vPitch-(e.clientY-drag3d[1])*0.01));
 drag3d=[e.clientX,e.clientY];});
window.addEventListener('mouseup',()=>drag3d=null);
function curYaw(){ // mirrored world (flip H) needs mirrored yaw or drag feels inverted
 let s=el('vl_fh').checked?-1:1;
 return s*(vYaw+(el('d3_wobble').checked?0.35*Math.sin(Date.now()/2200):0));}
function makeP(){let yaw=curYaw(),cy=Math.cos(yaw),sy=Math.sin(yaw),
  cp=Math.cos(vPitch),sp=Math.sin(vPitch),f=300*vZoom,camD=1.6,ox=170,oy=150,PIV=2;
 return function(p){let x=p.x,y=p.y,zz=p.z-PIV;   // orbit about the 2 m pivot
  let X=x*cy+zz*sy,Z1=-x*sy+zz*cy;
  let Y=y*cp-Z1*sp,Z=y*sp+Z1*cp+PIV;
  let w=Z+camD;if(w<0.25)return null;
  return [ox+X*f/w,oy-Y*f/w,w];};}
function coneRings(ctx,P,half){ // FoV cone edges to 4 m + 1 m depth rings
 function line(a,b){if(a&&b){ctx.beginPath();ctx.moveTo(a[0],a[1]);ctx.lineTo(b[0],b[1]);ctx.stroke();}}
 ctx.strokeStyle='#333';ctx.fillStyle='#555';ctx.font='9px monospace';
 let o=P({x:0,y:0,z:0});
 [[-1,-1],[1,-1],[1,1],[-1,1]].forEach(([a,b])=>line(o,P({x:a*half*4,y:b*half*4,z:4})));
 for(let m=1;m<=4;m++){let ring=[[-1,-1],[1,-1],[1,1],[-1,1],[-1,-1]].map(([a,b])=>P({x:a*half*m,y:b*half*m,z:m}));
  if(ring.every(q=>q)){ctx.beginPath();ring.forEach((q,i)=>i?ctx.lineTo(q[0],q[1]):ctx.moveTo(q[0],q[1]));ctx.stroke();
   ctx.fillText(m+'m',ring[2][0]+3,ring[2][1]);}}}
function draw3d(){
 let c=el('c_3d'),ctx=c.getContext('2d');ctx.clearRect(0,0,340,300);
 if(!F||!F.vl53.ok){el('m_3d').textContent='';return;}
 let res=F.vl53.res,half=Math.tan(22.5*Math.PI/180);
 let pts=[];
 for(let z=0;z<res*res;z++){
  let [dc,dr]=vlXY(z,res);
  let u=((dc+0.5)/res-0.5)*2*half,v=((dr+0.5)/res-0.5)*2*half;
  for(let t=0;t<2;t++){let d=F.vl53.d[t*64+z],st=F.vl53.st[t*64+z];
   if(d>0&&(st==5||st==9)){let zm=d/1000;pts.push({x:u*zm,y:-v*zm,z:zm,t:t});}}}
 if(el('d3_trails').checked){
  if(F.vl53.seq!=lastSeq3d){lastSeq3d=F.vl53.seq;trail3d.push(pts);if(trail3d.length>10)trail3d.shift();}
 } else trail3d=[pts];
 let P=makeP();
 coneRings(ctx,P,half);
 // baseline far-surface (back wall / floor) as a translucent sheet, if captured
 if(base&&base.vlF.length){ctx.fillStyle='rgba(120,120,255,0.10)';
  for(let z=0;z<res*res;z++){if(base.vlF[z]>0){let [dc,dr]=vlXY(z,res),zm=base.vlF[z]/1000;
   let u0=((dc)/res-0.5)*2*half,u1=((dc+1)/res-0.5)*2*half;
   let v0=((dr)/res-0.5)*2*half,v1=((dr+1)/res-0.5)*2*half;
   let q=[P({x:u0*zm,y:-v0*zm,z:zm}),P({x:u1*zm,y:-v0*zm,z:zm}),P({x:u1*zm,y:-v1*zm,z:zm}),P({x:u0*zm,y:-v1*zm,z:zm})];
   if(q.every(p=>p)){ctx.beginPath();q.forEach((p,i)=>i?ctx.lineTo(p[0],p[1]):ctx.moveTo(p[0],p[1]));ctx.closePath();ctx.fill();}}}}
 // trails then live points; size falls off with projected distance
 trail3d.forEach((fr,i)=>{let a=(i+1)/trail3d.length,live=(i==trail3d.length-1);
  fr.forEach(p=>{let q=P(p);if(!q)return;let r=Math.max(1.5,Math.min(7,12/q[2]));
   ctx.fillStyle=(p.t?'rgba(255,136,0,':'rgba(0,204,255,')+(0.12+0.75*a*(live?1:0.35))+')';
   ctx.beginPath();ctx.arc(q[0],q[1],live?r:r*0.7,0,7);ctx.fill();});});
 let ds=pts.map(p=>p.z);
 el('m_3d').textContent=pts.length+' pts  '+(ds.length?Math.min(...ds).toFixed(2)+'-'+Math.max(...ds).toFixed(2)+'m':'')+'  cyan=T0 orange=T1  drag=orbit wheel=zoom';
}
// TMF billboards: each zone-target is a translucent camera-space pane at its
// measured depth, spanning the zone's angular footprint. Confidence = opacity.
// Far panes draw first (painter's algorithm) so near returns overlay.
function drawT3d(){
 let c=el('c_t3d'),ctx=c.getContext('2d');ctx.clearRect(0,0,340,300);
 if(!F||!F.tmf.ok){el('m_t3d').textContent='';return;}
 let n=(F.tmf.map==7||F.tmf.map==4||F.tmf.map==5)?4:3;
 let fov=(F.tmf.map==6||F.tmf.map==7)?44:33;
 let half=Math.tan(fov/2*Math.PI/180);
 let P=makeP();
 coneRings(ctx,P,half);
 let rs=[...F.tmf.r].sort((a,b)=>b[2]-a[2]);
 let drawn=0;
 rs.forEach(r=>{
  let z=r[1]*9+(r[0]-1);if(z>=n*n||r[2]<=0)return;
  let col=z%n,row=Math.floor(z/n),zm=r[2]/1000,mg=0.07;
  let u0=((col+mg)/n-0.5)*2*half,u1=((col+1-mg)/n-0.5)*2*half;
  let v0=((row+mg)/n-0.5)*2*half,v1=((row+1-mg)/n-0.5)*2*half;
  let q=[P({x:u0*zm,y:-v0*zm,z:zm}),P({x:u1*zm,y:-v0*zm,z:zm}),
         P({x:u1*zm,y:-v1*zm,z:zm}),P({x:u0*zm,y:-v1*zm,z:zm})];
  if(!q.every(p=>p))return;
  let a=0.10+0.45*(r[3]/255);
  ctx.fillStyle='rgba(0,220,160,'+a.toFixed(2)+')';
  ctx.strokeStyle='rgba(90,255,200,'+Math.min(1,a+0.3).toFixed(2)+')';
  ctx.beginPath();q.forEach((p,i)=>i?ctx.lineTo(p[0],p[1]):ctx.moveTo(p[0],p[1]));
  ctx.closePath();ctx.fill();ctx.stroke();
  ctx.fillStyle='#cfc';ctx.font='9px monospace';
  ctx.fillText(r[2],(q[0][0]+q[2][0])/2-9,(q[0][1]+q[2][1])/2+3);
  drawn++;});
 el('m_t3d').textContent=drawn+' panes  map='+F.tmf.map+' ('+n+'x'+n+', '+fov+'deg)  opacity=confidence  shared camera';
}
function drawL1x(){
 let c=el('c_l1x'),ctx=c.getContext('2d');ctx.clearRect(0,0,320,90);
 if(!F.l1x.ok){el('m_l1x').textContent='not present (check XSHUT jumper on A0)';return;}
 hist.l1x.push(F.l1x.mm);if(hist.l1x.length>160)hist.l1x.shift();
 ctx.strokeStyle='#0cf';ctx.beginPath();
 hist.l1x.forEach((v,i)=>{let x=i*2,y=88-Math.min(1,v/4000)*84;i?ctx.lineTo(x,y):ctx.moveTo(x,y);});ctx.stroke();
 if(base&&base.l1x>0){let y=88-Math.min(1,base.l1x/4000)*84;
  ctx.strokeStyle='#666';ctx.setLineDash([4,4]);ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(320,y);ctx.stroke();ctx.setLineDash([]);}
 el('m_l1x').textContent=F.l1x.mm+'mm  status='+F.l1x.st+' (0=ok 1=sigma 2=signal 4=out-of-bounds 7=wrap)  sig='+F.l1x.sig+
  (base&&base.l1x>0?'  base='+base.l1x+'mm (dashed)':'');
}

// ---- knobs / enables ----
function kset(k,v){fetch('/api/set?'+k+'='+v);}
function toggleEn(k){en[k]^=1;el('en_'+k).className=en[k]?'on':'';fetch('/api/set?en_'+k+'='+en[k]);}
function scanI2C(){el('scanout').textContent='scanning...';
 fetch('/api/i2c_scan').then(r=>r.json()).then(j=>{
  el('scanout').textContent=j.map(d=>d.addr+(d.name?' '+d.name.split(' ')[0]:'')).join(', ')||'none';});}

// ---- polling ----
function pollFrame(){
 if(inflight)return;inflight=true;
 fetch('/api/frame').then(r=>r.json()).then(f=>{F=f;
  if(capN>0){
   if(f.mlx.ok)capBuf.mlx.push([...f.mlx.t]);
   if(f.vl53.ok){let N=[],Fa=[],Nt=[];for(let z=0;z<64;z++){let v=vlZone(f,z);N[z]=v.near;Fa[z]=v.far;Nt[z]=v.nt;}
    capBuf.vlN.push(N);capBuf.vlF.push(Fa);capBuf.vlNt.push(Nt);}
   if(f.tmf.ok)capBuf.tmf.push(tmfZones(f));
   if(f.xm.ok&&f.xm.app==2){let near=-1;f.xm.pk.forEach(p=>{if(p[0]>0&&(near<0||p[0]<near))near=p[0];});capBuf.xmPk.push(near);}
   if(f.l1x.ok&&f.l1x.st==0)capBuf.l1x.push(f.l1x.mm);
   el('bcap').textContent='capturing '+capN;
   if(--capN==0)finishBase();}
  drawMlx();drawVl();drawTmf();drawXm();drawL1x();runDetect();
 }).catch(()=>{}).finally(()=>{inflight=false;});}
function pollState(){fetch('/api/state').then(r=>r.json()).then(s=>{S=s;
 el('fw').textContent=s.v;
 el('stat').textContent='up '+s.up_s+'s  boot#'+s.boots+' rst='+s.rst+(s.mux?' mux':'')+'  heap '+Math.round(s.heap/1024)+'k  i2c '+(s.i2c_hz/1000)+'kHz  rssi '+s.rssi+
  '  |  mlx:'+s.mlx.st+' vl53:'+s.vl53.st+' tmf:'+s.tmf.st+' xm:'+s.xm.st+(s.xm_app==1?'(presence)':s.xm_app==2?'(distance)':'')+
  ' l1x:'+(s.l1x?s.l1x.st:'-');
 if(s.pf)el('bat').textContent='batt SOC '+s.soc+'% '+s.bv.toFixed(2)+'V '+s.ma+'mA  supply '+s.sv.toFixed(2)+'V'+(s.sgood?' ok':' --');
}).catch(()=>{});}
setInterval(pollFrame,300);setInterval(pollState,1000);pollState();
(function anim(){draw3d();drawT3d();requestAnimationFrame(anim);})(); // 60fps wobble, data at poll rate
</script></body></html>)HTML";

// ---- WiFi / setup -----------------------------------------------------------------
static void setupWifi() {
#if HAVE_SECRETS
  WiFi.mode(PB_STA_AP ? WIFI_AP_STA : WIFI_STA);
  WiFi.setHostname("presencebench");
  WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);
  Serial.print("WiFi connecting");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    logf("STA up, rssi %d, %s", (int)WiFi.RSSI(), PB_STA_AP ? "AP+STA" : "STA-only");
    Serial.print("Presence bench STA at http://");
    Serial.println(WiFi.localIP());
#if PB_STA_AP
    if (WiFi.softAP(AP_SSID, AP_PASS, WiFi.channel())) {
      Serial.print("AP '" AP_SSID "' -> http://");
      Serial.println(WiFi.softAPIP());
    }
#endif
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
  Serial.println();
  Serial.println("=== " PRESENCE_BENCH_VERSION " ===");
  gRstReason = (int)esp_reset_reason();
  {
    Preferences p;
    p.begin("pb", false);
    gBoots = p.getUInt("boots", 0) + 1;
    p.putUInt("boots", gBoots);
    // Pre-death breadcrumb from the previous run (written every 30 s by
    // breadcrumbTick): how long it lived and the battery voltage near the end --
    // the reboot-mystery instrument (rst=1 + low/sagging bv = supply collapse).
    uint32_t lup = p.getUInt("lastup", 0);
    uint32_t lbv = p.getUInt("lastbv", 0);
    p.end();
    // reset_reason: 1=poweron (full power loss), 3=software(OTA), 4=panic,
    // 5/6=watchdog, 9=brownout detector
    logf("boot #%lu: %s reset_reason=%d", (unsigned long)gBoots, PRESENCE_BENCH_VERSION,
         gRstReason);
    if (lup) logf("previous run: up=%lus bv=%.2fV (30s-resolution breadcrumb)",
                  (unsigned long)lup, lbv / 100.0f);
  }
#if PB_ENABLE_L1X
  // Gate the TOF400C off the bus BEFORE its rail powers: it boots at 0x29, the
  // VL53L5CX's address. Released by l1xInit() after the L5CX relocates to 0x2A.
  l1xGate(false);
#endif

#ifndef PB_BOARD_METRO
  // SDK init for rails + telemetry ONLY. Charging stays OFF: this is a sensor bench,
  // usually cell-less on USB, and enabling charge into a missing battery brownout-
  // loops (POWERFEATHER_NOTES). Attach a cell + port the solar guard before changing.
  Result pf = Result::Failure;
  for (int i = 0; i < 4 && pf != Result::Ok; i++) {
    pf = Board.init(2000, Mainboard::BatteryType::Generic_LFP);
    if (pf != Result::Ok) delay(250);
  }
  if (pf == Result::Ok) {
    gPfReady = true;
    Board.enableBatteryCharging(false);
    // Power-CYCLE the sensor rail: VSQT stays up across ESP reboots (battery/OTA),
    // so without this the sensors carry stale state from the previous image --
    // first bring-up left the VL53 half-stopped and it ranged silent (see LOG).
    Board.enableVSQT(false);
    Board.enable3V3(true);
    delay(600); // long enough for the chain's bulk caps to actually discharge
    Board.enableVSQT(true); // sensor power (STEMMA-QT rail), fresh POR
    logf("PowerFeather SDK Ok: VSQT power-cycled, charging OFF (bench)");
    // Clear a latched EN_HIZ (BQ REG0x16 bit 4). BQ state persists on battery
    // across reflashes, and a prior image (e.g. net_bench's solar guard) can leave
    // the input path disabled -- the board then silently DRAINS its cell while on
    // USB (seen on first bring-up: sv=0.00, ma=-290 with USB attached).
    {
      uint8_t r16 = 0;
      Wire1.beginTransmission(0x6A);
      Wire1.write(0x16);
      if (Wire1.endTransmission(false) == 0 && Wire1.requestFrom(0x6A, 1) == 1) {
        r16 = Wire1.read();
        logf("BQ REG0x16=0x%02X (EN_HIZ=%d)", r16, (r16 >> 4) & 1);
        if (r16 & (1 << 4)) {
          Wire1.beginTransmission(0x6A);
          Wire1.write(0x16);
          Wire1.write(r16 & ~(1 << 4));
          bool ok = Wire1.endTransmission() == 0;
          logf("BQ EN_HIZ was latched (REG0x16=0x%02X) -> cleared %s", r16,
               ok ? "ok" : "FAILED");
        }
      }
    }
    // Charging decision is DEFERRED to chargeTick() in the sensor task: the gauge
    // reads 0.00 V this early after Board.init (observed on .14), so a boot-time
    // guard false-negatives. See chargeTick for the brownout-mitigation rationale.
  } else {
    Serial.println("WARNING: Board.init failed -- VSQT rail may be unpowered; sensors will read MISSING");
  }
  delay(100); // rail settle before first probes
#else
  PB_WIRE.begin();
#endif
  PB_WIRE.setClock(PB_I2C_HZ);
  Serial.printf("I2C at %d Hz (shared SDK bus on PowerFeather -- soak-test at 400k)\n",
                (int)PB_I2C_HZ);
  // TCA9548A mux auto-detect (a write of 0x00 = all channels closed is also the
  // safe boot state). With the mux present, both 0x29 ToFs get their own port.
  {
    PB_WIRE.beginTransmission(PB_MUX_ADDR);
    PB_WIRE.write((uint8_t)0x00);
    gMuxPresent = (PB_WIRE.endTransmission() == 0);
    gMuxSel = -1;
    logf("TCA9548A mux at 0x%02X: %s", PB_MUX_ADDR,
         gMuxPresent ? "present (VL5=ch0, L1X=ch1)" : "not found (L1X gated if VL5 owns 0x29)");
  }

  setupWifi();
  if (MDNS.begin("presencebench")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://presencebench.local/");
  } else {
    Serial.println("mDNS start failed (use the IP)");
  }

  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/api/state", handleState);
  server.on("/api/frame", handleFrame);
  server.on("/api/i2c_scan", handleScan);
  server.on("/api/set", handleSet);
  server.on("/api/log", []() { // boot/status ring, oldest first
    String out;
    out.reserve(gLogCount * 100);
    for (uint8_t i = 0; i < gLogCount; i++)
      out += String(gLog[(gLogHead + 48 - gLogCount + i) % 48]) + "\n";
    server.send(200, "text/plain", out);
  });
  // Standard OTA (led_studio handler): curl -F "firmware=@x.ino.bin" http://<ip>/update
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

  gDataMux = xSemaphoreCreateMutex();
#if PB_NO_TASK
  logf("PB_NO_TASK bisect build: sensor task NOT created (no I2C/SDK/charging after setup)");
#else
  // All I2C lives on core 0; HTTP stays on core 1 (Arduino loop). Sensors init
  // lazily inside the task -- the dashboard is already reachable at this point.
  xTaskCreatePinnedToCore(sensorTask, "sensors", 16384, NULL, 1, NULL, 0);
  Serial.println("Dashboard up; sensors initializing in background.");
#endif
}

void loop() {
  server.handleClient();
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's') gScanReq = true;           // result prints on next /api/i2c_scan poll
    else if (c == 'r') gReinitReq = (1 << SEN_COUNT) - 1; // reinit all
  }
}
