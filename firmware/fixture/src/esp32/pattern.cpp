#include "pattern.h"

#include "esp_random.h"
#include "esp_timer.h"
#include "../core/decompression_session.h"
#include "../core/pattern_part.h"
#include "../core/pattern_sync.h"
#include "boot_guard_io.h"
#include "decompression.h"
#include "espnow_link.h"
#include "identity.h"
#include "nvs_store.h"
#include "solenoid.h"

// Shared between the loop (commands, beats, election) and the 1 ms esp_timer
// task (hit scheduling). Every access to these four objects holds gPatMux.
static portMUX_TYPE gPatMux = portMUX_INITIALIZER_UNLOCKED;
static PatternPart gPart;
static PatternSync gSync;
static PatternPlayer gPlayer;
static bool gPlaying = false;

static DecompressionSession gSession;
static esp_timer_handle_t gTimer = nullptr;
static uint32_t gBoot = 0;
static const char *gStopWhy = "";

struct PatternBlob {
  uint8_t version;
  PatternPart part;
  uint32_t crc;
};
static constexpr uint8_t BLOB_VERSION = 1;

static void timerTick(void *) {
  uint64_t now = esp_timer_get_time();
  bool gate = solenoidGateOn();
  PatternPlayer::Action a;
  uint16_t pulse;
  portENTER_CRITICAL(&gPatMux);
  bool playable = gPlaying && gSync.locked(now);
  a = gPlayer.poll(gPart, gSync.groupNow(now), playable, gate);
  pulse = gPart.pulseMs;
  portEXIT_CRITICAL(&gPatMux);
  if (a != PatternPlayer::FIRE) return;
  uint64_t onset = 0;
  bool ok = solenoidPatternPulse(pulse, &onset);
  portENTER_CRITICAL(&gPatMux);
  if (ok) gPlayer.fired(gPart, gSync.groupNow(onset));
  else gPlayer.refused(gPart);
  portEXIT_CRITICAL(&gPatMux);
}

void patternInit() {
  if (!decompressionBuild()) return;
  gBoot = esp_random() | 1U;
  PatternBlob blob = {};
  if (nvsReadPatternBlob(&blob, sizeof(blob)) == sizeof(blob) && blob.version == BLOB_VERSION &&
      blob.part.valid() && blob.part.crc() == blob.crc)
    gPart = blob.part;
  const esp_timer_create_args_t args = {
      .callback = &timerTick,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "fx_pattern",
      .skip_unhandled_events = true,
  };
  if (esp_timer_create(&args, &gTimer) != ESP_OK) gTimer = nullptr;
}

bool patternActive() { return decompressionBuild() && gPlaying; }
uint32_t patternSecondsLeft() {
  return patternActive() ? gSession.remainingSeconds(esp_timer_get_time()) : 0;
}

void patternStop(const char *why) {
  if (!gPlaying) return;
  if (gTimer) esp_timer_stop(gTimer);
  portENTER_CRITICAL(&gPatMux);
  gPlaying = false;
  gSync.stop();
  gPlayer.reset();
  portEXIT_CRITICAL(&gPatMux);
  solenoidSetPatternMode(false);
  gSession.set(esp_timer_get_time(), 0);
  gStopWhy = why;
}

static const char *startPlayback(uint32_t patternId, uint16_t minutes) {
  if (!gTimer) return "no timer";
  if (!gPart.patternId) return "no pattern stored";
  if (gPart.patternId != patternId) return "stored pattern id differs";
  if (!decompressionPowerReady()) return "power/maintenance not ready";
  if (gPlaying) patternStop("restarted");
  if (!bootGuardLoadArm()) return "load marker not persisted";
  if (!solenoidSetPatternMode(true)) return "solenoid busy";
  uint64_t now = esp_timer_get_time();
  portENTER_CRITICAL(&gPatMux);
  gSync.start(now, gMyId, gPart.patternId, gPart.loopUs, gBoot);
  gPlayer = PatternPlayer();
  gPlaying = true;
  portEXIT_CRITICAL(&gPatMux);
  gStopWhy = ""; // a stale reason from the previous session would mislead telemetry
  gSession.set(now, minutes);
  if (esp_timer_start_periodic(gTimer, 1000) != ESP_OK) {
    patternStop("timer start failed");
    return "timer start failed";
  }
  return nullptr;
}

