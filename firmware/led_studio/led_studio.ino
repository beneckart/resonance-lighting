// Resonance LED Studio -- merged interactive aesthetic bench tool. Drives EITHER
// the SK6812 "HEX" 37px RGB grid OR the single 4 W SK6812 RGBW point source on the
// SAME data pin (default GPIO10 / A0), with a UI toggle to hot-swap between them
// (reconfigures the NeoPixel type/length at runtime -- no reflash). Supersedes the
// separate hex_studio + rgbw_studio sketches.
//
// Workflow: blank the LEDs (All off), physically swap the module on the JST, then
// flip the mode toggle to match. Mismatched mode is harmless (both are SK6812) --
// worst case is wrong colors until refreshed; the firmware blanks on every switch.
//
// Build/flash (USB): ./build.sh --port /dev/ttyACM1   (override pin with --pin N)

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Adafruit_NeoPixel.h>

#define STUDIO_VERSION "led-studio-2026-07-30.6"

#ifndef DATA_PIN
#define DATA_PIN 10 // GPIO10 / A0
#endif
#ifndef STUDIO_SENSOR_TRIAD
#define STUDIO_SENSOR_TRIAD 0
#endif
#if STUDIO_SENSOR_TRIAD
#include <Adafruit_MSA301.h>
#include <Adafruit_BMP5xx.h>
#include <SparkFun_TMF882X_Library.h>
#endif
#ifndef STUDIO_L5CX
#define STUDIO_L5CX 0 // perimeter-enclosure build: VL53L5CX presence -> HEX gobo demo
#endif
#if STUDIO_L5CX
// Vendored copy of the fixture's trimmed ULD (see src/vl53l5cx_uld/VENDORED.md):
// NB_TARGET_PER_ZONE=2 (results arrays index zone*2+target), extra outputs
// disabled to keep the per-frame read short on the shared 100 kHz Wire1.
#include "src/vl53l5cx_uld/SparkFun_VL53L5CX_Library.h"
#endif
#define NUMPIXELS 37 // HEX max; RGBW mode uses length 1
#define CENTER 18
// Constructed for HEX (RGB) by default; switched to RGBW at runtime via applyMode().
Adafruit_NeoPixel strip(NUMPIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);

// PowerFeather SDK: we init it ONLY to program the charger's LFP profile (3.65 V
// ceiling) -- without this the BQ25628E runs its 4.2 V Li-ion default, which
// OVERCHARGES an attached LFP whenever USB/panel power is present (see
// POWERFEATHER_NOTES "chemistry flash order"). Added 2026-06-11 so studio sessions
// are safe with USB + cell simultaneously (and the cell charges correctly).
#include <PowerFeather.h>
#include "../powerfeather_solar_guard.h"
using namespace PowerFeather;
#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 (build.sh passes it) so the SDK targets the V2."
#endif
#ifndef STUDIO_BATTERY_MAH
#define STUDIO_BATTERY_MAH 2000
#endif
#ifndef STUDIO_BATTERY_TYPE
#define STUDIO_BATTERY_TYPE Mainboard::BatteryType::Generic_LFP
#endif
#ifndef STUDIO_CHARGE_MA
#define STUDIO_CHARGE_MA 500.0f
#endif
#ifndef STUDIO_MAINTAIN_V
#define STUDIO_MAINTAIN_V 4.6f
#endif
bool gPfReady = false; // SDK up -> /state carries battery stats (SOC matters for sag/brightness)

// PowerFeather V2: the switchable 3V3 header rail (powers the LED) is gated by GPIO4
// (EN_3V3, active HIGH). Kept as a fallback in case Board.init() fails -- the rail
// must be on either way. See firmware/POWERFEATHER_NOTES.md.
#define EN_3V3_PIN 4

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif
#define AP_SSID "ResonanceLED"
#define AP_PASS "resonance"

WebServer server(80);

// Per-device hostname/mDNS name: two studio boards on one LAN both claiming
// "ledstudio.local" made browser control land on the wrong unit (2026-07-30).
char gHostname[24] = "ledstudio";

// ---- Modes & animations ----------------------------------------------------
enum Mode { MODE_HEX = 0, MODE_RGBW = 1, MODE_RGB = 2 };
// HEX anims:      0 static, 1 spiral, 2 orbit, 3 breathe, 4 twinkle  (+ Split modifier)
// RGBW/RGB anims: 0 static, 1 hue, 2 breathe, 3 candle, 4 fade,
//                 5 ToF depth, 6 MSA311 tilt, 7 BMP581 relative elevation.
// MODE_RGB is a single high-power RGB pixel (no white die): same render path as
// RGBW but a 3-byte strip -- the W component is simply ignored by the library.
#if STUDIO_SENSOR_TRIAD
uint8_t gMode = MODE_RGBW;
#else
uint8_t gMode = MODE_HEX;
#endif
uint8_t gAnim = 0; // index within the current mode's animation set

// ---- Shared color state ----------------------------------------------------
uint8_t gR = 255, gG = 140, gB = 40, gW = 0; // warm amber default; W = RGBW only
#if STUDIO_L5CX
// Perimeter HEX demo: hex lumens are low next to the 4 W RGBW point source, so
// boot pinned to full brightness with the perceptual ramp off (Ben 2026-07-30).
uint8_t gBri = 255;
uint8_t gSpeed = 30;
bool gGamma = false;
#else
uint8_t gBri = 40;
uint8_t gSpeed = 30;
bool gGamma = true;
#endif

// ---- HEX state -------------------------------------------------------------
uint8_t gShape = 1;     // 0 center, 1 +ring1, 2 +ring1+2, 3 all
uint8_t gTrail = 3;
uint8_t gOrbitRing = 1; // 1..3
bool gFrozen = false;
uint8_t gSplit = 0; // Split-RGB style: 0 off, 1 triad (local offset), 2 rotate (120 deg about center)
uint32_t hexAnimPos = 0;
float hexBreathePhase = 0;
uint8_t lastLit = CENTER;
// Split (HEX): pure R/G/B triad for wide color fringing
float gSpread = 1.2f;
float gFringeAngle = 0;
uint8_t gAnchor = CENTER;
uint32_t anchorStep = 0;

// ---- RGBW state ------------------------------------------------------------
uint8_t gB2r = 0, gB2g = 120, gB2b = 255; // Color B for Fade
float rgbwPhase = 0;
float candleLevel = 1.0f, candleTarget = 1.0f;

uint32_t lastFrame = 0;

#if STUDIO_SENSOR_TRIAD
// Production downlight sensor chain. It shares Wire1 with the charger/gauge, so
// every access stays on Arduino loop() and restores ADR 0028's 100 kHz ceiling.
Adafruit_MSA311 studioMsa;
Adafruit_BMP5xx studioBmp;
SparkFun_TMF882X studioTmf;
bool gMsaPresent = false, gMsaReadOk = false;
bool gBmpPresent = false, gBmpReadOk = false;
bool gTmfPresent = false, gTmfReadOk = false;
bool gTmfActive = false;
float gAx = NAN, gAy = NAN, gAz = NAN;
float gRestX = 0.0f, gRestY = 0.0f, gRestZ = 1.0f;
bool gTiltZeroed = false;
float gTiltDeg = 0.0f;
float gPressureHpa = NAN, gPressureFilteredHpa = NAN, gPressureZeroHpa = NAN;
float gRelativeAltitudeM = 0.0f;
uint16_t gTofRawClosestMm = 0, gTofDepthMm = 0, gTofConfidence = 0;
float gTofDepthFilteredMm = NAN;
uint32_t gTmfReads = 0, gTmfErrors = 0, gTmfRecoveries = 0;
uint32_t gTmfLastReadMs = 0, gTmfCycleStartMs = 0, gTmfCycleReadBase = 0;
uint32_t gTmfNextStartMs = 0;
#endif

// ---- HEX geometry (7 rows 4-5-6-7-6-5-4, center=18) ------------------------
const uint8_t ROW_COUNT[7] = {4, 5, 6, 7, 6, 5, 4};
uint8_t ringOf[NUMPIXELS];
float pxAngle[NUMPIXELS];
uint8_t spiralOrder[NUMPIXELS];
uint8_t ringMembers[4][18];
uint8_t ringSize[4] = {0, 0, 0, 0};
float gX[NUMPIXELS], gY[NUMPIXELS];

uint8_t nearestPixel(float x, float y) {
  uint8_t best = 0;
  float bd = 1e9f;
  for (uint8_t i = 0; i < NUMPIXELS; i++) {
    float dx = gX[i] - x, dy = gY[i] - y, d = dx * dx + dy * dy;
    if (d < bd) { bd = d; best = i; }
  }
  return best;
}

