#include "click_model.h"

#include <string.h>

bool AtomClicks::observe(const uint8_t atom[3], uint32_t boot, uint32_t seq, uint32_t nowMs, ClickEvent *fired) {
  if (fired) fired->clicks = 0;
  for (const Seen &s : seen_)
    if (s.used && s.boot == boot && s.seq == seq && memcmp(s.atom, atom, 3) == 0) return false;  // another copy
  Seen &slot = seen_[seenNext_];
  seenNext_ = (seenNext_ + 1) % kSeen;
  memcpy(slot.atom, atom, 3);
  slot.boot = boot;
  slot.seq = seq;
  slot.used = true;

  Pending *p = nullptr;
  for (Pending &c : pending_)
    if (c.used && memcmp(c.atom, atom, 3) == 0) p = &c;
  if (p && nowMs - p->lastMs > kWindowMs) {  // an old click that tick() has not settled yet
    if (fired) {
      memcpy(fired->atom, p->atom, 3);
      fired->clicks = p->n;
    }
    p->n = 1;
    p->lastMs = nowMs;
    return true;  // caller dispatches the settled one; this press starts fresh
  }
  if (!p) {
    for (Pending &c : pending_)
      if (!c.used) { p = &c; break; }
    if (!p) p = &pending_[0];  // more Atoms than slots: reuse the oldest-indexed
    memcpy(p->atom, atom, 3);
    p->n = 0;
    p->used = true;
  }
  ++p->n;
  p->lastMs = nowMs;
  if (p->n >= 3) {
    if (fired) {
      memcpy(fired->atom, p->atom, 3);
      fired->clicks = 3;
    }
    p->used = false;
  } else if (fired) {
    fired->clicks = 0;  // still waiting for the window
  }
  return true;
}

bool AtomClicks::tick(uint32_t nowMs, ClickEvent *fired) {
  for (Pending &c : pending_) {
    if (c.used && nowMs - c.lastMs > kWindowMs) {
      if (fired) {
        memcpy(fired->atom, c.atom, 3);
        fired->clicks = c.n;
      }
      c.used = false;
      return true;
    }
  }
  return false;
}
