#include "tonelab_svc.h"

#include <Arduino.h>
#include <Preferences.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <freertos/semphr.h>
#include <string.h>

#include "../core/click_model.h"
#include "../core/rx_ring.h"
#include "mesh_tx.h"
#include "fixture/src/core/packet.h"

// Big buffers live in PSRAM: static RAM on this image is already ~64% used.
static Show *gShow = nullptr;      // live show (read by the player task)
static Show *gScratch = nullptr;   // parse target for a new upload
static char *gText = nullptr;      // stored show text
static size_t gTextLen = 0;
static bool gLoaded = false;
static SemaphoreHandle_t gLock = nullptr;  // guards gShow + play state
static TaskHandle_t gPlayer = nullptr;
static Preferences gPrefs;

// Play state (under gLock). gGen bumps on every change so the player restarts.
static int gPlaying = -1, gResume = -1;
static uint8_t gLoopsLeft = 0;  // 0 = until stopped
static uint32_t gLoopsDone = 0;
static volatile uint32_t gGen = 0;  // read by the player without the lock
static bool gAwake = false;
static uint8_t gMode = 0;
static uint32_t gModeResendUntil = 0, gModeNextMs = 0;
static uint32_t gPresses = 0;
static char gLast[64] = "";
static int gCycle[SHOW_MAX_RULES];  // next_pattern position per rule

static uint32_t gControllerBoot = 0;  // decompression lease identity (random per boot)
static AtomClicks gClicks;
static ClickEvent gQueued[4];
static uint8_t gQueuedN = 0;

static void lock() { if (gLock) xSemaphoreTake(gLock, portMAX_DELAY); }
static void unlock() { if (gLock) xSemaphoreGive(gLock); }

static void setLast(const char *text) {
  snprintf(gLast, sizeof(gLast), "%s", text);
  Serial.printf("tonelab: %s\n", gLast);
}

// ------------------------------------------------------------------ storage --
static void persistOverrides() {
  uint32_t act = 0, en = 0;
  for (size_t i = 0; i < gShow->patternCount && i < 32; ++i)
    if (gShow->patterns[i].active) act |= 1u << i;
  for (size_t i = 0; i < gShow->ruleCount && i < 32; ++i)
    if (gShow->rules[i].enabled) en |= 1u << i;
  gPrefs.putUInt("act", act);
  gPrefs.putUInt("en", en);
  gPrefs.putUInt("crc", gShow->crc);
}

static void applyOverrides() {
  if (gPrefs.getUInt("crc", 0) != gShow->crc) return;  // overrides belong to another show
  uint32_t act = gPrefs.getUInt("act", 0), en = gPrefs.getUInt("en", 0);
  for (size_t i = 0; i < gShow->patternCount && i < 32; ++i) gShow->patterns[i].active = act & (1u << i);
  for (size_t i = 0; i < gShow->ruleCount && i < 32; ++i) gShow->rules[i].enabled = en & (1u << i);
}

// ------------------------------------------------------------------- player --
static void waitUntilUs(int64_t dueUs, uint32_t gen) {
  for (;;) {
    int64_t left = dueUs - esp_timer_get_time();
    if (left <= 0 || gGen != gen) return;
    if (left > 2000) vTaskDelay(pdMS_TO_TICKS((uint32_t)(left / 1000) - 1));
    else if (left > 300) vTaskDelay(1);
    else return;  // close enough; ESP-NOW airtime dominates from here
  }
}