void buildGeometry() {
  uint8_t idx = 0;
  for (uint8_t r = 0; r < 7; r++) {
    uint8_t n = ROW_COUNT[r];
    float y = (3.0f - (float)r) * 0.8660254f;
    for (uint8_t j = 0; j < n; j++) {
      float x = (float)j - (float)(n - 1) / 2.0f;
      gX[idx] = x;
      gY[idx] = y;
      float d = sqrtf(x * x + y * y);
      uint8_t ring = (uint8_t)lroundf(d);
      if (ring > 3) ring = 3;
      ringOf[idx] = ring;
      pxAngle[idx] = atan2f(y, x);
      idx++;
    }
  }
  for (uint8_t i = 0; i < NUMPIXELS; i++) spiralOrder[i] = i;
  for (uint8_t i = 1; i < NUMPIXELS; i++) {
    uint8_t key = spiralOrder[i];
    int8_t k = i - 1;
    while (k >= 0) {
      uint8_t a = spiralOrder[k];
      bool greater = (ringOf[a] > ringOf[key]) ||
                     (ringOf[a] == ringOf[key] && pxAngle[a] > pxAngle[key]);
      if (!greater) break;
      spiralOrder[k + 1] = spiralOrder[k];
      k--;
    }
    spiralOrder[k + 1] = key;
  }
  for (uint8_t i = 0; i < NUMPIXELS; i++)
    ringMembers[ringOf[i]][ringSize[ringOf[i]]++] = i;
  for (uint8_t r = 0; r < 4; r++)
    for (uint8_t i = 1; i < ringSize[r]; i++) {
      uint8_t key = ringMembers[r][i];
      int8_t k = i - 1;
      while (k >= 0 && pxAngle[ringMembers[r][k]] > pxAngle[key]) {
        ringMembers[r][k + 1] = ringMembers[r][k];
        k--;
      }
      ringMembers[r][k + 1] = key;
    }
}

inline uint8_t gam(uint8_t v) { return gGamma ? Adafruit_NeoPixel::gamma8(v) : v; }

#if STUDIO_SENSOR_TRIAD
void captureTiltZero() {
  float mag = sqrtf(gAx * gAx + gAy * gAy + gAz * gAz);
  if (!gMsaReadOk || !isfinite(mag) || mag < 0.05f) return;
  gRestX = gAx / mag;
  gRestY = gAy / mag;
  gRestZ = gAz / mag;
  gTiltZeroed = true;
  gTiltDeg = 0.0f;
  Serial.printf("LED Studio tilt zero: [%.3f %.3f %.3f]\n", gRestX, gRestY, gRestZ);
}

void capturePressureZero() {
  if (!gBmpReadOk || !isfinite(gPressureFilteredHpa) || gPressureFilteredHpa <= 0.0f)
    return;
  gPressureZeroHpa = gPressureFilteredHpa;
  gRelativeAltitudeM = 0.0f;
  Serial.printf("LED Studio elevation zero: %.3f hPa\n", gPressureZeroHpa);
}

void handleTmfMeasurement(struct tmf882x_msg_meas_results *results) {
  if (!results) return;

  gTmfReadOk = true;
  gTmfReads++;
  gTmfLastReadMs = millis();
  gTofRawClosestMm = 0;
  uint16_t usableMm = 0, usableConf = 0;
  uint32_t count = min((uint32_t)TMF882X_MAX_MEAS_RESULTS,
                       results->num_results);
  for (uint32_t i = 0; i < count; ++i) {
    const tmf882x_meas_result &r = results->results[i];
    if (r.distance_mm == 0 || r.distance_mm > UINT16_MAX || r.confidence == 0)
      continue;
    uint16_t mm = (uint16_t)r.distance_mm;
    if (gTofRawClosestMm == 0 || mm < gTofRawClosestMm) gTofRawClosestMm = mm;
    // The enclosed sensor has a known ~20 mm fixture/window return. Ignore
    // that near-field geometry and use the closest confident scene target.
    if (mm < 80 || mm > 2500 || r.confidence < 20) continue;
    if (usableMm == 0 || mm < usableMm) {
      usableMm = mm;
      usableConf = (uint16_t)min((uint32_t)UINT16_MAX, r.confidence);
    }
  }
  if (usableMm) {
    gTofDepthMm = usableMm;
    gTofConfidence = usableConf;
    if (!isfinite(gTofDepthFilteredMm))
      gTofDepthFilteredMm = usableMm;
    else
      gTofDepthFilteredMm += 0.35f * ((float)usableMm - gTofDepthFilteredMm);
  }
}

void sensorTriadInit() {
  Wire1.setClock(100000);
  gMsaPresent = studioMsa.begin(MSA311_I2CADDR_DEFAULT, &Wire1);
  if (gMsaPresent) {
    studioMsa.setRange(MSA301_RANGE_4_G);
    studioMsa.setDataRate(MSA301_DATARATE_125_HZ);
    studioMsa.setBandwidth(MSA301_BANDWIDTH_62_5_HZ);
    studioMsa.setPowerMode(MSA301_NORMALMODE);
  }

  Wire1.setClock(100000);
  gBmpPresent = studioBmp.begin(BMP5XX_ALTERNATIVE_ADDRESS, &Wire1);
  if (gBmpPresent) {
    studioBmp.setTemperatureOversampling(BMP5XX_OVERSAMPLING_2X);
    studioBmp.setPressureOversampling(BMP5XX_OVERSAMPLING_16X);
    studioBmp.setIIRFilterCoeff(BMP5XX_IIR_FILTER_COEFF_3);
    studioBmp.setOutputDataRate(BMP5XX_ODR_10_HZ);
    studioBmp.setPowerMode(BMP5XX_POWERMODE_NORMAL);
    studioBmp.enablePressure(true);
  }

  Wire1.setClock(100000);
  gTmfPresent = studioTmf.begin(Wire1);
  if (gTmfPresent) {
    struct tmf882x_mode_app_config cfg;
    if (studioTmf.getTMF882XConfig(cfg)) {
      cfg.report_period_ms = 250;
      studioTmf.setTMF882XConfig(cfg);
    }
    studioTmf.setMeasurementHandler(handleTmfMeasurement);
    gTmfActive =
        tmf882x_start(&studioTmf.getTMF882XContext()) == 0;
    if (gTmfActive) {
      gTmfCycleStartMs = millis();
      gTmfCycleReadBase = gTmfReads;
    } else {
      gTmfErrors++;
      gTmfNextStartMs = millis() + 500;
    }
  }
  Wire1.setClock(100000);
  Serial.printf("LED Studio sensor triad @100kHz: MSA311=%d TMF8820=%d "
                "(async=%d) BMP581=%d\n",
                gMsaPresent, gTmfPresent, gTmfActive, gBmpPresent);
}

void sensorTriadTick() {
  uint32_t now = millis();
  static uint32_t nextMsaMs = 0, nextBmpMs = 0, nextTmfServiceMs = 0;

  if (gMsaPresent && now >= nextMsaMs) {
    nextMsaMs = now + 40;
    Wire1.setClock(100000);
    studioMsa.read();
    gAx = studioMsa.x_g;
    gAy = studioMsa.y_g;
    gAz = studioMsa.z_g;
    float mag = sqrtf(gAx * gAx + gAy * gAy + gAz * gAz);
    gMsaReadOk = isfinite(mag) && mag > 0.05f;
    if (gMsaReadOk) {
      if (!gTiltZeroed) captureTiltZero();
      float nx = gAx / mag, ny = gAy / mag, nz = gAz / mag;
      float dot = constrain(nx * gRestX + ny * gRestY + nz * gRestZ, -1.0f, 1.0f);
      gTiltDeg = acosf(dot) * 57.2957795f;
    }
  }

  if (gBmpPresent && now >= nextBmpMs) {
    nextBmpMs = now + 100;
    Wire1.setClock(100000);
    gBmpReadOk = studioBmp.performReading();
    if (gBmpReadOk) {
      gPressureHpa = studioBmp.pressure;
      if (!isfinite(gPressureFilteredHpa))
        gPressureFilteredHpa = gPressureHpa;
      else
        gPressureFilteredHpa += 0.25f * (gPressureHpa - gPressureFilteredHpa);
      if (!isfinite(gPressureZeroHpa)) capturePressureZero();
      if (isfinite(gPressureZeroHpa) && gPressureZeroHpa > 0.0f) {
        float ratio = gPressureFilteredHpa / gPressureZeroHpa;
        gRelativeAltitudeM = 44330.0f * (1.0f - powf(ratio, 0.19029495f));
      }
    }
  }

  // Cooperatively execute the same start -> process IRQ -> stop sequence used by
  // SparkFun's startMeasuring(results) wrapper. The wrapper blocks until a report,
  // starving WebServer for roughly 0.7-1.8 seconds on this bus. Splitting it across
  // loop() iterations preserves that proven one-shot behavior while keeping HTTP
  // responsive. All calls remain single-threaded on Wire1 at 100 kHz.
  if (gTmfPresent && gTmfActive && now >= nextTmfServiceMs) {
    nextTmfServiceMs = now + 10;
    Wire1.setClock(100000);
    int32_t rc = tmf882x_process_irq(&studioTmf.getTMF882XContext());
    if (gTmfReads != gTmfCycleReadBase) {
      // A complete report arrived through handleTmfMeasurement(). Stop before
      // starting a fresh asynchronous shot, just as the high-level wrapper does.
      tmf882x_stop(&studioTmf.getTMF882XContext());
      gTmfActive = false;
      gTmfNextStartMs = now + 50;
    } else if (rc != 0 || now - gTmfCycleStartMs > 700) {
      gTmfErrors++;
      tmf882x_stop(&studioTmf.getTMF882XContext());
      gTmfActive = false;
      gTmfReadOk = false;
      gTmfRecoveries++;
      gTmfNextStartMs = now + 500;
    }
    Wire1.setClock(100000);
  }

  if (gTmfPresent && !gTmfActive && now >= gTmfNextStartMs) {
    Wire1.setClock(100000);
    gTmfActive =
        tmf882x_start(&studioTmf.getTMF882XContext()) == 0;
    if (gTmfActive) {
      gTmfCycleStartMs = now;
      gTmfCycleReadBase = gTmfReads;
    } else {
      gTmfErrors++;
      gTmfRecoveries++;
      gTmfNextStartMs = now + 500;
    }
    Wire1.setClock(100000);
  }
}
#else
void sensorTriadInit() {}
void sensorTriadTick() {}
#endif