void patternSerialLine(const char *line) {
  if (!decompressionBuild()) {
    Serial.println("pattern: refused (not a decompression build)");
    return;
  }
  PatternCommand cmd;
  if (!patternParseLine(line, gMyId, cmd)) {
    if (cmd.error) Serial.printf("pattern: refused (%s)\n", cmd.error);
    return;
  }
  switch (cmd.type) {
  case PATTERN_CMD_SET: {
    if (gPlaying) patternStop("new part");
    PatternBlob blob = {BLOB_VERSION, cmd.part, cmd.part.crc()};
    if (!nvsWritePatternBlob(&blob, sizeof(blob))) {
      Serial.println("pattern: refused (NVS write failed)");
      return;
    }
    portENTER_CRITICAL(&gPatMux);
    gPart = cmd.part;
    portEXIT_CRITICAL(&gPatMux);
    Serial.printf("pattern: stored id=%08lx hits=%u loop_us=%lu pulse_ms=%u lead_us=%lu crc=%08lx\n",
                  (unsigned long)gPart.patternId, (unsigned)gPart.count, (unsigned long)gPart.loopUs,
                  (unsigned)gPart.pulseMs, (unsigned long)gPart.leadUs, (unsigned long)gPart.crc());
    break;
  }
  case PATTERN_CMD_PLAY: {
    const char *why = startPlayback(cmd.patternId, cmd.minutes);
    if (why) Serial.printf("pattern: refused (%s)\n", why);
    else Serial.printf("pattern: playing id=%08lx minutes=%u\n", (unsigned long)cmd.patternId,
                       (unsigned)cmd.minutes);
    break;
  }
  case PATTERN_CMD_STOP:
    patternStop("explicit stop");
    Serial.println("pattern: stopped");
    break;
  case PATTERN_CMD_CLEAR:
    patternStop("cleared");
    if (!nvsClearPatternBlob()) {
      Serial.println("pattern: refused (NVS clear failed)");
      return;
    }
    portENTER_CRITICAL(&gPatMux);
    gPart = PatternPart();
    portEXIT_CRITICAL(&gPatMux);
    Serial.println("pattern: cleared");
    break;
  }
}

void patternOnBeat(const NbPatternBeat &beat, uint64_t rxUs) {
  if (!gPlaying) return;
  uint64_t now = esp_timer_get_time();
  portENTER_CRITICAL(&gPatMux);
  gSync.onBeat(beat, rxUs, now);
  portEXIT_CRITICAL(&gPatMux);
}

void patternTick() {
  if (!decompressionBuild()) return;
  uint32_t tag;
  uint64_t txUs;
  bool txDone = espNowTakeTxDone(tag, txUs);
  if (!gPlaying) return;
  uint64_t now = esp_timer_get_time();
  if (!gSession.active(now)) { patternStop("session ended"); return; }
  if (!decompressionPowerReady()) { patternStop("power/maintenance veto"); return; }
  if (!solenoidPatternMode()) { patternStop("solenoid stopped"); return; }
  (void)bootGuardLoadArm(); // idempotent; re-arms after any quiet disarm, never in the timer

  NbPatternBeat beat = {};
  bool due;
  portENTER_CRITICAL(&gPatMux);
  if (txDone) gSync.beatSent(tag, txUs);
  gSync.tick(now);
  due = gSync.beatDue(now, beat);
  portEXIT_CRITICAL(&gPatMux);
  if (due) {
    fillHeader(&beat.h, NB_PATTERN_BEAT);
    espNowSendTagged(&beat, sizeof(beat), beat.beat_seq);
  }
}

void patternTelemetry(String &j) {
  if (!decompressionBuild()) return;
  uint64_t now = esp_timer_get_time();
  portENTER_CRITICAL(&gPatMux);
  PatternPart part = gPart;
  PatternSync sync = gSync;
  PatternPlayer player = gPlayer;
  bool playing = gPlaying;
  portEXIT_CRITICAL(&gPatMux);
  char id[9], cond[7], crc[9];
  snprintf(id, sizeof(id), "%08lx", (unsigned long)part.patternId);
  snprintf(cond, sizeof(cond), "%02X%02X%02X", sync.conductor[0], sync.conductor[1], sync.conductor[2]);
  snprintf(crc, sizeof(crc), "%08lx", (unsigned long)part.crc());
  static const char *roles[] = {"idle", "listen", "follow", "conduct"};
  j += ",\"pattern_id\":\"" + String(id) + "\"";
  j += ",\"pattern_crc\":\"" + String(crc) + "\"";
  j += ",\"pattern_hits_in_loop\":" + String(part.count);
  j += ",\"pattern_loop_us\":" + String((unsigned long)part.loopUs);
  j += ",\"pattern_playing\":" + String(playing ? "true" : "false");
  j += ",\"pattern_left_s\":" + String((unsigned long)patternSecondsLeft());
  j += ",\"pattern_role\":\"" + String(roles[sync.role & 3]) + "\"";
  j += ",\"pattern_locked\":" + String(playing && sync.locked(now) ? "true" : "false");
  j += ",\"pattern_conductor\":\"" + String(cond) + "\"";
  j += ",\"pattern_offset_us\":" + String((long long)sync.offsetUs);
  j += ",\"pattern_last_step_us\":" + String((long long)sync.lastStepUs);
  j += ",\"pattern_beats_rx\":" + String((unsigned long)sync.beatsRx);
  j += ",\"pattern_beats_tx\":" + String((unsigned long)sync.beatsTx);
  j += ",\"pattern_takeovers\":" + String((unsigned long)sync.takeovers);
  j += ",\"pattern_played\":" + String((unsigned long)player.hits);
  j += ",\"pattern_late_skips\":" + String((unsigned long)player.lateSkips);
  j += ",\"pattern_blocked\":" + String((unsigned long)player.blocked);
  j += ",\"pattern_max_late_us\":" + String((unsigned long)player.maxLateUs);
  j += ",\"pattern_stop_reason\":\"" + String(gStopWhy) + "\"";
}
