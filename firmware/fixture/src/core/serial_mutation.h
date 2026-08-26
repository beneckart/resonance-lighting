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

private:
  bool parseLine(const uint8_t myId[3], SerialMutationCommand &out) const;

  // Longest current line is 14 bytes (!S + 6 hex + ':' + 5 digits). Keep
  // bounded headroom for future explicit mutations without accepting an
  // arbitrary host stream.
  char line_[32];
  uint8_t length_;
  bool collecting_;
  bool invalid_;
};
