#include "behavior_glue.h"

#include <Arduino.h>

#include "../core/choreo/program.h"
#include "../core/daytime_ritual.h"
#include "../core/interaction_modulator.h"
#include "../core/lifecycle.h"
#include "../core/neighbor_table.h"
#include "../core/presence_wave.h"
#include "../core/show_schedule.h"
#include "../core/strike_policy.h"
#include "../core/time_consensus.h"
#include "board_power.h"
#include "espnow_link.h"
#include "identity.h"
#include "net_peer.h"
#include "nvs_store.h"
#include "ota_verify.h"
#include "power_glue.h"
#include "sensors/sensors.h"
#include "solenoid.h"
#include "telemetry.h"

#define RES_RX_HOLD_MS 600000UL      // accepted-control hold (10 min)
#define RES_BOOT_AWAKE_MS 600000UL   // cold-boot listen window (10 min)
#ifndef RES_WAKE_LISTEN_MS
#define RES_WAKE_LISTEN_MS 15000UL   // deep-sleep wake listen window
#endif
#ifndef RES_DAY_SLEEP_S
#define RES_DAY_SLEEP_S 300U         // DAY_CHARGE timer sleep
#endif
#ifndef RES_DAYTIME_RITUAL_CANARY_TARGET
#define RES_DAYTIME_RITUAL_CANARY_TARGET 0UL
#endif
#ifndef RES_DAYTIME_RITUAL_CANARY_HOUR
#define RES_DAYTIME_RITUAL_CANARY_HOUR 0UL
#endif
#define RES_CHOREO_KEEPALIVE_MS 1000
#define RES_NEIGHBOR_FRESH_MS 3000
#define RES_WAVE_CAPABLE_FLAG 0x08
#define RES_WAVE_VISITED_MAX 160
#define RES_WAVE_HOPS 150
#define RES_WAVE_ORIGIN_COOLDOWN_MS 2000
#ifndef RES_MSA_LOCAL_INTERACTION
#define RES_MSA_LOCAL_INTERACTION 0
#endif

static ChoreoRuntime gRuntime;
static NeighborTable gNeighbors;
static LifeState_t gLife;
static LifeConfig gLifeCfg;
static uint8_t gClass = FIXTURE_UNKNOWN;
static uint16_t gPixels = 1;
static int8_t gForceNight = -1;
static bool gShowActive = false;
static bool gStrikesAllowed = false;
static bool gProgramSuppressesLight = false;
static FrameBuffer gFrame;
static uint32_t gAwakeGraceUntilMs = 0;
static bool gSolarProbeActive = false;
static uint32_t gNextChoreoTxMs = 0;
static uint32_t gLastShowFrameRxMs = 0;
static uint8_t gLastProfile = 0xFF;
static uint8_t gLastCommissionDefault = 0xFF;
static TimeConsensus gTimeConsensus;
static bool gScheduleValid = false;
static bool gScheduleNight = false;
static bool gRitualKeepAwake = false;

static constexpr uint32_t kRitualRtcMagic = 0x52544C32UL; // "RTL2"
static constexpr uint32_t kDayEnergyCarryMagic = 0x454E5231UL; // "ENR1"
RTC_DATA_ATTR static uint32_t gRitualRtcMagic = 0;
RTC_DATA_ATTR static DaytimeRitualState gRitualState;
RTC_DATA_ATTR static DaytimeRitualAudit gRitualAudit;
RTC_DATA_ATTR static uint32_t gDayEnergyCarry = 0;

static uint32_t shortFixtureId() {
  return ((uint32_t)gMyId[0] << 16) | ((uint32_t)gMyId[1] << 8) |
         gMyId[2];
}

bool behaviorDaytimeRitualCanaryBuild() {
  return RES_DAYTIME_RITUAL_CANARY_TARGET != 0UL &&
         RES_DAYTIME_RITUAL_CANARY_HOUR != 0UL;
}

uint32_t behaviorDaytimeRitualCanaryTarget() {
  return (uint32_t)RES_DAYTIME_RITUAL_CANARY_TARGET;
}

bool behaviorDaytimeRitualCanaryTargetMatches() {
  return behaviorDaytimeRitualCanaryBuild() &&
         shortFixtureId() == behaviorDaytimeRitualCanaryTarget();
}

uint32_t behaviorDaytimeRitualCanaryHourKey() {
  return (uint32_t)RES_DAYTIME_RITUAL_CANARY_HOUR;
}

DaytimeRitualAudit behaviorDaytimeRitualAudit() { return gRitualAudit; }

static void beginRitualAudit(uint32_t hourKey, uint16_t uncertaintyMs) {
  if (gRitualAudit.hourKey != hourKey) {
    memset(&gRitualAudit, 0, sizeof(gRitualAudit));
    gRitualAudit.hourKey = hourKey;
    gRitualAudit.expectedMask = daytimeRitualExpectedMask(gMyId);
  }
  gRitualAudit.lastUncertaintyMs = uncertaintyMs;
}