#if STUDIO_L5CX
// ---- VL53L5CX presence (perimeter demo) ------------------------------------
// Loop-idiom port of sway_demo's proven tofInit/tofApply/tofTick (that sketch's
// loop-context L5CX is the sanctioned pattern; presence_bench's core-0 task
// quarantine is forbidden on this bus). The only multi-second call is begin()'s
// firmware upload, which runs in setup() BEFORE the web server exists; a begin()
// against an absent device NACKs fast. In-session recovery uses only the quick
// stop/config/start l5cxApply() path (2026-07-29 lesson: never block the UI on
// sensor calls from loop()). All access single-threaded on Wire1 at 100 kHz.
#define L5CX_RES 4
#define L5CX_ZONES (L5CX_RES * L5CX_RES)
#define L5CX_HZ 10        // sway_demo's proven 4x4 @ 10 Hz operating point
#define L5CX_FLOOR_MM 35  // enclosure window near-field return (~20 mm) sits below this
#define L5CX_CEIL_MM 3500
#define L5CX_HYST_MM 40
// Depth-driven interaction (2026-07-30 v2, Ben's spec): the piece idles RED and
// walks the ROYGBIV wheel as a visitor nears -- hue = (max - depth)/(max - min)
// -- while the gobo stays a near-touch surprise gated by the thresh slider.
// Visitor depth is measured AGAINST the learned scene (anomaly = zone reads
// closer than baseline by max(200 mm, 20%)), so static clutter reads as empty
// and someone is tracked from max range the moment they enter the FoV. Gobo
// triggers: near (<= thresh) OR occlusion (>=60% of baseline-valid zones lose
// their target -- a palm covering the window returns non-valid statuses).
// Baselines: unset zones act as a FAR sentinel (a person in a previously empty
// zone is anomalous immediately -- v1 seeded their entry point and popped the
// gobo at 1.6 m); adaptation is asymmetric (revealed-farther adapts fast,
// closer adapts on a ~1 min tau so a lingering visitor slowly fades back to
// scene; Re-zero for instant fixes) and pauses entirely during presence.
#define L5CX_ANOM_MIN_MM 200
#define L5CX_FAR_SENTINEL_MM 4000.0f
#define L5CX_COLOR_MAX_MM 2500.0f
#define L5CX_OCCL_MIN_BASE 6
SparkFun_VL53L5CX gVl;
bool gVlPresent = false, gVlRanging = false;
uint32_t gVlRetryAtMs = 0, gVlLastFrameMs = 0;
uint32_t gVlReads = 0, gVlErrors = 0;
int16_t gVlClosestMm = 0;        // closest valid scene target (0 = clear)
int16_t gVlZoneMm[L5CX_ZONES];   // per-zone closest valid target (-1 = none)
float gVlBaseMm[L5CX_ZONES];     // learned static scene (-1 = unset/far)
uint32_t gVlBaseFastUntilMs = 0; // fast-seed window after (re)zero
uint16_t gPresenceThreshMm = 500; // gobo threshold; /set?thresh= (40..600).
// 500 mm default from dad-testing 2026-07-30: visitors step right up to the
// piece but don't know where the sensor is, so arm's-length pops the surprise.
bool gPresence = false;
uint8_t gPresenceWhy = 0; // bit0 near, bit2 occlusion (last hit)
uint8_t gPresenceHits = 0, gPresenceMisses = 0;
int16_t gVlVisitorMm = -1; // closest anomalous (closer-than-scene) target
float gColorT = 0.0f;      // smoothed 0..1 rainbow position (0 = red/idle)

void l5cxZeroScene() {
  for (uint8_t z = 0; z < L5CX_ZONES; z++) {
    gVlBaseMm[z] = -1.0f;
    gVlZoneMm[z] = -1;
  }
  gVlBaseFastUntilMs = millis() + 3000;
}

bool l5cxApply() {
  // Only stop if actually ranging: stop_ranging on a fresh device hangs on an
  // MCU-stop bit that never asserts (see src/vl53l5cx_uld/VENDORED.md).
  if (gVlRanging) {
    gVl.stopRanging();
    gVlRanging = false;
  }
  if (!gVl.setResolution(L5CX_ZONES)) { Serial.println("[l5cx] setResolution FAILED"); return false; }
  if (!gVl.setRangingFrequency(L5CX_HZ)) { Serial.println("[l5cx] setRangingFrequency FAILED"); return false; }
  gVlRanging = gVl.startRanging();
  if (!gVlRanging) Serial.println("[l5cx] startRanging FAILED");
  gVlLastFrameMs = millis();
  // Arm the fast-seed window from RANGING START, not init entry: begin()'s
  // multi-second fw upload otherwise eats the window before the first frame,
  // and the scene gets learned from the FAR sentinel at the ~1 min tau
  // (observed on .4 first boot: baseline ~3.7 m over a 1.2 m garage).
  if (gVlRanging) gVlBaseFastUntilMs = millis() + 3000;
  return gVlRanging;
}

void l5cxInit() {
  Wire1.begin();
  Wire1.setClock(100000);
  l5cxZeroScene();
  uint32_t t0 = millis();
  Serial.println("[l5cx] VL53L5CX begin (fw blob upload over 100 kHz I2C, several s)...");
  if (!gVl.begin(0x29, Wire1)) {
    Serial.println("[l5cx] begin FAILED (absent/unpowered? retry in 30 s)");
    gVlPresent = false;
    gVlRetryAtMs = millis() + 30000;
    return;
  }
  gVl.setWireMaxPacketSize(124); // ESP32 Wire buffer is 128
  gVlPresent = true;
  bool ok = l5cxApply();
  Serial.printf("[l5cx] up in %lu ms: %dx%d @ %d Hz, presence <= %u mm (+%u hyst)\n",
                (unsigned long)(millis() - t0), L5CX_RES, L5CX_RES, L5CX_HZ,
                gPresenceThreshMm, (unsigned)L5CX_HYST_MM);
  if (!ok) gVlRetryAtMs = millis() + 30000;
}

