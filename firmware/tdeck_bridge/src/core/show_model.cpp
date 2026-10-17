#include "show_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

uint32_t showCrc32(const uint8_t *data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;  // zlib.crc32
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int b = 0; b < 8; ++b) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

static bool fail(char *err, size_t errLen, const char *msg, int line) {
  if (err && errLen) {
    if (line > 0) snprintf(err, errLen, "line %d: %s", line, msg);
    else snprintf(err, errLen, "%s", msg);
  }
  return false;
}

static bool parseHex6(const char *s, uint8_t out[3]) {
  if (!s || strlen(s) != 6) return false;
  for (int i = 0; i < 3; ++i) {
    char pair[3] = {s[i * 2], s[i * 2 + 1], 0};
    char *end = nullptr;
    long v = strtol(pair, &end, 16);
    if (!end || *end) return false;
    out[i] = (uint8_t)v;
  }
  return true;
}

static void copyField(char *dst, size_t cap, const char *src) {
  snprintf(dst, cap, "%s", src ? src : "");
}

static int findLantern(const Show &s, const uint8_t id[3]) {
  for (size_t i = 0; i < s.lanternCount; ++i)
    if (memcmp(s.lanterns[i].id, id, 3) == 0) return (int)i;
  return -1;
}

int showFindPattern(const Show &show, const char *id) {
  for (size_t i = 0; i < show.patternCount; ++i)
    if (strcmp(show.patterns[i].id, id) == 0) return (int)i;
  return -1;
}

static uint8_t clickFrom(const char *s) {
  if (!strcmp(s, "single")) return CLICK_SINGLE;
  if (!strcmp(s, "double")) return CLICK_DOUBLE;
  if (!strcmp(s, "triple")) return CLICK_TRIPLE;
  return CLICK_ANY;
}

static uint8_t actionFrom(const char *s) {
  if (!strcmp(s, "performance")) return ACT_PERFORMANCE;
  if (!strcmp(s, "next_active_pattern")) return ACT_NEXT_ACTIVE_PATTERN;
  if (!strcmp(s, "play_pattern")) return ACT_PLAY_PATTERN;
  if (!strcmp(s, "next_pattern")) return ACT_NEXT_PATTERN;
  if (!strcmp(s, "stop")) return ACT_STOP;
  return ACT_NONE;
}

// Pending hit before sorting: pattern index + step + lantern.
struct RawHit {
  uint8_t pattern;
  uint16_t step;
  uint8_t lantern;
};

void *gShowScratch = nullptr;
static_assert(sizeof(RawHit) * SHOW_MAX_HITS <= SHOW_SCRATCH_BYTES, "parse scratch too small");

