#include "decompression.h"
#include <Arduino.h>
#include "esp_timer.h"
#include "../core/decompression_session.h"
#include "../core/decompression_replay.h"
#include "../core/fixture_context.h"
#include "../core/presence_wave.h"
#include "../core/squad_trigger.h"
#include "nvs_store.h"
#include "sensors/sensors.h"
#include "telemetry.h"
#include "espnow_link.h"
#include "identity.h"
#include "board_power.h"
#include "boot_guard_io.h"
#include "maintenance.h"
#include "ota_verify.h"
#include "power_glue.h"
#include "solenoid.h"
#include "pattern.h"

static DecompressionSession gSession;
static DecompressionSession gRadioSession;
static DecompressionReplay gReplay;
static NbDecompressionControl gRadioPlay;
static bool gRadioPlaying = false;
static uint8_t gResultCopies = 0;
static uint32_t gNextResultMs = 0;
static SignatureSequence gRadioResult;
static bool gWakePending = false;
// Atom presses seen (ADR 0083 PLAY admitted as NEW), for host tools.
static bool gRelay = false;
static uint32_t gRadioPlays = 0, gRadioLastBoot = 0, gRadioLastSeq = 0, gRadioLastMs = 0;
static uint8_t gRadioLastSrc[3] = {0, 0, 0};
static bool gWasActive = false;

static bool dingsKeepAwake();
bool decompressionActive() {
  return decompressionBuild() && (gSession.active(esp_timer_get_time()) ||
                                  gRadioSession.active(esp_timer_get_time()) ||
                                  patternActive() || dingsKeepAwake());
}

// ---- event mode -------------------------------------------------------------
static constexpr uint32_t DING_MIN_GAP_MS = 3000;      // per lantern, between presence dings
static constexpr uint32_t DING_PARTNER_DELAY_MS = 400; // partner chimes after the sensing lantern
static constexpr uint32_t MODE_FLASH_MS = 1500;
static uint32_t gModeFlashUntil = 0;
static TmfPresenceGate gDingGate;
static bool gDingGateInit = false;
static uint32_t gLastDingMs = 0, gPartnerDueMs = 0;
static bool gPartnerPending = false;
static uint8_t gPartnerCopies = 0;  // copies of the partner strike still to send

uint8_t decompressionEventMode() { return (gCfg.eventMode >= 1 && gCfg.eventMode <= 4) ? gCfg.eventMode : 4; }
static bool modeDings(uint8_t m) { return m == NB_MODE_LANTERN_DINGS || m == NB_MODE_DECOMPRESSION; }
static bool modeCycle(uint8_t m) { return m == NB_MODE_PERIMETER_CYCLE || m == NB_MODE_DECOMPRESSION; }

// Event image (RES_SQUAD_TRIGGER_ANY): every lantern keeps itself awake in either
// mode, so nothing depends on a perimeter. Otherwise mode 2 keeps a sensing
// downlight awake (it must watch). The power policy's FULL/DIM gate applies.
#ifndef RES_SQUAD_TRIGGER_ANY
#define RES_SQUAD_TRIGGER_ANY 0
#endif
static bool dingsKeepAwake() {
  // Stay up just long enough to show a mode change (travel's red flash, then sleep).
  if (gModeFlashUntil && (int32_t)(millis() - gModeFlashUntil) < 0) return true;
  if (decompressionEventMode() == NB_MODE_TRAVEL) return false;  // packed: normal sleep
  if (gTelemetryFixtureClass != FIXTURE_DOWNLIGHT || !decompressionPowerReady()) return false;
  if (RES_SQUAD_TRIGGER_ANY) return true;
  return modeDings(decompressionEventMode()) && sensors().tmfPresent;
}

