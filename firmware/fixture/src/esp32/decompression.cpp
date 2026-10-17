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

uint8_t decompressionEventMode() { return gCfg.eventMode == 2 ? 2 : 1; }

// Event image (RES_SQUAD_TRIGGER_ANY): every lantern keeps itself awake in either
// mode, so nothing depends on a perimeter. Otherwise mode 2 keeps a sensing
// downlight awake (it must watch). The power policy's FULL/DIM gate applies.
#ifndef RES_SQUAD_TRIGGER_ANY
#define RES_SQUAD_TRIGGER_ANY 0
#endif
static bool dingsKeepAwake() {
  if (gTelemetryFixtureClass != FIXTURE_DOWNLIGHT || !decompressionPowerReady()) return false;
  if (RES_SQUAD_TRIGGER_ANY) return true;
  return decompressionEventMode() == NB_MODE_LANTERN_DINGS && sensors().tmfPresent;
}

// ---- event cycle on the trigger lantern -------------------------------------
static SquadCycle gLanternCycle;
static uint32_t gCycleDue[SQUAD_COUNT];
static uint8_t gCycleCopies[SQUAD_COUNT];
static NbTargetU16 gCycleCmd[SQUAD_COUNT];
static uint8_t gPressSrc[3] = {0, 0, 0};
static uint32_t gPressSeq = 0;

static bool isTriggerLantern() {
  int self = squadIndex(gMyId);
  return SQUAD_TRIGGER_LANTERN >= 0 && self == SQUAD_TRIGGER_LANTERN;
}

static void cyclePress(uint32_t nowMs, const char *source) {
  if (!gLanternCycle.canFire(nowMs, SQUAD_ALL) || !decompressionPowerReady()) return;  // 2.5 s cooldown
  uint8_t step = gLanternCycle.next;
  uint16_t mask = gLanternCycle.targets();
  gLanternCycle.sent(nowMs);
  for (unsigned i = 0; i < SQUAD_COUNT; ++i) {
    if (!(mask & (1U << i))) continue;
    gCycleDue[i] = nowMs + 60 + squadStepOffsetMs(step, i);  // pair: the higher lantern a beat later
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

void decompressionOnCyclePress(const NbCyclePress &press) {
  if (!decompressionBuild() || press.confirm != NB_CYCLE_PRESS_CONFIRM || !isTriggerLantern() ||
      decompressionEventMode() != NB_MODE_PERIMETER_CYCLE) return;
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
  if (changed) {
    gModeFlashUntil = millis() + MODE_FLASH_MS;
    gDingGateInit = false;  // relearn the background in the new mode
    gPartnerPending = false;
    Serial.printf("event mode -> %s\n", m.mode == 2 ? "lantern dings" : "perimeter cycle");
  }
}

bool decompressionModeFlash(uint32_t nowMs, uint8_t rgb[3]) {
  if (!gModeFlashUntil || (int32_t)(nowMs - gModeFlashUntil) >= 0) return false;
  bool dings = decompressionEventMode() == NB_MODE_LANTERN_DINGS;
  rgb[0] = 0; rgb[1] = dings ? 160 : 0; rgb[2] = dings ? 0 : 160;  // green = dings, blue = cycle
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
  cycleSendTick(nowMs);
  if (gTelemetryFixtureClass != FIXTURE_DOWNLIGHT) return;
  bool dings = decompressionEventMode() == NB_MODE_LANTERN_DINGS;
  bool trigger = !dings && isTriggerLantern();
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
