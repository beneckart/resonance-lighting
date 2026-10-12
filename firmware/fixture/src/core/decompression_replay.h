#pragma once
#include "packet.h"

// At-most-once admission across interleaved lease/play retries. Reject older
// sequence numbers, including a delayed PLAY after a later lease. This is RF
// duplicate suppression, not authentication or persistent replay protection.
struct DecompressionReplay {
  enum Verdict { NEW, DUPLICATE, STALE, FULL };
  struct Entry {
    bool used = false;
    uint8_t source[3] = {};
    uint32_t boot = 0, seq = 0, seenMs = 0;
    uint8_t status = NB_DECOMP_REFUSED;
  } entries[8];

  Verdict admit(const NbDecompressionControl &c, uint32_t now, Entry *&entry) {
    entry = nullptr;
    Entry *available = nullptr;
    for (auto &e : entries) {
      if (e.used && e.boot == c.controller_boot &&
          memcmp(e.source, c.h.src_id, 3) == 0) {
        entry = &e;
        if (c.h.seq == e.seq) return DUPLICATE;
        if ((int32_t)(c.h.seq - e.seq) <= 0) return STALE;
        e.seq = c.h.seq;
        e.seenMs = now;
        e.status = NB_DECOMP_REFUSED;
        return NEW;
      }
      // A retired controller boot can be forgotten after ten minutes; the
      // sender's bounded retransmission window is only 100 ms.
      if (!e.used || now - e.seenMs > 600000UL) available = &e;
    }
    if (!available) return FULL;
    entry = available;
    entry->used = true;
    memcpy(entry->source, c.h.src_id, 3);
    entry->boot = c.controller_boot;
    entry->seq = c.h.seq;
    entry->seenMs = now;
    entry->status = NB_DECOMP_REFUSED;
    return NEW;
  }
};