static bool observeCanaryWindow(const TimeEstimate &wall) {
  if (!behaviorDaytimeRitualCanaryTargetMatches() || !wall.valid ||
      wall.subMs >= 1000)
    return false;
  uint64_t nowMs = (uint64_t)wall.utcS * 1000ULL + wall.subMs;
  uint64_t hourMs =
      (uint64_t)behaviorDaytimeRitualCanaryHourKey() * 3600000ULL;
  uint64_t startMs = hourMs -
                     (uint64_t)DAYTIME_RITUAL_PREWAKE_S * 1000ULL;
  uint64_t endMs = hourMs + (uint64_t)DAYTIME_RITUAL_END_S * 1000ULL;
  uint8_t before = gRitualAudit.flags;
  if (nowMs >= startMs && nowMs <= endMs) {
    beginRitualAudit(behaviorDaytimeRitualCanaryHourKey(),
                     wall.uncertaintyMs);
    gRitualAudit.flags |= DAYTIME_RITUAL_AUDIT_WINDOW_SEEN;
  } else if (nowMs > endMs &&
             gRitualAudit.hourKey == behaviorDaytimeRitualCanaryHourKey() &&
             (gRitualAudit.flags & DAYTIME_RITUAL_AUDIT_WINDOW_SEEN)) {
    gRitualAudit.flags |= DAYTIME_RITUAL_AUDIT_WINDOW_COMPLETE;
  }
  return before != gRitualAudit.flags;
}

struct PresenceWaveState {
  uint32_t eventId;
  uint8_t hue;
  uint8_t pointValue;
  uint8_t depth;
  uint8_t hopsRemaining;
  bool activated;
  uint8_t fanoutSent;
  uint32_t nextFanoutMs;
  uint8_t visited[RES_WAVE_VISITED_MAX][3];
  uint8_t visitedCount;
};

static PresenceWaveState gWave;
static TmfPresenceGate gPresence;
static Vl53CoverGate gVl53Cover;
static bool gWaveDisplayActive = false;
static uint8_t gWaveDisplayHue = 0;
static uint8_t gWaveDisplayValue = 96;
static uint32_t gLastPresenceOriginMs = 0;
static uint32_t gLastWaveActivityMs = 0;
static bool gPresencePending = false;
static uint32_t gPresenceFireAtMs = 0;
static bool gTofPresenceActive = false;
static bool gTofPresenceRising = false;
static uint32_t gRetiredWaveIds[4] = {};
static uint8_t gRetiredWavePos = 0;

static uint8_t configuredAutonomousProgram() {
  if (gCfg.profile == PROFILE_PROD) return PROG_GH_CA;
  return gCfg.commissionDefault == COMMISSION_DEFAULT_CA
             ? PROG_GH_CA
             : PROG_COMMISSION_DARK;
}

static bool commissionListenerFallback() {
  return gCfg.profile == PROFILE_DEV &&
         gCfg.commissionDefault == COMMISSION_DEFAULT_LISTENER;
}

static bool commissionDarkFallback() {
  return gCfg.profile == PROFILE_DEV &&
         gCfg.commissionDefault == COMMISSION_DEFAULT_DARK;
}

static void applyLocalInteraction(FrameBuffer &frame) {
  const SensorSnapshot &snapshot = sensors();
  LocalInteractionInputs inputs = {};
  inputs.fixtureClass = gClass;
  inputs.tofPresenceActive =
      gClass == FIXTURE_DOWNLIGHT && gTofPresenceActive;
  if (gClass == FIXTURE_DOWNLIGHT && snapshot.tmfPresent && snapshot.tmfOk &&
      snapshot.tofDepthMm) {
    inputs.tofValid = true;
    float filtered = snapshot.tofDepthFilteredMm;
    inputs.tofDistanceMm =
        filtered >= 1.0f && filtered <= 65535.0f
            ? (uint16_t)(filtered + 0.5f)
            : snapshot.tofDepthMm;
  } else if (gClass == FIXTURE_PERIMETER && snapshot.vlPresent &&
             snapshot.vlOk && snapshot.vlClosestMm) {
    inputs.tofValid = true;
    inputs.tofDistanceMm = snapshot.vlClosestMm;
  }
  inputs.msaValid = snapshot.msaPresent && snapshot.msaOk;
  if (snapshot.swayEnvG > 0.0f) {
    float swayMg = snapshot.swayEnvG * 1000.0f;
    inputs.msaSwayMg =
        swayMg >= 65535.0f ? 65535u : (uint16_t)(swayMg + 0.5f);
  }
  inputs.msaInteractionEnabled = RES_MSA_LOCAL_INTERACTION != 0;
  interactionApply(frame, inputs);
}

static void waveClear() {
  if (gWave.eventId)
    gRetiredWaveIds[gRetiredWavePos++ & 0x03] = gWave.eventId;
  memset(&gWave, 0, sizeof(gWave));
  gWaveDisplayActive = false;
  gPresencePending = false;
  gWaveDisplayHue = 0;
  gWaveDisplayValue = 96;
}

static bool waveIsRetired(uint32_t eventId) {
  for (uint8_t i = 0; i < 4; ++i)
    if (gRetiredWaveIds[i] == eventId) return true;
  return false;
}

static void waveRememberVisited(const uint8_t id[3]) {
  if (waveIdSeen(gWave.visited, gWave.visitedCount, id)) return;
  if (gWave.visitedCount < RES_WAVE_VISITED_MAX)
    memcpy(gWave.visited[gWave.visitedCount++], id, 3);
}

static void waveBegin(uint32_t eventId, uint8_t hue, uint8_t value) {
  if (gWave.eventId && gWave.eventId != eventId) {
    gRetiredWaveIds[gRetiredWavePos++ & 0x03] = gWave.eventId;
  }
  memset(&gWave, 0, sizeof(gWave));
  gWave.eventId = eventId;
  gWave.hue = hue;
  gWave.pointValue = value ? value : 96;
}

