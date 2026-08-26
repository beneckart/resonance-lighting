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
  // !S + six hex digits + ':' + one-to-five canonical decimal digits.
  if (invalid_ || length_ < 10 || length_ > 14) return false;
  if (line_[0] != '!' || line_[1] != 'S' || line_[8] != ':') return false;

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

  // Reject zero and non-canonical leading-zero forms. Besides removing
  // ambiguity, this guarantees that every accepted duration was explicit.
  if (line_[9] < '1' || line_[9] > '9') return false;
  uint32_t seconds = 0;
  for (size_t i = 9; i < length_; i++) {
    if (line_[i] < '0' || line_[i] > '9') return false;
    seconds = seconds * 10U + (uint32_t)(line_[i] - '0');
    if (seconds > 65535U) return false;
  }

  out.type = SERIAL_MUTATION_SLEEP;
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
    length_ = 1;
    return SERIAL_MUTATION_CONSUMED;
  }

  if (byte == '\r' || byte == '\n') {
    bool ready = parseLine(myId, out);
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
