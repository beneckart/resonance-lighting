// Resonance Sway Demo -- MSA311 accelerometer (STEMMA-QT / Wire1) drives the
// single 4 W SK6812 RGBW point source on GPIO10. Bench tool for the
// motion-reactive lighting idea: the color follows the SWAY (high-passed accel
// delta -- the "wind" signal), the TILT direction, or both, selectable live
// from the built-in web UI, which also draws a bubble-level + sway-pulse
// graphic of the sensor state for verification.
//
// Signal path (50 Hz):
//   accel -> low-pass (~0.4 s)   = gravity vector -> pitch/roll/tilt vs a
//                                  calibrated rest pose (Re-zero on the web UI)
//   accel - gravity (high-pass)  = sway magnitude -> fast-attack / slow-decay
//                                  envelope -> hue + brightness (+W flash)
//
// Wiring: MSA311 on the STEMMA-QT connector = Wire1 (GPIO47/48), which is the
// SHARED charger/gauge bus -- it stays at 100 kHz, never faster
// (POWERFEATHER_NOTES "Wire1 at >100 kHz can OPEN YOUR BATTERY SWITCH").
// RGBW data on GPIO10 / A0, V+ from the switchable 3V3 header rail (GPIO4).
//
// Since .3: a VL53L5CX multizone ToF (same Wire1 bus, 0x29) fits a plane to the
// ground and reports GEOMETRIC tilt -- immune to the pendulum degeneracy that
// blinds accel-only tilt while hanging. The web UI shows both tilt estimates
// side by side (filled dot = accel, cyan ring = ToF) plus the zone heatmap and
// height above ground. Note the two sensors' axes are aligned only as well as
// the breakouts are physically squared to each other on the rig.
//
// Build/flash (USB): ./build.sh --port /dev/ttyACM1
// Web: http://swaydemo.local/ (mDNS) or the IP in the serial banner (115200).
// OTA: curl -F "firmware=@sway_demo.ino.bin" http://<ip>/update

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MSA301.h> // library also provides Adafruit_MSA311 (part id 0x13)
#include "src/vl53l5cx/SparkFun_VL53L5CX_Library.h" // vendored (see src/vl53l5cx/VENDORED.md)

#define FW_VERSION "sway-demo-2026-07-07.6"

