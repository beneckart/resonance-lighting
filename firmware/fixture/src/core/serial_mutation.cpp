#include "serial_mutation.h"

#include <string.h>

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

SerialMutationParser::SerialMutationParser() { reset(); }

void SerialMutationParser::reset() {
  memset(line_, 0, sizeof(line_));
  length_ = 0;
  collecting_ = false;
  invalid_ = false;
}

bool SerialMutationParser::parseLine(const uint8_t myId[3],
                                     SerialMutationCommand &out) const {
  // Sentinel + verb + six hex digits + ':' + canonical decimal value.
  if (invalid_) return false;
  if (line_[0] == '!' && line_[1] == 'P' && length_ >= 10 && line_[8] == ' ') {
    // Pattern lines are parsed by pattern_part; here only the exact target is
    // checked so other lanterns' lines stay silent.
    uint8_t target[3] = {0, 0, 0};
    for (size_t i = 0; i < 6; i++) {
      int nibble = hexNibble(line_[2 + i]);
      if (nibble < 0) return false;
      if ((i & 1U) == 0) target[i / 2] = (uint8_t)(nibble << 4);
      else target[i / 2] |= (uint8_t)nibble;
    }
    if (memcmp(target, myId, sizeof(target)) != 0) return false;
    out.type = SERIAL_MUTATION_PATTERN;
    out.value = 0;
    return true;
  }
  if (length_ < 10 || length_ > 14) return false;
  if (line_[0] != '!' || line_[8] != ':') return false;
  uint8_t type = line_[1] == 'S' ? SERIAL_MUTATION_SLEEP :
                 line_[1] == 'K' ? SERIAL_MUTATION_SIGNATURE :
                 line_[1] == 'D' ? SERIAL_MUTATION_DECOMPRESSION :
                 line_[1] == 'R' ? SERIAL_MUTATION_RELAY :
                 SERIAL_MUTATION_NONE;
  if (type == SERIAL_MUTATION_NONE) return false;

  uint8_t target[3] = {0, 0, 0};
  for (size_t i = 0; i < 6; i++) {
    int nibble = hexNibble(line_[2 + i]);
    if (nibble < 0) return false;
    if ((i & 1U) == 0)
      target[i / 2] = (uint8_t)(nibble << 4);
    else
      target[i / 2] |= (uint8_t)nibble;
  }
  if (memcmp(target, myId, sizeof(target)) != 0) return false;

  // Reject non-canonical leading zeros; zero is explicit cancel/end for K/D.
  if (line_[9] < '0' || line_[9] > '9') return false;
  if (line_[9] == '0' && (length_ != 10 || type == SERIAL_MUTATION_SLEEP))
    return false;
  uint32_t seconds = 0;
  for (size_t i = 9; i < length_; i++) {
    if (line_[i] < '0' || line_[i] > '9') return false;
    seconds = seconds * 10U + (uint32_t)(line_[i] - '0');
    if (seconds > 65535U) return false;
  }

  if ((type == SERIAL_MUTATION_SIGNATURE || type == SERIAL_MUTATION_RELAY) && seconds > 1) return false;
  if (type == SERIAL_MUTATION_DECOMPRESSION && seconds > 120) return false;
  out.type = type;
  out.value = (uint16_t)seconds;
  return true;
}

SerialMutationFeedResult
SerialMutationParser::feed(uint8_t byte, const uint8_t myId[3],
                           SerialMutationCommand &out) {
  out.type = SERIAL_MUTATION_NONE;
  out.value = 0;

  if (!collecting_) {
    if (byte != '!') return SERIAL_MUTATION_PASS;
    collecting_ = true;
    line_[0] = '!';
    line_[1] = 0;
    length_ = 1;
    return SERIAL_MUTATION_CONSUMED;
  }

  if (byte == '\r' || byte == '\n') {
    bool ready = parseLine(myId, out);
    if (ready && out.type == SERIAL_MUTATION_PATTERN) {
      // Keep the text for line(); the next '!' overwrites it from the start.
      collecting_ = false;
      invalid_ = false;
      length_ = 0;
      return SERIAL_MUTATION_READY;
    }
    reset();
    return ready ? SERIAL_MUTATION_READY : SERIAL_MUTATION_CONSUMED;
  }

  // A second sentinel cannot smuggle a valid suffix out of an invalid line.
  // Mark the whole line bad and wait for its terminator to resynchronize.
  if (byte < 0x20 || byte > 0x7E || byte == '!') {
    invalid_ = true;
    return SERIAL_MUTATION_CONSUMED;
  }

  if ((size_t)length_ + 1 >= sizeof(line_)) {
    invalid_ = true;
    return SERIAL_MUTATION_CONSUMED;
  }
  line_[length_++] = (char)byte;
  line_[length_] = 0;
  return SERIAL_MUTATION_CONSUMED;
}