static void waveActivate(uint8_t depth, uint8_t hopsRemaining) {
  if (gWave.activated) return;
  gWave.activated = true;
  gWave.depth = depth;
  gWave.hopsRemaining = hopsRemaining;
  gWave.fanoutSent = 0;
  gWave.nextFanoutMs = millis() + 140 + (esp_random() % 240);
  gWaveDisplayActive = true;
  gWaveDisplayHue = gWave.hue;
  gWaveDisplayValue = gWave.pointValue;
  Serial.printf("presence-wave: event=%08lx hue=%u depth=%u visited=%u\n",
                (unsigned long)gWave.eventId, gWave.hue, depth,
                gWave.visitedCount);
}

static void waveSendTarget(const uint8_t target[3], uint8_t depth,
                           uint8_t hopsRemaining) {
  NbEvent event;
  memset(&event, 0, sizeof(event));
  fillHeader(&event.h, NB_EVENT);
  event.event_id = gWave.eventId;
  event.kind = NB_EVENT_PRESENCE_WAVE;
  event.hop_limit = hopsRemaining;
  memcpy(&event.params[NB_EVENT_TARGET_OFFSET], target, 3);
  event.params[NB_EVENT_HUE_OFFSET] = gWave.hue;
  event.params[NB_EVENT_VALUE_OFFSET] = gWave.pointValue;
  event.params[NB_EVENT_DEPTH_OFFSET] = depth;
  espNowSendRaw(&event, sizeof(event));
}

static void waveOrigin() {
  uint32_t eventId = esp_random();
  if (!eventId) eventId = 1;
  uint8_t hue = (uint8_t)esp_random();
  uint8_t delta = hue > gWaveDisplayHue ? hue - gWaveDisplayHue
                                        : gWaveDisplayHue - hue;
  if (gWaveDisplayActive && (delta < 32 || delta > 224)) hue += 85;
  waveBegin(eventId, hue, 96);
  waveRememberVisited(gMyId);
  waveActivate(0, RES_WAVE_HOPS);
  // Announce the root as an already-visited target. Our own broadcast echo is
  // ignored locally, while every peer initializes the same event ledger.
  waveSendTarget(gMyId, 0, RES_WAVE_HOPS);
  gLastWaveActivityMs = millis();
}

void behaviorOnEvent(const NbEvent &event) {
  if (event.kind != NB_EVENT_PRESENCE_WAVE || !event.event_id) return;
  // Bridge/direct authority suppresses autonomous presence propagation. A
  // blackout must not collect a hidden wave that reappears when its lease ends.
  if (gRuntime.leaseActive()) return;
  gLastWaveActivityMs = millis();
  gPresencePending = false; // earliest nearby origin wins the random backoff
  const uint8_t *target = &event.params[NB_EVENT_TARGET_OFFSET];
  if (event.event_id != gWave.eventId) {
    if (waveIsRetired(event.event_id)) return;
    waveBegin(event.event_id, event.params[NB_EVENT_HUE_OFFSET],
              event.params[NB_EVENT_VALUE_OFFSET]);
  }
  waveRememberVisited(target);
  if (memcmp(target, gMyId, 3) == 0)
    waveActivate(event.params[NB_EVENT_DEPTH_OFFSET], event.hop_limit);
}

void behaviorOnTimeQuality(const NbTimeQuality &time, const uint8_t srcId[3]) {
  timeConsensusObserve(gTimeConsensus, time, srcId, millis());
}