static void playerTask(void *) {
  static uint8_t ids[32][3];
  static uint16_t pulses[32];
  for (;;) {
    lock();
    int pat = gPlaying;
    uint32_t gen = gGen;
    unlock();
    if (pat < 0) {
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200));
      continue;
    }
    int64_t t0 = esp_timer_get_time() + 80000;  // small lead so step 0 lands on time
    uint32_t loop = 0;
    while (gGen == gen) {
      lock();
      ShowPattern p = gShow->patterns[pat];
      unlock();
      double stepUs = 60000000.0 / ((double)p.bpm * p.stepsPerBeat);
      double loopUs = stepUs * p.steps;
      uint16_t h = 0;
      while (h < p.hitCount && gGen == gen) {
        lock();
        uint16_t step = gShow->hits[p.firstHit + h].step;
        uint8_t n = 0;
        while (h < p.hitCount && gShow->hits[p.firstHit + h].step == step && n < 32) {
          const ShowLantern &l = gShow->lanterns[gShow->hits[p.firstHit + h].lantern];
          memcpy(ids[n], l.id, 3);
          pulses[n] = l.pulseMs;
          ++n;
          ++h;
        }
        unlock();
        waitUntilUs(t0 + (int64_t)(loop * loopUs + step * stepUs), gen);
        if (gGen != gen) break;
        meshStrikeRound(ids, pulses, n, 3, 3);
      }
      if (gGen != gen) break;
      waitUntilUs(t0 + (int64_t)((loop + 1) * loopUs), gen);  // let the loop's tail play out
      ++loop;
      lock();
      if (gGen == gen) {
        ++gLoopsDone;
        if (gLoopsLeft && gLoopsDone >= gLoopsLeft) {  // performance finished
          int back = gResume;
          gPlaying = back;
          gResume = -1;
          gLoopsLeft = 0;
          gLoopsDone = 0;
          ++gGen;
          if (back >= 0) snprintf(gLast, sizeof(gLast), "back to %s", gShow->patterns[back].name);
          else snprintf(gLast, sizeof(gLast), "performance done");
        }
      }
      unlock();
    }
  }
}

static void startLocked(int pattern, uint8_t loops, int resume) {
  gPlaying = pattern;
  gLoopsLeft = loops;
  gLoopsDone = 0;
  gResume = resume;
  ++gGen;
  if (gPlayer) xTaskNotifyGive(gPlayer);
}

bool tonelabPlay(int pattern, uint8_t loops) {
  lock();
  bool ok = gLoaded && pattern >= 0 && (size_t)pattern < gShow->patternCount && gShow->patterns[pattern].hitCount;
  if (ok) {
    startLocked(pattern, loops, -1);
    snprintf(gLast, sizeof(gLast), "playing %s", gShow->patterns[pattern].name);
  }
  unlock();
  return ok;
}

void tonelabStop() {
  lock();
  startLocked(-1, 0, -1);
  snprintf(gLast, sizeof(gLast), "stopped");
  unlock();
}

// ----------------------------------------------------------------- actions --
static void runRuleLocked(const ShowRule &r, size_t ri) {
  switch (r.action) {
    case ACT_PERFORMANCE: {
      // Return to what was looping (or to where an unfinished performance would return).
      int back = gLoopsLeft ? gResume : gPlaying;
      startLocked(r.pattern, r.loops, back);
      snprintf(gLast, sizeof(gLast), "%s x%u", gShow->patterns[r.pattern].name, r.loops);
      break;
    }
    case ACT_NEXT_ACTIVE_PATTERN: {
      if (gLoopsLeft) {  // mid-performance: change where it returns to
        gResume = showNextActive(*gShow, gResume);
        snprintf(gLast, sizeof(gLast), "after: %s", gResume >= 0 ? gShow->patterns[gResume].name : "nothing Active");
      } else {
        int next = showNextActive(*gShow, gPlaying);
        if (next >= 0) {
          startLocked(next, 0, -1);
          snprintf(gLast, sizeof(gLast), "now: %s", gShow->patterns[next].name);
        } else {
          snprintf(gLast, sizeof(gLast), "no Active patterns");
        }
      }
      break;
    }
    case ACT_PLAY_PATTERN:
      startLocked(r.pattern, 0, -1);
      snprintf(gLast, sizeof(gLast), "playing %s", gShow->patterns[r.pattern].name);
      break;
    case ACT_NEXT_PATTERN:
      if (r.listCount) {
        gCycle[ri] = (gCycle[ri] + 1) % r.listCount;
        startLocked(r.list[gCycle[ri]], 0, -1);
        snprintf(gLast, sizeof(gLast), "playing %s", gShow->patterns[r.list[gCycle[ri]]].name);
      }
      break;
    case ACT_STOP:
      startLocked(-1, 0, -1);
      snprintf(gLast, sizeof(gLast), "stopped");
      break;
    default:
      break;
  }
}

static void dispatchClick(const ClickEvent &ev) {
  static const char *kNames[] = {"", "single", "double", "triple"};
  lock();
  bool any = false;
  for (size_t i = 0; gLoaded && i < gShow->ruleCount; ++i) {
    const ShowRule &r = gShow->rules[i];
    if (!r.enabled) continue;
    if (!r.anyAtom && memcmp(r.atom, ev.atom, 3) != 0) continue;
    if (r.click != CLICK_ANY && r.click != ev.clicks) continue;
    runRuleLocked(r, i);
    any = true;
  }
  if (!any) snprintf(gLast, sizeof(gLast), "%s click: no rule", kNames[ev.clicks > 3 ? 3 : ev.clicks]);
  Serial.printf("tonelab: atom %02X%02X%02X %s click -> %s\n", ev.atom[0], ev.atom[1], ev.atom[2],
                kNames[ev.clicks > 3 ? 3 : ev.clicks], gLast);
  unlock();
}

