#include "decompression.h"
#include <Arduino.h>
#include "esp_timer.h"
#include "../core/decompression_session.h"
#include "../core/decompression_replay.h"
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

bool decompressionActive() {
  return decompressionBuild() && (gSession.active(esp_timer_get_time()) ||
                                  gRadioSession.active(esp_timer_get_time()) ||
                                  patternActive());
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
