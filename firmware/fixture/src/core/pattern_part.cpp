#include "pattern_part.h"

#include <string.h>

const char *PatternPart::problem() const {
  if (!patternId) return "pattern id must be nonzero";
  if (loopUs < MIN_LOOP_US || loopUs > MAX_LOOP_US) return "loop must be 0.25..60 s";
  if (pulseMs < MIN_PULSE_MS || pulseMs > MAX_PULSE_MS) return "pulse must be 5..100 ms";
  if (leadUs > MAX_LEAD_US) return "lead must be 0..200 ms";
  if (count > MAX_HITS) return "at most 32 hits";
  for (uint8_t i = 0; i < count; ++i) {
    if (offsetsUs[i] >= loopUs) return "hit outside the loop";
    if (i && offsetsUs[i] <= offsetsUs[i - 1]) return "hits must be strictly increasing";
    if (i && offsetsUs[i] - offsetsUs[i - 1] < MIN_SPACING_US) return "hits closer than 250 ms";
  }
  if (count > 1 && loopUs - offsetsUs[count - 1] + offsetsUs[0] < MIN_SPACING_US)
    return "hits closer than 250 ms across the loop wrap";
  if (count == 1 && loopUs < MIN_SPACING_US) return "loop shorter than hit spacing";
  return nullptr;
}

static uint32_t crcByte(uint32_t crc, uint8_t b) {
  crc ^= b;
  for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
  return crc;
}
static uint32_t crcU32(uint32_t crc, uint32_t v) {
  for (int i = 0; i < 4; ++i) crc = crcByte(crc, (uint8_t)(v >> (8 * i)));
  return crc;
}

uint32_t PatternPart::crc() const {
  uint32_t c = 0xFFFFFFFFU;
  c = crcU32(c, patternId);
  c = crcU32(c, loopUs);
  c = crcU32(c, pulseMs);
  c = crcU32(c, leadUs);
  c = crcU32(c, count);
  for (uint8_t i = 0; i < count && i < MAX_HITS; ++i) c = crcU32(c, offsetsUs[i]);
  return ~c;
}

// ---- line parsing ----------------------------------------------------------

static int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

// Cursor over one line; every token is separated by exactly one space.
struct Cursor {
  const char *p;
  bool atEnd() const { return *p == 0; }
  bool space() {
    if (*p != ' ') return false;
    ++p;
    return true;
  }
  bool word(const char *w) {
    size_t n = strlen(w);
    if (strncmp(p, w, n) != 0 || (p[n] != ' ' && p[n] != 0)) return false;
    p += n;
    return true;
  }
  bool hex32(uint32_t &out) {
    uint32_t v = 0;
    int n = 0;
    while (hexVal(*p) >= 0) {
      if (++n > 8) return false;
      v = (v << 4) | (uint32_t)hexVal(*p++);
    }
    out = v;
    return n > 0;
  }
  bool dec32(uint32_t &out) {
    uint64_t v = 0;
    int n = 0;
    while (*p >= '0' && *p <= '9') {
      if (++n > 10) return false;
      v = v * 10 + (uint64_t)(*p++ - '0');
    }
    if (!n || v > 0xFFFFFFFFULL) return false;
    out = (uint32_t)v;
    return true;
  }
};

bool patternParseLine(const char *line, const uint8_t myId[3], PatternCommand &out) {
  out = PatternCommand();
  if (!line || strlen(line) >= PATTERN_LINE_MAX || line[0] != '!' || line[1] != 'P') {
    out.error = "not a pattern line";
    return false;
  }
  uint8_t target[3];
  for (int i = 0; i < 6; ++i) {
    int v = hexVal(line[2 + i]);
    if (v < 0) {
      out.error = "bad target id";
      return false;
    }
    if (i & 1) target[i / 2] |= (uint8_t)v;
    else target[i / 2] = (uint8_t)(v << 4);
  }
  if (memcmp(target, myId, 3) != 0) return false; // another lantern's line
  Cursor c{line + 8};
  if (!c.space()) {
    out.error = "expected a space after the id";
    return false;
  }

  if (c.word("STOP") || c.word("CLEAR")) {
    bool stop = line[9] == 'S';
    if (!c.atEnd()) {
      out.error = "unexpected text after command";
      return false;
    }
    out.type = stop ? PATTERN_CMD_STOP : PATTERN_CMD_CLEAR;
    return true;
  }
  if (c.word("PLAY")) {
    uint32_t id = 0, minutes = 0;
    if (!c.space() || !c.hex32(id) || !c.space() || !c.dec32(minutes) || !c.atEnd()) {
      out.error = "use PLAY <pattern-id-hex> <minutes>";
      return false;
    }
    if (!id || minutes < 1 || minutes > 120) {
      out.error = "pattern id nonzero, minutes 1..120";
      return false;
    }
    out.type = PATTERN_CMD_PLAY;
    out.patternId = id;
    out.minutes = (uint16_t)minutes;
    return true;
  }
  if (c.word("SET")) {
    PatternPart p;
    uint32_t pulse = 0;
    if (!c.space() || !c.hex32(p.patternId) || !c.space() || !c.dec32(p.loopUs) ||
        !c.space() || !c.dec32(pulse) || !c.space() || !c.dec32(p.leadUs) || !c.space()) {
      out.error = "use SET <id-hex> <loop-us> <pulse-ms> <lead-us> <offsets|->";
      return false;
    }
    if (pulse > 0xFFFF) {
      out.error = "pulse must be 5..100 ms";
      return false;
    }
    p.pulseMs = (uint16_t)pulse;
    if (*c.p == '-') {
      ++c.p; // silent member
    } else {
      while (true) {
        uint32_t off = 0;
        if (p.count >= PatternPart::MAX_HITS) {
          out.error = "at most 32 hits";
          return false;
        }
        if (!c.dec32(off)) {
          out.error = "bad hit offset";
          return false;
        }
        p.offsetsUs[p.count++] = off;
        if (*c.p != ',') break;
        ++c.p;
      }
    }
    if (!c.atEnd()) {
      out.error = "unexpected text after offsets";
      return false;
    }
    if (const char *why = p.problem()) {
      out.error = why;
      return false;
    }
    out.type = PATTERN_CMD_SET;
    out.part = p;
    return true;
  }
  out.error = "unknown pattern command";
  return false;
}
