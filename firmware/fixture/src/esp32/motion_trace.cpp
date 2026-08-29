#include "motion_trace.h"

#include <Arduino.h>
#include <WebServer.h>
#include <esp_heap_caps.h>

#include "../core/fixture_context.h"
#include "../core/interaction_modulator.h"
#include "../core/motion_trace.h"
#include "../core/version.h"
#include "behavior_glue.h"
#include "identity.h"
#include "led_driver.h"
#include "sensors/sensors.h"
#include "telemetry.h"

static MotionTraceBuffer gTrace;
static uint32_t gLastMsaRead = 0;
static bool gPendingPresenceRising = false;

bool motionTraceBuild() {
#if defined(RES_MSA_TRACE_TARGET)
  return true;
#else
  return false;
#endif
}

uint32_t motionTraceTargetId() {
#if defined(RES_MSA_TRACE_TARGET)
  return (uint32_t)RES_MSA_TRACE_TARGET;
#else
  return 0;
#endif
}

bool motionTraceTargetMatches() {
#if defined(RES_MSA_TRACE_TARGET)
  uint32_t actual = ((uint32_t)gMyId[0] << 16) |
                    ((uint32_t)gMyId[1] << 8) | gMyId[2];
  return actual == (uint32_t)RES_MSA_TRACE_TARGET;
#else
  return false;
#endif
}

uint32_t motionTraceCapacity() { return gTrace.capacity; }
uint32_t motionTraceCount() { return gTrace.count; }
uint32_t motionTraceOverwrites() { return gTrace.overwrites; }

void motionTraceInit() {
#if defined(RES_MSA_TRACE_TARGET)
  if (!motionTraceTargetMatches()) {
    Serial.printf("motion-trace: target mismatch build=%06lX actual=%s; disabled\n",
                  (unsigned long)motionTraceTargetId(), gShortId.c_str());
    return;
  }

  // 8192 samples retain about 5.5 minutes at the fixture's cooperative 25 Hz
  // poll cadence. Keep a short internal-RAM fallback so a PSRAM fault still
  // yields a useful, explicitly reported 20-second diagnostic trace.
  uint32_t capacity = 8192;
  MotionTraceSample *storage = (MotionTraceSample *)heap_caps_malloc(
      sizeof(MotionTraceSample) * capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!storage) {
    capacity = 512;
    storage = (MotionTraceSample *)heap_caps_malloc(
        sizeof(MotionTraceSample) * capacity, MALLOC_CAP_8BIT);
  }
  motionTraceBufferInit(gTrace, storage, storage ? capacity : 0);
  Serial.printf("motion-trace: armed target=%s capacity=%lu samples bytes=%lu\n",
                gShortId.c_str(), (unsigned long)gTrace.capacity,
                (unsigned long)(gTrace.capacity * sizeof(MotionTraceSample)));
#endif
}

static int16_t signedMilli(float value) {
  float scaled = value * 1000.0f;
  if (scaled > 32767.0f) return 32767;
  if (scaled < -32768.0f) return -32768;
  return (int16_t)(scaled >= 0 ? scaled + 0.5f : scaled - 0.5f);
}

static uint16_t unsignedMilli(float value) {
  if (!(value > 0.0f)) return 0;
  float scaled = value * 1000.0f;
  if (scaled >= 65535.0f) return 65535;
  return (uint16_t)(scaled + 0.5f);
}

static uint16_t centiDegrees(float value) {
  if (!(value > 0.0f)) return 0;
  float scaled = value * 100.0f;
  if (scaled >= 65535.0f) return 65535;
  return (uint16_t)(scaled + 0.5f);
}

static uint16_t positiveMillimetres(float value) {
  if (!(value > 0.0f)) return 0;
  if (value >= 65535.0f) return 65535;
  return (uint16_t)(value + 0.5f);
}

