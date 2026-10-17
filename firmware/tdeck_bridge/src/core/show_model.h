#pragma once

#include <stddef.h>
#include <stdint.h>

// Tone Lab show file (ops/bench/decompression_tone_lab.py, "Export for
// T-Deck"): the patterns and Atom button rules prepared on the laptop, played
// on site over radio. Pure core: no Arduino, no radio. Format (one record per
// line, '|' separated, free text last, CRC32 over every byte before END):
//
//   TLSHOW|1|<created utc>|<site>
//   CFG|<channel>|<min hit spacing ms>
//   L|<id6>|<tone>|<pulse ms>|<group>|<L|P>|<name>
//   P|<pattern id>|<active 0/1>|<bpm>|<steps>|<steps per beat>|<name>
//   H|<pattern id>|<id6>|<step,step,...>
//   B|<rule id>|<enabled>|<any|id6>|<single|double|triple|any>|<action>|<arg>|<loops>|<name>
//   A|<id6>|<name>
//   END|<crc32 hex>

static constexpr size_t SHOW_MAX_LANTERNS = 32;
static constexpr size_t SHOW_MAX_PATTERNS = 32;
static constexpr size_t SHOW_MAX_HITS = 1536;
static constexpr size_t SHOW_MAX_RULES = 16;
static constexpr size_t SHOW_MAX_LIST = 8;
static constexpr size_t SHOW_MAX_TEXT = 16384;

struct ShowLantern {
  uint8_t id[3];
  uint8_t tone;
  uint16_t pulseMs;
  uint8_t group;
  bool perimeter;
  char name[24];
};

struct ShowHit {
  uint16_t step;
  uint8_t lantern;  // index into Show::lanterns
};

struct ShowPattern {
  char id[24];
  char name[32];
  bool active;
  float bpm;
  uint16_t steps;
  uint8_t stepsPerBeat;
  uint16_t firstHit;  // into Show::hits, sorted by step
  uint16_t hitCount;
};

enum ShowClick : uint8_t { CLICK_ANY = 0, CLICK_SINGLE = 1, CLICK_DOUBLE = 2, CLICK_TRIPLE = 3 };

enum ShowAction : uint8_t {
  ACT_NONE = 0,
  ACT_PERFORMANCE,          // play pattern N loops, then back to what was playing
  ACT_NEXT_ACTIVE_PATTERN,  // move to the next Active pattern
  ACT_PLAY_PATTERN,         // play pattern on loop
  ACT_NEXT_PATTERN,         // cycle through a list
  ACT_STOP,
};

struct ShowRule {
  char id[24];
  char name[32];
  bool enabled;
  bool anyAtom;
  uint8_t atom[3];
  uint8_t click;   // ShowClick
  uint8_t action;  // ShowAction
  int16_t pattern; // index for PERFORMANCE / PLAY_PATTERN, else -1
  uint8_t loops;
  uint8_t list[SHOW_MAX_LIST];
  uint8_t listCount;
};

struct Show {
  char created[28];
  char site[28];
  uint8_t channel;
  uint16_t minSpacingMs;
  ShowLantern lanterns[SHOW_MAX_LANTERNS];
  size_t lanternCount;
  ShowPattern patterns[SHOW_MAX_PATTERNS];
  size_t patternCount;
  ShowHit hits[SHOW_MAX_HITS];
  size_t hitCount;
  ShowRule rules[SHOW_MAX_RULES];
  size_t ruleCount;
  uint32_t crc;
};

uint32_t showCrc32(const uint8_t *data, size_t len);

// Parse and verify a complete show text. On failure returns false and writes a
// short reason; `out` is then unspecified (callers parse into scratch storage).
bool showParse(const char *text, size_t len, Show &out, char *err, size_t errLen);

// Timing for one pattern at its own BPM.
float showStepMs(const ShowPattern &p);
float showLoopMs(const ShowPattern &p);

// Next Active pattern after `current` in name order (the Tone Lab's rotation
// order); with no current pattern, the first Active one. -1 if none is Active.
int showNextActive(const Show &show, int current);

int showFindPattern(const Show &show, const char *id);
