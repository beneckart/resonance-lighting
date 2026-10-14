#include "pattern_sync.h"

#include <string.h>

static int cmpId(const uint8_t a[3], const uint8_t b[3]) { return memcmp(a, b, 3); }
// Elapsed time that never wraps when a timestamp (e.g. a radio receipt taken in
// another task) is slightly ahead of the caller's "now".
static uint64_t since(uint64_t now, uint64_t then) { return now > then ? now - then : 0; }

void PatternSync::start(uint64_t now, const uint8_t id[3], uint32_t pattern, uint32_t loop,
                        uint32_t boot) {
  uint32_t keepBeat = beatSeq; // beat_seq stays monotonic for this boot nonce
  *this = PatternSync();
  beatSeq = keepBeat;
  memcpy(myId, id, 3);
  patternId = pattern;
  loopUs = loop;
  myBoot = boot ? boot : 1;
  role = LISTEN;
  roleSinceUs = now;
  // Spread start-up and takeover decisions so one lantern usually acts first
  // and the rest hear its beat before their own timer expires.
  uint32_t h = ((uint32_t)id[0] << 16 | (uint32_t)id[1] << 8 | id[2]) * 2654435761U;
  staggerUs = (uint64_t)(h >> 8) % STAGGER_US;
}

bool PatternSync::locked(uint64_t now) const {
  if (role == CONDUCT) return since(now, roleSinceUs) >= SETTLE_US;
  return role == FOLLOW && sampleCount >= LOCK_SAMPLES;
}

void PatternSync::becomeConductor(uint64_t now) {
  role = CONDUCT;
  roleSinceUs = now;
  haveTx = false;
  nextBeatUs = now; // announce immediately
  memcpy(conductor, myId, 3);
  conductorBoot = myBoot;
}

void PatternSync::follow(const uint8_t src[3], uint32_t boot, uint64_t now) {
  if (role == CONDUCT) yields++;
  role = FOLLOW;
  roleSinceUs = now;
  memcpy(conductor, src, 3);
  conductorBoot = boot;
  memset(pending, 0, sizeof(pending));
  sampleCount = sampleNext = 0;
}

bool PatternSync::beatDue(uint64_t now, NbPatternBeat &out) {
  if (role != CONDUCT || now < nextBeatUs) return false;
  nextBeatUs = now + BEAT_PERIOD_US;
  out.pattern_id = patternId;
  out.loop_us = loopUs;
  out.conductor_boot = myBoot;
  out.beat_seq = ++beatSeq ? beatSeq : ++beatSeq; // skip 0 on wrap
  out.prev_beat_seq = haveTx ? lastTxSeq : 0;
  out.prev_tx_group_us = haveTx ? lastTxGroupUs : 0;
  out.flags = 0;
  out.reserved = 0;
  beatsTx++;
  return true;
}

void PatternSync::beatSent(uint32_t seq, uint64_t txDoneLocalUs) {
  if (role != CONDUCT) return;
  lastTxSeq = seq;
  lastTxGroupUs = groupNow(txDoneLocalUs);
  haveTx = true;
}

