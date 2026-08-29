#include "sensors.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MSA301.h>
#include <SparkFun_TMF882X_Library.h>
#include <new>

#include "../../core/filters.h"
#include "../../core/fixture_context.h"
#include "../../core/tmf_recovery.h"
#include "../../core/tof_grid.h"
#include "../board_power.h"
#include "vl53l5cx_uld/SparkFun_VL53L5CX_Library.h"

static SensorSnapshot gSnap;
static uint8_t gClass = FIXTURE_UNKNOWN;

const SensorSnapshot &sensors() { return gSnap; }

// ---- MSA311 (all sensored classes) -----------------------------------------
static Adafruit_MSA311 gMsa;
static AccelFilter gAccel;
static uint32_t gNextMsaMs = 0;
static uint32_t gZeroAtMs = 0;

static void msaInit() {
  Wire1.setClock(100000);
  gSnap.msaPresent = gMsa.begin(MSA311_I2CADDR_DEFAULT, &Wire1);
  if (gSnap.msaPresent) {
    gMsa.setRange(MSA301_RANGE_4_G);
    gMsa.setDataRate(MSA301_DATARATE_125_HZ);
    gMsa.setBandwidth(MSA301_BANDWIDTH_62_5_HZ);
    gMsa.setPowerMode(MSA301_NORMALMODE);
  }
  accelFilterInit(gAccel);
  gZeroAtMs = millis() + 3000; // auto re-zero once the mount settles post-boot
}

static void msaReinitAfterRailCycle() {
  // Adafruit_MSA311::begin() allocates a new bus object every time and provides
  // no destructor. The existing bus object remains valid across a power cycle,
  // so verify the address and restore the device registers without leaking.
  Wire1.setClock(100000);
  Wire1.beginTransmission(MSA311_I2CADDR_DEFAULT);
  gSnap.msaPresent = Wire1.endTransmission() == 0;
  gSnap.msaOk = false;
  if (gSnap.msaPresent) {
    gMsa.setRange(MSA301_RANGE_4_G);
    gMsa.setDataRate(MSA301_DATARATE_125_HZ);
    gMsa.setBandwidth(MSA301_BANDWIDTH_62_5_HZ);
    gMsa.setPowerMode(MSA301_NORMALMODE);
  }
  gNextMsaMs = 0;
  accelFilterInit(gAccel);
  gZeroAtMs = millis() + 3000;
}

static void msaTick(uint32_t now) {
  if (!gSnap.msaPresent || now < gNextMsaMs) return;
  gNextMsaMs = now + 40;
  Wire1.setClock(100000);
  gMsa.read();
  float ax = gMsa.x_g, ay = gMsa.y_g, az = gMsa.z_g;
  float mag = sqrtf(ax * ax + ay * ay + az * az);
  gSnap.msaOk = isfinite(mag) && mag > 0.05f;
  if (!gSnap.msaOk) return;
  accelFilterSample(gAccel, ax, ay, az);
  if (!gAccel.restZeroed && gZeroAtMs && now >= gZeroAtMs) {
    if (accelFilterZero(gAccel)) gZeroAtMs = 0;
  }
  ++gSnap.msaReads;
  gSnap.msaSampleMs = now;
  gSnap.accelXG = ax;
  gSnap.accelYG = ay;
  gSnap.accelZG = az;
  gSnap.gravityXG = gAccel.gx;
  gSnap.gravityYG = gAccel.gy;
  gSnap.gravityZG = gAccel.gz;
  gSnap.tiltDeg = gAccel.tiltDeg;
  gSnap.swayEnvG = gAccel.swayEnv;
}

// ---- TMF8820 cooperative one-shot machine (downlight) ----------------------
// Ported verbatim from led_studio (the mandated pattern): SparkFun's blocking
// startMeasuring() starves the loop 0.7-1.8 s on the 100 kHz bus; splitting
// start -> process-irq -> stop across loop() iterations keeps everything
// responsive. All calls single-threaded on Wire1.
static SparkFun_TMF882X gTmf;
static bool gTmfActive = false;
static uint32_t gTmfCycleStartMs = 0, gTmfNextStartMs = 0, gNextTmfServiceMs = 0;
static uint32_t gTmfCycleReadBase = 0;
static TmfRecoveryPolicy gTmfRecovery;
static bool gTmfDomainResetPending = false;

static void tmfObserveFailure() {
  if (tmfRecoveryObserve(gTmfRecovery, false) == TMF_RECOVERY_DOMAIN_RESET)
    gTmfDomainResetPending = true;
}

