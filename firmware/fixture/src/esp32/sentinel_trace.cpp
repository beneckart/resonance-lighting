#include "sentinel_trace.h"

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>

#include "../core/fixture_context.h"
#include "../core/presence_wave.h"
#include "../core/sentinel_trace.h"
#include "../core/version.h"
#include "board_power.h"
#include "espnow_link.h"
#include "identity.h"
#include "loads.h"
#include "maintenance.h"
#include "ota_verify.h"
#include "sensors/sensors.h"
#include "telemetry.h"

static constexpr uint32_t kRadioSettleMs = 30000UL;
static constexpr uint32_t kBaselineMs = 10UL * 60UL * 1000UL;
static constexpr uint32_t kTofWarmupMs = 30000UL;
static constexpr uint32_t kTofActiveMs = 10UL * 60UL * 1000UL;
static constexpr uint32_t kSampleMs = 1000UL;
static constexpr uint32_t kMaintenanceRetryMs = 60000UL;

static SentinelTraceBuffer gTrace;
static uint8_t gPhase = SENTINEL_TRACE_DISABLED;
static uint32_t gPhaseStartMs = 0;
static uint32_t gNextSampleMs = 0;
static uint32_t gNextMaintRetryMs = 0;
static bool gSensorRailOn = false;
static Vl53CoverGate gCoverGate;
static bool gPresenceRisingPending = false;

bool sentinelTraceBuild() {
#if defined(RES_SENTINEL_TRACE_TARGET)
  return true;
#else
  return false;
#endif
}

uint32_t sentinelTraceTargetId() {
#if defined(RES_SENTINEL_TRACE_TARGET)
  return (uint32_t)RES_SENTINEL_TRACE_TARGET;
#else
  return 0;
#endif
}

bool sentinelTraceTargetMatches() {
#if defined(RES_SENTINEL_TRACE_TARGET)
  uint32_t actual = ((uint32_t)gMyId[0] << 16) |
                    ((uint32_t)gMyId[1] << 8) | gMyId[2];
  return actual == (uint32_t)RES_SENTINEL_TRACE_TARGET;
#else
  return false;
#endif
}

bool sentinelTraceSkipInitialSensors() {
  return sentinelTraceBuild() && sentinelTraceTargetMatches();
}

bool sentinelTraceOwnsLoop() {
  return sentinelTraceTargetMatches() &&
         gPhase >= SENTINEL_TRACE_BASELINE_A &&
         gPhase <= SENTINEL_TRACE_RETRIEVAL;
}

uint8_t sentinelTracePhase() { return gPhase; }
uint32_t sentinelTraceCapacity() { return gTrace.capacity; }
uint32_t sentinelTraceCount() { return gTrace.count; }
uint32_t sentinelTraceOverwrites() { return gTrace.overwrites; }

static void setPhase(uint8_t phase) {
  gPhase = phase;
  gPhaseStartMs = millis();
  Serial.printf("sentinel-trace: -> %s\n", sentinelTracePhaseName(phase));
}

void sentinelTraceInit() {
#if defined(RES_SENTINEL_TRACE_TARGET)
  if (!sentinelTraceTargetMatches()) {
    Serial.printf("sentinel-trace: target mismatch build=%06lX actual=%s; disabled\n",
                  (unsigned long)sentinelTraceTargetId(), gShortId.c_str());
    return;
  }
  if (gTelemetryFixtureClass != FIXTURE_PERIMETER) {
    gPhase = SENTINEL_TRACE_ERROR;
    Serial.printf("sentinel-trace: target class=%s, expected perimeter; refusing experiment\n",
                  fixtureClassName(gTelemetryFixtureClass));
    return;
  }

  uint32_t capacity = 4096;
  SentinelTraceSample *storage = (SentinelTraceSample *)heap_caps_malloc(
      sizeof(SentinelTraceSample) * capacity,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!storage) {
    capacity = 1024;
    storage = (SentinelTraceSample *)heap_caps_malloc(
        sizeof(SentinelTraceSample) * capacity, MALLOC_CAP_8BIT);
  }
  sentinelTraceBufferInit(gTrace, storage, storage ? capacity : 0);
  if (!gTrace.capacity) {
    gPhase = SENTINEL_TRACE_ERROR;
    Serial.println("sentinel-trace: buffer allocation failed; refusing experiment");
    return;
  }
  vl53CoverInit(gCoverGate);
  gSensorRailOn = true; // class probe left the verified VSQT domain on
  setPhase(SENTINEL_TRACE_RADIO_SETTLE);
  Serial.printf("sentinel-trace: armed target=%s capacity=%lu bytes=%lu A/B/A=600/600/600s\n",
                gShortId.c_str(), (unsigned long)gTrace.capacity,
                (unsigned long)(gTrace.capacity * sizeof(SentinelTraceSample)));
#endif
}