static void waveTick(uint32_t now) {
  // Consume each sensor report exactly once even while another program owns
  // the renderer. Downlights use the learned TMF approach edge; reachable
  // perimeter fixtures use a deliberate broad VL53L5CX palm-cover edge.
  gTofPresenceRising = false;
  const SensorSnapshot &snapshot = sensors();
#if defined(RES_CANOPY_PRESENCE_DISTANT_RANGE)
  if (gClass == FIXTURE_DOWNLIGHT && snapshot.tmfPresent && snapshot.tmfOk) {
    bool active = tmfDistantRangePresent(snapshot.tofZoneMm,
                                         snapshot.tofZoneConfidence);
    gTofPresenceRising = active && !gTofPresenceActive;
    gTofPresenceActive = active;
  } else {
    gTofPresenceActive = false;
  }
#else
  if (gClass == FIXTURE_DOWNLIGHT && snapshot.tmfPresent && snapshot.tmfOk)
    gTofPresenceRising =
        tmfPresenceObserve(gPresence, snapshot.tmfReads, snapshot.tofZoneMm,
                           snapshot.tofZoneConfidence);
  else if (gClass == FIXTURE_PERIMETER && snapshot.vlPresent && snapshot.vlOk)
    gTofPresenceRising =
        vl53CoverObserve(gVl53Cover, snapshot.vlReads, snapshot.vlNearZones);
  gTofPresenceActive =
      gClass == FIXTURE_DOWNLIGHT && snapshot.tmfPresent && snapshot.tmfOk &&
      gPresence.latched;
#endif

#if defined(RES_CANOPY_PRESENCE_DISTANT_RANGE)
  // This exact-target false-positive diagnostic must remain local. The raw
  // predicate can chatter on bamboo/self-geometry, so never originate or relay
  // a presence wave from this image.
  gPresencePending = false;
  return;
#endif

  if (gRuntime.leaseActive()) {
    gPresencePending = false;
    return;
  }
  // The legacy listener color wipe remains a canopy/downlight interaction.
  if (gClass != FIXTURE_DOWNLIGHT) {
    gPresencePending = false;
    return;
  }
  // The ready-beacon fallback owns the local ToF presence interaction. A
  // selected commission CA fallback is a different autonomous program, so do
  // not create hidden presence-wave traffic underneath it.
  if (gCfg.profile == PROFILE_DEV && !commissionListenerFallback()) {
    gPresencePending = false;
    return;
  }
  if (gTofPresenceRising &&
      now - gLastPresenceOriginMs >= RES_WAVE_ORIGIN_COOLDOWN_MS &&
      powerBudget().brightness_cap > 0) {
    // A person can be visible to adjacent canopies. Randomize the origin by a
    // few hundred ms; the first event heard cancels every other pending origin
    // so one physical approach yields one hue instead of a boot-time palette.
    gPresencePending = true;
    gPresenceFireAtMs = now + 120 + (esp_random() % 500);
  }
  if (gPresencePending && (int32_t)(now - gPresenceFireAtMs) >= 0) {
    gPresencePending = false;
    if (!gLastWaveActivityMs || now - gLastWaveActivityMs >= 1000) {
      gLastPresenceOriginMs = now;
      waveOrigin();
    }
  }

  if (!gWave.activated || gWave.hopsRemaining == 0 ||
      gWave.fanoutSent >= 2 || (int32_t)(now - gWave.nextFanoutMs) < 0)
    return;

  NeighborView views[NB_NEIGHBOR_REPORT_MAX];
  uint8_t count = neighborSnapshot(gNeighbors, now, RES_NEIGHBOR_FRESH_MS,
                                   views, NB_NEIGHBOR_REPORT_MAX);
  const NeighborView *chosen = nullptr;
  for (uint8_t i = 0; i < count; ++i) {
    if (!(views[i].flags & RES_WAVE_CAPABLE_FLAG)) continue;
    if (memcmp(views[i].id, gMyId, 3) == 0) continue;
    if (waveIdSeen(gWave.visited, gWave.visitedCount, views[i].id)) continue;
    chosen = &views[i];
    break;
  }
  if (!chosen) {
    gWave.fanoutSent = 2;
    return;
  }

  waveRememberVisited(chosen->id);
  waveSendTarget(chosen->id, (uint8_t)(gWave.depth + 1),
                 (uint8_t)(gWave.hopsRemaining - 1));
  ++gWave.fanoutSent;
  gWave.nextFanoutMs = now + 80 + (esp_random() % 100);
}

void behaviorInit(uint8_t fixtureClass, uint16_t pixelCount, uint32_t seed) {
  gClass = fixtureClass;
  gPixels = pixelCount;
  gRuntime.init(fixtureClass, pixelCount, seed, configuredAutonomousProgram());
  neighborTableInit(gNeighbors);
  lifeInit(gLife);
  if (gRitualRtcMagic != kRitualRtcMagic) {
    daytimeRitualInit(gRitualState);
    memset(&gRitualAudit, 0, sizeof(gRitualAudit));
    gRitualRtcMagic = kRitualRtcMagic;
  }
  bool carriedEnergy = esp_reset_reason() == ESP_RST_DEEPSLEEP &&
                       gDayEnergyCarry == kDayEnergyCarryMagic;
  // The carry is one-hop only. A fresh measurement must explicitly re-arm it
  // before the next timer sleep, so vanished solar cannot remain permission.
  gDayEnergyCarry = 0;
  if (carriedEnergy) {
    gLife.state = LIFE_DAY_ACTIVE;
    Serial.println("lifecycle: restored one-wake daytime energy readiness");
  }
  gLifeCfg = lifeConfigDefaults(gCfg.profile == PROFILE_DEV);
  gLifeCfg.daySleepS = RES_DAY_SLEEP_S;
  gLifeCfg.nightMaxMin = gCfg.nightMaxMin;
  gLastProfile = gCfg.profile;
  gLastCommissionDefault = gCfg.commissionDefault;
  gAwakeGraceUntilMs = millis() + (esp_reset_reason() == ESP_RST_DEEPSLEEP
                                       ? RES_WAKE_LISTEN_MS
                                       : RES_BOOT_AWAKE_MS);
  gSolarProbeActive = false;
  gTofPresenceActive = false;
  gRitualKeepAwake = false;
  frameClear(gFrame);
  gFrame.count = (uint8_t)pixelCount;
  tmfPresenceInit(gPresence);
  vl53CoverInit(gVl53Cover);
  memset(&gWave, 0, sizeof(gWave));
  timeConsensusInit(gTimeConsensus);
}

void behaviorForceNight(int8_t force) { gForceNight = force; }
int8_t behaviorForcedNight() { return gForceNight; }
uint8_t behaviorLifeState() { return gLife.state; }
bool behaviorStrikesAllowed() { return gStrikesAllowed; }
uint16_t behaviorDaySleepS() { return gLifeCfg.daySleepS; }
uint32_t behaviorWakeListenMs() { return RES_WAKE_LISTEN_MS; }
bool behaviorTofPresenceActive() { return gTofPresenceActive; }
bool behaviorTofPresenceRising() { return gTofPresenceRising; }

bool behaviorStrikePermitted() {
  if (gStrikesAllowed) return true;
#if defined(RES_SOLENOID_TEST_OVERRIDE)
  // Targeted bring-up images may exercise a solarnoid indoors or from a
  // depleted cap bank with no useful panel current. Keep the production night
  // veto and the FULL-tier battery veto; relax only the solar-surplus gate.
  return gLife.state != LIFE_NIGHT_SHOW && powerBudget().tier == LedTier::FULL;
#else
  return gCfg.profile == PROFILE_DEV && gLife.state != LIFE_NIGHT_SHOW &&
         powerBudget().tier == LedTier::FULL;
#endif
}