void l5cxTick() {
  uint32_t now = millis();
  if (!gVlPresent || !gVlRanging) {
    if (gVlRetryAtMs && now >= gVlRetryAtMs) {
      gVlRetryAtMs = 0;
      l5cxInit(); // absent device NACKs in ms; full upload only on real recovery
    }
    return;
  }
  static uint32_t nextPollMs = 0;
  if (now < nextPollMs) return;
  nextPollMs = now + 1000 / L5CX_HZ / 2; // poll at 2x frame rate for low latency
  Wire1.setClock(100000);
  if (now - gVlLastFrameMs > 5000) { // sway_demo-style self-heal: quick re-apply
    Serial.println("[l5cx] ranging stalled -> re-apply");
    if (!l5cxApply()) {
      gVlErrors++;
      gVlRetryAtMs = now + 30000;
    }
    return;
  }
  if (!gVl.isDataReady()) return;
  static VL53L5CX_ResultsData results;
  if (!gVl.getRangingData(&results)) {
    gVlErrors++;
    return;
  }
  gVlLastFrameMs = now;
  gVlReads++;
  int16_t closest = 0;
  for (uint8_t z = 0; z < L5CX_ZONES; z++) {
    uint8_t nt = results.nb_target_detected[z];
    if (nt > VL53L5CX_NB_TARGET_PER_ZONE) nt = VL53L5CX_NB_TARGET_PER_ZONE;
    int16_t zoneBest = -1;
    for (uint8_t t = 0; t < nt; t++) {
      uint16_t i = z * VL53L5CX_NB_TARGET_PER_ZONE + t;
      uint8_t st = results.target_status[i];
      int16_t d = results.distance_mm[i];
      if ((st != 5 && st != 9) || d < L5CX_FLOOR_MM || d > L5CX_CEIL_MM) continue;
      if (zoneBest < 0 || d < zoneBest) zoneBest = d;
      if (!closest || d < closest) closest = d;
    }
    gVlZoneMm[z] = zoneBest;
  }
  gVlClosestMm = closest;

  // Visitor depth = closest target that is anomalous vs the learned scene
  // (unset zones count as FAR, so a person entering an empty sightline is
  // anomalous immediately, from max range). Static clutter is scene, not visitor.
  uint8_t why = 0;
  uint8_t nBaseValid = 0, nLost = 0;
  int16_t visitor = -1;
  for (uint8_t z = 0; z < L5CX_ZONES; z++) {
    float base = gVlBaseMm[z] > 0 ? gVlBaseMm[z] : L5CX_FAR_SENTINEL_MM;
    int16_t cur = gVlZoneMm[z];
    if (gVlBaseMm[z] > 0) {
      nBaseValid++;
      if (cur < 0) nLost++;
    }
    if (cur >= 0) {
      float margin = base * 0.20f;
      if (margin < L5CX_ANOM_MIN_MM) margin = L5CX_ANOM_MIN_MM;
      if ((float)cur < base - margin && (visitor < 0 || cur < visitor))
        visitor = cur;
    }
  }
  gVlVisitorMm = visitor;
  // Near tests the VISITOR (anomalous-vs-scene), not the raw closest return:
  // at a 500 mm gobo zone, static clutter inside the threshold must not pin
  // the gobo on. A partially-covering palm is anomalous and still fires here;
  // a fully-covering palm fires occlusion below.
  if (visitor >= 0 && visitor <= (int16_t)(gPresence ? gPresenceThreshMm + L5CX_HYST_MM
                                                     : gPresenceThreshMm))
    why |= 1;
  if (nBaseValid >= L5CX_OCCL_MIN_BASE && nLost * 10 >= nBaseValid * 6) why |= 4;

  // Rainbow position: red (0) when the scene is empty, toward violet as the
  // visitor closes on the gobo threshold -- t = (max - depth)/(max - min).
  float target = 0.0f;
  if (gPresence) {
    target = 1.0f; // hold the top of the wheel under the gobo
  } else if (visitor >= 0) {
    float mind = (float)gPresenceThreshMm;
    target = (L5CX_COLOR_MAX_MM - (float)visitor) / (L5CX_COLOR_MAX_MM - mind);
    if (target < 0.0f) target = 0.0f;
    if (target > 1.0f) target = 1.0f;
  }
  // Track toward the target fast, decay back to red gently when they leave.
  // Snap, don't sweep, into the gobo: while a trigger is confirming (why set,
  // presence not yet latched) the wheel HOLDS -- a hand appearing suddenly
  // from the blind spot otherwise zips the full rainbow for ~300 ms before
  // the white snap. Ben 2026-07-30: leaning snap but undecided -- delete the
  // `!why` guard to bring the zip back.
  float prevT = gColorT;
  if (gPresence) {
    gColorT = 1.0f; // hold violet under the white; release decays from the top
  } else if (!why) {
    float alpha = target > gColorT ? 0.45f : 0.12f;
    gColorT += alpha * (target - gColorT);
  }

  // Scene learning: asymmetric. Revealed-farther adapts fast (an occluder
  // left); closer adapts on a ~1 min tau (a lingering visitor slowly becomes
  // scenery and the piece fades back to red); frozen entirely during presence.
  // Fast-seed window after boot/Re-zero captures the true empty scene.
  if (!gPresence) {
    bool fastSeed = millis() < gVlBaseFastUntilMs;
    for (uint8_t z = 0; z < L5CX_ZONES; z++) {
      int16_t cur = gVlZoneMm[z];
      if (cur < 0) continue;
      if (fastSeed) {
        gVlBaseMm[z] = gVlBaseMm[z] <= 0 ? (float)cur
                                         : gVlBaseMm[z] + 0.5f * ((float)cur - gVlBaseMm[z]);
        continue;
      }
      float base = gVlBaseMm[z] > 0 ? gVlBaseMm[z] : L5CX_FAR_SENTINEL_MM;
      // Closer-alpha 0.002 = a ~1 min tau: a perfectly still visitor slowly
      // fades the piece back to red. Ben likes this (2026-07-30) and it keeps
      // the baseline self-healing; if Nevada City opinions prefer stationary
      // visitors to hold their color indefinitely, set the closer alpha to 0
      // (people never absorb; Re-zero then handles real scene changes).
      float a = (float)cur > base ? 0.2f : 0.002f;
      gVlBaseMm[z] = base + a * ((float)cur - base);
    }
  }

  // Gobo: enter after 2 consecutive hit-frames (~200 ms), release after 3
  // consecutive clear frames; near path keeps +hyst while held.
  bool prior = gPresence;
  if (!gPresence) {
    gPresenceHits = why ? (uint8_t)(gPresenceHits + 1) : 0;
    if (gPresenceHits >= 2) {
      gPresence = true;
      gPresenceMisses = 0;
    }
  } else {
    gPresenceMisses = why ? 0 : (uint8_t)(gPresenceMisses + 1);
    if (gPresenceMisses >= 3) {
      gPresence = false;
      gPresenceHits = 0;
    }
  }
  if (why) gPresenceWhy = why;
  // Latency is the art (Ben 2026-07-30): render at sensor rate on any material
  // change -- gobo transitions and visible color motion -- instead of waiting
  // out the speed-paced frame timer.
  if ((gPresence != prior || fabsf(gColorT - prevT) > 0.01f) &&
      gMode == MODE_HEX && gAnim == 5 && !gFrozen) {
    lastFrame = millis();
    renderFrame();
  }
}
#else
void l5cxInit() {}
void l5cxTick() {}
#endif

// ---- HEX rendering ---------------------------------------------------------
void setPxHex(uint16_t i, float factor) {
  if (factor < 0) factor = 0;
  if (factor > 1) factor = 1;
  float s = (float)gBri / 255.0f * factor;
  strip.setPixelColor(i, gam((uint8_t)(gR * s)), gam((uint8_t)(gG * s)),
                       gam((uint8_t)(gB * s)));
}

bool inShape(uint8_t i) {
  uint8_t r = ringOf[i];
  switch (gShape) {
    case 0: return r == 0;
    case 1: return r <= 1;
    case 2: return r <= 2;
    default: return true;
  }
}

// Split the base point into pure R/G/B across three pixels (scaled by factor,
// max-combining on overlap), by the current style:
//  - gSplit==1 (triad):  R/G/B on a small triangle OFFSET from baseIdx by `spread`
//    at 120 deg apart (orientation = `rotate`). A local color-fringe cluster.
//  - gSplit==2 (rotate): R at baseIdx itself, G/B at baseIdx ROTATED 120/240 deg
//    about the grid center (same radius). A 3-fold rotationally-symmetric split.
void splitInto(uint8_t baseIdx, float factor, uint8_t *R, uint8_t *G, uint8_t *B) {
  const float T = 2.0943951f; // 120 deg
  if (factor < 0) factor = 0;
  if (factor > 1) factor = 1;
  uint8_t v = (uint8_t)((float)gBri * factor);
  float ax = gX[baseIdx], ay = gY[baseIdx];
  uint8_t pr, pg, pb;
  if (gSplit == 2) { // rotate the point 120/240 deg about the center
    float rad = sqrtf(ax * ax + ay * ay), th = atan2f(ay, ax);
    pr = baseIdx;
    pg = nearestPixel(rad * cosf(th + T), rad * sinf(th + T));
    pb = nearestPixel(rad * cosf(th + 2 * T), rad * sinf(th + 2 * T));
  } else { // local triad offset by `spread`
    pr = nearestPixel(ax + gSpread * cosf(gFringeAngle),
                      ay + gSpread * sinf(gFringeAngle));
    pg = nearestPixel(ax + gSpread * cosf(gFringeAngle + T),
                      ay + gSpread * sinf(gFringeAngle + T));
    pb = nearestPixel(ax + gSpread * cosf(gFringeAngle + 2 * T),
                      ay + gSpread * sinf(gFringeAngle + 2 * T));
  }
  R[pr] = max(R[pr], v);
  G[pg] = max(G[pg], v);
  B[pb] = max(B[pb], v);
}

void showSplit(uint8_t *R, uint8_t *G, uint8_t *B) {
  for (uint16_t i = 0; i < NUMPIXELS; i++)
    strip.setPixelColor(i, gam(R[i]), gam(G[i]), gam(B[i]));
  strip.show();
}

void renderStaticHex() {
  if (gSplit) { // static split at the anchor
    uint8_t R[NUMPIXELS] = {0}, G[NUMPIXELS] = {0}, B[NUMPIXELS] = {0};
    splitInto(gAnchor, 1.0f, R, G, B);
    lastLit = gAnchor;
    showSplit(R, G, B);
    return;
  }
  for (uint16_t i = 0; i < NUMPIXELS; i++) setPxHex(i, inShape(i) ? 1.0f : 0.0f);
  strip.show();
}

// Map a monotonic step counter to a position along the path. Orbit (anim 2) wraps
// seamlessly around the ring; Spiral (anim 1) ping-pongs (0..n-1..1..0) so it
// reverses at the ends instead of jumping from the outer tip back to the center.
int pathIndex(long step, int n) {
  if (n <= 1) return 0;
  if (gAnim == 2) return (int)(((step % n) + n) % n); // orbit: seamless ring wrap
  long period = 2 * (n - 1);                          // spiral: ping-pong
  long m = ((step % period) + period) % period;
  return (int)(m < n ? m : period - m);
}

