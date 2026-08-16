#include "behavior_glue.h"

#include <Arduino.h>

#include "../core/choreo/program.h"
#include "../core/lifecycle.h"
#include "../core/neighbor_table.h"
#include "board_power.h"
#include "espnow_link.h"
#include "identity.h"
#include "net_peer.h"
#include "nvs_store.h"
#include "ota_verify.h"
#include "power_glue.h"
#include "telemetry.h"

#define RES_RX_HOLD_MS 600000UL      // heard-anything hold (10 min)
#define RES_BOOT_AWAKE_MS 600000UL   // cold-boot listen window (10 min)
#define RES_WAKE_LISTEN_MS 15000UL   // deep-sleep wake listen window
#define RES_CHOREO_KEEPALIVE_MS 1000
#define RES_NEIGHBOR_FRESH_MS 3000

static ChoreoRuntime gRuntime;
static NeighborTable gNeighbors;
static LifeState_t gLife;
static LifeConfig gLifeCfg;
static uint8_t gClass = FIXTURE_UNKNOWN;
static uint16_t gPixels = 1;
static int8_t gForceNight = -1;
static bool gShowActive = false;
static bool gStrikesAllowed = false;
static FrameBuffer gFrame;
static uint32_t gAwakeGraceUntilMs = 0;
static uint32_t gNextChoreoTxMs = 0;
static uint32_t gLastShowFrameRxMs = 0;
static uint8_t gLastProfile = 0xFF;

void behaviorInit(uint8_t fixtureClass, uint16_t pixelCount, uint32_t seed) {
  gClass = fixtureClass;
  gPixels = pixelCount;
  gRuntime.init(fixtureClass, pixelCount, seed,
                gCfg.profile == PROFILE_DEV ? PROG_COMMISSION_DARK : PROG_GH_CA);
  neighborTableInit(gNeighbors);
  lifeInit(gLife);
  gLifeCfg = lifeConfigDefaults(gCfg.profile == PROFILE_DEV);
  gLifeCfg.nightMaxMin = gCfg.nightMaxMin;
  gLastProfile = gCfg.profile;
  gAwakeGraceUntilMs = millis() + (esp_reset_reason() == ESP_RST_DEEPSLEEP
                                       ? RES_WAKE_LISTEN_MS
                                       : RES_BOOT_AWAKE_MS);
  frameClear(gFrame);
  gFrame.count = (uint8_t)pixelCount;
}

void behaviorForceNight(int8_t force) { gForceNight = force; }
int8_t behaviorForcedNight() { return gForceNight; }
uint8_t behaviorLifeState() { return gLife.state; }
bool behaviorStrikesAllowed() { return gStrikesAllowed; }

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

void behaviorOnChoreoState(const uint8_t srcId[3], int8_t rssi, const NbChoreoState &cs) {
  NeighborEntry *e = neighborUpsert(gNeighbors, srcId, millis(), rssi);
  if (!e) return;
  e->choreoState = cs.state;
  e->programId = cs.program_id;
  e->generation = cs.generation;
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
}


#ifdef RES_BASIC_LISTENER
// Basic supervised posture. A steady half-scale red means the fixture is
// awake and listening. Bridge leases override it; expiry returns directly to
// it. There are no boot, supply, identity, or sensor-created animations.
static void quietIdleFrame(FrameBuffer &f, uint16_t pixels, uint32_t) {
  f.count = (uint8_t)pixels;
  frameClear(f);
  for (uint16_t i = 0; i < f.count; i++) f.px[i][0] = 128;
}
#endif