void PatternSync::onBeat(const NbPatternBeat &b, uint64_t rxLocalUs, uint64_t now) {
  if (role == IDLE || !nbPatternBeatValid(b) || b.pattern_id != patternId || b.loop_us != loopUs)
    return;
  const uint8_t *src = b.h.src_id;
  if (cmpId(src, myId) == 0) return;
  beatsRx++;

  if (role == CONDUCT) {
    if (cmpId(src, myId) > 0) return; // a higher ID will yield to us
    follow(src, b.conductor_boot, now);
  } else if (role == LISTEN) {
    follow(src, b.conductor_boot, now);
  } else { // FOLLOW
    bool same = cmpId(src, conductor) == 0;
    if (!same) {
      bool currentAlive = since(now, lastBeatRxUs) < TAKEOVER_US;
      if (currentAlive && cmpId(src, conductor) > 0) return; // stay with the lower ID
      follow(src, b.conductor_boot, now);
    } else if (b.conductor_boot != conductorBoot) {
      follow(src, b.conductor_boot, now); // conductor rebooted: restart pairing
    }
  }
  lastBeatRxUs = now;

  // Two-step pairing: this beat says when the previous one left the radio.
  if (b.prev_beat_seq) {
    for (auto &p : pending) {
      if (p.seq && p.seq == b.prev_beat_seq) {
        int64_t sample = (int64_t)b.prev_tx_group_us - (int64_t)p.rxUs;
        samples[sampleNext] = sample;
        sampleNext = (uint8_t)((sampleNext + 1) % SAMPLES);
        if (sampleCount < SAMPLES) sampleCount++;
        samplesTaken++;
        int64_t sorted[SAMPLES];
        for (uint8_t i = 0; i < sampleCount; ++i) sorted[i] = samples[i];
        for (uint8_t i = 1; i < sampleCount; ++i) // insertion sort, n <= 5
          for (uint8_t j = i; j > 0 && sorted[j - 1] > sorted[j]; --j) {
            int64_t t = sorted[j]; sorted[j] = sorted[j - 1]; sorted[j - 1] = t;
          }
        int64_t median = sorted[sampleCount / 2];
        lastStepUs = median - offsetUs;
        offsetUs = median;
        p.seq = 0;
        break;
      }
    }
  }
  pending[pendingNext] = {b.beat_seq, rxLocalUs};
  pendingNext = (uint8_t)((pendingNext + 1) % 4);
}

void PatternSync::tick(uint64_t now) {
  if (role == LISTEN && since(now, roleSinceUs) >= LISTEN_US + staggerUs) {
    becomeConductor(now);
  } else if (role == FOLLOW && since(now, lastBeatRxUs) >= TAKEOVER_US + staggerUs) {
    takeovers++;
    becomeConductor(now); // keeps offset: the loop phase continues seamlessly
  }
}

// ---- player ------------------------------------------------------------------

uint64_t PatternPlayer::nextHit(const PatternPart &p, uint64_t after) {
  if (!p.count || !p.loopUs) return UINT64_MAX;
  uint64_t base = after - after % p.loopUs;
  for (int k = 0; k < 2; ++k, base += p.loopUs)
    for (uint8_t i = 0; i < p.count; ++i)
      if (base + p.offsetsUs[i] > after) return base + p.offsetsUs[i];
  return UINT64_MAX; // unreachable for a valid part
}

PatternPlayer::Action PatternPlayer::poll(const PatternPart &p, uint64_t groupNow, bool playable,
                                          bool gateOn) {
  if (!playable || !p.count) {
    primed = false;
    return WAIT;
  }
  if (!primed) { // start with the first hit we can still play on time
    nextTargetUs = nextHit(p, groupNow + p.leadUs + START_MARGIN_US);
    primed = true;
  }
  uint64_t fireAt = nextTargetUs - p.leadUs;
  if (groupNow < fireAt) return WAIT;
  if (groupNow - fireAt > MAX_LATE_US) { // never replay or compress late hits
    lateSkips++;
    nextTargetUs = nextHit(p, groupNow + p.leadUs);
    return SKIP;
  }
  if (gateOn) { // previous pulse still driving: skip this hit, do not queue it
    blocked++;
    nextTargetUs = nextHit(p, nextTargetUs);
    return SKIP;
  }
  return FIRE;
}

void PatternPlayer::fired(const PatternPart &p, uint64_t groupAtOnset) {
  uint64_t fireAt = nextTargetUs - p.leadUs;
  uint64_t late = groupAtOnset > fireAt ? groupAtOnset - fireAt : 0;
  if (late > maxLateUs) maxLateUs = (uint32_t)late;
  hits++;
  nextTargetUs = nextHit(p, nextTargetUs);
}

void PatternPlayer::refused(const PatternPart &p) {
  blocked++;
  nextTargetUs = nextHit(p, nextTargetUs);
}