static void handleProgramStrike(const ProgramOutputs &out) {
  if (!out.strikeRequested) return;
  if (!strikePolicyMayAttempt(StrikeOrigin::AUTONOMOUS_PROGRAM,
                              behaviorStrikePermitted())) {
    Serial.println("solenoid: choreography knock refused (lifecycle/power gate)");
    return;
  }
  uint16_t pulseMs = out.strikePulseMs ? out.strikePulseMs : 40;
  if (!solenoidStrike(pulseMs, "choreography"))
    Serial.println("solenoid: choreography knock blocked (arm/rest/mechanism gate)");
}

void behaviorOnChoreoState(const uint8_t srcId[3], int8_t rssi, const NbChoreoState &cs) {
  NeighborEntry *e = neighborUpsert(gNeighbors, srcId, millis(), rssi);
  if (!e) return;
  e->choreoState = cs.state;
  e->programId = cs.program_id;
  e->generation = cs.generation;
  e->flags = cs.flags;
}

void behaviorOnPeerHeartbeat(const uint8_t srcId[3], int8_t rssi, uint8_t caState,
                             uint8_t) {
  // Old net_bench peers on a mixed bench still participate: their heartbeat
  // ca_state byte rides the same slot our GH state mirrors into.
  NeighborEntry *e = neighborUpsert(gNeighbors, srcId, millis(), rssi);
  if (!e) return;
  e->choreoState = caState;
}

void behaviorOnProgramSet(const NbProgramSet &ps) {
  if (!nbTargetMatches(ps.target_id, gMyId)) return;
  bool ok = gRuntime.applyProgramSet(ps.program_id, ps.lease_s, ps.seed, ps.flags,
                                     ps.params, millis());
  if (ok) transportWakeDarkRelease();
  if (ok && ps.lease_s) waveClear();
  Serial.printf("program-set: prog=%u lease=%us -> %s\n", ps.program_id,
                ps.lease_s, ok ? "applied" : "REJECTED (unknown program)");
}

void behaviorOnNeighborSet(const NbNeighborSet &ns) {
  if (!nbTargetMatches(ns.target_id, gMyId)) return;
  if ((ns.flags & 0x02) || ns.count == 0) {
    neighborClearPinned(gNeighbors);
    Serial.println("neighbor-set: cleared (RSSI mode)");
    return;
  }
  uint8_t count = ns.count > NB_NEIGHBOR_SET_MAX ? NB_NEIGHBOR_SET_MAX : ns.count;
  neighborSetPinned(gNeighbors, ns.neighbor_ids, count);
  Serial.printf("neighbor-set: pinned %u neighbors\n", count);
  // NVS persistence of the pinned map (flags bit0) is an M2 item; the 2x10
  // rig re-pushes after reboots via the host script for now.
}

void behaviorOnDirectFrame(uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                           uint8_t flags) {
  // Micro-lease grant + staleness bookkeeping live in the runtime.
  uint32_t now = millis();
  DirectFrameState fs = {now, r, g, b, w, flags};
  gRuntime.noteDirectFrame(fs, now);
  transportWakeDarkRelease();
  if (flags & 0x01) waveClear();
}

uint8_t behaviorNeighborSnapshot(NeighborView *out, uint8_t maxOut) {
  // A 15 s window covers the production 0.2 Hz heartbeat cadence with margin;
  // unlike CA behavior, the survey must not be constrained by a pinned map.
  return neighborSurveySnapshot(gNeighbors, millis(), 15000UL, out, maxOut);
}


#ifdef RES_BASIC_LISTENER
// Basic supervised posture. Canopy/downlight fixtures use their efficient,
// pleasant dedicated warm-white die. The 37-pixel perimeter wash is only
// 16/255 linear red to avoid multiplying the listener load by every HEX die;
// single-pixel RGB classes retain half-scale red. Tags/leases override it.
static void quietIdleFrame(FrameBuffer &f, uint16_t pixels, uint32_t) {
  f.count = (uint8_t)pixels;
  frameClear(f);
  if (gWaveDisplayActive) {
    uint8_t value = gClass == FIXTURE_PERIMETER
                        ? (gWaveDisplayValue > 14 ? 14 : gWaveDisplayValue)
                        : gWaveDisplayValue;
    uint8_t r, g, b;
    waveHueToRgb(gWaveDisplayHue, value, r, g, b);
    for (uint16_t i = 0; i < f.count; ++i) {
      f.px[i][0] = r;
      f.px[i][1] = g;
      f.px[i][2] = b;
    }
    return;
  }
  if (gClass == FIXTURE_DOWNLIGHT) {
    f.px[0][3] = 128;
  } else {
    uint8_t red = gClass == FIXTURE_PERIMETER ? 16 : 128;
    for (uint16_t i = 0; i < f.count; i++) f.px[i][0] = red;
  }
}
#endif