void behaviorTick() {
  uint32_t now = millis();

  // Profile flips (NB_PROFILE / serial F) re-derive the lifecycle config live.
  if (gCfg.profile != gLastProfile) {
    gLastProfile = gCfg.profile;
    gLifeCfg = lifeConfigDefaults(gCfg.profile == PROFILE_DEV);
    gLifeCfg.nightMaxMin = gCfg.nightMaxMin;
    gRuntime.setAutonomousProgram(
        gCfg.profile == PROFILE_DEV ? PROG_COMMISSION_DARK : PROG_GH_CA,
        now, true);
  }

  const PowerBudget &pb = powerBudget();
  LifeInputs li = {};
  li.nowMs = now;
  li.supplyGood = supplyGood();
  li.supplyMa = supplyMa();
  li.battV = batteryVolts();
  li.tier = (uint8_t)pb.tier;
  li.lastRxMs = espNowLastRxMs();
  li.awakeGraceUntilMs = gAwakeGraceUntilMs;
  li.rxHoldMs = RES_RX_HOLD_MS;
  li.forceNight = gForceNight;
  LifeOutputs lo = lifeTick(gLife, li, gLifeCfg);

  if (lo.stateChanged)
    Serial.printf("lifecycle: -> %u (supply=%d/%.0fmA bv=%.3f)\n", lo.state,
                  li.supplyGood ? 1 : 0, li.supplyMa, li.battV);

  gShowActive = lo.showActive;
  gStrikesAllowed = lo.strikesAllowed && pb.may_strike;
  gNetLifeState = lo.state;
  gNetNightMin = lo.nightMin;
  gTelemetryLifeState = lo.state;

  // Feed the runtime a fresh ShowFrame (micro-lease path).
  const ShowFrameIn &sf = netPeerLastShowFrame();
  if (sf.rx_ms && sf.rx_ms != gLastShowFrameRxMs) {
    gLastShowFrameRxMs = sf.rx_ms;
    ShowFrameState fs = {sf.rx_ms, sf.phase, sf.hue, sf.flags, sf.val,
                         sf.bright, sf.effect, sf.beat_phase, sf.energy};
    gRuntime.noteShowFrame(fs, now);
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
    ProgramOutputs pout = {};
    gRuntime.tick(pin, pout);
    gFrame = pout.frame;
#ifdef RES_BASIC_LISTENER
    // Slave/bench posture (Elliot 2026-08-15): no autonomous show — with no
    // explicit lease, render a LOW-RED idle beacon ("power efficient and
    // shows that it is ready for command") instead of the default programs.
    // Radio, sensors, telemetry, and every commanded path (DIRECT stream,
    // bridge show, identify) stay fully live.
    if (!gRuntime.leaseActive()) quietIdleFrame(gFrame, gPixels, now);
#endif
    gNetCaState = pout.txState;
    gNetProgram = gRuntime.activeProgram();
    gTelemetryProgram = gNetProgram;

    // Choreo tx: 1 Hz keepalive + edge-triggered bursts, power-vetoed.
    if (gCfg.profile == PROFILE_PROD && pb.may_tx_show &&
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
                           (gCfg.profile == PROFILE_DEV ? 0x04 : 0));
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
    ShowFrameState fs = {sf.rx_ms, sf.phase, sf.hue, sf.flags, sf.val,
                         sf.bright, sf.effect, sf.beat_phase, sf.energy};
    ProgramInputs pin = {};
    pin.nowMs = now;
    pin.dtMs = 0;
    pin.fixtureClass = gClass;
    pin.pixelCount = gPixels;
    pin.neighbors = nullptr;
    pin.neighborCount = 0;
    pin.showFrame = &fs;
    pin.tier = (uint8_t)pb.tier;
    pin.tickDivider = pb.tick_divider;
    ProgramOutputs pout = {};
    gRuntime.tick(pin, pout);
    gFrame = pout.frame;
    if (!gRuntime.leaseActive()) quietIdleFrame(gFrame, gPixels, now);
    gNetCaState = 0;
    gNetProgram = gRuntime.activeProgram();
    gTelemetryProgram = gNetProgram;
    // 1 Hz choreo-state keepalive in day states too: the operator's "always
    // know their state" contract — without this, program truth reaches the
    // daemon only on sparse full heartbeats (~60 s lag, measured).
    // state tx deliberately NOT power-vetoed here: on the bench a low cell
    // must still report truthfully (Luigi at 21%% went state-silent, measured)
    if ((int32_t)(now - gNextChoreoTxMs) >= 0) {
      NbChoreoState cs;
      memset(&cs, 0, sizeof(cs));
      fillHeader(&cs.h, NB_CHOREO_STATE);
      cs.program_id = gRuntime.activeProgram();
      cs.flags = (uint8_t)((pb.brightness_cap == 0 ? 0x01 : 0) |
                           (gRuntime.leaseActive() ? 0x02 : 0) |
                           (gCfg.profile == PROFILE_DEV ? 0x04 : 0));
      espNowSendRaw(&cs, sizeof(cs));
      gNextChoreoTxMs = now + RES_CHOREO_KEEPALIVE_MS;
    }
#else
    gNetCaState = 0;
    gNetProgram = 0;
    gTelemetryProgram = 0;
#endif
  }

  // Prod day-charge duty cycle. Blocked by pending OTA verify and maintenance
  // handled elsewhere; the wake listen window re-arms via behaviorInit's grace.
  if (lo.wantSleep && !otaVerifyPending())
    enterTimedDeepSleep(lo.sleepS, "day-charge");
}

bool behaviorFrame(FrameBuffer &f) {
#ifdef RES_BASIC_LISTENER
  // Quiet posture always has a frame: the low-red listening beacon (or a
  // leased/commanded frame). Keeps the LED rail up whenever the chip is awake.
  f = gFrame;
  return true;
#else
  if (!gShowActive) return false;
  // In commissioning, loss/expiry of bridge authority means electrically
  // dark, not a locally invented pattern. Returning false lets renderTick cut
  // the LED rail instead of powering it merely to render a zero frame.
  if (gCfg.profile == PROFILE_DEV && !gRuntime.leaseActive()) return false;
  f = gFrame;
  return true;
#endif
}
