#include "stream_svc.h"

#include <Arduino.h>
#include <string.h>

#include "../core/census.h"
#include "../core/direct_frame_plan.h"
#include "../core/pattern_model.h"
#include "census_svc.h"
#include "mesh_tx.h"

static StreamMode gMode = StreamMode::OFF;
static uint8_t gClassFilter = 0;
static uint8_t gR = 0, gG = 0, gB = 0, gW = 0, gDim = 255;
static int gTargets = 0;
static uint32_t gLastWaveMs = 0;
static uint32_t gModeStartedMs = 0;
static uint32_t gGeneration = 0;
static PatternSettings gPatternSettings = patternDefaultSettings();
static portMUX_TYPE gStreamMux = portMUX_INITIALIZER_UNLOCKED;

StreamMode streamMode() {
  portENTER_CRITICAL(&gStreamMux);
  StreamMode mode = gMode;
  portEXIT_CRITICAL(&gStreamMux);
  return mode;
}

uint32_t streamElapsedMs(uint32_t nowMs) {
  portENTER_CRITICAL(&gStreamMux);
  uint32_t elapsed = gMode == StreamMode::OFF ? 0 : nowMs - gModeStartedMs;
  portEXIT_CRITICAL(&gStreamMux);
  return elapsed;
}

int streamTargetCount() {
  portENTER_CRITICAL(&gStreamMux);
  int targets = gTargets;
  portEXIT_CRITICAL(&gStreamMux);
  return targets;
}

static bool streamStart(StreamMode mode, uint8_t classFilter, uint8_t r,
                        uint8_t g, uint8_t b, uint8_t w, uint8_t dim) {
  uint32_t now = millis();
  portENTER_CRITICAL(&gStreamMux);
  gClassFilter = classFilter;
  gR = r;
  gG = g;
  gB = b;
  gW = w;
  gDim = dim;
  gMode = mode;
  gModeStartedMs = now;
  gLastWaveMs = 0;
  ++gGeneration;
  portEXIT_CRITICAL(&gStreamMux);
  return true;
}

bool streamSolid(uint8_t classFilter, uint8_t r, uint8_t g, uint8_t b,
                 uint8_t w, uint8_t dim) {
  return streamStart(StreamMode::SOLID, classFilter, r, g, b, w, dim);
}

bool streamBlink(uint8_t classFilter, uint8_t r, uint8_t g, uint8_t b,
                 uint8_t w, uint8_t dim) {
  return streamStart(StreamMode::BLINK, classFilter, r, g, b, w, dim);
}

bool streamPattern(const PatternSettings &settings, uint32_t startedMs) {
  PatternSettings safe = patternSanitize(settings);
  portENTER_CRITICAL(&gStreamMux);
  gPatternSettings = safe;
  gMode = StreamMode::PATTERN;
  gModeStartedMs = startedMs;
  gLastWaveMs = 0;
  ++gGeneration;
  portEXIT_CRITICAL(&gStreamMux);
  return true;
}

void streamPatternStop() {
  portENTER_CRITICAL(&gStreamMux);
  if (gMode == StreamMode::PATTERN) {
    gMode = StreamMode::OFF;
    gTargets = 0;
    ++gGeneration;
  }
  portEXIT_CRITICAL(&gStreamMux);
}

bool streamPatternActive() { return streamMode() == StreamMode::PATTERN; }

void streamStop() {
  portENTER_CRITICAL(&gStreamMux);
  gMode = StreamMode::OFF;
  gTargets = 0;
  ++gGeneration;
  portEXIT_CRITICAL(&gStreamMux);
}

void streamSvcTick(uint32_t nowMs) {
  StreamMode mode;
  uint8_t classFilter, r, g, b, w, dim;
  uint32_t modeStartedMs, generation;
  PatternSettings patternSettings;
  portENTER_CRITICAL(&gStreamMux);
  if (gMode == StreamMode::OFF || nowMs - gLastWaveMs < 125) {
    portEXIT_CRITICAL(&gStreamMux);
    return;
  }
  gLastWaveMs = nowMs;
  mode = gMode;
  classFilter = gClassFilter;
  r = gR;
  g = gG;
  b = gB;
  w = gW;
  dim = gDim;
  modeStartedMs = gModeStartedMs;
  generation = gGeneration;
  patternSettings = gPatternSettings;
  portEXIT_CRITICAL(&gStreamMux);

  static CensusView rows[CENSUS_MAX_TRACKED];
  static DirectPlanEntry plan[CENSUS_MAX_TRACKED];
  static PatternNode patternNodes[CENSUS_MAX_TRACKED];
  static PatternFrameEntry patternPlan[CENSUS_MAX_TRACKED];
  size_t n = censusSnapshotSafe(rows, CENSUS_MAX_TRACKED, nowMs);
  size_t planned = 0;
  if (mode == StreamMode::PATTERN) {
    for (size_t i = 0; i < n; ++i) {
      memcpy(patternNodes[i].id, rows[i].id, sizeof(patternNodes[i].id));
      patternNodes[i].fixtureClass = rows[i].fixtureClass;
      patternNodes[i].ageMs = rows[i].ageMs;
    }
    planned = patternPlanFrame(patternNodes, n, censusFreshMsSafe(),
                               patternSettings, nowMs - modeStartedMs,
                               patternPlan, CENSUS_MAX_TRACKED);
  } else {
    bool visible = mode != StreamMode::BLINK ||
                   directFrameBlinkVisible(nowMs - modeStartedMs);
    planned = directFramePlan(rows, n, censusFreshMsSafe(), classFilter, r, g,
                              b, w, dim, visible, plan,
                              CENSUS_MAX_TRACKED);
  }

  portENTER_CRITICAL(&gStreamMux);
  bool stillOwner = gGeneration == generation && gMode == mode;
  if (stillOwner) gTargets = (int)planned;
  portEXIT_CRITICAL(&gStreamMux);
  if (!stillOwner) return;
  uint8_t flags = mode == StreamMode::BLINK
                      ? 0x03 /*micro-lease + hard-cut blink edge*/
                      : 0x01 /*micro-lease*/;
  MeshDirectEntry entries[18];
  size_t chunks = directFrameChunkCount(planned, 18);
  for (size_t chunk = 0; chunk < chunks; ++chunk) {
    size_t count = directFrameChunkSize(planned, chunk, 18);
    size_t start = chunk * 18;
    for (size_t i = 0; i < count; ++i) {
      if (mode == StreamMode::PATTERN) {
        memcpy(entries[i].id, patternPlan[start + i].id, 3);
        entries[i].r = patternPlan[start + i].r;
        entries[i].g = patternPlan[start + i].g;
        entries[i].b = patternPlan[start + i].b;
        entries[i].w = patternPlan[start + i].w;
      } else {
        memcpy(entries[i].id, plan[start + i].id, 3);
        entries[i].r = plan[start + i].r;
        entries[i].g = plan[start + i].g;
        entries[i].b = plan[start + i].b;
        entries[i].w = plan[start + i].w;
      }
    }
    meshDirectFrame(entries, (uint8_t)count, flags);
  }
}