// ---- event cycle on the trigger lantern -------------------------------------
static SquadCycle gLanternCycle;       // perimeter cycle: rising pairs
static SquadSingleCycle gSingleCycle;  // decompression: one lantern per approach
static uint32_t gCycleDue[SQUAD_COUNT];
static uint8_t gCycleCopies[SQUAD_COUNT];
static NbTargetU16 gCycleCmd[SQUAD_COUNT];
static uint8_t gPressSrc[3] = {0, 0, 0};
static uint32_t gPressSeq = 0;

static bool isTriggerLantern() {
  int self = squadIndex(gMyId);
  return SQUAD_TRIGGER_LANTERN >= 0 && self == SQUAD_TRIGGER_LANTERN;
}

static bool gFreakoutActive();
static void cyclePress(uint32_t nowMs, const char *source) {
  if (!decompressionPowerReady() || gFreakoutActive()) return;
  bool single = decompressionEventMode() == NB_MODE_DECOMPRESSION;
  uint8_t step;
  uint16_t mask;
  if (single) {  // 2.5 s cooldown in both cycles
    if (!gSingleCycle.canFire(nowMs, SQUAD_ALL)) return;
    step = gSingleCycle.next; mask = gSingleCycle.targets(); gSingleCycle.sent(nowMs);
  } else {
    if (!gLanternCycle.canFire(nowMs, SQUAD_ALL)) return;
    step = gLanternCycle.next; mask = gLanternCycle.targets(); gLanternCycle.sent(nowMs);
  }
  for (unsigned i = 0; i < SQUAD_COUNT; ++i) {
    if (!(mask & (1U << i))) continue;
    // pair (perimeter cycle): the higher lantern a beat later; single steps and chords together
    gCycleDue[i] = nowMs + 60 + (single ? 0 : squadStepOffsetMs(step, i));
    gCycleCopies[i] = 3;
    memset(&gCycleCmd[i], 0, sizeof(gCycleCmd[i]));
  }
  Serial.printf("cycle: step=%u targets=%03x source=%s\n", (unsigned)step + 1, mask, source);
}

static void cycleSendTick(uint32_t nowMs) {
  int self = squadIndex(gMyId);
  for (unsigned i = 0; i < SQUAD_COUNT; ++i) {
    if (!gCycleCopies[i] || (int32_t)(nowMs - gCycleDue[i]) < 0) continue;
    if ((int)i == self) {  // our own lantern: strike locally, once
      solenoidStrike(SQUAD[i].pulseMs, "cycle");
      gCycleCopies[i] = 0;
      continue;
    }
    if (!espNowTxAvailable()) return;  // never stack a send on an unfinished TX
    if (gCycleCopies[i] == 3) {        // one header (seq) for all copies of this strike
      fillHeader(&gCycleCmd[i].h, NB_TARGET_SOLENOID);
      memcpy(gCycleCmd[i].target_id, SQUAD[i].id, 3);
      gCycleCmd[i].value = SQUAD[i].pulseMs;
    }
    espNowSendRaw(&gCycleCmd[i], sizeof(gCycleCmd[i]));
    --gCycleCopies[i];
    gCycleDue[i] = nowMs + 25;
    return;  // one send per tick
  }
}

// ---- Freakout Mode on the trigger lantern ----------------------------------
static constexpr uint8_t FREAKOUT_LOOPS = 2;
static constexpr uint16_t HOLD_NEEDED_MS = 1500;
static constexpr uint8_t HOLD_PERIMETERS = 4;      // all four perimeters
static constexpr uint32_t HOLD_FRESH_MS = 700;     // a hold report must be this recent
static constexpr uint32_t FREAKOUT_REARM_MS = 8000; // after a hold-triggered freakout
static bool gFreakout = false;
static bool gFreakoutActive() { return gFreakout; }
static uint32_t gFreakoutStartMs = 0, gFreakoutLastHoldMs = 0;
static uint16_t gFreakoutNextStep = 0;
struct HoldSrc { uint8_t id[3]; uint32_t rxMs; uint16_t heldMs; bool used; };
static HoldSrc gHolds[8];
struct AtomSeen { uint8_t id[3]; uint32_t boot, seq; };
static AtomSeen gAtomSeen[4];
static uint8_t gAtomSeenNext = 0;

