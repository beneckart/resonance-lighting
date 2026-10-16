#pragma once

#include <stddef.h>
#include <stdint.h>

// USB mutations use a separate fail-closed envelope from the legacy one-byte
// bench/report commands. A mutation is not actionable until a complete bounded
// line has arrived. The first supported form is:
//
//   !S<short-mac>:<seconds>\n
//
// Example: !S9E668C:10
//
// The exact target and explicit duration make host/driver probe bytes harmless;
// a bare or partial S can never request the historical six-hour default.
enum SerialMutationType : uint8_t {
  SERIAL_MUTATION_NONE = 0,
  SERIAL_MUTATION_SLEEP = 1,
  SERIAL_MUTATION_SIGNATURE = 2, // !K<id>:1 play, :0 cancel
  SERIAL_MUTATION_DECOMPRESSION = 3, // !D<id>:0..120 session minutes
  SERIAL_MUTATION_PATTERN = 4, // !P<id> ... (ADR 0084); full line via line()
  SERIAL_MUTATION_PROTECT_TEST = 5, // !R<id>:1 latch, :2 reset trial, :3 latch/sleep
  SERIAL_MUTATION_TRIGGER_SESSION = 6, // !T<id>:0..120 session minutes
  SERIAL_MUTATION_TRIGGER_PRESS = 7, // !J<id>:1 next interaction
  SERIAL_MUTATION_TRIGGER_MIN = 8, // !L<id>:mm minimum, RAM; min < max < clear
  SERIAL_MUTATION_TRIGGER_MAX = 9, // !U<id>:mm maximum, RAM
  SERIAL_MUTATION_TRIGGER_CLEAR = 10, // !V<id>:mm rearm boundary, RAM
};

struct SerialMutationCommand {
  uint8_t type;
  uint16_t value;
};

enum SerialMutationFeedResult : uint8_t {
  SERIAL_MUTATION_PASS = 0,     // byte is not part of a mutation envelope
  SERIAL_MUTATION_CONSUMED = 1, // envelope is incomplete or rejected
  SERIAL_MUTATION_READY = 2,    // out contains one validated command
};

class SerialMutationParser {
public:
  SerialMutationParser();

  SerialMutationFeedResult feed(uint8_t byte, const uint8_t myId[3],
                                SerialMutationCommand &out);
  void reset();
  bool collecting() const { return collecting_; }
  // The last READY line, without its terminator. Valid until the next feed().
  const char *line() const { return line_; }

private:
  bool parseLine(const uint8_t myId[3], SerialMutationCommand &out) const;

  // Short mutations are at most 14 bytes (!S + 6 hex + ':' + 5 digits).
  // ADR 0084 pattern lines (!P...) carry up to 32 hit offsets and stay under
  // 384 bytes; anything longer still poisons the envelope.
  char line_[384];
  uint16_t length_;
  bool collecting_;
  bool invalid_;
};