#ifndef DATA_PIN
#define DATA_PIN 10 // GPIO10 / A0, direct-GPIO LED data (ADR 0018/0022)
#endif
Adafruit_NeoPixel strip(1, DATA_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_MSA311 msa;

// PowerFeather SDK: rails + telemetry, plus GUARDED charging (since .3 -- Ben's
// demo unit carries a cell now). Charging is off at boot and enabled one-shot by
// chargeTick() only once the gauge reports a plausible cell voltage: enabling
// charge into a missing battery brownout-loops (POWERFEATHER_NOTES), and the LFP
// 3.65 V ceiling is a safe undercharge even for a mislabeled Li-ion cell.
#include <PowerFeather.h>
#include "../powerfeather_solar_guard.h"
using namespace PowerFeather;
#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 (build.sh passes it) so the SDK targets the V2."
#endif
#define SWAY_MAINTAIN_V 4.6f // correct for USB; re-tune toward the panel MPP for solar work
bool gPfReady = false;
bool gChargeOn = false;

#define EN_3V3_PIN 4 // switchable 3V3 header rail (LED V+), active HIGH; fallback if SDK init fails

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif
#define AP_SSID "ResonanceSway"
#define AP_PASS "resonance"

WebServer server(80);

// ---- Motion state -----------------------------------------------------------
#define SAMPLE_MS 20        // 50 Hz processing (sensor ODR 125 Hz)
#define ALPHA_G 0.06f       // gravity low-pass per sample (~0.4 s time constant)
#define ENV_ATTACK 0.5f     // envelope rise fraction per sample
#define ENV_DECAY 0.96f     // envelope decay per sample (~0.5 s tail)
#define TILT_FULL_DEG 45.0f // tilt that maps to full brightness in tilt mode
#define AZ_DEADBAND_DEG 3.0f // below this tilt the azimuth is noise -> hold hue

bool gMsaOk = false;
float gAx = 0, gAy = 0, gAz = 0;    // last raw sample (g)
float gGx = 0, gGy = 0, gGz = 1;    // gravity low-pass (g)
bool gGravSeeded = false;
float gSwayInst = 0, gEnv = 0;      // high-pass magnitude + envelope (g)
float gPitchRaw = 0, gRollRaw = 0;  // from gravity, absolute (deg)
float gPitch0 = 0, gRoll0 = 0;      // calibrated rest pose (deg)
float gRestX = 0, gRestY = 0, gRestZ = 1; // calibrated rest gravity unit vector
float gPitch = 0, gRoll = 0;        // deltas vs rest (deg) -- the bubble position
float gTiltDeg = 0, gAzDeg = 0;     // tilt magnitude + direction vs rest
bool gCalPending = true;            // auto re-zero shortly after boot
uint32_t gCalDueMs = 0;

// ---- Color mapping ----------------------------------------------------------
enum Mode { MODE_SWAY = 0, MODE_TILT = 1, MODE_BOTH = 2 };
uint8_t gMode = MODE_SWAY;
uint8_t gSens = 50;       // 1..100 -> full-scale sway 1.5 g .. 0.03 g (exponential)
float gFullScaleG = 0.22f;
uint8_t gBase = 60;       // brightness at rest (30 fell into the gamma dead-zone: rgbw=1,0,0)
bool gGamma = true;
bool gLedOn = true;
// Calm amber (hue16 ~4000) sweeping to violet (~48000) as sway rises.
#define HUE_CALM 4000
#define HUE_SPAN 44000
uint16_t gHue = HUE_CALM; // last rendered hue (held through the azimuth deadband)
uint8_t gOutR = 0, gOutG = 0, gOutB = 0, gOutW = 0; // what the LED is showing

void applySens() { gFullScaleG = 1.5f * powf(0.02f, (gSens - 1) / 99.0f); }

// ---- Sensor -----------------------------------------------------------------
bool msaInit() {
  if (!msa.begin(MSA311_I2CADDR_DEFAULT, &Wire1)) return false;
  // begin() defaults to 500 Hz ODR / 250 Hz BW; calmer settings for a 50 Hz loop.
  msa.setRange(MSA301_RANGE_4_G); // sway spikes clear 1 g easily
  msa.setDataRate(MSA301_DATARATE_125_HZ);
  msa.setBandwidth(MSA301_BANDWIDTH_62_5_HZ);
  msa.setPowerMode(MSA301_NORMALMODE);
  return true;
}

void msaRetryTick() { // hot-plug friendly: keep probing if the sensor is missing
  static uint32_t nextMs = 0;
  if (gMsaOk || millis() < nextMs) return;
  nextMs = millis() + 5000;
  gMsaOk = msaInit();
  if (gMsaOk) {
    Serial.println("MSA311 found (late)");
    gGravSeeded = false;
    gCalPending = true;
    gCalDueMs = millis() + 1500;
  }
}

void captureRest() {
  float gm = sqrtf(gGx * gGx + gGy * gGy + gGz * gGz);
  if (gm < 0.05f) return;
  gRestX = gGx / gm;
  gRestY = gGy / gm;
  gRestZ = gGz / gm;
  gPitch0 = gPitchRaw;
  gRoll0 = gRollRaw;
  gCalPending = false;
  Serial.printf("rest pose captured: pitch0=%.1f roll0=%.1f g=[%.2f %.2f %.2f]\n",
                gPitch0, gRoll0, gRestX, gRestY, gRestZ);
}

// ---- VL53L5CX ground-plane tilt ----------------------------------------------
// Least-squares plane over the multizone ranges = tilt relative to the actual
// ground (geometric, not inertial). 4x4 @ 10 Hz keeps the per-frame read short
// on the shared 100 kHz bus, since this sketch reads from loop() (presence_bench
// runs 8x8 from a dedicated task). One robust pass drops outlier zones (a person
// / cable in the FoV); per zone we keep the FARTHEST valid target, since ground
// sits behind whatever partially occludes it.
#define TOF_RES 4 // 4 or 8 (8x8 max 15 Hz; mind the loop-blocking read time)
#define TOF_HZ 10
#define TOF_FOV_DEG 45.0f
#define TOF_ZONES (TOF_RES * TOF_RES)
#define TOF_MIN_FIT 8 // fewer clean zones than this -> no fit reported
SparkFun_VL53L5CX tof;
VL53L5CX_ResultsData tofData;
bool gTofOk = false, gTofRanging = false;
uint32_t gTofSeq = 0, gTofLastFrameMs = 0, gTofRetryAtMs = 0;
int16_t gTofD[TOF_ZONES];  // farthest-valid-target range per zone, mm (-1 = none)
uint32_t gTofUsedMask = 0; // zones the (robust) plane fit actually used
float gTofTiltX = 0, gTofTiltY = 0; // tilt components (deg) RELATIVE to the zeroed mount
float gTofTilt = 0, gTofAz = 0;     // magnitude + direction (deg) of that delta
// Zero reference = unit ground-normal at cal time + a basis spanning its
// perpendicular plane, all fixed in the sensor frame. Relative tilt is then the
// exact 3D angle acos(n . n0) -- spin-invariant for ANY mount tilt (a fixed
// component-wise offset is NOT: yaw rotates the swing term under it, and the
// tangent-plane projection distorts at a 15 deg mount). Azimuth remains in the
// spinning body frame -- no yaw reference on board.
float gTofN0[3] = {0, 0, 1}, gTofE1[3] = {1, 0, 0}, gTofE2[3] = {0, 1, 0};
bool gTofCalPending = true;         // auto-zero on the first good fit; Re-zero re-arms
float gTofHmm = 0;                  // range to ground along boresight (mm)
uint8_t gTofValid = 0, gTofUsed = 0;
float tofRayX[TOF_ZONES], tofRayY[TOF_ZONES], tofRayZ[TOF_ZONES];

void tofBuildRays() { // zone-center rays on a tangent-plane grid across the FoV
  float half = tanf(TOF_FOV_DEG * 0.5f * 0.0174533f);
  for (int r = 0; r < TOF_RES; r++)
    for (int c = 0; c < TOF_RES; c++) {
      float u = ((c + 0.5f) / TOF_RES * 2.0f - 1.0f) * half;
      float v = ((r + 0.5f) / TOF_RES * 2.0f - 1.0f) * half;
      float n = sqrtf(u * u + v * v + 1.0f);
      int i = r * TOF_RES + c;
      tofRayX[i] = u / n;
      tofRayY[i] = v / n;
      tofRayZ[i] = 1.0f / n;
    }
}

// Fit z = a*x + b*y + c over the kept points (normal equations, Cramer).
bool planeFitLS(const float *px, const float *py, const float *pz, const bool *keep,
                uint8_t n, float *a, float *b, float *c) {
  double sx = 0, sy = 0, sz = 0, sxx = 0, syy = 0, sxy = 0, sxz = 0, syz = 0;
  uint16_t m = 0;
  for (uint8_t k = 0; k < n; k++) {
    if (!keep[k]) continue;
    double x = px[k], y = py[k], z = pz[k];
    sx += x; sy += y; sz += z;
    sxx += x * x; syy += y * y; sxy += x * y;
    sxz += x * z; syz += y * z;
    m++;
  }
  if (m < TOF_MIN_FIT) return false;
  double det = sxx * (syy * m - sy * sy) - sxy * (sxy * m - sy * sx) + sx * (sxy * sy - syy * sx);
  if (fabs(det) < 1e-3) return false;
  double da = sxz * (syy * m - sy * sy) - sxy * (syz * m - sy * sz) + sx * (syz * sy - syy * sz);
  double db = sxx * (syz * m - sz * sy) - sxz * (sxy * m - sy * sx) + sx * (sxy * sz - syz * sx);
  double dc = sxx * (syy * sz - syz * sy) - sxy * (sxy * sz - syz * sx) + sxz * (sxy * sy - syy * sx);
  *a = (float)(da / det);
  *b = (float)(db / det);
  *c = (float)(dc / det);
  return true;
}

bool tofApply() {
  // Only stop if actually ranging: stop_ranging on a fresh device hangs on an
  // MCU-stop bit that never asserts (see src/vl53l5cx/VENDORED.md).
  if (gTofRanging) {
    tof.stopRanging();
    gTofRanging = false;
  }
  if (!tof.setResolution(TOF_ZONES)) { Serial.println("[tof] setResolution FAILED"); return false; }
  if (!tof.setRangingFrequency(TOF_HZ)) { Serial.println("[tof] setRangingFrequency FAILED"); return false; }
  gTofRanging = tof.startRanging();
  if (!gTofRanging) Serial.println("[tof] startRanging FAILED");
  gTofLastFrameMs = millis();
  return gTofRanging;
}

void tofInit() {
  uint32_t t0 = millis();
  Serial.println("[tof] VL53L5CX begin (fw blob upload over 100 kHz I2C, several s)...");
  if (!tof.begin(0x29, Wire1)) {
    Serial.println("[tof] begin FAILED (absent/unpowered? retry in 30 s)");
    gTofOk = false;
    gTofRetryAtMs = millis() + 30000;
    return;
  }
  tof.setWireMaxPacketSize(124); // ESP32 Wire buffer is 128
  gTofOk = tofApply();
  Serial.printf("[tof] up in %lu ms: %dx%d @ %d Hz -> ground-plane tilt\n",
                (unsigned long)(millis() - t0), TOF_RES, TOF_RES, TOF_HZ);
  if (!gTofOk) gTofRetryAtMs = millis() + 30000;
}

void tofTick() {
  uint32_t now = millis();
  if (!gTofOk) {
    if (gTofRetryAtMs && now >= gTofRetryAtMs) {
      gTofRetryAtMs = 0;
      tofInit();
    }
    return;
  }
  static uint32_t nextPollMs = 0;
  if (now < nextPollMs) return;
  nextPollMs = now + 40;
  if (gTofRanging && now - gTofLastFrameMs > 5000) { // presence_bench-style self-heal
    Serial.println("[tof] ranging stalled -> re-apply");
    gTofOk = tofApply();
    if (!gTofOk) gTofRetryAtMs = now + 30000;
    return;
  }
  if (!tof.isDataReady()) return;
  if (!tof.getRangingData(&tofData)) return;
  gTofLastFrameMs = now;
  gTofSeq++;

  float px[TOF_ZONES], py[TOF_ZONES], pz[TOF_ZONES];
  uint8_t zoneOf[TOF_ZONES], nP = 0;
  gTofValid = 0;
  for (uint8_t z = 0; z < TOF_ZONES; z++) {
    int16_t best = -1;
    uint8_t nt = tofData.nb_target_detected[z];
    if (nt > VL53L5CX_NB_TARGET_PER_ZONE) nt = VL53L5CX_NB_TARGET_PER_ZONE;
    for (uint8_t t = 0; t < nt; t++) {
      uint16_t i = z * VL53L5CX_NB_TARGET_PER_ZONE + t;
      uint8_t st = tofData.target_status[i];
      int16_t d = tofData.distance_mm[i];
      if ((st == 5 || st == 9) && d > 30 && d > best) best = d; // farthest valid = ground
    }
    gTofD[z] = best;
    if (best > 0) {
      px[nP] = best * tofRayX[z];
      py[nP] = best * tofRayY[z];
      pz[nP] = best * tofRayZ[z];
      zoneOf[nP] = z;
      nP++;
      gTofValid++;
    }
  }

  gTofUsedMask = 0;
  gTofUsed = 0;
  bool keep[TOF_ZONES];
  for (uint8_t k = 0; k < nP; k++) keep[k] = true;
  float a, b, c;
  bool ok = planeFitLS(px, py, pz, keep, nP, &a, &b, &c);
  if (ok) { // one robust pass: drop outliers (person/cable), refit on the rest
    uint8_t nKeep = 0;
    for (uint8_t k = 0; k < nP; k++) {
      keep[k] = fabsf(pz[k] - (a * px[k] + b * py[k] + c)) < 120.0f;
      if (keep[k]) nKeep++;
    }
    if (nKeep >= TOF_MIN_FIT) {
      if (nKeep < nP) ok = planeFitLS(px, py, pz, keep, nP, &a, &b, &c);
    } else { // too crowded to trim -- keep the all-points fit rather than nothing
      for (uint8_t k = 0; k < nP; k++) keep[k] = true;
    }
  }
  if (ok) {
    for (uint8_t k = 0; k < nP; k++)
      if (keep[k]) {
        gTofUsedMask |= (1UL << zoneOf[k]);
        gTofUsed++;
      }
    float nn = sqrtf(a * a + b * b + 1.0f); // unit ground normal, +z toward sensor
    float nx = -a / nn, ny = -b / nn, nz = 1.0f / nn;
    if (gTofCalPending) { // zero against the (possibly jury-rigged, tilted) mount
      gTofN0[0] = nx;
      gTofN0[1] = ny;
      gTofN0[2] = nz;
      // e1 = x_hat projected off n0 (n0 stays within ~45 deg of boresight, so
      // this never degenerates); e2 = n0 x e1
      float d = gTofN0[0];
      gTofE1[0] = 1.0f - d * gTofN0[0];
      gTofE1[1] = -d * gTofN0[1];
      gTofE1[2] = -d * gTofN0[2];
      float e1n = sqrtf(gTofE1[0] * gTofE1[0] + gTofE1[1] * gTofE1[1] + gTofE1[2] * gTofE1[2]);
      for (int i = 0; i < 3; i++) gTofE1[i] /= e1n;
      gTofE2[0] = gTofN0[1] * gTofE1[2] - gTofN0[2] * gTofE1[1];
      gTofE2[1] = gTofN0[2] * gTofE1[0] - gTofN0[0] * gTofE1[2];
      gTofE2[2] = gTofN0[0] * gTofE1[1] - gTofN0[1] * gTofE1[0];
      gTofCalPending = false;
      Serial.printf("[tof] mount zeroed: %.1f deg off boresight\n",
                    acosf(nz > 1 ? 1 : nz) * 57.29578f);
    }
    float dot = nx * gTofN0[0] + ny * gTofN0[1] + nz * gTofN0[2];
    if (dot > 1) dot = 1;
    if (dot < -1) dot = -1;
    gTofTilt = acosf(dot) * 57.29578f; // exact relative tilt: spin-invariant
    float pxv = nx - dot * gTofN0[0], pyv = ny - dot * gTofN0[1], pzv = nz - dot * gTofN0[2];
    float pn = sqrtf(pxv * pxv + pyv * pyv + pzv * pzv);
    if (pn > 1e-6f) { // direction of the lean in the (spinning) body frame
      float u1 = (pxv * gTofE1[0] + pyv * gTofE1[1] + pzv * gTofE1[2]) / pn;
      float u2 = (pxv * gTofE2[0] + pyv * gTofE2[1] + pzv * gTofE2[2]) / pn;
      gTofTiltX = gTofTilt * u1;
      gTofTiltY = gTofTilt * u2;
    } else {
      gTofTiltX = gTofTiltY = 0;
    }
    gTofAz = atan2f(gTofTiltY, gTofTiltX) * 57.29578f;
    gTofHmm = c; // range along boresight (x=y=0)
  }
}

// ---- LED rendering ----------------------------------------------------------
void showRGBW(uint32_t rgb, uint8_t w) {
  if (gGamma) {
    rgb = Adafruit_NeoPixel::gamma32(rgb);
    w = Adafruit_NeoPixel::gamma8(w);
  }
  gOutR = (rgb >> 16) & 0xFF;
  gOutG = (rgb >> 8) & 0xFF;
  gOutB = rgb & 0xFF;
  gOutW = w;
  strip.setPixelColor(0, gOutR, gOutG, gOutB, gOutW);
  strip.show();
}

void renderMissing() { // sensor absent: slow red breathe so the bench state is obvious
  float f = 0.5f + 0.5f * sinf(millis() * 0.003f);
  showRGBW(Adafruit_NeoPixel::Color((uint8_t)(30 + 60 * f), 0, 0), 0);
}

void renderLed() {
  if (!gLedOn) {
    if (gOutR | gOutG | gOutB | gOutW) showRGBW(0, 0);
    return;
  }
  float envN = gEnv / gFullScaleG;
  if (envN > 1) envN = 1;
  float tiltN = gTiltDeg / TILT_FULL_DEG;
  if (tiltN > 1) tiltN = 1;
  uint16_t azHue = (uint16_t)((gAzDeg + 180.0f) / 360.0f * 65535.0f);

  float lift = 0; // 0..1 brightness above the resting base
  switch (gMode) {
    case MODE_SWAY: // hue sweeps amber -> violet with sway energy
      gHue = HUE_CALM + (uint16_t)(envN * (float)HUE_SPAN);
      lift = envN;
      break;
    case MODE_TILT: // hue = which way it leans, brightness = how far
      if (gTiltDeg >= AZ_DEADBAND_DEG) gHue = azHue;
      lift = tiltN;
      break;
    default: // BOTH: tilt steers the hue, sway pumps the brightness
      if (gTiltDeg >= AZ_DEADBAND_DEG) gHue = azHue;
      lift = envN > tiltN * 0.5f ? envN : tiltN * 0.5f;
      break;
  }
  uint8_t val = gBase + (uint8_t)(lift * (float)(255 - gBase));
  // White die: a strong sway spike (top 20% of scale) flashes the W channel.
  uint8_t w = envN > 0.8f ? (uint8_t)((envN - 0.8f) * 5.0f * 255.0f) : 0;
  showRGBW(Adafruit_NeoPixel::ColorHSV(gHue, 255, val), w);
}

void sensorTick() {
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  if (now - lastMs < SAMPLE_MS) return;
  lastMs = now;
  if (!gMsaOk) {
    renderMissing();
    return;
  }
  msa.read();
  gAx = msa.x_g;
  gAy = msa.y_g;
  gAz = msa.z_g;
  if (!gGravSeeded) {
    gGx = gAx;
    gGy = gAy;
    gGz = gAz;
    gGravSeeded = true;
  }
  gGx += ALPHA_G * (gAx - gGx);
  gGy += ALPHA_G * (gAy - gGy);
  gGz += ALPHA_G * (gAz - gGz);

  // Sway = what's left after gravity is removed (the delta signal).
  float hx = gAx - gGx, hy = gAy - gGy, hz = gAz - gGz;
  gSwayInst = sqrtf(hx * hx + hy * hy + hz * hz);
  if (gSwayInst > gEnv) gEnv += ENV_ATTACK * (gSwayInst - gEnv);
  else gEnv *= ENV_DECAY;

  // Tilt from the smoothed gravity vector.
  float gm = sqrtf(gGx * gGx + gGy * gGy + gGz * gGz);
  if (gm > 0.05f) {
    gPitchRaw = atan2f(-gGx, sqrtf(gGy * gGy + gGz * gGz)) * 57.29578f;
    gRollRaw = atan2f(gGy, gGz) * 57.29578f;
    gPitch = gPitchRaw - gPitch0;
    gRoll = gRollRaw - gRoll0;
    float dot = (gGx * gRestX + gGy * gRestY + gGz * gRestZ) / gm;
    if (dot > 1) dot = 1;
    if (dot < -1) dot = -1;
    gTiltDeg = acosf(dot) * 57.29578f;
    gAzDeg = atan2f(gRoll, gPitch) * 57.29578f;
  }
  if (gCalPending && now >= gCalDueMs && gGravSeeded) captureRest();
  renderLed();
}

// ---- Battery stats cache (led_studio pattern: one short SDK read per 800 ms) --
float gBatV = 0, gBatMa = 0, gSupV = 0, gSupMa = 0;
uint8_t gSoc = 0;
bool gSupGood = false;

void batteryTick() {
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
}

// One-shot guarded charge-enable (presence_bench pattern): the gauge reads 0.00 V
// right after Board.init, so the decision waits for a real voltage from the
// round-robin. Plausible cell -> gentle 500 mA charge under the LFP profile.
void chargeTick() {
  static bool done = false;
  if (done || !gPfReady || millis() < 6000) return;
  if (gBatV < 0.1f) {
    if (millis() > 60000) {
      done = true;
      Serial.println("no battery reading after 60 s -> charging stays OFF");
    }
    return;
  }
  done = true;
  if (gBatV > 2.5f && gBatV < 4.4f) {
    Board.setBatteryChargingMaxCurrent(500);
    Board.enableBatteryCharging(true);
    gChargeOn = true;
    pfSolarGuardInit("sway_demo", SWAY_MAINTAIN_V, true);
    Serial.printf("battery %.2fV present -> charging ON (500 mA, LFP 3.65 V ceiling)\n", gBatV);
  } else {
    Serial.printf("battery %.2fV implausible -> charging stays OFF\n", gBatV);
  }
}

void solarGuardTick() { // project baseline for any charging-enabled sketch
  if (!gPfReady || !gChargeOn) return;
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
  pfSolarGuardTick("sway_demo", sv, sma, good, SWAY_MAINTAIN_V, true);
}

// ---- Web UI -----------------------------------------------------------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>Sway Demo</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:14px;max-width:520px}
 h2{margin:.2em 0}
 .row{margin:10px 0}
 label{display:block;font-size:13px;color:#aaa;margin-bottom:3px}
 input[type=range]{width:100%;height:30px}
 .btns{display:flex;flex-wrap:wrap;gap:6px}
 button{flex:1 1 auto;min-width:64px;padding:11px 8px;font-size:14px;border:0;border-radius:8px;background:#333;color:#eee}
 button.on{background:#0a7;color:#fff}
 canvas{background:#000;border-radius:10px;display:block;margin:0 auto}
 #vals{font-family:monospace;font-size:13px;background:#000;padding:8px;border-radius:6px;white-space:pre;color:#6f6;overflow-x:auto}
 #warn{color:#f66;font-weight:bold}
 hr{border:0;border-top:1px solid #333;margin:14px 0}
</style></head><body>
<h2>Sway Demo <span style="font-size:12px;color:#888" id=fw></span></h2>
<div id=warn></div>

<div class=row><canvas id=lvl width=300 height=300></canvas>
 <label style="text-align:center;margin-top:4px">filled dot = accel tilt (LED color) &middot; cyan ring = ToF ground-plane tilt</label></div>
<div class=row><label>ToF ground plane (gray = range, green box = used in fit)</label>
 <canvas id=tofc width=160 height=160></canvas>
 <div id=tofinfo style="font-family:monospace;font-size:12px;color:#0cf;margin-top:4px">...</div></div>
<div class=row><label>Sway envelope (last 30 s, line = full scale)</label>
 <canvas id=spark width=300 height=60></canvas></div>
<div class=row><div id=vals>...</div></div>

<hr>
<div class=row><label>Color source</label><div class=btns>
 <button id=m0 onclick="mode(0)">Sway</button>
 <button id=m1 onclick="mode(1)">Tilt</button>
 <button id=m2 onclick="mode(2)">Both</button>
</div></div>
<div class=row><label>Sway sensitivity <span id=sensl></span></label>
 <input type=range id=sens min=1 max=100 value=50 oninput="ch('sens',this.value)"></div>
<div class=row><label>Base brightness <span id=basel></span></label>
 <input type=range id=base min=0 max=255 value=60 oninput="ch('base',this.value)"></div>
<div class=row><div class=btns>
 <button onclick="send('cal=1')">Re-zero tilt</button>
 <button id=led onclick="toggleLed()">LED: on</button>
 <button id=gam onclick="toggleGamma()">Gamma: on</button>
</div></div>
<div class=row><label>Battery</label><div id=bat>...</div></div>

<script>
let st={mode:0,sens:50,base:60,led:1,gamma:1};
let hist=[],trail=[];
function send(q){fetch('/set?'+q);}
function ch(k,v){st[k]=+v;send(k+'='+v);syncLabels();}
function mode(m){st.mode=m;send('mode='+m);hl(m);}
function hl(m){for(let i=0;i<3;i++)document.getElementById('m'+i).className=(i==m?'on':'');}
function toggleLed(){st.led^=1;send('led='+st.led);let e=document.getElementById('led');
 e.textContent='LED: '+(st.led?'on':'off');e.className=st.led?'on':'';}
function toggleGamma(){st.gamma^=1;send('gamma='+st.gamma);let e=document.getElementById('gam');
 e.textContent='Gamma: '+(st.gamma?'on':'off');e.className=st.gamma?'on':'';}
function syncLabels(){
 sensl.textContent=st.sens+' (full scale '+(1.5*Math.pow(0.02,(st.sens-1)/99)).toFixed(2)+' g)';
 basel.textContent=st.base;}
function drawLevel(s){
 const c=document.getElementById('lvl'),x=c.getContext('2d'),R=130,cx=150,cy=150;
 const col='rgb('+s.r+','+s.g+','+s.b+')';
 x.clearRect(0,0,300,300);
 x.strokeStyle='#333';x.lineWidth=1;
 for(const d of [10,20,30,45]){x.beginPath();x.arc(cx,cy,d/45*R,0,7);x.stroke();}
 x.beginPath();x.moveTo(cx-R,cy);x.lineTo(cx+R,cy);x.moveTo(cx,cy-R);x.lineTo(cx,cy+R);x.stroke();
 x.fillStyle='#666';x.font='11px monospace';
 x.fillText('+pitch',cx+4,cy-R+10);x.fillText('+roll',cx+R-34,cy-4);
 if(s.envn>0.01){ // sway pulse ring
  x.strokeStyle=col;x.globalAlpha=0.9;x.lineWidth=4;
  x.beginPath();x.arc(cx,cy,Math.max(6,s.envn*R),0,7);x.stroke();x.globalAlpha=1;}
 const bx=cx+Math.max(-1,Math.min(1,s.roll/45))*R,
       by=cy-Math.max(-1,Math.min(1,s.pitch/45))*R;
 trail.push([bx,by]);if(trail.length>40)trail.shift();
 for(let i=0;i<trail.length;i++){x.globalAlpha=i/trail.length*0.5;
  x.fillStyle=col;x.beginPath();x.arc(trail[i][0],trail[i][1],3,0,7);x.fill();}
 x.globalAlpha=1;x.fillStyle=col;x.strokeStyle='#fff';x.lineWidth=2;
 x.beginPath();x.arc(bx,by,12,0,7);x.fill();x.stroke();
 if(s.w>0){x.fillStyle='rgba(255,255,255,'+(s.w/255)+')';
  x.beginPath();x.arc(bx,by,6,0,7);x.fill();}
 if(s.tof&&s.tu>=8){ // ToF ground-plane tilt, same 45-deg mapping
  const tx=cx+Math.max(-1,Math.min(1,s.ttx/45))*R,
        ty=cy-Math.max(-1,Math.min(1,s.tty/45))*R;
  x.strokeStyle='#0cf';x.lineWidth=3;
  x.beginPath();x.arc(tx,ty,10,0,7);x.stroke();}
}
function drawTof(s){
 const c=document.getElementById('tofc'),x=c.getContext('2d'),n=s.tres,cs=160/n;
 x.clearRect(0,0,160,160);
 if(!s.tof){x.fillStyle='#888';x.font='13px monospace';x.fillText('no VL53L5CX',35,84);
  tofinfo.textContent='ToF not found (retrying)';return;}
 let mn=1e9,mx=-1e9;
 for(const d of s.td)if(d>0){if(d<mn)mn=d;if(d>mx)mx=d;}
 if(mx<=mn){mn=0;mx=1;}
 const um=parseInt(s.tum,16);
 for(let z=0;z<n*n;z++){const r=(z/n)|0,cc=z%n,d=s.td[z],px=cc*cs,py=r*cs;
  if(d<0)x.fillStyle='#311';
  else{const g=Math.round(45+180*(1-(d-mn)/(mx-mn)));x.fillStyle='rgb('+g+','+g+','+g+')';}
  x.fillRect(px,py,cs-1,cs-1);
  if(um&(1<<z)){x.strokeStyle='#0a7';x.lineWidth=2;x.strokeRect(px+1,py+1,cs-3,cs-3);}}
 tofinfo.textContent='height '+(s.th/1000).toFixed(3)+' m   tilt '+s.ttilt.toFixed(1)+
  ' deg  az '+s.taz.toFixed(0)+' deg (vs mount, zeroed at '+s.tt0.toFixed(1)+
  ' deg)   zones '+s.tu+'/'+s.tv+'   frame #'+s.tseq;
}
function drawSpark(s){
 hist.push(s.env/s.fs);if(hist.length>300)hist.shift();
 const c=document.getElementById('spark'),x=c.getContext('2d'),h=60;
 x.clearRect(0,0,300,60);
 x.strokeStyle='#555';x.beginPath();x.moveTo(0,2);x.lineTo(300,2);x.stroke(); // full scale
 x.strokeStyle='#0a7';x.lineWidth=1.5;x.beginPath();
 for(let i=0;i<hist.length;i++){const y=h-Math.min(1,hist[i])*(h-4);
  i?x.lineTo(i,y):x.moveTo(i,y);}
 x.stroke();
}
function tick(){fetch('/state').then(r=>r.json()).then(s=>{
 document.getElementById('fw').textContent=s.fw;
 document.getElementById('warn').textContent=s.msa?'':'MSA311 NOT FOUND -- check the STEMMA cable (retrying every 5 s)';
 if(s.msa){drawLevel(s);drawSpark(s);}
 drawTof(s);
 vals.textContent=
  'accel  ['+s.ax.toFixed(3)+' '+s.ay.toFixed(3)+' '+s.az.toFixed(3)+'] g\n'+
  'tilt   '+s.tilt.toFixed(1)+' deg   az '+s.azdeg.toFixed(0)+' deg   pitch '+s.pitch.toFixed(1)+'  roll '+s.roll.toFixed(1)+'\n'+
  'sway   '+s.sway.toFixed(3)+' g   env '+s.env.toFixed(3)+' g  ('+(100*s.envn).toFixed(0)+'% of '+s.fs.toFixed(2)+' g)\n'+
  'LED    rgbw='+s.r+','+s.g+','+s.b+','+s.w;
 let bat=document.getElementById('bat');
 if(!s.pf){bat.textContent='no battery data (SDK init failed)';}
 else{let act=s.ma>30?('charging +'+s.ma+'mA'):(s.ma<-30?('discharging '+s.ma+'mA'):'idle ~'+s.ma+'mA');
  bat.textContent='SOC '+s.soc+'%  '+s.bv.toFixed(3)+'V  '+act+
   (s.sgood?('  |  supply '+s.sv.toFixed(2)+'V ok'):'  |  on battery')+
   (s.chg?'':'  |  charger disabled');}
 setTimeout(tick,100);}).catch(()=>setTimeout(tick,600));}
hl(0);syncLabels();tick();
</script></body></html>)HTML";

void handleSet() {
  if (server.hasArg("mode")) gMode = constrain(server.arg("mode").toInt(), 0, 2);
  if (server.hasArg("sens")) {
    gSens = constrain(server.arg("sens").toInt(), 1, 100);
    applySens();
  }
  if (server.hasArg("base")) gBase = constrain(server.arg("base").toInt(), 0, 255);
  if (server.hasArg("led")) gLedOn = server.arg("led").toInt() != 0;
  if (server.hasArg("gamma")) gGamma = server.arg("gamma").toInt() != 0;
  if (server.hasArg("cal")) {
    gCalPending = true;
    gCalDueMs = 0;         // accel: capture on the next sample
    gTofCalPending = true; // ToF: capture on the next good plane fit
  }
  server.send(200, "text/plain", "ok");
}

void handleState() {
  float envN = gEnv / gFullScaleG;
  if (envN > 1) envN = 1;
  static char buf[1400];
  int p = snprintf(buf, sizeof(buf),
           "{\"fw\":\"%s\",\"msa\":%d,"
           "\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,"
           "\"pitch\":%.1f,\"roll\":%.1f,\"tilt\":%.1f,\"azdeg\":%.0f,"
           "\"sway\":%.3f,\"env\":%.3f,\"envn\":%.3f,\"fs\":%.3f,"
           "\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,"
           "\"mode\":%u,\"sens\":%u,\"base\":%u,\"led\":%d,\"gamma\":%d,"
           "\"pf\":%d,\"chg\":%d,\"bv\":%.3f,\"ma\":%.0f,\"soc\":%u,\"sv\":%.2f,\"sgood\":%d,"
           "\"tof\":%d,\"tres\":%d,\"ttx\":%.1f,\"tty\":%.1f,\"ttilt\":%.1f,\"taz\":%.0f,"
           "\"tt0\":%.1f,\"th\":%.0f,\"tv\":%u,\"tu\":%u,\"tseq\":%lu,\"tum\":\"%04lX\",\"td\":[",
           FW_VERSION, gMsaOk ? 1 : 0, gAx, gAy, gAz, gPitch, gRoll, gTiltDeg,
           gAzDeg, gSwayInst, gEnv, envN, gFullScaleG, gOutR, gOutG, gOutB,
           gOutW, gMode, gSens, gBase, gLedOn ? 1 : 0, gGamma ? 1 : 0,
           gPfReady ? 1 : 0, gChargeOn ? 1 : 0, gBatV, gBatMa, gSoc, gSupV,
           gSupGood ? 1 : 0, gTofOk ? 1 : 0, TOF_RES, gTofTiltX, gTofTiltY,
           gTofTilt, gTofAz,
           acosf(gTofN0[2] > 1 ? 1 : gTofN0[2]) * 57.29578f,
           gTofHmm, gTofValid, gTofUsed,
           (unsigned long)gTofSeq, (unsigned long)gTofUsedMask);
  for (int z = 0; z < TOF_ZONES && p < (int)sizeof(buf) - 16; z++)
    p += snprintf(buf + p, sizeof(buf) - p, z ? ",%d" : "%d", (int)gTofD[z]);
  snprintf(buf + p, sizeof(buf) - p, "]}");
  server.send(200, "application/json", buf);
}

void handleTofRaw() { // bench debug: last frame's raw targets (both), unfiltered
  static char buf[1400];
  int p = snprintf(buf, sizeof(buf), "{\"seq\":%lu,\"z\":[", (unsigned long)gTofSeq);
  for (int z = 0; z < TOF_ZONES && p < (int)sizeof(buf) - 80; z++) {
    p += snprintf(buf + p, sizeof(buf) - p, "%s{\"nt\":%u", z ? "," : "",
                  tofData.nb_target_detected[z]);
    for (int t = 0; t < VL53L5CX_NB_TARGET_PER_ZONE; t++) {
      uint16_t i = z * VL53L5CX_NB_TARGET_PER_ZONE + t;
      p += snprintf(buf + p, sizeof(buf) - p, ",\"d%d\":%d,\"s%d\":%u", t,
                    (int)tofData.distance_mm[i], t, tofData.target_status[i]);
    }
    p += snprintf(buf + p, sizeof(buf) - p, "}");
  }
  snprintf(buf + p, sizeof(buf) - p, "]}");
  server.send(200, "application/json", buf);
}

void setupWifi() {
#if HAVE_SECRETS
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname("swaydemo");
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
    Serial.print("Sway Demo STA at http://");
    Serial.println(WiFi.localIP());
    if (apOk) {
      Serial.print("Sway Demo AP '" AP_SSID "' -> http://");
      Serial.println(WiFi.softAPIP());
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
  Serial.println("Sway Demo " FW_VERSION);

  // SDK init for rails + telemetry only (charging OFF -- see note at the include).
  Result pf = Result::Failure;
  for (int i = 0; i < 4 && pf != Result::Ok; i++) {
    pf = Board.init(2000, Mainboard::BatteryType::Generic_LFP);
    if (pf != Result::Ok) delay(250);
  }
  if (pf == Result::Ok) {
    gPfReady = true;
    Board.enableBatteryCharging(false); // chargeTick() enables it once the gauge warms up
    Board.setSupplyMaintainVoltage(SWAY_MAINTAIN_V);
    // Power-CYCLE the sensor rail so the MSA311 gets a fresh POR (VSQT persists
    // across ESP reboots; presence_bench learned this the hard way).
    Board.enableVSQT(false);
    Board.enable3V3(true); // LED rail
    delay(600);
    Board.enableVSQT(true);
    Serial.println("PowerFeather SDK Ok: VSQT power-cycled, 3V3 on, charging OFF (bench)");
    // Clear a latched BQ EN_HIZ (REG0x16 bit 4) left by a prior image -- otherwise
    // the board can silently drain its cell while on USB (presence_bench pattern).
    uint8_t r16 = 0;
    Wire1.beginTransmission(0x6A);
    Wire1.write(0x16);
    if (Wire1.endTransmission(false) == 0 && Wire1.requestFrom(0x6A, 1) == 1) {
      r16 = Wire1.read();
      if (r16 & (1 << 4)) {
        Wire1.beginTransmission(0x6A);
        Wire1.write(0x16);
        Wire1.write(r16 & ~(1 << 4));
        Serial.printf("BQ EN_HIZ was latched (REG0x16=0x%02X) -> cleared %s\n", r16,
                      Wire1.endTransmission() == 0 ? "ok" : "FAILED");
      }
    }
  } else {
    Serial.println("WARNING: Board.init failed -- VSQT may be unpowered (MSA311 will "
                   "read missing); enabling 3V3 + Wire1 manually");
    Wire1.begin(47, 48, 100000); // STEMMA bus pins, manual bring-up
  }
  pinMode(EN_3V3_PIN, OUTPUT);
  digitalWrite(EN_3V3_PIN, HIGH); // LED rail fallback/no-op
  delay(100);                     // rail settle before first probe
  Wire1.setClock(100000); // shared charger/gauge bus: 100 kHz, NEVER faster

  strip.begin();
  strip.setBrightness(255);
  strip.clear();
  strip.show();

  applySens();
  gMsaOk = msaInit();
  Serial.printf("MSA311 on Wire1 (GPIO47/48 @ 100 kHz): %s\n",
                gMsaOk ? "OK" : "NOT FOUND (will retry every 5 s)");
  gCalDueMs = millis() + 1500; // auto re-zero once the gravity filter settles
  tofBuildRays();
  tofInit(); // several seconds (fw blob upload) -- before WiFi so boot order is stable

  setupWifi();
  if (MDNS.begin("swaydemo")) { // http://swaydemo.local/
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://swaydemo.local/");
  } else {
    Serial.println("mDNS start failed (use the IP)");
  }
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/set", handleSet);
  server.on("/state", handleState);
  server.on("/tofraw", handleTofRaw);
  // Standard OTA (led_studio/net_bench pattern) so tweaks never need the tether:
  //   curl -F "firmware=@sway_demo.ino.bin" http://<ip>/update
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
  Serial.printf("Sway Demo ready: LED on GPIO%d, mode=sway, full scale %.2f g\n",
                DATA_PIN, gFullScaleG);
}

void loop() {
  server.handleClient();
  msaRetryTick();
  sensorTick();
  tofTick();
  batteryTick();
  chargeTick();
  solarGuardTick();
}