void tonelabRunRule(int rule) {
  lock();
  if (gLoaded && rule >= 0 && (size_t)rule < gShow->ruleCount) runRuleLocked(gShow->rules[rule], (size_t)rule);
  unlock();
}

// --------------------------------------------------------------- radio in --
void tonelabObserve(const RxItem &item) {
  const NbHeader *h = (const NbHeader *)item.data;
  if (h->type != NB_DECOMPRESSION_CONTROL || item.len < sizeof(NbDecompressionControl)) return;
  const NbDecompressionControl *c = (const NbDecompressionControl *)item.data;
  if (c->action != NB_DECOMP_PLAY || !c->controller_boot) return;
  ClickEvent ev;
  if (gClicks.observe(c->h.src_id, c->controller_boot, c->h.seq, item.ms, &ev)) {
    ++gPresses;
    if (ev.clicks && gQueuedN < 4) gQueued[gQueuedN++] = ev;
  }
}

// -------------------------------------------------------------------- tick --
void tonelabTick(uint32_t nowMs) {
  for (uint8_t i = 0; i < gQueuedN; ++i) dispatchClick(gQueued[i]);
  gQueuedN = 0;
  ClickEvent ev;
  while (gClicks.tick(nowMs, &ev)) dispatchClick(ev);

  if (gMode && (int32_t)(nowMs - gModeResendUntil) < 0 && (int32_t)(nowMs - gModeNextMs) >= 0) {
    gModeNextMs = nowMs + 2000;
    meshEventMode(gMode, true);
  }

  // Keep-awake: renew one lantern's 1-minute lease every 400 ms (a full pass
  // every few seconds). One small send at a time keeps pattern timing clean.
  static uint32_t nextLeaseMs = 0;
  static size_t leaseIdx = 0;
  if (gAwake && gLoaded && (int32_t)(nowMs - nextLeaseMs) >= 0) {
    nextLeaseMs = nowMs + 400;
    uint8_t id[1][3];
    bool have = false;
    lock();
    if (gShow->lanternCount) {
      leaseIdx %= gShow->lanternCount;
      memcpy(id[0], gShow->lanterns[leaseIdx].id, 3);
      ++leaseIdx;
      have = true;
    }
    unlock();
    if (have) meshDecompressionSession(id, 1, 1, gControllerBoot);
  }
}

void tonelabSetMode(uint8_t mode) {
  if (mode != 1 && mode != 2) return;
  gMode = mode;
  gPrefs.putUChar("mode", mode);
  gModeResendUntil = millis() + 60000;
  gModeNextMs = 0;
  setLast(mode == 2 ? "mode: lantern dings" : "mode: perimeter cycle");
}

void tonelabSetAwake(bool on) {
  gAwake = on;
  gPrefs.putBool("awake", on);
  setLast(on ? "keeping lanterns awake" : "keep-awake off");
}

void tonelabSetActive(int pattern, bool active) {
  lock();
  if (gLoaded && pattern >= 0 && (size_t)pattern < gShow->patternCount) {
    gShow->patterns[pattern].active = active;
    persistOverrides();
  }
  unlock();
}

void tonelabSetRuleEnabled(int rule, bool enabled) {
  lock();
  if (gLoaded && rule >= 0 && (size_t)rule < gShow->ruleCount) {
    gShow->rules[rule].enabled = enabled;
    persistOverrides();
  }
  unlock();
}

// ------------------------------------------------------------------ upload --
bool tonelabLoadShow(const char *text, size_t len, char *err, size_t errLen) {
  if (!gScratch || !gText) {
    snprintf(err, errLen, "no memory");
    return false;
  }
  if (!showParse(text, len, *gScratch, err, errLen)) return false;
  lock();
  startLocked(-1, 0, -1);
  Show *old = gShow;
  gShow = gScratch;
  gScratch = old;
  memcpy(gText, text, len);
  gTextLen = len;
  gLoaded = true;
  memset(gCycle, 0xFF, sizeof(gCycle));
  bool ok = gPrefs.putBytes("show", gText, gTextLen) == gTextLen;
  persistOverrides();  // a new show starts from its own Active/enabled flags
  snprintf(gLast, sizeof(gLast), "show loaded: %u patterns", (unsigned)gShow->patternCount);
  unlock();
  if (!ok) snprintf(err, errLen, "loaded but NOT saved (storage full)");
  return true;
}