static void freakoutStart(uint32_t nowMs, const char *why) {
  if (gFreakout || !decompressionPowerReady()) return;  // presses during a freakout are ignored
  gFreakout = true;
  gFreakoutStartMs = nowMs + 80;
  gFreakoutNextStep = 0;
  Serial.printf("freakout: x%u (%s)\n", (unsigned)FREAKOUT_LOOPS, why);
}

static void freakoutTick(uint32_t nowMs) {
  if (!gFreakout) return;
  const uint16_t total = (uint16_t)SQUAD_FREAKOUT_STEPS * FREAKOUT_LOOPS;
  while (gFreakoutNextStep < total &&
         (int32_t)(nowMs - (gFreakoutStartMs + (uint32_t)gFreakoutNextStep * SQUAD_FREAKOUT_STEP_MS)) >= 0) {
    uint16_t mask = SQUAD_FREAKOUT[gFreakoutNextStep % SQUAD_FREAKOUT_STEPS];
    for (unsigned i = 0; i < SQUAD_COUNT; ++i) {
      if (!(mask & (1U << i))) continue;
      gCycleDue[i] = nowMs;
      gCycleCopies[i] = 2;  // the fixture's 80 ms rest gate dedupes the copy
      memset(&gCycleCmd[i], 0, sizeof(gCycleCmd[i]));
    }
    ++gFreakoutNextStep;
  }
  if (gFreakoutNextStep >= total) gFreakout = false;
}

void decompressionOnPresenceHold(const NbPresenceHold &hold, uint32_t rxMs) {
  if (!decompressionBuild() || hold.confirm != NB_PRESENCE_HOLD_CONFIRM || !isTriggerLantern()) return;
  uint8_t m = decompressionEventMode();
  if (m != NB_MODE_PERIMETER_CYCLE && m != NB_MODE_DECOMPRESSION) return;
  HoldSrc *slot = nullptr;
  for (HoldSrc &h : gHolds) if (h.used && memcmp(h.id, hold.h.src_id, 3) == 0) slot = &h;
  if (!slot) for (HoldSrc &h : gHolds) if (!h.used || rxMs - h.rxMs > 5000) { slot = &h; break; }
  if (!slot) return;
  memcpy(slot->id, hold.h.src_id, 3);
  slot->rxMs = rxMs; slot->heldMs = hold.held_ms; slot->used = true;
  uint8_t held = 0;
  for (const HoldSrc &h : gHolds)
    if (h.used && rxMs - h.rxMs <= HOLD_FRESH_MS && h.heldMs >= HOLD_NEEDED_MS) ++held;
  if (held >= HOLD_PERIMETERS && rxMs - gFreakoutLastHoldMs >= FREAKOUT_REARM_MS) {
    gFreakoutLastHoldMs = rxMs;
    freakoutStart(rxMs, "four perimeters held");
  }
}

void decompressionOnAtomPress(const NbDecompressionControl &c) {
  // Any Atom press (normal or T-Deck mode, any target) = Freakout Mode, once.
  if (!decompressionBuild() || c.action != NB_DECOMP_PLAY || !c.controller_boot || !isTriggerLantern()) return;
  uint8_t m = decompressionEventMode();
  if (m == NB_MODE_TRAVEL) return;
  for (const AtomSeen &a : gAtomSeen)
    if (a.boot == c.controller_boot && a.seq == c.h.seq && memcmp(a.id, c.h.src_id, 3) == 0) return;  // copy
  AtomSeen &slot = gAtomSeen[gAtomSeenNext];
  gAtomSeenNext = (uint8_t)((gAtomSeenNext + 1) % 4);
  memcpy(slot.id, c.h.src_id, 3); slot.boot = c.controller_boot; slot.seq = c.h.seq;
  freakoutStart(millis(), "atom button");
}

