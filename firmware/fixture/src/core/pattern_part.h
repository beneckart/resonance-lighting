// ADR 0084: one lantern's part of a synchronized decompression pattern, and the
// bounded USB command line that sets, plays, stops or clears it.
#pragma once

#include <stddef.h>
#include <stdint.h>

struct PatternPart {
  static constexpr uint8_t MAX_HITS = 32;
  static constexpr uint32_t MIN_LOOP_US = 250000;
  static constexpr uint32_t MAX_LOOP_US = 60000000;
  static constexpr uint32_t MIN_SPACING_US = 250000; // includes the loop wrap
  static constexpr uint32_t MAX_LEAD_US = 200000;
  static constexpr uint16_t MIN_PULSE_MS = 5;
  static constexpr uint16_t MAX_PULSE_MS = 100;

  uint32_t patternId = 0; // 0 = no part stored
  uint32_t loopUs = 0;
  uint16_t pulseMs = 0;
  uint32_t leadUs = 0;
  uint8_t count = 0;      // 0 = silent member: keeps time, may conduct
  uint32_t offsetsUs[MAX_HITS] = {};

  // Null when valid, otherwise a short reason.
  const char *problem() const;
  bool valid() const { return problem() == nullptr; }
  // CRC32 (IEEE) over the canonical little-endian field encoding; the host
  // recomputes it to verify each lantern holds exactly what was sent.
  uint32_t crc() const;
};

enum PatternCommandType : uint8_t {
  PATTERN_CMD_NONE = 0,
  PATTERN_CMD_SET = 1,
  PATTERN_CMD_PLAY = 2,
  PATTERN_CMD_STOP = 3,
  PATTERN_CMD_CLEAR = 4,
};

struct PatternCommand {
  uint8_t type = PATTERN_CMD_NONE;
  PatternPart part;       // SET
  uint32_t patternId = 0; // PLAY
  uint16_t minutes = 0;   // PLAY, 1..120
  const char *error = nullptr;
};

// Longest legal line: SET with 32 eight-digit offsets is under 360 bytes.
#define PATTERN_LINE_MAX 384

// `line` is one complete line without its terminator, starting "!P<id> ".
// Returns false for another target (silently ignored, out.error null) or for a
// malformed command (out.error set). Nothing is partially applied.
bool patternParseLine(const char *line, const uint8_t myId[3], PatternCommand &out);