void behaviorTick() {
  uint32_t now = millis();

  waveTick(now);

  // Profile/default flips re-derive the lifecycle and fallback live. An active
  // lease remains authoritative; the selected fallback takes over on release.
  bool profileChanged = gCfg.profile != gLastProfile;
  if (profileChanged || gCfg.commissionDefault != gLastCommissionDefault) {
    gLastProfile = gCfg.profile;
    gLastCommissionDefault = gCfg.commissionDefault;
    gLifeCfg = lifeConfigDefaults(gCfg.profile == PROFILE_DEV);
    gLifeCfg.nightMaxMin = gCfg.nightMaxMin;
    // COMMISSION is a real lifecycle state. Reinitialize on a live profile
    // change so commission -> field cannot remain stranded in that state until
    // reboot; the new field posture starts conservatively in DAY_CHARGE.
    if (profileChanged) {
      lifeInit(gLife);
      gSolarProbeActive = false;
    }
    gRuntime.setAutonomousProgram(configuredAutonomousProgram(), now, true);
    if (!commissionListenerFallback()) waveClear();
  }

  const PowerBudget &pb = powerBudget();
  LifeInputs li = {};
  li.nowMs = now;
  li.supplyGood = supplyGood();
  li.supplyMa = supplyMa();
  li.battV = batteryVolts();
  li.tier = (uint8_t)pb.tier;
  li.lastRxMs = espNowLastControlRxMs();
  li.awakeGraceUntilMs = gAwakeGraceUntilMs;
  li.rxHoldMs = RES_RX_HOLD_MS;
  TimeEstimate wall = timeConsensusEstimate(gTimeConsensus, now);
  if (wall.valid) {
    bool wasValid = gScheduleValid;
    bool wasNight = gScheduleNight;
    ShowScheduleResult scheduled = showScheduleAt(wall.utcS);
    gScheduleValid = true;
    gScheduleNight = scheduled.night;
    if (!wasValid || wasNight != gScheduleNight)
      Serial.printf("schedule: utc=%lu solar=%.1f -> %s (src=%u votes=%u)\n",
                    (unsigned long)wall.utcS, scheduled.solarElevationDeg,
                    gScheduleNight ? "night" : "day", wall.source, wall.votes);
  } else {
    gScheduleValid = false;
  }
  // Explicit bridge/serial override wins. With AUTO selected, trustworthy UTC
  // supersedes the panel-current dusk heuristic through the existing seam.
  li.forceNight = gForceNight >= 0 ? gForceNight
                                   : (gScheduleValid ? (gScheduleNight ? 1 : 0) : -1);
  LifeOutputs lo = lifeTick(gLife, li, gLifeCfg);

  if (lo.solarProbeActive != gSolarProbeActive) {
    if (lo.solarProbeActive) {
      Serial.printf("lifecycle: solar probe start (supply=%.0fmA)\n", li.supplyMa);
    } else if (lo.state == LIFE_DAY_ACTIVE) {
      Serial.printf("lifecycle: solar probe confirmed (supply=%.0fmA)\n",
                    li.supplyMa);
    } else {
      Serial.printf("lifecycle: solar probe ended (supply=%d/%.0fmA)\n",
                    li.supplyGood ? 1 : 0, li.supplyMa);
    }
    gSolarProbeActive = lo.solarProbeActive;
  }

  if (lo.stateChanged)
    Serial.printf("lifecycle: -> %u (supply=%d/%.0fmA bv=%.3f)\n", lo.state,
                  li.supplyGood ? 1 : 0, li.supplyMa, li.battV);

  gShowActive = lo.showActive;
  gStrikesAllowed = lo.strikesAllowed && pb.may_strike;
  gNetLifeState = lo.state;
  gNetNightMin = lo.nightMin;
  gTelemetryLifeState = lo.state;

  // Feed the runtime a fresh ShowFrame (micro-lease path) before evaluating
  // autonomy. A frame received on this loop owns the ritual veto immediately.
  const ShowFrameIn &sf = netPeerLastShowFrame();
  if (sf.rx_ms && sf.rx_ms != gLastShowFrameRxMs) {
    gLastShowFrameRxMs = sf.rx_ms;
    ShowFrameState fs = {sf.rx_ms, sf.phase, sf.hue, sf.flags, sf.val,
                         sf.bright, sf.effect, sf.beat_phase, sf.energy};
    gRuntime.noteShowFrame(fs, now);
  }

  DaytimeRitualInputs ritualIn = {};
  bool ritualCanary = behaviorDaytimeRitualCanaryBuild();
  bool ritualTargetOk = !ritualCanary ||
                        behaviorDaytimeRitualCanaryTargetMatches();
  ritualIn.enabled = gCfg.profile == PROFILE_PROD &&
                     gClass == FIXTURE_DOWNLIGHT && ritualTargetOk;
  ritualIn.scheduledDay = gScheduleValid && !gScheduleNight;
  ritualIn.energyReady = gStrikesAllowed;
  ritualIn.authorityFree = !gRuntime.leaseActive();
  ritualIn.utcValid = wall.valid;
  ritualIn.utcS = wall.utcS;
  ritualIn.subMs = wall.subMs;
  ritualIn.uncertaintyMs = wall.uncertaintyMs;
  memcpy(ritualIn.fixtureId, gMyId, sizeof(ritualIn.fixtureId));
  ritualIn.allowedHourKey = ritualCanary
                                ? behaviorDaytimeRitualCanaryHourKey()
                                : 0;
  bool ritualAuditChanged = observeCanaryWindow(wall);
  DaytimeRitualOutputs ritual =
      daytimeRitualTick(gRitualState, ritualIn);
  gRitualKeepAwake = ritual.keepAwake;
  if (ritual.strikeRequested) {
    const char *eventName = daytimeRitualEventName(ritual.event);
    beginRitualAudit(ritual.hourKey, wall.uncertaintyMs);
    uint8_t eventMask = daytimeRitualEventMask(ritual.event);
    gRitualAudit.attemptedMask |= eventMask;
    Serial.printf("daytime-ritual: hour=%lu event=%s attempt\n",
                  (unsigned long)ritual.hourKey, eventName);
    if (!strikePolicyMayAttempt(StrikeOrigin::AUTONOMOUS_PROGRAM,
                                behaviorStrikePermitted())) {
      gRitualAudit.policyRefusedMask |= eventMask;
      Serial.printf("daytime-ritual: %s refused (energy gate)\n", eventName);
    } else if (!solenoidStrike(RES_SOLENOID_DEFAULT_MS, eventName)) {
      gRitualAudit.mechanismBlockedMask |= eventMask;
      Serial.printf("daytime-ritual: %s blocked (mechanism gate)\n", eventName);
    } else {
      gRitualAudit.firedMask |= eventMask;
    }
    // A full heartbeat immediately after every bounded act makes the canary
    // visible without opening the enclosure or waiting for the 60 s cadence.
    netPeerSendHeartbeat(true);
  } else if (ritualAuditChanged) {
    netPeerSendHeartbeat(true);
  }

  if (gShowActive) {
    NeighborView views[NEIGHBOR_PINNED_MAX];
    uint8_t nviews = neighborSnapshot(gNeighbors, now, RES_NEIGHBOR_FRESH_MS,
                                      views, NEIGHBOR_PINNED_MAX);
    ShowFrameState fs = {sf.rx_ms, sf.phase, sf.hue, sf.flags, sf.val,
                         sf.bright, sf.effect, sf.beat_phase, sf.energy};
    ProgramInputs pin = {};
    pin.nowMs = now;
    pin.dtMs = 0;
    pin.fixtureClass = gClass;
    pin.pixelCount = gPixels;
    pin.neighbors = views;
    pin.neighborCount = nviews;
    pin.showFrame = &fs;
    pin.tier = (uint8_t)pb.tier;
    pin.tickDivider = pb.tick_divider;
    pin.synchronizedPalette = !gRuntime.leaseActive();
    pin.utcValid = wall.valid;
    pin.utcS = wall.utcS;
    pin.tofPresenceRising = gTofPresenceRising;
    ProgramOutputs pout = {};
    gRuntime.tick(pin, pout);
    gFrame = pout.frame;
    gProgramSuppressesLight = pout.suppressLight;
    handleProgramStrike(pout);
#ifdef RES_BASIC_LISTENER
    // Slave/bench posture (Elliot 2026-08-15): no autonomous show — with no
    // explicit lease, render a LOW-RED idle beacon ("power efficient and
    // shows that it is ready for command") instead of the default programs.
    // Radio, sensors, telemetry, and every commanded path (DIRECT stream,
    // bridge show, identify) stay fully live.
    if (commissionListenerFallback() && !gRuntime.leaseActive())
      quietIdleFrame(gFrame, gPixels, now);
#endif
    if (!gProgramSuppressesLight) applyLocalInteraction(gFrame);
    gNetCaState = pout.txState;
    gNetProgram = gRuntime.activeProgram();
    gTelemetryProgram = gNetProgram;

    // Choreo tx: 1 Hz keepalive + edge-triggered bursts. Production remains
    // power-vetoed; the supervised basic-listener image also advertises its
    // wave capability in commission mode so the demo graph can form.
    bool allowChoreoTx = gCfg.profile == PROFILE_PROD && pb.may_tx_show;
#ifdef RES_BASIC_LISTENER
    allowChoreoTx = allowChoreoTx || gCfg.profile == PROFILE_DEV;
#endif
    if (allowChoreoTx &&
        (pout.sendNow || (int32_t)(now - gNextChoreoTxMs) >= 0)) {
      NbChoreoState cs;
      memset(&cs, 0, sizeof(cs));
      fillHeader(&cs.h, NB_CHOREO_STATE);
      cs.program_id = gRuntime.activeProgram();
      cs.generation = pout.generation;
      cs.state = pout.txState;
      cs.intensity = pout.txIntensity;
      cs.phase_ms = pout.phaseMs;
      cs.flags = (uint8_t)((pb.brightness_cap == 0 ? 0x01 : 0) |
                           (gRuntime.leaseActive() ? 0x02 : 0) |
                           (gCfg.profile == PROFILE_DEV ? 0x04 : 0) |
                           RES_WAVE_CAPABLE_FLAG);
      espNowSendRaw(&cs, sizeof(cs));
      uint32_t jit = esp_random() % 600;
      gNextChoreoTxMs = now + RES_CHOREO_KEEPALIVE_MS - 300 + jit; // +/-30%
    }
  } else {
#ifdef RES_BASIC_LISTENER
    // Full-control posture: the program engine runs in DAY/BOOT states too,
    // so commanded frames (DIRECT stream / programs) render around the clock
    // and telemetry reports the true active program. First beacon build only
    // rendered at night AND painted the beacon over live commands (measured:
    // T2 green-on-command FAIL, prog stuck 0, 2026-08-15 evening).
    NeighborView views[NEIGHBOR_PINNED_MAX];
    uint8_t nviews = neighborSnapshot(gNeighbors, now, RES_NEIGHBOR_FRESH_MS,
                                      views, NEIGHBOR_PINNED_MAX);
    ShowFrameState fs = {sf.rx_ms, sf.phase, sf.hue, sf.flags, sf.val,
                         sf.bright, sf.effect, sf.beat_phase, sf.energy};
    ProgramInputs pin = {};
    pin.nowMs = now;
    pin.dtMs = 0;
    pin.fixtureClass = gClass;
    pin.pixelCount = gPixels;
    pin.neighbors = views;
    pin.neighborCount = nviews;
    pin.showFrame = &fs;
    pin.tier = (uint8_t)pb.tier;
    pin.tickDivider = pb.tick_divider;
    pin.synchronizedPalette = !gRuntime.leaseActive();
    pin.utcValid = wall.valid;
    pin.utcS = wall.utcS;
    pin.tofPresenceRising = gTofPresenceRising;
    ProgramOutputs pout = {};
    gRuntime.tick(pin, pout);
    gFrame = pout.frame;
    gProgramSuppressesLight = pout.suppressLight;
    handleProgramStrike(pout);
    if (commissionListenerFallback() && !gRuntime.leaseActive())
      quietIdleFrame(gFrame, gPixels, now);
    if (!gProgramSuppressesLight) applyLocalInteraction(gFrame);
    gNetCaState = pout.txState;
    gNetProgram = gRuntime.activeProgram();
    gTelemetryProgram = gNetProgram;
    // 1 Hz choreo-state keepalive in day states too: the operator's "always
    // know their state" contract — without this, program truth reaches the
    // daemon only on sparse full heartbeats (~60 s lag, measured).
    // state tx deliberately NOT power-vetoed here: on the bench a low cell
    // must still report truthfully (Luigi at 21%% went state-silent, measured)
    if (pout.sendNow || (int32_t)(now - gNextChoreoTxMs) >= 0) {
      NbChoreoState cs;
      memset(&cs, 0, sizeof(cs));
      fillHeader(&cs.h, NB_CHOREO_STATE);
      cs.program_id = gRuntime.activeProgram();
      cs.generation = pout.generation;
      cs.state = pout.txState;
      cs.intensity = pout.txIntensity;
      cs.phase_ms = pout.phaseMs;
      cs.flags = (uint8_t)((pb.brightness_cap == 0 ? 0x01 : 0) |
                           (gRuntime.leaseActive() ? 0x02 : 0) |
                           (gCfg.profile == PROFILE_DEV ? 0x04 : 0) |
                           RES_WAVE_CAPABLE_FLAG);
      espNowSendRaw(&cs, sizeof(cs));
      uint32_t jit = esp_random() % 600;
      gNextChoreoTxMs = now + RES_CHOREO_KEEPALIVE_MS - 300 + jit;
    }
#else
    gNetCaState = 0;
    gNetProgram = 0;
    gTelemetryProgram = 0;
#endif
  }

  // Prod daytime duty cycle. Blocked by pending OTA verify and maintenance
  // handled elsewhere; the wake listen window re-arms via behaviorInit's grace.
  // An operator-selected program lease promises that the receiver remains
  // reachable for its full duration. In particular, a long DARK/blackout lease
  // must not hit the generic 10-minute control hold, deep-sleep, and lose its
  // RAM lease on reboot. Once the lease expires, normal day sleep resumes.
  if (lo.wantSleep && !gRitualKeepAwake && powerWakeSampleWindowComplete() &&
      !gRuntime.leaseActive() && !otaVerifyPending()) {
    uint16_t sleepS = lo.sleepS;
    if (gCfg.profile == PROFILE_PROD && gClass == FIXTURE_DOWNLIGHT &&
        ritualTargetOk && gStrikesAllowed && gScheduleValid &&
        !gScheduleNight && wall.valid) {
      sleepS = daytimeRitualSleepSForHour(
          wall.utcS, wall.subMs, sleepS,
          ritualCanary ? behaviorDaytimeRitualCanaryHourKey() : 0);
    }
    // Restore DAY_ACTIVE for only the next timer wake, and only while the
    // current measurement still grants actual strike permission.
    gDayEnergyCarry =
        gLife.state == LIFE_DAY_ACTIVE && gStrikesAllowed
            ? kDayEnergyCarryMagic
            : 0;
    enterTimedDeepSleep(sleepS, SLEEP_CAUSE_DAY_CHARGE);
  }
}