void renderFrameHex() {
  switch (gAnim) {
    case 1:
    case 2: {
      const uint8_t *order;
      uint16_t n;
      if (gAnim == 1) { order = spiralOrder; n = NUMPIXELS; }
      else {
        uint8_t r = gOrbitRing < 1 ? 1 : (gOrbitRing > 3 ? 3 : gOrbitRing);
        order = ringMembers[r];
        n = ringSize[r];
      }
      if (gSplit) { // a moving split at the head + each trail step
        uint8_t R[NUMPIXELS] = {0}, G[NUMPIXELS] = {0}, B[NUMPIXELS] = {0};
        for (int t = 0; t <= gTrail; t++) {
          int p = pathIndex((long)hexAnimPos - t, n);
          float f = 1.0f - (float)t / (float)(gTrail + 1);
          splitInto(order[p], f, R, G, B);
          if (t == 0) lastLit = order[p];
        }
        showSplit(R, G, B);
      } else {
        for (uint16_t i = 0; i < NUMPIXELS; i++) strip.setPixelColor(i, 0);
        for (int t = 0; t <= gTrail; t++) {
          int p = pathIndex((long)hexAnimPos - t, n);
          float f = 1.0f - (float)t / (float)(gTrail + 1);
          setPxHex(order[p], f);
          if (t == 0) lastLit = order[p];
        }
        strip.show();
      }
      if (!gFrozen) hexAnimPos++;
      break;
    }
    case 3: {
      float f = 0.5f + 0.5f * sinf(hexBreathePhase);
      if (gSplit) {
        uint8_t R[NUMPIXELS] = {0}, G[NUMPIXELS] = {0}, B[NUMPIXELS] = {0};
        splitInto(gAnchor, f, R, G, B);
        lastLit = gAnchor;
        showSplit(R, G, B);
      } else {
        for (uint16_t i = 0; i < NUMPIXELS; i++) setPxHex(i, inShape(i) ? f : 0.0f);
        strip.show();
      }
      if (!gFrozen) hexBreathePhase += 0.15f;
      break;
    }
    case 4: {
      for (uint16_t i = 0; i < NUMPIXELS; i++) {
        uint32_t c = strip.getPixelColor(i);
        uint8_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
        strip.setPixelColor(i, r * 7 / 8, g * 7 / 8, b * 7 / 8);
      }
      if (!gFrozen && (esp_random() & 0x3) == 0) {
        uint8_t tries = 0, i;
        do { i = esp_random() % NUMPIXELS; } while (!inShape(i) && ++tries < 20);
        if (inShape(i)) { lastLit = i; setPxHex(i, 1.0f); }
      }
      strip.show();
      break;
    }
#if STUDIO_L5CX
    case 5: { // Presence: depth-driven ROYGBIV on all 37 px <-> center-white gobo.
      // Idles pure red; gColorT (0..1, from visitor depth vs the learned scene)
      // walks R->O->Y->G->B->I->V as they close on the gobo threshold.
      if (gPresence) {
        for (uint16_t i = 0; i < NUMPIXELS; i++) strip.setPixelColor(i, 0);
        uint8_t v = gam(gBri); // white pinned to the brightness slider (255 default)
        strip.setPixelColor(CENTER, v, v, v);
        lastLit = CENTER;
        strip.show();
        break;
      }
      static const uint8_t RB[7][3] = {{255, 0, 0},  {255, 127, 0}, {255, 255, 0},
                                       {0, 255, 0},  {0, 0, 255},   {75, 0, 130},
                                       {148, 0, 211}};
      float phase = gColorT * 6.0f;
      if (phase > 5.999f) phase = 5.999f;
      uint8_t k = (uint8_t)phase;
      float f = phase - k;
      uint8_t cr = (uint8_t)(RB[k][0] + (int)((RB[k + 1][0] - RB[k][0]) * f));
      uint8_t cg = (uint8_t)(RB[k][1] + (int)((RB[k + 1][1] - RB[k][1]) * f));
      uint8_t cb = (uint8_t)(RB[k][2] + (int)((RB[k + 1][2] - RB[k][2]) * f));
      float s = (float)gBri / 255.0f;
      uint8_t pr = gam((uint8_t)(cr * s)), pg = gam((uint8_t)(cg * s)),
              pb = gam((uint8_t)(cb * s));
      for (uint16_t i = 0; i < NUMPIXELS; i++) strip.setPixelColor(i, pr, pg, pb);
      lastLit = CENTER;
      strip.show();
      break;
    }
#endif
    default:
      break;
  }
}

// ---- RGBW rendering --------------------------------------------------------
void setRGBWpix(uint8_t r, uint8_t g, uint8_t b, uint8_t w, float f) {
  float s = (float)gBri / 255.0f * f;
  if (s < 0) s = 0;
  if (s > 1) s = 1;
  strip.setPixelColor(0, gam((uint8_t)(r * s)), gam((uint8_t)(g * s)),
                       gam((uint8_t)(b * s)), gam((uint8_t)(w * s)));
  strip.show();
}

void renderStaticRGBW() { setRGBWpix(gR, gG, gB, gW, 1.0f); }