void motionTraceTick() {
#if defined(RES_MSA_TRACE_TARGET)
  if (!motionTraceTargetMatches() || !gTrace.capacity) return;
  // The TMF edge and the 25 Hz MSA sample need not land in the same loop.
  // Hold the one-loop edge until it has been committed beside a motion sample.
  gPendingPresenceRising =
      gPendingPresenceRising || behaviorTofPresenceRising();
  const SensorSnapshot &sensor = sensors();
  if (!sensor.msaOk || !sensor.msaReads || sensor.msaReads == gLastMsaRead)
    return;
  gLastMsaRead = sensor.msaReads;

  MotionTraceSample sample = {};
  sample.uptimeMs = sensor.msaSampleMs;
  sample.accelMg[0] = signedMilli(sensor.accelXG);
  sample.accelMg[1] = signedMilli(sensor.accelYG);
  sample.accelMg[2] = signedMilli(sensor.accelZG);
  sample.gravityMg[0] = signedMilli(sensor.gravityXG);
  sample.gravityMg[1] = signedMilli(sensor.gravityYG);
  sample.gravityMg[2] = signedMilli(sensor.gravityZG);
  sample.tiltCdeg = centiDegrees(sensor.tiltDeg);
  sample.swayMg = unsignedMilli(sensor.swayEnvG);
  sample.fixtureClass = gTelemetryFixtureClass;
  if (gTelemetryFixtureClass == FIXTURE_DOWNLIGHT) {
    sample.rangeSensor = MOTION_TRACE_RANGE_TMF8820;
    sample.rangeReads = sensor.tmfReads;
    sample.rangeFrameMs = sensor.tmfFrameMs;
    sample.closestMm = sensor.tofDepthMm;
    sample.closestConfidence = sensor.tofConfidence;
    memcpy(sample.zonePrimary, sensor.tofZoneMm, sizeof(sensor.tofZoneMm));
    memcpy(sample.zoneAuxiliary, sensor.tofZoneConfidence,
           sizeof(sensor.tofZoneConfidence));
  } else if (gTelemetryFixtureClass == FIXTURE_PERIMETER) {
    sample.rangeSensor = MOTION_TRACE_RANGE_VL53L5CX;
    sample.rangeReads = sensor.vlReads;
    sample.rangeFrameMs = sensor.vlFrameMs;
    sample.closestMm = sensor.vlClosestMm;
    memcpy(sample.zonePrimary, sensor.vlZoneNearestMm,
           sizeof(sensor.vlZoneNearestMm));
    memcpy(sample.zoneAuxiliary, sensor.vlZoneGroundMm,
           sizeof(sensor.vlZoneGroundMm));
    sample.planeAMilli = signedMilli(sensor.vlPlaneA);
    sample.planeBMilli = signedMilli(sensor.vlPlaneB);
    sample.planeCMm = positiveMillimetres(sensor.vlPlaneC);
    sample.planeTiltCdeg = centiDegrees(sensor.vlTiltDeg);
    sample.planeValid = sensor.vlPlaneValid ? 1 : 0;
    sample.planeZones = sensor.vlZones;
    sample.validZones = sensor.vlValidZones;
    sample.targetZones = sensor.vlTargetZones;
    sample.nearZones = sensor.vlNearZones;
  }
  sample.presenceActive = behaviorTofPresenceActive() ? 1 : 0;
  sample.presenceRising = gPendingPresenceRising ? 1 : 0;
  sample.rangeInteractionActive =
      (gTelemetryFixtureClass == FIXTURE_DOWNLIGHT && sample.presenceActive) ||
      (gTelemetryFixtureClass == FIXTURE_PERIMETER &&
       sample.closestMm >= RES_TOF_INTERACTION_NEAR_MM &&
       sample.closestMm <= RES_TOF_INTERACTION_MAX_MM);
  gPendingPresenceRising = false;
  sample.lifeState = gTelemetryLifeState;
  sample.program = gTelemetryProgram;
  sample.powerTier = gTelemetryPowerTier;
  LedOutputSnapshot led = ledOutputSnapshot();
  sample.ledRailOn = led.railOn;
  sample.ledR = led.r;
  sample.ledG = led.g;
  sample.ledB = led.b;
  sample.ledW = led.w;
  sample.ledLitPixels = led.litPixels;
  motionTraceBufferAppend(gTrace, sample);
#endif
}

static uint32_t parseUint32(const String &text, uint32_t fallback) {
  if (!text.length()) return fallback;
  char *end = nullptr;
  unsigned long value = strtoul(text.c_str(), &end, 10);
  return end && *end == '\0' ? (uint32_t)value : fallback;
}

static void appendArray(String &body, const uint16_t *values, uint8_t count) {
  body += '[';
  for (uint8_t i = 0; i < count; ++i) {
    if (i) body += ',';
    body += String(values[i]);
  }
  body += ']';
}