void decompressionOnCyclePress(const NbCyclePress &press) {
  if (!decompressionBuild() || press.confirm != NB_CYCLE_PRESS_CONFIRM || !isTriggerLantern() ||
      !modeCycle(decompressionEventMode())) return;
  if (memcmp(press.h.src_id, gPressSrc, 3) == 0 && press.h.seq == gPressSeq) return;  // another copy
  memcpy(gPressSrc, press.h.src_id, 3);
  gPressSeq = press.h.seq;
  cyclePress(millis(), "perimeter");
}

void decompressionOnEventMode(const NbEventMode &m) {
  if (!decompressionBuild() || !nbEventModeValid(m)) return;
  bool changed = gCfg.eventMode != m.mode;
  if (m.persist) nvsPersistEventMode(m.mode);
  else gCfg.eventMode = m.mode;
  if (m.mode == NB_MODE_TRAVEL) {
    // Packed up: drop every keep-awake source we hold (a leftover 60-minute USB
    // session from the bench, controller radio leases) so the lantern sleeps
    // right after the red flash instead of up to an hour later. Repeats are
    // harmless; a fresh lease from a T-Deck/Atom with keep-awake on still wins.
    uint64_t t = esp_timer_get_time();
    gSession.set(t, 0);
    gRadioSession.set(t, 0);
  }
  if (changed) {
    gModeFlashUntil = millis() + MODE_FLASH_MS;
    gDingGateInit = false;  // relearn the background in the new mode
    gPartnerPending = false;
    Serial.printf("event mode -> %s\n", m.mode == 4 ? "decompression" : m.mode == 3 ? "travel" :
                  m.mode == 2 ? "lantern dings" : "perimeter cycle");
  }
}

bool decompressionModeFlash(uint32_t nowMs, uint8_t rgb[3]) {
  if (!gModeFlashUntil || (int32_t)(nowMs - gModeFlashUntil) >= 0) return false;
  uint8_t mode = decompressionEventMode();
  bool deco = mode == NB_MODE_DECOMPRESSION;          // purple = decompression
  rgb[0] = (mode == NB_MODE_TRAVEL || deco) ? 160 : 0; // red = travel
  rgb[1] = mode == NB_MODE_LANTERN_DINGS ? 160 : 0;    // green = dings
  rgb[2] = (mode == NB_MODE_PERIMETER_CYCLE || deco) ? 160 : 0; // blue = cycle
  return true;
}