static void handleTmfMeasurement(struct tmf882x_msg_meas_results *results) {
  if (!results) return;
  gSnap.tmfOk = true;
  gSnap.tmfReads++;
  tmfRecoveryObserve(gTmfRecovery, true);
  // Every completed report describes a new scene. Clear the previous return
  // before scanning so an empty report cannot masquerade as a person who is
  // still present (the dashboard and presence gate both consume this truth).
  gSnap.tofDepthMm = 0;
  gSnap.tofConfidence = 0;
  memset(gSnap.tofZoneMm, 0, sizeof(gSnap.tofZoneMm));
  memset(gSnap.tofZoneConfidence, 0, sizeof(gSnap.tofZoneConfidence));
  uint16_t usableMm = 0, usableConf = 0;
  uint32_t count = min((uint32_t)TMF882X_MAX_MEAS_RESULTS, results->num_results);
  for (uint32_t i = 0; i < count; ++i) {
    const tmf882x_meas_result &r = results->results[i];
    if (r.distance_mm == 0 || r.distance_mm > UINT16_MAX || r.confidence == 0)
      continue;
    uint16_t mm = (uint16_t)r.distance_mm;
    // The enclosed sensor has a known ~20 mm fixture/window return. Ignore the
    // near field and use the closest confident scene target.
    // TMF8820 is specified through 5000 mm. Keep the entire supported range:
    // at the installed ~15 ft canopy geometry, a person's head is commonly
    // 2700-3200 mm away even though the ground is near the range limit.
    if (mm < 80 || mm > 5000 || r.confidence < 20) continue;
    if (r.channel >= 1 && r.channel <= 9) {
      uint8_t zone = (uint8_t)(r.channel - 1);
      if (!gSnap.tofZoneMm[zone] || mm < gSnap.tofZoneMm[zone]) {
        gSnap.tofZoneMm[zone] = mm;
        gSnap.tofZoneConfidence[zone] =
            (uint16_t)min((uint32_t)UINT16_MAX, r.confidence);
      }
    }
    if (usableMm == 0 || mm < usableMm) {
      usableMm = mm;
      usableConf = (uint16_t)min((uint32_t)UINT16_MAX, r.confidence);
    }
  }
  if (usableMm) {
    gSnap.tofDepthMm = usableMm;
    gSnap.tofConfidence = usableConf;
    if (!isfinite(gSnap.tofDepthFilteredMm) || gSnap.tofDepthFilteredMm <= 0.0f)
      gSnap.tofDepthFilteredMm = usableMm;
    else
      gSnap.tofDepthFilteredMm += 0.35f * ((float)usableMm - gSnap.tofDepthFilteredMm);
  }
}

static void tmfInit() {
  Wire1.setClock(100000);
  gTmfActive = false;
  gTmfCycleStartMs = 0;
  gTmfNextStartMs = 0;
  gNextTmfServiceMs = 0;
  gSnap.tmfPresent = gTmf.begin(Wire1);
  if (!gSnap.tmfPresent) return;
  struct tmf882x_mode_app_config cfg;
  if (gTmf.getTMF882XConfig(cfg)) {
    cfg.report_period_ms = 250;
    gTmf.setTMF882XConfig(cfg);
  }
  gTmf.setMeasurementHandler(handleTmfMeasurement);
  gTmfActive = tmf882x_start(&gTmf.getTMF882XContext()) == 0;
  if (gTmfActive) {
    gTmfCycleStartMs = millis();
    gTmfCycleReadBase = gSnap.tmfReads;
  } else {
    gSnap.tmfErrors++;
    tmfObserveFailure();
    gTmfNextStartMs = millis() + 500;
  }
  Wire1.setClock(100000);
}