static int16_t signedMa(float value) {
  if (value > 32767.0f) return 32767;
  if (value < -32768.0f) return -32768;
  return (int16_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

static uint16_t milliVolts(float value) {
  if (!(value > 0.0f)) return 0;
  float scaled = value * 1000.0f;
  if (scaled >= 65535.0f) return 65535;
  return (uint16_t)(scaled + 0.5f);
}

static void sampleNow(uint32_t now) {
  SentinelTraceSample sample = {};
  const SensorSnapshot &sensor = sensors();
  const BqSnapshot &bq = bqSnapshot();
  sample.uptimeMs = now;
  sample.phaseElapsedMs = now - gPhaseStartMs;
  sample.vlReads = sensor.vlReads;
  sample.batteryMa = signedMa(batteryMa());
  sample.batteryRawMa = signedMa(batteryMaRaw());
  sample.supplyMa = signedMa(supplyMa());
  sample.batteryMv = milliVolts(batteryVolts());
  sample.supplyMv = milliVolts(supplyVolts());
  sample.vlClosestMm = sensor.vlClosestMm;
  int soc = batterySocPct();
  sample.socPct = soc < -1 ? -1 : (soc > 100 ? 100 : (int8_t)soc);
  sample.phase = gPhase;
  sample.powerFlags = powerSampleFlags();
  sample.supplyGood = supplyGood() ? 1 : 0;
  sample.chargingEnabled = chargingEnabled() ? 1 : 0;
  sample.chargerPhase = bq.stat1 == 0xFF ? 0xFF : (bq.stat1 >> 3) & 0x03;
  sample.chargerFault = bq.fault0;
  sample.radioOn = espNowUp() ? 1 : 0;
  sample.sensorRailOn = gSensorRailOn ? 1 : 0;
  sample.vlOk = sensor.vlOk ? 1 : 0;
  sample.vlNearZones = sensor.vlNearZones;
  sample.vlValidZones = sensor.vlValidZones;
  sample.presenceRising = gPresenceRisingPending ? 1 : 0;
  gPresenceRisingPending = false;
  sentinelTraceBufferAppend(gTrace, sample);
}

static void radioAndSensorsOff() {
  allLoadsOff("sentinel trace baseline");
  espNowDeinit();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  railEnableVSQT(false);
  gSensorRailOn = false;
}

static void startTof() {
  esp_task_wdt_reset();
  gSensorRailOn = railEnableVSQT(true);
  if (!gSensorRailOn) {
    setPhase(SENTINEL_TRACE_ERROR);
    Serial.println("sentinel-trace: VSQT enable failed; refusing ToF phase");
    return;
  }
  delay(150);
  sensorsInit(FIXTURE_PERIMETER);
  esp_task_wdt_reset();
  vl53CoverInit(gCoverGate);
  setPhase(SENTINEL_TRACE_TOF_WARMUP);
}

static void beginRetrieval() {
  railEnableVSQT(false);
  gSensorRailOn = false;
  setPhase(SENTINEL_TRACE_RETRIEVAL);
  if (!enterMaintenance()) {
    gNextMaintRetryMs = millis() + kMaintenanceRetryMs;
    Serial.println("sentinel-trace: maintenance start failed; retry in 60s");
  }
}

void sentinelTraceTick() {
#if defined(RES_SENTINEL_TRACE_TARGET)
  if (!sentinelTraceTargetMatches() || !gTrace.capacity ||
      gPhase == SENTINEL_TRACE_ERROR)
    return;
  uint32_t now = millis();

  if (gPhase == SENTINEL_TRACE_RADIO_SETTLE) {
    if (now - gPhaseStartMs >= kRadioSettleMs && !otaVerifyPending()) {
      radioAndSensorsOff();
      setPhase(SENTINEL_TRACE_BASELINE_A);
      gNextSampleMs = millis();
    }
    return;
  }

  if (gPhase == SENTINEL_TRACE_RETRIEVAL) {
    if (maintMode() != MODE_MAINT && gNextMaintRetryMs &&
        (int32_t)(now - gNextMaintRetryMs) >= 0) {
      gNextMaintRetryMs = now + kMaintenanceRetryMs;
      enterMaintenance();
    }
    return;
  }

  if (gPhase == SENTINEL_TRACE_TOF_WARMUP ||
      gPhase == SENTINEL_TRACE_TOF_ACTIVE) {
    sensorsTick();
    const SensorSnapshot &sensor = sensors();
    if (sensor.vlPresent && sensor.vlOk)
      gPresenceRisingPending =
          gPresenceRisingPending ||
          vl53CoverObserve(gCoverGate, sensor.vlReads, sensor.vlNearZones);
  }

  if ((int32_t)(now - gNextSampleMs) >= 0) {
    gNextSampleMs = now + kSampleMs;
    sampleNow(now);
  }

  uint32_t elapsed = now - gPhaseStartMs;
  if (gPhase == SENTINEL_TRACE_BASELINE_A && elapsed >= kBaselineMs) {
    startTof();
  } else if (gPhase == SENTINEL_TRACE_TOF_WARMUP &&
             elapsed >= kTofWarmupMs) {
    setPhase(SENTINEL_TRACE_TOF_ACTIVE);
  } else if (gPhase == SENTINEL_TRACE_TOF_ACTIVE &&
             elapsed >= kTofActiveMs) {
    railEnableVSQT(false);
    gSensorRailOn = false;
    setPhase(SENTINEL_TRACE_BASELINE_B);
  } else if (gPhase == SENTINEL_TRACE_BASELINE_B &&
             elapsed >= kBaselineMs) {
    beginRetrieval();
  }
#endif
}

static uint32_t parseUint32(const String &text, uint32_t fallback) {
  if (!text.length()) return fallback;
  char *end = nullptr;
  unsigned long value = strtoul(text.c_str(), &end, 10);
  return end && *end == '\0' ? (uint32_t)value : fallback;
}

void sentinelTraceHandleHttp(WebServer &server) {
#if !defined(RES_SENTINEL_TRACE_TARGET)
  server.send(404, "application/json",
              "{\"ok\":false,\"error\":\"not a sentinel trace build\"}\n");
#else
  if (!sentinelTraceTargetMatches()) {
    server.send(409, "application/json",
                "{\"ok\":false,\"error\":\"sentinel target mismatch\"}\n");
    return;
  }
  if (!gTrace.capacity) {
    server.send(503, "application/json",
                "{\"ok\":false,\"error\":\"trace buffer unavailable\"}\n");
    return;
  }
  uint32_t after = server.hasArg("after")
                       ? parseUint32(server.arg("after"), 0)
                       : 0;
  uint32_t requested = server.hasArg("max")
                           ? parseUint32(server.arg("max"), 16)
                           : 16;
  if (requested > 32) requested = 32;
  SentinelTraceSample batch[32];
  uint32_t count = requested
                       ? sentinelTraceBufferCollectAfter(gTrace, after, batch,
                                                         requested)
                       : 0;

  String body;
  body.reserve(512 + count * 360);
  body += "{\"kind\":\"meta\",\"schema\":1,\"fixture_id\":\"";
  body += gShortId;
  body += "\",\"fw\":\"" RES_FIXTURE_VERSION "\",\"target\":\"";
  char target[7];
  snprintf(target, sizeof(target), "%06lX",
           (unsigned long)sentinelTraceTargetId());
  body += target;
  body += "\",\"sample_hz\":1,\"sample_bytes\":";
  body += String((unsigned)sizeof(SentinelTraceSample));
  body += ",\"phase\":\"" + String(sentinelTracePhaseName(gPhase)) + "\"";
  body += ",\"capacity\":" + String((unsigned long)gTrace.capacity);
  body += ",\"count\":" + String((unsigned long)gTrace.count);
  body += ",\"oldest_seq\":" +
          String((unsigned long)sentinelTraceBufferOldestSeq(gTrace));
  body += ",\"newest_seq\":" +
          String((unsigned long)sentinelTraceBufferNewestSeq(gTrace));
  body += ",\"overwrites\":" + String((unsigned long)gTrace.overwrites);
  body += "}\n";

  for (uint32_t i = 0; i < count; ++i) {
    const SentinelTraceSample &s = batch[i];
    body += "{\"kind\":\"sample\",\"seq\":" + String((unsigned long)s.seq);
    body += ",\"uptime_ms\":" + String((unsigned long)s.uptimeMs);
    body += ",\"phase_elapsed_ms\":" + String((unsigned long)s.phaseElapsedMs);
    body += ",\"phase\":\"" + String(sentinelTracePhaseName(s.phase)) + "\"";
    body += ",\"battery_mv\":" + String(s.batteryMv);
    body += ",\"battery_ma\":" + String(s.batteryMa);
    body += ",\"battery_raw_ma\":" + String(s.batteryRawMa);
    body += ",\"supply_mv\":" + String(s.supplyMv);
    body += ",\"supply_ma\":" + String(s.supplyMa);
    body += ",\"soc_pct\":" + String(s.socPct);
    body += ",\"power_flags\":" + String(s.powerFlags);
    body += ",\"supply_good\":" + String(s.supplyGood);
    body += ",\"charging_enabled\":" + String(s.chargingEnabled);
    body += ",\"charger_phase\":" + String(s.chargerPhase);
    body += ",\"charger_fault\":" + String(s.chargerFault);
    body += ",\"radio_on\":" + String(s.radioOn);
    body += ",\"sensor_rail_on\":" + String(s.sensorRailOn);
    body += ",\"vl_reads\":" + String((unsigned long)s.vlReads);
    body += ",\"vl_ok\":" + String(s.vlOk);
    body += ",\"vl_closest_mm\":" + String(s.vlClosestMm);
    body += ",\"vl_near_zones\":" + String(s.vlNearZones);
    body += ",\"vl_valid_zones\":" + String(s.vlValidZones);
    body += ",\"presence_rising\":" + String(s.presenceRising) + "}\n";
  }
  server.send(200, "application/x-ndjson", body);
#endif
}