void decompressionDingTick(uint32_t nowMs) {
  if (!decompressionBuild()) return;
  int self = squadIndex(gMyId);
  // Partner strike: up to three copies ~25 ms apart, never stacked on an unfinished
  // TX (the squad trigger's lesson); the receiver's 80 ms rest gate dedupes them.
  if (gPartnerPending && (int32_t)(nowMs - gPartnerDueMs) >= 0 && espNowTxAvailable()) {
    static NbTargetU16 cmd;
    if (self >= 0 && SQUAD[self].partner >= 0) {
      const SquadMember &p = SQUAD[SQUAD[self].partner];
      if (gPartnerCopies == 3) {  // first copy: one header (seq) for all copies
        memset(&cmd, 0, sizeof(cmd));
        fillHeader(&cmd.h, NB_TARGET_SOLENOID);
        memcpy(cmd.target_id, p.id, 3);
        cmd.value = p.pulseMs;
      }
      espNowSendRaw(&cmd, sizeof(cmd));
    }
    gPartnerDueMs = nowMs + 25;
    if (--gPartnerCopies == 0) gPartnerPending = false;
  }
  freakoutTick(nowMs);
  cycleSendTick(nowMs);
  if (gTelemetryFixtureClass != FIXTURE_DOWNLIGHT) return;
  // Decompression: every sensing lantern (Leeloo included) dings itself; only
  // perimeters advance the cycle. Perimeter cycle: Leeloo's sensor advances it.
  bool dings = modeDings(decompressionEventMode());
  bool trigger = decompressionEventMode() == NB_MODE_PERIMETER_CYCLE && isTriggerLantern();
  if (!dings && !trigger) return;
  const SensorSnapshot &sn = sensors();
  if (!sn.tmfPresent || !sn.tmfOk) return;
  if (!gDingGateInit) { tmfPresenceInit(gDingGate); gDingGateInit = true; }
  bool rising = tmfPresenceObserve(gDingGate, sn.tmfReads, sn.tofZoneMm, sn.tofZoneConfidence);
  if (rising && trigger) { cyclePress(nowMs, "lantern"); return; }  // mode 1: advance the cycle
  if (!rising || nowMs - gLastDingMs < DING_MIN_GAP_MS || !decompressionPowerReady()) return;
  gLastDingMs = nowMs;
  uint16_t pulse = self >= 0 ? SQUAD[self].pulseMs : 40;
  if (solenoidStrike(pulse, "presence ding")) Serial.println("ding: presence");
  if (self >= 0 && SQUAD[self].partner >= 0) {
    gPartnerDueMs = nowMs + DING_PARTNER_DELAY_MS;
    gPartnerPending = true;
    gPartnerCopies = 3;
  }
}
uint32_t decompressionSecondsLeft() {
  if (!decompressionBuild()) return 0;
  uint32_t local = gSession.remainingSeconds(esp_timer_get_time());
  uint32_t radio = decompressionRadioSecondsLeft();
  uint32_t pattern = patternSecondsLeft();
  uint32_t best = local > radio ? local : radio;
  return best > pattern ? best : pattern;
}
uint32_t decompressionRadioSecondsLeft() {
  return decompressionBuild() ? gRadioSession.remainingSeconds(esp_timer_get_time()) : 0;
}
bool decompressionSetSession(uint16_t minutes) {
  if (!decompressionBuild() || minutes > 120) return false;
  if (!minutes) {
    gRadioSession.set(esp_timer_get_time(), 0);
    solenoidStop("decompression session ended");
  }
  return gSession.set(esp_timer_get_time(), minutes);
}
bool decompressionPowerReady() {
  return decompressionBuild() && maintMode() == MODE_COMMS && !otaVerifyPending() &&
      !bootGuardParked() && pfIsReady() && batteryPresent() &&
      (uint8_t)powerBudget().tier <= (uint8_t)LedTier::DIM;
}
bool decompressionPlaySignature() {
  if (!decompressionPowerReady()) return false;
  if (!solenoidSignatureStart()) return false;
  // Explicit first trigger opens one hour; later hits do not extend it.
  if (!decompressionActive()) decompressionSetSession(60);
  return true;
}

static void radioReceipt(const NbDecompressionControl &c, uint8_t status,
                         const SignatureSequence *result = nullptr) {
  NbDecompressionReceipt reply = {};
  fillHeader(&reply.h, NB_DECOMPRESSION_RECEIPT);
  memcpy(reply.request_source, c.h.src_id, 3);
  reply.controller_boot = c.controller_boot;
  reply.request_seq = c.h.seq;
  reply.action = c.action;
  reply.status = status;
  reply.lease_left_s = decompressionRadioSecondsLeft();
  SignatureSequence state = result ? *result : solenoidSignatureSnapshot();
  reply.signature_state = state.result;
  reply.signature_hits = state.hits;
  reply.signature_max_late_us = state.maxLateUs;
  espNowSendRaw(&reply, sizeof(reply));
}