static void tmfTick(uint32_t now) {
  if (!gSnap.tmfPresent) return;
  if (gTmfActive && now >= gNextTmfServiceMs) {
    gNextTmfServiceMs = now + 10;
    Wire1.setClock(100000);
    int32_t rc = tmf882x_process_irq(&gTmf.getTMF882XContext());
    if (gSnap.tmfReads != gTmfCycleReadBase) {
      // Complete report arrived: stop before the next asynchronous one-shot,
      // exactly as the high-level wrapper does.
      tmf882x_stop(&gTmf.getTMF882XContext());
      gTmfActive = false;
      gTmfNextStartMs = now + 50;
    } else if (rc != 0 || now - gTmfCycleStartMs > 700) {
      gSnap.tmfErrors++;
      tmf882x_stop(&gTmf.getTMF882XContext());
      gTmfActive = false;
      gSnap.tmfOk = false;
      gSnap.tmfRecoveries++;
      tmfObserveFailure();
      gTmfNextStartMs = now + 500;
    }
    Wire1.setClock(100000);
  }
  if (!gTmfActive && now >= gTmfNextStartMs) {
    Wire1.setClock(100000);
    gTmfActive = tmf882x_start(&gTmf.getTMF882XContext()) == 0;
    if (gTmfActive) {
      gTmfCycleStartMs = now;
      gTmfCycleReadBase = gSnap.tmfReads;
    } else {
      gSnap.tmfErrors++;
      gSnap.tmfRecoveries++;
      tmfObserveFailure();
      gTmfNextStartMs = now + 500;
    }
    Wire1.setClock(100000);
  }
}

// ---- VL53L5CX ground-plane tilt (perimeter) --------------------------------
#define TOF_RES 4
#define TOF_HZ 5
#define TOF_ZONES (TOF_RES * TOF_RES)
#define TOF_MIN_FIT 8

static SparkFun_VL53L5CX gVl;
static bool gVlRanging = false;
static uint32_t gVlRetryAtMs = 0, gNextVlMs = 0;
static float gVlRayX[TOF_ZONES], gVlRayY[TOF_ZONES], gVlRayZ[TOF_ZONES];
static float gVlRestA = 0, gVlRestB = 0;
static bool gVlRestSet = false;

static void vlBuildRays() {
  // 45-degree square FoV: each zone's center ray in sensor frame, unit z.
  for (int r = 0; r < TOF_RES; r++)
    for (int c = 0; c < TOF_RES; c++) {
      int i = r * TOF_RES + c;
      float u = ((c + 0.5f) / TOF_RES - 0.5f) * 0.8284f; // tan(22.5)*2
      float v = ((r + 0.5f) / TOF_RES - 0.5f) * 0.8284f;
      gVlRayX[i] = u;
      gVlRayY[i] = v;
      gVlRayZ[i] = 1.0f;
    }
}

static void vlInit() {
  uint32_t t0 = millis();
  Wire1.setClock(100000);
  Serial.println("[tof] VL53L5CX begin (fw blob upload over 100 kHz I2C, several s)...");
  if (!gVl.begin(0x29, Wire1)) {
    Serial.println("[tof] begin FAILED (absent/unpowered? retry in 30 s)");
    gSnap.vlPresent = false;
    gVlRetryAtMs = millis() + 30000;
    return;
  }
  gVl.setWireMaxPacketSize(124); // ESP32 Wire buffer is 128
  gSnap.vlPresent = true;
  // Only stop if actually ranging: stop_ranging on a fresh device hangs on an
  // MCU-stop bit that never asserts (see vl53l5cx_uld/VENDORED.md).
  if (gVlRanging) {
    gVl.stopRanging();
    gVlRanging = false;
  }
  if (!gVl.setResolution(TOF_ZONES) || !gVl.setRangingFrequency(TOF_HZ)) {
    Serial.println("[tof] config FAILED");
    gSnap.vlOk = false;
    gVlRetryAtMs = millis() + 30000;
    return;
  }
  gVlRanging = gVl.startRanging();
  gSnap.vlOk = gVlRanging;
  vlBuildRays();
  Serial.printf("[tof] up in %lu ms: %dx%d @ %d Hz -> ground-plane tilt\n",
                (unsigned long)(millis() - t0), TOF_RES, TOF_RES, TOF_HZ);
  if (!gVlRanging) gVlRetryAtMs = millis() + 30000;
}