void motionTraceHandleHttp(WebServer &server) {
#if !defined(RES_MSA_TRACE_TARGET)
  server.send(404, "application/json", "{\"ok\":false,\"error\":\"not a trace build\"}\n");
#else
  if (!motionTraceTargetMatches()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"trace target mismatch\"}\n");
    return;
  }
  if (!gTrace.capacity) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"trace buffer unavailable\"}\n");
    return;
  }

  uint32_t after = server.hasArg("after")
                       ? parseUint32(server.arg("after"), 0)
                       : 0;
  uint32_t requested = server.hasArg("max")
                           ? parseUint32(server.arg("max"), 16)
                           : 16;
  if (requested > 16) requested = 16;
  // Static storage avoids placing ~2 kB of packed trace records underneath
  // WebServer's own handler stack. The server is single-threaded in loop().
  static MotionTraceSample batch[16];
  uint32_t count = requested
                       ? motionTraceBufferCollectAfter(gTrace, after, batch,
                                                       requested)
                       : 0;

  String body;
  body.reserve(640 + count * 520);
  body += "{\"kind\":\"meta\",\"schema\":2,\"fixture_id\":\"";
  body += gShortId;
  body += "\",\"fw\":\"" RES_FIXTURE_VERSION "\",\"target\":\"";
  char target[7];
  snprintf(target, sizeof(target), "%06lX",
           (unsigned long)motionTraceTargetId());
  body += target;
  body += "\",\"fixture_class\":\"";
  body += fixtureClassName(gTelemetryFixtureClass);
  body += "\",\"sample_hz\":25,\"sample_bytes\":";
  body += String((unsigned)sizeof(MotionTraceSample));
  body += ",\"capacity\":" + String((unsigned long)gTrace.capacity);
  body += ",\"count\":" + String((unsigned long)gTrace.count);
  body += ",\"oldest_seq\":" +
          String((unsigned long)motionTraceBufferOldestSeq(gTrace));
  body += ",\"newest_seq\":" +
          String((unsigned long)motionTraceBufferNewestSeq(gTrace));
  body += ",\"overwrites\":" + String((unsigned long)gTrace.overwrites);
  body += "}\n";

  for (uint32_t i = 0; i < count; ++i) {
    const MotionTraceSample &sample = batch[i];
    body += "{\"kind\":\"sample\",\"seq\":" +
            String((unsigned long)sample.seq);
    body += ",\"uptime_ms\":" + String((unsigned long)sample.uptimeMs);
    body += ",\"accel_mg\":[" + String(sample.accelMg[0]) + ',' +
            String(sample.accelMg[1]) + ',' + String(sample.accelMg[2]) + ']';
    body += ",\"gravity_mg\":[" + String(sample.gravityMg[0]) + ',' +
            String(sample.gravityMg[1]) + ',' + String(sample.gravityMg[2]) + ']';
    body += ",\"tilt_cdeg\":" + String(sample.tiltCdeg);
    body += ",\"sway_mg\":" + String(sample.swayMg);
    body += ",\"fixture_class\":" + String(sample.fixtureClass);
    body += ",\"range_sensor\":" + String(sample.rangeSensor);
    body += ",\"range_reads\":" + String((unsigned long)sample.rangeReads);
    body += ",\"range_frame_ms\":" +
            String((unsigned long)sample.rangeFrameMs);
    body += ",\"closest_mm\":" + String(sample.closestMm);
    if (sample.rangeSensor == MOTION_TRACE_RANGE_TMF8820) {
      body += ",\"closest_confidence\":" + String(sample.closestConfidence);
      body += ",\"tmf_zone_mm\":";
      appendArray(body, sample.zonePrimary, 9);
      body += ",\"tmf_zone_confidence\":";
      appendArray(body, sample.zoneAuxiliary, 9);
    } else if (sample.rangeSensor == MOTION_TRACE_RANGE_VL53L5CX) {
      body += ",\"vl_zone_nearest_mm\":";
      appendArray(body, sample.zonePrimary, MOTION_TRACE_ZONE_COUNT);
      body += ",\"vl_zone_ground_mm\":";
      appendArray(body, sample.zoneAuxiliary, MOTION_TRACE_ZONE_COUNT);
      body += ",\"vl_plane_valid\":" + String(sample.planeValid);
      body += ",\"vl_plane_a_milli\":" + String(sample.planeAMilli);
      body += ",\"vl_plane_b_milli\":" + String(sample.planeBMilli);
      body += ",\"vl_plane_c_mm\":" + String(sample.planeCMm);
      body += ",\"vl_plane_tilt_cdeg\":" + String(sample.planeTiltCdeg);
      body += ",\"vl_plane_zones\":" + String(sample.planeZones);
      body += ",\"vl_valid_zones\":" + String(sample.validZones);
      body += ",\"vl_target_zones\":" + String(sample.targetZones);
      body += ",\"vl_near_zones\":" + String(sample.nearZones);
      body += ",\"vl_no_return\":" + String(sample.validZones == 0 ? 1 : 0);
    }
    body += ",\"presence_active\":" + String(sample.presenceActive);
    body += ",\"presence_rising\":" + String(sample.presenceRising);
    body += ",\"range_interaction_active\":" +
            String(sample.rangeInteractionActive);
    body += ",\"life_state\":" + String(sample.lifeState);
    body += ",\"program\":" + String(sample.program);
    body += ",\"power_tier\":" + String(sample.powerTier);
    body += ",\"led\":[" + String(sample.ledRailOn) + ',' +
            String(sample.ledR) + ',' + String(sample.ledG) + ',' +
            String(sample.ledB) + ',' + String(sample.ledW) + ',' +
            String(sample.ledLitPixels) + "]}\n";
  }
  server.send(200, "application/x-ndjson", body);
#endif
}