void renderFrameRGBW() {
  switch (gAnim) {
    case 1: { // hue cycle
      uint16_t hue = (uint16_t)((uint32_t)rgbwPhase & 0xFFFF);
      uint32_t c = strip.ColorHSV(hue, 255, gBri);
      if (gGamma) c = strip.gamma32(c);
      strip.setPixelColor(0, c);
      strip.show();
      rgbwPhase += 256;
      break;
    }
    case 2: { // breathe
      float f = 0.5f + 0.5f * sinf(rgbwPhase);
      setRGBWpix(gR, gG, gB, gW, f);
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
#if STUDIO_SENSOR_TRIAD
    case 5: { // ToF: selected color brightens as a scene target approaches
      if (!gTmfReadOk || !isfinite(gTofDepthFilteredMm)) {
        setRGBWpix(255, 0, 0, 0, 0.12f);
        break;
      }
      float proximity = constrain((1200.0f - gTofDepthFilteredMm) / 1080.0f,
                                  0.0f, 1.0f);
      setRGBWpix(gR, gG, gB, gW, 0.08f + 0.92f * proximity);
      break;
    }
    case 6: { // MSA311: selected color brightens with relative tilt
      if (!gMsaReadOk) {
        setRGBWpix(255, 0, 0, 0, 0.12f);
        break;
      }
      float amount = constrain(gTiltDeg / 35.0f, 0.0f, 1.0f);
      setRGBWpix(gR, gG, gB, gW, 0.10f + 0.90f * amount);
      break;
    }
    case 7: { // BMP581: +/-1.5 m around zero maps dim -> bright
      if (!gBmpReadOk || !isfinite(gPressureZeroHpa)) {
        setRGBWpix(255, 0, 0, 0, 0.12f);
        break;
      }
      float amount = constrain((gRelativeAltitudeM + 1.5f) / 3.0f, 0.0f, 1.0f);
      setRGBWpix(gR, gG, gB, gW, 0.08f + 0.92f * amount);
      break;
    }
#endif
    default:
      break;
  }
}

// ---- Mode + dispatch -------------------------------------------------------
void applyMode() {
  strip.clear();
  strip.show();
  if (gMode == MODE_RGBW) {
    // RGBW, not GRBW: slot-tested on the production 4 W module 2026-07-11
    // (led_sol_bench /raw) -- GRBW had R/G silently swapped in every prior
    // studio session. MODE_RGB below is a different module and is UNVERIFIED.
    strip.updateType(NEO_RGBW + NEO_KHZ800);
    strip.updateLength(1);
  } else if (gMode == MODE_RGB) {
    strip.updateType(NEO_GRB + NEO_KHZ800);
    strip.updateLength(1);
  } else {
    strip.updateType(NEO_GRB + NEO_KHZ800);
    strip.updateLength(NUMPIXELS);
  }
  strip.clear();
  strip.show();
  gAnim = 0;
  hexAnimPos = 0;
  hexBreathePhase = 0;
  rgbwPhase = 0;
  candleLevel = candleTarget = 1.0f;
}

bool isAnimating() {
#if STUDIO_L5CX
  // Anim 5 (presence) must keep rendering every frame to track the sensor.
  if (gMode == MODE_HEX) return gAnim >= 1 && gAnim <= 5 && !gFrozen;
#else
  if (gMode == MODE_HEX) return gAnim >= 1 && gAnim <= 4 && !gFrozen;
#endif
#if STUDIO_SENSOR_TRIAD
  return gAnim >= 1 && gAnim <= 7;
#else
  return gAnim >= 1 && gAnim <= 4;
#endif
}

void renderStatic() {
  if (gMode == MODE_HEX)
    renderStaticHex();
  else
    renderStaticRGBW();
}

void renderFrame() {
  if (gMode == MODE_HEX)
    renderFrameHex();
  else
    renderFrameRGBW();
}

void applyAfterSet() {
  if (gMode == MODE_HEX) {
    if (gAnim == 0) renderStaticHex();
    else if (gFrozen) renderFrameHex();
  } else {
    if (gAnim == 0) renderStaticRGBW();
  }
}

// ---- Web UI ----------------------------------------------------------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>LED Studio</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:14px;max-width:520px}
 h2{margin:.2em 0}
 .row{margin:10px 0}
 label{display:block;font-size:13px;color:#aaa;margin-bottom:3px}
 input[type=range]{width:100%;height:30px}
 .btns{display:flex;flex-wrap:wrap;gap:6px}
 button{flex:1 1 auto;min-width:64px;padding:11px 8px;font-size:14px;border:0;border-radius:8px;background:#333;color:#eee}
 button.on{background:#0a7;color:#fff}
 .mode button{padding:13px;font-size:15px}
 #rb,.readback{font-family:monospace;font-size:13px;background:#000;padding:8px;border-radius:6px;white-space:pre-wrap;color:#6f6}
 .sw{display:inline-block;width:22px;height:22px;border-radius:5px;vertical-align:middle;border:1px solid #555}
 .hide{display:none}
 hr{border:0;border-top:1px solid #333;margin:14px 0}
</style></head><body>
<h2>LED Studio</h2>

<div class=row><label>Module (blank LEDs + swap before toggling)</label><div class="btns mode">
 <button id=md0 onclick="mode(0)">HEX grid (37px)</button>
 <button id=md1 onclick="mode(1)">RGBW point</button>
 <button id=md2 onclick="mode(2)">RGB point</button>
</div></div>
<hr>

<div class=row><label>Color (RGB) <span id=sw class=sw></span></label>
 <input type=color id=col value="#ff8c28" oninput="setCol(this.value)"></div>
<div class=row><label>R <span id=rl></span></label><input type=range id=r min=0 max=255 value=255 oninput="ch('r',this.value)"></div>
<div class=row><label>G <span id=gl></span></label><input type=range id=g min=0 max=255 value=140 oninput="ch('g',this.value)"></div>
<div class=row><label>B <span id=bl></span></label><input type=range id=b min=0 max=255 value=40 oninput="ch('b',this.value)"></div>
<div class=row id=wrow><label>W (white die, RGBW) <span id=wl></span></label><input type=range id=w min=0 max=255 value=0 oninput="ch('w',this.value)"></div>
<div class=row><label>Brightness <span id=bril></span></label><input type=range id=bri min=0 max=255 value=40 oninput="ch('bri',this.value)"></div>
<div class=row><label>Speed <span id=spl></span></label><input type=range id=sp min=1 max=100 value=30 oninput="ch('speed',this.value)"></div>

<!-- HEX-only controls -->
<div id=hexUI>
<hr>
<div class=row><label>Shape</label><div class=btns>
 <button id=sh0 onclick="shape(0)">Center</button>
 <button id=sh1 onclick="shape(1)">+Inner ring</button>
 <button id=sh2 onclick="shape(2)">+Two rings</button>
 <button id=sh3 onclick="shape(3)">All</button>
</div></div>
<div class=row><label>Animation</label><div class=btns>
 <button id=ah0 onclick="anim(0)">Static</button>
 <button id=ah1 onclick="anim(1)">Spiral</button>
 <button id=ah2 onclick="anim(2)">Orbit</button>
 <button id=ah3 onclick="anim(3)">Breathe</button>
 <button id=ah4 onclick="anim(4)">Twinkle</button>
 <button id=ah5 onclick="anim(5)" style="display:none">Presence</button>
</div></div>
<div class=row id=thRow style="display:none"><label>Gobo threshold (mm) <span id=thl>500</span></label>
 <input type=range id=th min=40 max=600 value=500 oninput="ch('thresh',this.value);thl.textContent=this.value"></div>
<div class=row id=zoneRow style="display:none"><label>L5CX zones, mm (red=near, orange=closer than scene, dark red=lost vs scene) <span id=whyl></span></label>
 <div id=zgrid style="display:grid;grid-template-columns:repeat(4,1fr);gap:4px"></div>
 <div class=btns style="margin-top:6px"><button onclick="send('zero_scene=1')">Re-zero scene</button></div></div>
<div class=row><label>Split RGB (applies to Static / Spiral / Orbit / Breathe)</label><div class=btns>
 <button id=sp0 onclick="splitMode(0)">Off</button>
 <button id=sp1 onclick="splitMode(1)">Triad</button>
 <button id=sp2 onclick="splitMode(2)">Rotate 120&deg;</button>
</div><label style="margin-top:4px">Triad = local R/G/B offset cluster (use Fringe spread/rotate). Rotate = R at the point, G/B the same point rotated 120/240&deg; about the grid center.</label></div>
<div class=row><label>Trail (spiral/orbit) <span id=trl></span></label><input type=range id=tr min=0 max=10 value=3 oninput="ch('trail',this.value)"></div>
<div class=row><label>Orbit ring</label><div class=btns>
 <button onclick="ch('ring',1)">1</button><button onclick="ch('ring',2)">2</button><button onclick="ch('ring',3)">3</button>
</div></div>
<div class=row><label>Fringe spread (Split) <span id=fsl></span></label><input type=range id=fs min=0 max=30 value=12 oninput="ch('spread',this.value)"></div>
<div class=row><label>Fringe rotate (Split) <span id=frl2></span></label><input type=range id=fr2 min=0 max=360 value=0 oninput="ch('rotate',this.value)"></div>
<div class=row><div class=btns>
 <button id=frz onclick="toggleFreeze()">Freeze</button>
 <button onclick="send('step=1')">Step +</button>
</div></div>
</div>

<!-- RGBW-only controls -->
<div id=rgbwUI class=hide>
<hr>
<div id=whiteBlock>
<div class=row><label>White / warmth presets</label><div class=btns>
 <button onclick="preset('wonly')">W only</button>
 <button onclick="preset('rgbw')">RGB white</button>
 <button onclick="preset('full')">RGBW full</button>
 <button onclick="preset('candle')">Warm amber</button>
</div></div>
<div class=row><label>Warmth crossfade (RGB white &harr; W) <span id=warl></span></label>
 <input type=range id=war min=0 max=100 value=0 oninput="warmth(this.value)"></div>
</div>
<div class=row><label>Animation</label><div class=btns>
 <button id=ar0 onclick="anim(0)">Static</button>
 <button id=ar1 onclick="anim(1)">Hue cycle</button>
 <button id=ar2 onclick="anim(2)">Breathe</button>
 <button id=ar3 onclick="anim(3)">Candle</button>
 <button id=ar4 onclick="anim(4)">Fade</button>
</div></div>
<div id=sensorUI class=hide>
<div class=row><label>Reactive sensor modulation</label><div class=btns>
 <button id=ar5 onclick="anim(5)">ToF depth</button>
 <button id=ar6 onclick="anim(6)">Tilt</button>
 <button id=ar7 onclick="anim(7)">Elevation</button>
</div></div>
<div class=row><div class=btns>
 <button onclick="send('zero_tilt=1')">Re-zero tilt</button>
 <button onclick="send('zero_alt=1')">Zero elevation</button>
</div></div>
<div class=row><label>Live sensors</label><div id=sensorRb class=readback>...</div></div>
</div>
<div class=row><label>Color B (for Fade)</label>
 <input type=color id=colb value="#0078ff" oninput="setColB(this.value)"></div>
</div>

<hr>
<div class=row><div class=btns>
 <button id=gam onclick="toggleGamma()">Gamma: on</button>
 <button onclick="send('off=1')">All off</button>
</div></div>
<div class=row><label>Current settings</label><div id=rb>...</div></div>
<div class=row><label>Battery</label><div id=bat>...</div></div>
<div class=row><label>Network</label><div id=net class=readback>...</div></div>

<script>
let st={mode:0,r:255,g:140,b:40,w:0,bri:40,speed:30,anim:0,gamma:1,shape:1,trail:3,ring:1,
 spread:12,rotate:0,b2r:0,b2g:120,b2b:255,frozen:0,split:0,lit:18,anchor:18,triad:0};
function send(q){fetch('/set?'+q);}
function ch(k,v){v=+v;st[k]=v;send(k+'='+v);syncLabels();}
function hx(v){return ('0'+(+v).toString(16)).slice(-2);}
function hl(p,n,cnt){for(let i=0;i<cnt;i++){let e=document.getElementById(p+i);if(e)e.className=(i==n?'on':'');}}
function applyModeUI(m){
 hl('md',m,3);
 document.getElementById('hexUI').className=(m==0?'':'hide');
 document.getElementById('rgbwUI').className=((m==1||m==2)?'':'hide');
 document.getElementById('whiteBlock').style.display=(m==1?'':'none'); // white die = RGBW only
 document.getElementById('wrow').style.display=(m==1?'':'none');
 document.getElementById('sensorUI').className=(m==1&&st.triad?'':'hide');
}
function mode(m){st.mode=m;st.anim=0;send('mode='+m);applyModeUI(m);hl('ah',0,6);hl('ar',0,8);}
function anim(n){st.anim=n;send('anim='+n);hl(st.mode==0?'ah':'ar',n,st.mode==0?6:8);}
function splitMode(n){st.split=n;send('split='+n);hl('sp',n,3);}
function shape(n){st.shape=n;send('shape='+n);hl('sh',n,4);}
function setCol(hex){let r=parseInt(hex.substr(1,2),16),g=parseInt(hex.substr(3,2),16),b=parseInt(hex.substr(5,2),16);
 st.r=r;st.g=g;st.b=b;document.getElementById('r').value=r;document.getElementById('g').value=g;document.getElementById('b').value=b;
 send('r='+r+'&g='+g+'&b='+b);syncLabels();}
function setColB(hex){st.b2r=parseInt(hex.substr(1,2),16);st.b2g=parseInt(hex.substr(3,2),16);st.b2b=parseInt(hex.substr(5,2),16);
 send('b2r='+st.b2r+'&b2g='+st.b2g+'&b2b='+st.b2b);}
function setRGBW(r,g,b,w){st.r=r;st.g=g;st.b=b;st.w=w;
 for(const k of ['r','g','b','w'])document.getElementById(k).value=st[k];
 send('r='+r+'&g='+g+'&b='+b+'&w='+w);syncLabels();}
function preset(p){if(p=='wonly')setRGBW(0,0,0,255);else if(p=='rgbw')setRGBW(255,255,255,0);
 else if(p=='full')setRGBW(255,255,255,255);else if(p=='candle')setRGBW(255,120,25,40);}
function warmth(v){let f=v/100;document.getElementById('warl').textContent=v+'%';
 let c=Math.round(255*(1-f)),wv=Math.round(255*f);setRGBW(c,c,c,wv);}
function toggleFreeze(){st.frozen^=1;send('freeze='+st.frozen);document.getElementById('frz').className=st.frozen?'on':'';}
function toggleGamma(){st.gamma^=1;send('gamma='+st.gamma);let e=document.getElementById('gam');e.textContent='Gamma: '+(st.gamma?'on':'off');e.className=st.gamma?'on':'';}
function syncLabels(){rl.textContent=st.r;gl.textContent=st.g;bl.textContent=st.b;wl.textContent=st.w;bril.textContent=st.bri;
 spl.textContent=st.speed;trl.textContent=st.trail;fsl.textContent=(st.spread/10).toFixed(1);frl2.textContent=st.rotate;
 let c='#'+hx(st.r)+hx(st.g)+hx(st.b);sw.style.background=c;col.value=c;}
let refreshing=false;
function refresh(){
 if(refreshing)return;
 refreshing=true;
 let t0=performance.now();
 fetch('/state',{cache:'no-store'}).then(r=>r.json()).then(s=>{Object.assign(st,s);applyModeUI(st.mode);
 let an=st.mode==0?['static','spiral','orbit','breathe','twinkle','presence'][s.anim]
                  :['static','hue','breathe','candle','fade','ToF depth','tilt','elevation'][s.anim];
 hl(st.mode==0?'ah':'ar',st.anim,st.mode==0?6:8);
 let ah5=document.getElementById('ah5');if(ah5)ah5.style.display=s.l5cx?'':'none';
 let thr=document.getElementById('thRow');if(thr)thr.style.display=(s.l5cx&&st.mode==0)?'':'none';
 let zrow=document.getElementById('zoneRow');if(zrow)zrow.style.display=(s.l5cx&&st.mode==0)?'':'none';
 if(s.l5cx&&s.zmm){let zg=document.getElementById('zgrid');
  if(!zg.childElementCount)for(let i=0;i<16;i++){let d=document.createElement('div');
   d.style.cssText='padding:6px 2px;text-align:center;border-radius:4px;background:#222;font-family:monospace;font-size:12px';zg.appendChild(d);}
  for(let i=0;i<16;i++){let c=zg.children[i],v=s.zmm[i],b=s.zbase[i],bg='#222';
   if(v<0){bg=b>0?'#411':'#222';}
   else if(v<=s.thresh_mm){bg='#a22';}
   else if(b>0&&v<b-Math.max(200,b*0.2)){bg='#a62';}
   else{bg='#131';}
   c.style.background=bg;c.textContent=v<0?'-':v;}
  whyl.textContent=(s.presence?'GOBO':'idle')+' '+((s.why&1)?'[near]':'')+((s.why&4)?'[occlusion]':'')+
   '  visitor='+(s.visitor_mm>=0?s.visitor_mm+'mm':'none')+'  wheel='+s.ct+'%';}
 rb.textContent='mode='+['HEX','RGBW','RGB'][st.mode]+'  anim='+an+
  '\nrgb'+(st.mode==1?'w':'')+'='+s.r+','+s.g+','+s.b+(st.mode==1?','+s.w:'')+'  hex=#'+hx(s.r)+hx(s.g)+hx(s.b)+
  '\nbri='+s.bri+'  gamma='+(s.gamma?'on':'off')+'  speed='+s.speed+
  (st.mode==0?'\nshape='+['center','+ring1','+ring2','all'][s.shape]+'  lit='+s.lit+
    (s.split?'  split='+['off','triad','rotate'][s.split]+'[anchor='+s.anchor+(s.split==1?' spread='+(st.spread/10).toFixed(1)+' rot='+st.rotate:'')+']':''):
    '\ncolorB=#'+hx(st.b2r)+hx(st.b2g)+hx(st.b2b))+
  (s.l5cx?'\nl5cx '+(s.l5cx_ok?((s.closest_mm?s.closest_mm+' mm':'clear')+'  presence='+s.presence+'  thresh='+s.thresh_mm+' mm'):'DOWN (reads '+s.l5cx_reads+', errs '+s.l5cx_errors+')'):'');
 let bat=document.getElementById('bat');
 if(!s.pf){bat.textContent='no battery data (SDK init failed)';}
 else{let act=s.ma>30?('charging +'+s.ma+'mA'):(s.ma<-30?('discharging '+s.ma+'mA'):'idle ~'+s.ma+'mA');
  bat.textContent='SOC '+s.soc+'%  '+s.bv.toFixed(3)+'V  '+act+
   (s.sgood?('  |  supply '+s.sv.toFixed(2)+'V ok'):'  |  on battery');}
 net.textContent='WiFi '+s.rssi+' dBm  |  response '+Math.round(performance.now()-t0)+' ms';
 if(s.triad){sensorRb.textContent=
  'ToF '+(s.tmf_ok?(s.tof_mm+' mm (raw '+s.tof_raw_mm+', conf '+s.tof_conf+')'):'not ready')+
  '\nTilt '+(s.msa_ok?s.tilt_deg.toFixed(1)+' deg':'not ready')+
  '\nPressure '+(s.bmp_ok?s.pressure_hpa.toFixed(3)+' hPa, elevation '+s.alt_m.toFixed(2)+' m':'not ready')+
  '\nTMF async '+(s.tmf_active?'measuring':'between shots')+
   ', age '+(s.tmf_reads?s.tmf_age_ms+' ms':'waiting')+', recoveries '+s.tmf_recoveries;}
 }).catch(e=>{net.textContent='request failed: '+e;}).finally(()=>{refreshing=false;});}
applyModeUI(0);hl('sh',1,4);hl('ah',0,6);hl('sp',0,3);syncLabels();setInterval(refresh,600);refresh();
</script></body></html>)HTML";

void handleSet() {
  if (server.hasArg("r")) gR = server.arg("r").toInt();
  if (server.hasArg("g")) gG = server.arg("g").toInt();
  if (server.hasArg("b")) gB = server.arg("b").toInt();
  if (server.hasArg("w")) gW = server.arg("w").toInt();
  if (server.hasArg("bri")) gBri = server.arg("bri").toInt();
  if (server.hasArg("speed")) gSpeed = server.arg("speed").toInt();
  if (server.hasArg("gamma")) gGamma = server.arg("gamma").toInt() != 0;
  if (server.hasArg("shape")) gShape = server.arg("shape").toInt();
  if (server.hasArg("trail")) gTrail = server.arg("trail").toInt();
  if (server.hasArg("ring")) gOrbitRing = server.arg("ring").toInt();
#if STUDIO_L5CX
  if (server.hasArg("thresh"))
    gPresenceThreshMm = (uint16_t)constrain(server.arg("thresh").toInt(), 40, 600);
  if (server.hasArg("zero_scene")) l5cxZeroScene();
#endif
  if (server.hasArg("freeze")) gFrozen = server.arg("freeze").toInt() != 0;
  if (server.hasArg("spread")) gSpread = server.arg("spread").toInt() / 10.0f;
  if (server.hasArg("rotate")) gFringeAngle = server.arg("rotate").toInt() * 0.0174533f;
  if (server.hasArg("b2r")) gB2r = server.arg("b2r").toInt();
  if (server.hasArg("b2g")) gB2g = server.arg("b2g").toInt();
  if (server.hasArg("b2b")) gB2b = server.arg("b2b").toInt();
  if (server.hasArg("split")) gSplit = constrain(server.arg("split").toInt(), 0, 2);
#if STUDIO_SENSOR_TRIAD
  if (server.hasArg("zero_tilt")) captureTiltZero();
  if (server.hasArg("zero_alt")) capturePressureZero();
#endif
  if (server.hasArg("step")) {
    if (gMode == MODE_HEX && (gAnim == 1 || gAnim == 2)) hexAnimPos++;
    else if (gMode == MODE_HEX && gAnim == 0 && gSplit)
      gAnchor = spiralOrder[(++anchorStep) % NUMPIXELS]; // walk the static triad
  }
  if (server.hasArg("mode")) {
    int m = server.arg("mode").toInt();
    gMode = (m == 1) ? MODE_RGBW : (m == 2) ? MODE_RGB : MODE_HEX;
    applyMode();
  }
  if (server.hasArg("off")) {
    gAnim = 0;
    strip.clear();
    strip.show();
  }
  if (server.hasArg("anim")) {
    int requested = server.arg("anim").toInt();
#if STUDIO_SENSOR_TRIAD
    gAnim = constrain(requested, 0, 7);
#elif STUDIO_L5CX
    gAnim = constrain(requested, 0, gMode == MODE_HEX ? 5 : 4);
#else
    gAnim = constrain(requested, 0, 4);
#endif
    hexAnimPos = 0;
    hexBreathePhase = 0;
    rgbwPhase = 0;
    strip.clear();
  }
  applyAfterSet();
  server.send(200, "text/plain", "ok");
}

// Battery stats CACHE: live SDK reads in handleState stalled the animation loop
// (charger reads trigger ADC one-shots = tens of ms, x5 fields, every 600 ms poll).
// Instead loop() refreshes ONE field per ~800 ms round-robin (a single short I2C
// transaction per frame at worst) and /state serves the cache instantly.
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

void solarGuardTick() {
  if (!gPfReady) return;
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
  pfSolarGuardTick("led_studio", sv, sma, good, STUDIO_MAINTAIN_V, true);
}

void handleState() {
  float bv = gBatV, ma = gBatMa, sv = gSupV, sma = gSupMa;
  uint8_t soc = gSoc;
  bool sgood = gSupGood;
  int32_t rssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  char buf[1400];
#if STUDIO_SENSOR_TRIAD
  uint32_t tmfAgeMs =
      gTmfLastReadMs ? millis() - gTmfLastReadMs : UINT32_MAX;
  snprintf(buf, sizeof(buf),
           "{\"fw\":\"%s\",\"triad\":1,\"mode\":%u,\"anim\":%u,\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"bri\":%u,"
           "\"speed\":%u,\"gamma\":%u,\"shape\":%u,\"lit\":%u,\"anchor\":%u,\"split\":%u,"
           "\"rssi\":%ld,\"pf\":%d,\"bv\":%.3f,\"ma\":%.0f,\"soc\":%u,\"sv\":%.3f,\"sma\":%.0f,\"sgood\":%d,"
           "\"msa_present\":%d,\"msa_ok\":%d,\"ax\":%.4f,\"ay\":%.4f,\"az\":%.4f,\"tilt_deg\":%.2f,"
           "\"bmp_present\":%d,\"bmp_ok\":%d,\"pressure_hpa\":%.3f,\"alt_m\":%.3f,"
           "\"tmf_present\":%d,\"tmf_ok\":%d,\"tof_mm\":%u,\"tof_raw_mm\":%u,\"tof_conf\":%u,"
           "\"tmf_active\":%d,\"tmf_age_ms\":%lu,\"tmf_reads\":%lu,"
           "\"tmf_errors\":%lu,\"tmf_recoveries\":%lu}",
           STUDIO_VERSION, gMode, gAnim, gR, gG, gB, gW, gBri,
           gSpeed, gGamma ? 1 : 0, gShape,
           lastLit, gAnchor, gSplit, (long)rssi, gPfReady ? 1 : 0,
           bv, ma, soc, sv, sma,
           sgood ? 1 : 0, gMsaPresent ? 1 : 0, gMsaReadOk ? 1 : 0,
           gMsaReadOk ? gAx : 0.0f, gMsaReadOk ? gAy : 0.0f,
           gMsaReadOk ? gAz : 0.0f, gTiltDeg, gBmpPresent ? 1 : 0,
           gBmpReadOk ? 1 : 0, gBmpReadOk ? gPressureFilteredHpa : 0.0f,
           gRelativeAltitudeM, gTmfPresent ? 1 : 0, gTmfReadOk ? 1 : 0,
           gTofDepthMm, gTofRawClosestMm, gTofConfidence,
           gTmfActive ? 1 : 0, (unsigned long)tmfAgeMs,
           (unsigned long)gTmfReads, (unsigned long)gTmfErrors,
           (unsigned long)gTmfRecoveries);
#elif STUDIO_L5CX
  char zmm[128], zbase[128];
  {
    size_t p = 0, q = 0;
    for (uint8_t z = 0; z < L5CX_ZONES; z++) {
      p += snprintf(zmm + p, sizeof(zmm) - p, "%s%d", z ? "," : "", (int)gVlZoneMm[z]);
      q += snprintf(zbase + q, sizeof(zbase) - q, "%s%d", z ? "," : "",
                    (int)(gVlBaseMm[z] > 0 ? gVlBaseMm[z] + 0.5f : -1));
    }
  }
  snprintf(buf, sizeof(buf),
           "{\"fw\":\"%s\",\"triad\":0,\"l5cx\":1,\"mode\":%u,\"anim\":%u,\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"bri\":%u,"
           "\"speed\":%u,\"gamma\":%u,\"shape\":%u,\"lit\":%u,\"anchor\":%u,\"split\":%u,"
           "\"rssi\":%ld,\"pf\":%d,\"bv\":%.3f,\"ma\":%.0f,\"soc\":%u,\"sv\":%.3f,\"sma\":%.0f,\"sgood\":%d,"
           "\"l5cx_ok\":%d,\"closest_mm\":%d,\"visitor_mm\":%d,\"ct\":%d,"
           "\"presence\":%d,\"why\":%u,\"thresh_mm\":%u,"
           "\"zmm\":[%s],\"zbase\":[%s],"
           "\"l5cx_reads\":%lu,\"l5cx_errors\":%lu}",
           STUDIO_VERSION, gMode, gAnim, gR, gG, gB, gW, gBri,
           gSpeed, gGamma ? 1 : 0, gShape, lastLit, gAnchor, gSplit,
           (long)rssi, gPfReady ? 1 : 0, bv, ma, soc, sv, sma,
           sgood ? 1 : 0, (gVlPresent && gVlRanging) ? 1 : 0, (int)gVlClosestMm,
           (int)gVlVisitorMm, (int)(gColorT * 100.0f + 0.5f),
           gPresence ? 1 : 0, gPresenceWhy, gPresenceThreshMm, zmm, zbase,
           (unsigned long)gVlReads, (unsigned long)gVlErrors);
#else
  snprintf(buf, sizeof(buf),
           "{\"fw\":\"%s\",\"triad\":0,\"mode\":%u,\"anim\":%u,\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"bri\":%u,"
           "\"speed\":%u,\"gamma\":%u,\"shape\":%u,\"lit\":%u,\"anchor\":%u,\"split\":%u,"
           "\"rssi\":%ld,\"pf\":%d,\"bv\":%.3f,\"ma\":%.0f,\"soc\":%u,\"sv\":%.3f,\"sma\":%.0f,\"sgood\":%d}",
           STUDIO_VERSION, gMode, gAnim, gR, gG, gB, gW, gBri,
           gSpeed, gGamma ? 1 : 0, gShape, lastLit, gAnchor, gSplit,
           (long)rssi, gPfReady ? 1 : 0, bv, ma, soc, sv, sma,
           sgood ? 1 : 0);
#endif
  server.send(200, "application/json", buf);
}

void setupWifi() {
#if HAVE_SECRETS
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(gHostname);
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
    Serial.print("LED Studio STA at http://");
    Serial.println(WiFi.localIP());
    if (apOk) {
      Serial.print("LED Studio AP '" AP_SSID "' -> http://");
      Serial.println(WiFi.softAPIP());
    } else {
      Serial.println("LED Studio AP start failed");
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
  Serial.begin(115200);
  delay(200);
  // LFP-safe charger config (see note at the SDK include). Retry a few times like
  // the bench firmwares; fall back to the manual rail-enable if init won't come up.
  Result pf = Result::Failure;
  for (int i = 0; i < 4 && pf != Result::Ok; i++) {
    pf = Board.init((uint16_t)STUDIO_BATTERY_MAH, STUDIO_BATTERY_TYPE);
    if (pf != Result::Ok) delay(250);
  }
  if (pf == Result::Ok) {
    gPfReady = true;
    Board.setSupplyMaintainVoltage(STUDIO_MAINTAIN_V);
    Board.setBatteryChargingMaxCurrent(STUDIO_CHARGE_MA); // gentle USB-friendly charge
    Board.enableBatteryCharging(true);
    pfSolarGuardInit("led_studio", STUDIO_MAINTAIN_V, true);
    Board.enable3V3(true); // LED rail (SDK path)
    Serial.printf("PowerFeather SDK Ok: %u mAh LFP, charge %.0f mA, maintain %.1f V, 3V3 on\n",
                  (unsigned)STUDIO_BATTERY_MAH, (double)STUDIO_CHARGE_MA,
                  (double)STUDIO_MAINTAIN_V);
  } else {
    Serial.println("WARNING: Board.init failed -- charger UNCONFIGURED (do NOT attach a cell "
                   "while on USB); enabling 3V3 rail manually");
  }
  pinMode(EN_3V3_PIN, OUTPUT);
  digitalWrite(EN_3V3_PIN, HIGH); // enable the switchable 3V3 header rail (fallback/no-op)
  delay(20);
  buildGeometry();
  strip.begin();
  strip.setBrightness(255);
  strip.clear();
  strip.show();
  applyMode();
#if STUDIO_L5CX
  gAnim = 5; // boot straight into the presence interaction (applyMode zeroed it)
#endif
  sensorTriadInit();
  sensorTriadTick();
  l5cxInit();
  {
    // "ledstudio-<last 3 MAC bytes>", matching the fleet fixture_id suffix.
    uint64_t mac = ESP.getEfuseMac();
    snprintf(gHostname, sizeof(gHostname), "ledstudio-%02x%02x%02x",
             (uint8_t)(mac >> 24), (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
  }
  setupWifi();
  if (MDNS.begin(gHostname)) { // per-device name -- works on STA and the AP
    MDNS.addService("http", "tcp", 80);
    Serial.printf("mDNS: http://%s.local/\n", gHostname);
  } else {
    Serial.println("mDNS start failed (use the IP)");
  }
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/set", handleSet);
  server.on("/state", handleState);
  // Standard OTA (same handler as power_bench) so studio tweaks never need a tether:
  //   curl -F "firmware=@led_studio.ino.bin" http://<ip>/update   (or the GET form)
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
  Serial.printf("%s ready (GPIO%d, sensor-triad=%d, l5cx=%d). HEX ring sizes %u/%u/%u/%u\n",
                STUDIO_VERSION, DATA_PIN, STUDIO_SENSOR_TRIAD, STUDIO_L5CX,
                ringSize[0], ringSize[1], ringSize[2], ringSize[3]);
  renderStatic();
}

void loop() {
  server.handleClient();
  sensorTriadTick();
  l5cxTick();
  batteryTick();
  solarGuardTick();
  if (isAnimating() && millis() - lastFrame >= (uint32_t)(400 - (gSpeed - 1) * (375.0f / 99.0f))) {
    lastFrame = millis();
    renderFrame();
  }
}