static void vlTick(uint32_t now) {
  if (!gSnap.vlPresent || !gVlRanging) {
    if (gVlRetryAtMs && now >= gVlRetryAtMs) {
      gVlRetryAtMs = 0;
      vlInit();
    }
    return;
  }
  if (now < gNextVlMs) return;
  gNextVlMs = now + 1000 / TOF_HZ;
  Wire1.setClock(100000);
  if (!gVl.isDataReady()) return;
  static VL53L5CX_ResultsData results;
  if (!gVl.getRangingData(&results)) return;
  ++gSnap.vlReads;
  // ULD per-target arrays are zone-major (zone * VL53L5CX_NB_TARGET_PER_ZONE
  // + target, per-zone nb_target_detected gates stale entries); selection
  // lives in core/tof_grid so the native tests pin that layout.
  static_assert((size_t)TOF_ZONES * VL53L5CX_NB_TARGET_PER_ZONE <=
                    sizeof(results.distance_mm) / sizeof(results.distance_mm[0]),
                "zone scan overruns distance_mm");
  static_assert((size_t)TOF_ZONES * VL53L5CX_NB_TARGET_PER_ZONE <=
                    sizeof(results.target_status) / sizeof(results.target_status[0]),
                "zone scan overruns target_status");
  static_assert((size_t)TOF_ZONES <= sizeof(results.nb_target_detected) /
                                         sizeof(results.nb_target_detected[0]),
                "zone scan overruns nb_target_detected");
  uint16_t zoneMm[TOF_ZONES];
  uint16_t closest = 0;
  uint8_t kept = l5cxSelectGround(results.distance_mm, results.target_status,
                                  results.nb_target_detected, TOF_ZONES,
                                  VL53L5CX_NB_TARGET_PER_ZONE, 50, 4000,
                                  zoneMm, &closest);
  gSnap.vlNearZones = l5cxCountNearZones(
      results.distance_mm, results.target_status, results.nb_target_detected,
      TOF_ZONES, VL53L5CX_NB_TARGET_PER_ZONE, 30, 350);
  gSnap.vlTargetZones = 0;
  for (int zone = 0; zone < TOF_ZONES; ++zone)
    if (results.nb_target_detected[zone]) ++gSnap.vlTargetZones;
  gSnap.vlValidZones = l5cxSelectNearest(
      results.distance_mm, results.target_status, results.nb_target_detected,
      TOF_ZONES, VL53L5CX_NB_TARGET_PER_ZONE, 30, 4000,
      gSnap.vlZoneNearestMm);
  float px[TOF_ZONES], py[TOF_ZONES], pz[TOF_ZONES];
  bool keep[TOF_ZONES];
  for (int i = 0; i < TOF_ZONES; i++) {
    keep[i] = zoneMm[i] != 0;
    if (keep[i]) {
      px[i] = gVlRayX[i] * zoneMm[i];
      py[i] = gVlRayY[i] * zoneMm[i];
      pz[i] = gVlRayZ[i] * zoneMm[i];
    } else {
      px[i] = py[i] = pz[i] = 0;
    }
  }
  float a, b, c;
  gSnap.vlZones = kept;
  if (planeFitLS(px, py, pz, keep, TOF_ZONES, TOF_MIN_FIT, &a, &b, &c)) {
    if (!gVlRestSet) {
      gVlRestA = a;
      gVlRestB = b;
      gVlRestSet = true;
    }
    gSnap.vlTiltDeg = planeTiltDeg(a, b, gVlRestA, gVlRestB);
    gSnap.vlOk = true;
  }
  gSnap.vlClosestMm = closest;
}

// ---- BMP581 minimal raw driver (uplight) -----------------------------------
// The Adafruit BMP5xx library is not a dependency here: forced-mode one-shots
// via raw registers keep the module tiny and the bus usage explicit.
#define BMP_ADDR 0x47
#define BMP_REG_CHIP_ID 0x01
#define BMP_REG_TEMP_XLSB 0x1D
#define BMP_REG_OSR_CONFIG 0x36
#define BMP_REG_ODR_CONFIG 0x37

static uint32_t gNextBmpMs = 0;
static uint32_t gBmpReadyMs = 0;
static bool gBmpConverting = false;

static bool bmpWrite(uint8_t reg, uint8_t val) {
  Wire1.beginTransmission(BMP_ADDR);
  Wire1.write(reg);
  Wire1.write(val);
  return Wire1.endTransmission() == 0;
}
static bool bmpRead(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire1.beginTransmission(BMP_ADDR);
  Wire1.write(reg);
  if (Wire1.endTransmission(false) != 0) return false;
  if (Wire1.requestFrom((int)BMP_ADDR, (int)len) != len) return false;
  for (uint8_t i = 0; i < len; i++) buf[i] = (uint8_t)Wire1.read();
  return true;
}

static void bmpInit() {
  Wire1.setClock(100000);
  uint8_t id = 0;
  gSnap.bmpPresent = bmpRead(BMP_REG_CHIP_ID, &id, 1) && id == 0x50;
  if (!gSnap.bmpPresent) return;
  // press_en | osr_p 16x | osr_t 2x (matches the led_studio profile).
  bmpWrite(BMP_REG_OSR_CONFIG, 0x61);
}