void decompressionOnRadio(const NbDecompressionControl &c, uint32_t receivedMs) {
  if (!decompressionBuild() || !nbDecompressionValid(c, gMyId) ||
      millis() - receivedMs > 500) return; // never play a backlogged press
  DecompressionReplay::Entry *entry;
  auto verdict = gReplay.admit(c, millis(), entry);
  if (verdict != DecompressionReplay::NEW) {
    radioReceipt(c, verdict == DecompressionReplay::DUPLICATE ?
                      entry->status : NB_DECOMP_STALE);
    return;
  }
  uint8_t status = NB_DECOMP_REFUSED;
  if (c.action == NB_DECOMP_PLAY) {
    gRadioPlays++;
    memcpy(gRadioLastSrc, c.h.src_id, 3);
    gRadioLastBoot = c.controller_boot;
    gRadioLastSeq = c.h.seq;
    gRadioLastMs = millis();
    if (gRelay) { // the host decides what this press does; never start the phrase here
      entry->status = NB_DECOMP_READY;
      radioReceipt(c, NB_DECOMP_READY);
      return;
    }
  }
  if (c.action == NB_DECOMP_SESSION) {
    if (!c.minutes) {
      gRadioSession.set(esp_timer_get_time(), 0);
      if (gRadioPlaying) solenoidStop("radio session ended");
      status = NB_DECOMP_ENDED;
    } else if (decompressionPowerReady()) {
      gRadioSession.set(esp_timer_get_time(), c.minutes);
      status = NB_DECOMP_READY;
    }
  } else if (decompressionPowerReady() && decompressionRadioSecondsLeft() &&
             !gRadioPlaying && !gResultCopies && solenoidSignatureStart()) {
    // Do not call the USER/USB entry point: a radio press must not create or
    // extend its own one-hour lease after the controller disappears.
    gRadioPlay = c;
    gRadioPlaying = true;
    status = NB_DECOMP_STARTED;
  }
  entry->status = status; // refusals are consumed too; retries cannot fire later
  radioReceipt(c, status);
}
void decompressionButtonWake() { gWakePending = true; }
void decompressionSetRelay(bool on) { gRelay = on; }
bool decompressionRelay() { return gRelay; }
void decompressionRadioTelemetry(String &j) {
  char src[7];
  snprintf(src, sizeof(src), "%02X%02X%02X", gRadioLastSrc[0], gRadioLastSrc[1], gRadioLastSrc[2]);
  j += ",\"event_mode\":" + String((unsigned)decompressionEventMode());
  j += ",\"radio_relay\":" + String(gRelay ? "true" : "false");
  j += ",\"radio_plays\":" + String((unsigned long)gRadioPlays);
  j += ",\"radio_last_src\":\"" + String(src) + "\"";
  j += ",\"radio_last_boot\":" + String((unsigned long)gRadioLastBoot);
  j += ",\"radio_last_seq\":" + String((unsigned long)gRadioLastSeq);
  j += ",\"radio_last_age_ms\":" + String(gRadioPlays ? (unsigned long)(millis() - gRadioLastMs) : 0UL);
}
void decompressionTick() {
  if (!decompressionBuild()) return;
  patternTick();
  bool active = decompressionActive();
  if ((gWasActive && !active) || !pfIsReady() || !batteryPresent() ||
      (uint8_t)powerBudget().tier > (uint8_t)LedTier::DIM)
    solenoidStop("decompression expiry/power veto");
  if (gRadioPlaying && !decompressionRadioSecondsLeft())
    solenoidStop("radio lease expired");
  if (gRadioPlaying && !solenoidSignatureSnapshot().active()) {
    gRadioResult = solenoidSignatureSnapshot();
    gRadioPlaying = false;
    gResultCopies = 3;
    gNextResultMs = millis();
  }
  if (gResultCopies && (int32_t)(millis() - gNextResultMs) >= 0) {
    radioReceipt(gRadioPlay, gRadioResult.result == SignatureSequence::COMPLETE ?
                                NB_DECOMP_COMPLETE : NB_DECOMP_ABORTED, &gRadioResult);
    --gResultCopies;
    gNextResultMs = millis() + 100;
  }
  gWasActive = active;
  if (gWakePending && millis() >= 5000) {
    gWakePending = false;
    (void)decompressionPlaySignature();
  }
}