bool behaviorFrame(FrameBuffer &f) {
  // Timer wake from a shipping sleep must never invent a light show inside
  // the container. Radio and telemetry are live; an explicit bridge program
  // command releases this latch after unpacking.
  if (transportWakeDarkActive()) return false;
  if (gProgramSuppressesLight) return false;
#ifdef RES_BASIC_LISTENER
  // A bridge DARK lease is electrically dark, not merely an all-zero frame.
  // Returning false lets renderTick blank data and cut the LED rail.
  if (gRuntime.darkLeaseActive()) return false;
  // Strict commission dark is a selectable no-command diagnostic posture.
  if (commissionDarkFallback() && !gRuntime.leaseActive()) return false;
  // Field posture is autonomous only at scheduled night. During scheduled day
  // it is electrically dark unless a direct/program lease deliberately
  // overrides the baseline. Commission retains ADR 0039's ready beacon.
  if (gCfg.profile == PROFILE_PROD && !gShowActive && !gRuntime.leaseActive())
    return false;
  f = gFrame;
  return true;
#else
  if (!gShowActive) return false;
  // In commissioning, loss/expiry of bridge authority means electrically
  // dark, not a locally invented pattern. Returning false lets renderTick cut
  // the LED rail instead of powering it merely to render a zero frame.
  if (gCfg.profile == PROFILE_DEV && !gRuntime.leaseActive() &&
      gCfg.commissionDefault != COMMISSION_DEFAULT_CA)
    return false;
  f = gFrame;
  return true;
#endif
}