bool showParse(const char *text, size_t len, Show &out, char *err, size_t errLen) {
  if (!text || len == 0) return fail(err, errLen, "empty show", 0);
  if (len > SHOW_MAX_TEXT) return fail(err, errLen, "show too large", 0);
  memset(&out, 0, sizeof(out));

  // CRC: every byte before the final "END|" line.
  const char *endTag = nullptr;
  for (size_t i = 0; i + 4 <= len; ++i)
    if ((i == 0 || text[i - 1] == '\n') && memcmp(text + i, "END|", 4) == 0) endTag = text + i;
  if (!endTag) return fail(err, errLen, "missing END line", 0);
  char crcHex[9] = {};
  size_t k = 0;
  for (const char *c = endTag + 4; c < text + len && k < 8 && *c != '\n' && *c != '\r'; ++c) crcHex[k++] = *c;
  uint32_t want = (uint32_t)strtoul(crcHex, nullptr, 16);
  uint32_t got = showCrc32((const uint8_t *)text, (size_t)(endTag - text));
  if (k != 8 || want != got) return fail(err, errLen, "checksum mismatch (transfer damaged)", 0);
  out.crc = got;

  // Scratch for unsorted hits: caller-provided (PSRAM on the device) or heap.
  RawHit *raw = (RawHit *)gShowScratch;
  bool ownRaw = false;
  if (!raw) {
    raw = (RawHit *)malloc(sizeof(RawHit) * SHOW_MAX_HITS);
    ownRaw = true;
  }
  if (!raw) return fail(err, errLen, "no memory", 0);
  struct Free { RawHit *p; bool own; ~Free() { if (own) free(p); } } freeRaw{raw, ownRaw};
  size_t rawCount = 0;
  bool header = false;
  int lineNo = 0;
  const char *p = text;
  const char *stop = endTag;
  char line[512];
  while (p < stop) {
    const char *nl = (const char *)memchr(p, '\n', (size_t)(stop - p));
    size_t n = nl ? (size_t)(nl - p) : (size_t)(stop - p);
    ++lineNo;
    if (n >= sizeof(line)) return fail(err, errLen, "line too long", lineNo);
    memcpy(line, p, n);
    line[n] = 0;
    if (n && line[n - 1] == '\r') line[n - 1] = 0;
    p = nl ? nl + 1 : stop;
    if (!line[0]) continue;

    char *f[10] = {};
    int nf = 0;
    char *cur = line;
    while (cur && nf < 10) {
      f[nf++] = cur;
      char *bar = strchr(cur, '|');
      if (bar) { *bar = 0; cur = bar + 1; } else cur = nullptr;
    }
    const char *tag = f[0];

    if (!strcmp(tag, "TLSHOW")) {
      if (nf < 4 || strcmp(f[1], "1")) return fail(err, errLen, "unsupported show version", lineNo);
      copyField(out.created, sizeof(out.created), f[2]);
      copyField(out.site, sizeof(out.site), f[3]);
      header = true;
      continue;
    }
    if (!header) return fail(err, errLen, "not a Tone Lab show file", lineNo);
    if (!strcmp(tag, "CFG")) {
      if (nf < 3) return fail(err, errLen, "bad CFG", lineNo);
      out.channel = (uint8_t)atoi(f[1]);
      out.minSpacingMs = (uint16_t)atoi(f[2]);
    } else if (!strcmp(tag, "L")) {
      if (nf < 7) return fail(err, errLen, "bad lantern", lineNo);
      if (out.lanternCount >= SHOW_MAX_LANTERNS) return fail(err, errLen, "too many lanterns", lineNo);
      ShowLantern &l = out.lanterns[out.lanternCount];
      if (!parseHex6(f[1], l.id)) return fail(err, errLen, "bad lantern id", lineNo);
      l.tone = (uint8_t)atoi(f[2]);
      int pulse = atoi(f[3]);
      if (pulse < 5 || pulse > 300) return fail(err, errLen, "pulse out of range", lineNo);
      l.pulseMs = (uint16_t)pulse;
      l.group = (uint8_t)atoi(f[4]);
      l.perimeter = f[5][0] == 'P';
      copyField(l.name, sizeof(l.name), f[6]);
      ++out.lanternCount;
    } else if (!strcmp(tag, "P")) {
      if (nf < 7) return fail(err, errLen, "bad pattern", lineNo);
      if (out.patternCount >= SHOW_MAX_PATTERNS) return fail(err, errLen, "too many patterns", lineNo);
      ShowPattern &pt = out.patterns[out.patternCount];
      copyField(pt.id, sizeof(pt.id), f[1]);
      pt.active = f[2][0] == '1';
      pt.bpm = (float)atof(f[3]);
      int steps = atoi(f[4]), spb = atoi(f[5]);
      if (pt.bpm < 5 || pt.bpm > 300 || steps < 1 || steps > 256 || spb < 1 || spb > 8)
        return fail(err, errLen, "pattern timing out of range", lineNo);
      pt.steps = (uint16_t)steps;
      pt.stepsPerBeat = (uint8_t)spb;
      copyField(pt.name, sizeof(pt.name), f[6]);
      ++out.patternCount;
    } else if (!strcmp(tag, "H")) {
      if (nf < 4) return fail(err, errLen, "bad hits", lineNo);
      int pi = showFindPattern(out, f[1]);
      uint8_t id[3];
      if (pi < 0) return fail(err, errLen, "hits for an unknown pattern", lineNo);
      if (!parseHex6(f[2], id)) return fail(err, errLen, "bad hit lantern", lineNo);
      int li = findLantern(out, id);
      if (li < 0) return fail(err, errLen, "hit for an unknown lantern", lineNo);
      for (char *tok = strtok(f[3], ","); tok; tok = strtok(nullptr, ",")) {
        int step = atoi(tok);
        if (step < 0 || step >= out.patterns[pi].steps) return fail(err, errLen, "hit step out of range", lineNo);
        if (rawCount >= SHOW_MAX_HITS) return fail(err, errLen, "too many hits", lineNo);
        raw[rawCount++] = {(uint8_t)pi, (uint16_t)step, (uint8_t)li};
      }
    } else if (!strcmp(tag, "B")) {
      if (nf < 9) return fail(err, errLen, "bad rule", lineNo);
      if (out.ruleCount >= SHOW_MAX_RULES) return fail(err, errLen, "too many rules", lineNo);
      ShowRule &r = out.rules[out.ruleCount];
      copyField(r.id, sizeof(r.id), f[1]);
      r.enabled = f[2][0] == '1';
      r.anyAtom = !strcmp(f[3], "any");
      if (!r.anyAtom && !parseHex6(f[3], r.atom)) return fail(err, errLen, "bad rule atom", lineNo);
      r.click = clickFrom(f[4]);
      r.action = actionFrom(f[5]);
      if (r.action == ACT_NONE) return fail(err, errLen, "unknown rule action", lineNo);
      r.pattern = -1;
      if (r.action == ACT_PERFORMANCE || r.action == ACT_PLAY_PATTERN) {
        r.pattern = (int16_t)showFindPattern(out, f[6]);
        if (r.pattern < 0) return fail(err, errLen, "rule names an unknown pattern", lineNo);
      } else if (r.action == ACT_NEXT_PATTERN) {
        for (char *tok = strtok(f[6], ","); tok && r.listCount < SHOW_MAX_LIST; tok = strtok(nullptr, ",")) {
          int pi = showFindPattern(out, tok);
          if (pi >= 0) r.list[r.listCount++] = (uint8_t)pi;
        }
      }
      int loops = atoi(f[7]);
      r.loops = (uint8_t)(loops < 1 ? 1 : loops > 32 ? 32 : loops);
      copyField(r.name, sizeof(r.name), f[8]);
      ++out.ruleCount;
    }
    // A| atom names and unknown tags are ignored: newer exports stay loadable.
  }
  if (!header) return fail(err, errLen, "not a Tone Lab show file", 0);

  // Group hits by pattern, sorted by step (stable insertion sort; n is small).
  for (size_t i = 1; i < rawCount; ++i) {
    RawHit v = raw[i];
    size_t j = i;
    while (j > 0 && (raw[j - 1].pattern > v.pattern ||
                     (raw[j - 1].pattern == v.pattern && raw[j - 1].step > v.step))) {
      raw[j] = raw[j - 1];
      --j;
    }
    raw[j] = v;
  }
  for (size_t i = 0; i < rawCount; ++i) {
    out.hits[i] = {raw[i].step, raw[i].lantern};
    ShowPattern &pt = out.patterns[raw[i].pattern];
    if (pt.hitCount == 0) pt.firstHit = (uint16_t)i;
    ++pt.hitCount;
  }
  out.hitCount = rawCount;
  return true;
}

float showStepMs(const ShowPattern &p) { return 60000.0f / (p.bpm * (float)p.stepsPerBeat); }

float showLoopMs(const ShowPattern &p) { return showStepMs(p) * (float)p.steps; }

int showNextActive(const Show &show, int current) {
  // Candidates in case-insensitive name order, like the Tone Lab's active_patterns().
  int order[SHOW_MAX_PATTERNS];
  size_t n = 0;
  for (size_t i = 0; i < show.patternCount; ++i)
    if (show.patterns[i].active && show.patterns[i].hitCount) order[n++] = (int)i;
  if (!n) return -1;
  for (size_t i = 1; i < n; ++i) {
    int v = order[i];
    size_t j = i;
    while (j > 0 && strcasecmp(show.patterns[order[j - 1]].name, show.patterns[v].name) > 0) {
      order[j] = order[j - 1];
      --j;
    }
    order[j] = v;
  }
  for (size_t i = 0; i < n; ++i)
    if (order[i] == current) return order[(i + 1) % n];
  return order[0];
}