// ------------------------------------------------------------------ status --
TonelabStatus tonelabStatus() {
  TonelabStatus s = {};
  lock();
  s.loaded = gLoaded;
  if (gLoaded) {
    snprintf(s.created, sizeof(s.created), "%s", gShow->created);
    snprintf(s.site, sizeof(s.site), "%s", gShow->site);
    s.patterns = gShow->patternCount;
    s.rules = gShow->ruleCount;
    s.lanterns = gShow->lanternCount;
  }
  s.playing = gPlaying;
  s.loopsLeft = gLoopsLeft;
  s.loopsDone = gLoopsDone;
  s.resume = gResume;
  s.awake = gAwake;
  s.presses = gPresses;
  s.mode = gMode;
  snprintf(s.last, sizeof(s.last), "%s", gLast);
  unlock();
  return s;
}

bool tonelabPattern(int i, ShowPattern &out) {
  lock();
  bool ok = gLoaded && i >= 0 && (size_t)i < gShow->patternCount;
  if (ok) out = gShow->patterns[i];
  unlock();
  return ok;
}

bool tonelabRule(int i, ShowRule &out) {
  lock();
  bool ok = gLoaded && i >= 0 && (size_t)i < gShow->ruleCount;
  if (ok) out = gShow->rules[i];
  unlock();
  return ok;
}

void tonelabPrintStatus() {
  TonelabStatus s = tonelabStatus();
  Serial.printf("tonelab loaded=%d created=%s site=%s patterns=%u rules=%u lanterns=%u playing=%d loops_left=%d "
                "loops_done=%u resume=%d awake=%d presses=%u last=\"%s\"\n",
                s.loaded, s.created, s.site, (unsigned)s.patterns, (unsigned)s.rules, (unsigned)s.lanterns,
                s.playing, s.loopsLeft, (unsigned)s.loopsDone, s.resume, s.awake, (unsigned)s.presses, s.last);
  for (size_t i = 0; i < s.patterns; ++i) {
    ShowPattern p;
    if (tonelabPattern((int)i, p))
      Serial.printf("  P%u %s%s bpm=%.0f steps=%u hits=%u\n", (unsigned)i, p.active ? "*" : " ", p.name, p.bpm,
                    p.steps, p.hitCount);
  }
  for (size_t i = 0; i < s.rules; ++i) {
    ShowRule r;
    if (tonelabRule((int)i, r)) Serial.printf("  R%u %s %s\n", (unsigned)i, r.enabled ? "on " : "off", r.name);
  }
}

// ------------------------------------------------------------------- begin --
void tonelabBegin() {
  gLock = xSemaphoreCreateMutex();
  gShow = (Show *)ps_calloc(1, sizeof(Show));
  gScratch = (Show *)ps_calloc(1, sizeof(Show));
  gText = (char *)ps_malloc(SHOW_MAX_TEXT);
  gShowScratch = ps_malloc(SHOW_SCRATCH_BYTES);
  do { gControllerBoot = esp_random(); } while (!gControllerBoot);
  memset(gCycle, 0xFF, sizeof(gCycle));
  gPrefs.begin("tonelab", false);
  gAwake = gPrefs.getBool("awake", false);
  gMode = gPrefs.getUChar("mode", 0);
  if (gShow && gScratch && gText) {
    size_t n = gPrefs.getBytesLength("show");
    if (n && n <= SHOW_MAX_TEXT && gPrefs.getBytes("show", gText, n) == n) {
      char err[80];
      if (showParse(gText, n, *gShow, err, sizeof(err))) {
        gTextLen = n;
        gLoaded = true;
        applyOverrides();
      } else {
        Serial.printf("tonelab: stored show rejected: %s\n", err);
      }
    }
  } else {
    Serial.println("tonelab: PSRAM allocation FAILED");
  }
  xTaskCreatePinnedToCore(playerTask, "tonelab", 3072, nullptr, 5, &gPlayer, 1);
  Serial.printf("tonelab: %s, keep-awake %s\n", gLoaded ? "show loaded" : "no show yet", gAwake ? "on" : "off");
}
