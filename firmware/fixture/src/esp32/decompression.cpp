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

static DecompressionSession gSession;
static DecompressionSession gRadioSession;
static DecompressionReplay gReplay;
static NbDecompressionControl gRadioPlay;
static bool gRadioPlaying = false;
static uint8_t gResultCopies = 0;
static uint32_t gNextResultMs = 0;
static SignatureSequence gRadioResult;
static bool gWakePending = false;
static bool gWasActive = false;

bool decompressionActive() {
  return decompressionBuild() && (gSession.active(esp_timer_get_time()) ||
                                  gRadioSession.active(esp_timer_get_time()));
}
uint32_t decompressionSecondsLeft() {
  if (!decompressionBuild()) return 0;
  uint32_t local = gSession.remainingSeconds(esp_timer_get_time());
  uint32_t radio = decompressionRadioSecondsLeft();
  return local > radio ? local : radio;
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
static bool signaturePowerReady() {
  return decompressionBuild() && maintMode() == MODE_COMMS && !otaVerifyPending() &&
      !bootGuardParked() && pfIsReady() && batteryPresent() &&
      (uint8_t)powerBudget().tier <= (uint8_t)LedTier::DIM;
}
bool decompressionPlaySignature() {
  if (!signaturePowerReady()) return false;
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
  if (c.action == NB_DECOMP_SESSION) {
    if (!c.minutes) {
      gRadioSession.set(esp_timer_get_time(), 0);
      if (gRadioPlaying) solenoidStop("radio session ended");
      status = NB_DECOMP_ENDED;
    } else if (signaturePowerReady()) {
      gRadioSession.set(esp_timer_get_time(), c.minutes);
      status = NB_DECOMP_READY;
    }
  } else if (signaturePowerReady() && decompressionRadioSecondsLeft() &&
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
void decompressionTick() {
  if (!decompressionBuild()) return;
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