static void bmpTick(uint32_t now) {
  if (!gSnap.bmpPresent) return;
  Wire1.setClock(100000);
  if (!gBmpConverting) {
    if (now < gNextBmpMs) return;
    gNextBmpMs = now + 1000; // 1 Hz env cadence
    // Forced one-shot conversion (mode bits 0-1 = 0b10).
    if (bmpWrite(BMP_REG_ODR_CONFIG, 0x02)) {
      gBmpConverting = true;
      gBmpReadyMs = now + 80; // 16x pressure OSR conversion time + margin
    }
    return;
  }
  if (now < gBmpReadyMs) return;
  gBmpConverting = false;
  uint8_t d[6];
  if (!bmpRead(BMP_REG_TEMP_XLSB, d, 6)) {
    gSnap.bmpOk = false;
    return;
  }
  int32_t traw = (int32_t)((uint32_t)d[2] << 16 | (uint32_t)d[1] << 8 | d[0]);
  uint32_t praw = (uint32_t)d[5] << 16 | (uint32_t)d[4] << 8 | d[3];
  if (traw & 0x800000) traw -= 0x1000000; // 24-bit two's complement
  float tempC = (float)traw / 65536.0f;
  float pressHpa = (float)praw / 64.0f / 100.0f;
  if (pressHpa < 300.0f || pressHpa > 1200.0f) return; // implausible: drop
  gSnap.bmpOk = true;
  gSnap.tempC = tempC;
  if (gSnap.pressureHpa <= 0.0f) gSnap.pressureHpa = pressHpa;
  else gSnap.pressureHpa += 0.25f * (pressHpa - gSnap.pressureHpa);
}

static void recoverDownlightSensorDomain() {
  gTmfDomainResetPending = false;
  gSnap.tmfDomainResets++;
  Serial.printf("[tof] %u consecutive failures -> bounded VSQT domain reset %u/1\n",
                (unsigned)TMF_DOMAIN_RESET_FAILURES,
                (unsigned)gSnap.tmfDomainResets);

  if (gTmfActive && gSnap.tmfPresent) {
    Wire1.setClock(100000);
    tmf882x_stop(&gTmf.getTMF882XContext());
  }
  gTmfActive = false;
  gSnap.msaOk = gSnap.tmfOk = gSnap.bmpOk = false;
  gBmpConverting = false;
  gNextBmpMs = gBmpReadyMs = 0;

  bool railOk = railCycleVSQT();

  // The SparkFun wrapper treats begin() as a no-op after its first success.
  // Reconstruct it so the strong retry performs tmf882x_init/open, firmware
  // upload, application-mode switch, configuration, and ranging start again.
  gTmf.~SparkFun_TMF882X();
  new (&gTmf) SparkFun_TMF882X();

  msaReinitAfterRailCycle();
  gSnap.tmfPresent = false;
  tmfInit();
  bmpInit();
  Wire1.setClock(100000);
  Serial.printf("[tof] domain reset: rail=%s msa=%s tmf=%s bmp=%s\n",
                railOk ? "verified" : "ERR",
                gSnap.msaPresent ? "present" : "absent",
                gSnap.tmfPresent ? "present" : "absent",
                gSnap.bmpPresent ? "present" : "absent");
}

// ---- dispatch ---------------------------------------------------------------
void sensorsInit(uint8_t fixtureClass) {
  gClass = fixtureClass;
  memset(&gSnap, 0, sizeof(gSnap));
  tmfRecoveryInit(gTmfRecovery);
  gTmfDomainResetPending = false;
  switch (fixtureClass) {
  case FIXTURE_DOWNLIGHT:
    msaInit();
    tmfInit();
    bmpInit(); // optional environmental logger; present on NC batch 1
    break;
  case FIXTURE_PERIMETER:
    msaInit();
    vlInit();
    break;
  case FIXTURE_UPLIGHT:
    msaInit();
    bmpInit();
    break;
  default:
    break; // chandelier: none
  }
  Wire1.setClock(100000);
}

void sensorsTick() {
  if (gClass == FIXTURE_DOWNLIGHT && gTmfDomainResetPending)
    recoverDownlightSensorDomain();
  uint32_t now = millis();
  switch (gClass) {
  case FIXTURE_DOWNLIGHT:
    msaTick(now);
    tmfTick(now);
    bmpTick(now);
    break;
  case FIXTURE_PERIMETER:
    msaTick(now);
    vlTick(now);
    break;
  case FIXTURE_UPLIGHT:
    msaTick(now);
    bmpTick(now);
    break;
  default:
    break;
  }
}
