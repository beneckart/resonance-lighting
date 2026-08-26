#include "test_util.h"

#include <cstdint>
#include <string>

#include "../src/core/serial_mutation.h"

static int feed(SerialMutationParser &parser, const std::string &bytes,
                const uint8_t id[3], uint16_t *lastValue = nullptr) {
  int ready = 0;
  for (unsigned char byte : bytes) {
    SerialMutationCommand command{};
    if (parser.feed(byte, id, command) == SERIAL_MUTATION_READY) {
      CHECK_EQ(command.type, SERIAL_MUTATION_SLEEP);
      ready++;
      if (lastValue) *lastValue = command.value;
    }
  }
  return ready;
}

int main() {
  const uint8_t akuma[3] = {0x9E, 0x66, 0x8C};
  uint16_t value = 0;

  {
    SerialMutationParser p;
    CHECK_EQ(feed(p, "!S9E668C:10\n", akuma, &value), 1);
    CHECK_EQ(value, 10);
    CHECK(!p.collecting());
  }
  {
    SerialMutationParser p;
    CHECK_EQ(feed(p, "!S9e668c:65535\r", akuma, &value), 1);
    CHECK_EQ(value, 65535);
  }

  // The historical hazard: a lone S, S1, or arbitrary uppercase-S boot/probe
  // chatter must never request the old implicit six-hour sleep.
  {
    SerialMutationParser p;
    CHECK_EQ(feed(p, "S\nS1\nS10\nSaved PC:0x4037b126\r\nSPIWP:0xee\r\n", akuma), 0);
  }
  {
    SerialMutationParser p;
    CHECK_EQ(feed(p, "!S9E668C:10", akuma), 0); // no terminator, no action
    CHECK(p.collecting());
    CHECK_EQ(feed(p, "\n", akuma, &value), 1);
    CHECK_EQ(value, 10);
  }

  const char *invalid[] = {
      "!S9E668C:\n",       "!S9E668C:0\n",      "!S9E668C:01\n",
      "!S9E668C:65536\n",  "!S9E668C:-1\n",     "!S9E668C:1x\n",
      "!S9E668:10\n",      "!S9E668CC:10\n",    "!s9E668C:10\n",
      "!SFFFFFF:10\n",     "!S9E668C,10\n",     "!S9E668C:10 extra\n",
      "!!S9E668C:10\n",    "!garbageS9E668C:10\n",
  };
  for (const char *candidate : invalid) {
    SerialMutationParser p;
    CHECK_EQ(feed(p, candidate, akuma), 0);
  }

  // Nonprintable and overlong input poison the whole envelope, after which a
  // newline (or CR) restores clean parsing for the next explicit command.
  {
    SerialMutationParser p;
    std::string bad = "!S9E";
    bad.push_back('\0');
    bad += "668C:10\n!S9E668C:2\n";
    CHECK_EQ(feed(p, bad, akuma, &value), 1);
    CHECK_EQ(value, 2);
  }
  {
    SerialMutationParser p;
    std::string bad = "!" + std::string(100, 'A') + "\n!S9E668C:3\n";
    CHECK_EQ(feed(p, bad, akuma, &value), 1);
    CHECK_EQ(value, 3);
  }

  // Deterministic byte-stream fuzz: driver/boot noise may contain every byte,
  // including S and !, but cannot produce an accepted command accidentally.
  {
    SerialMutationParser p;
    uint32_t state = 0x5A17C0DEU;
    int commands = 0;
    for (int i = 0; i < 250000; i++) {
      state = state * 1664525U + 1013904223U;
      uint8_t byte = (uint8_t)(state >> 24);
      SerialMutationCommand command{};
      if (p.feed(byte, akuma, command) == SERIAL_MUTATION_READY) commands++;
    }
    CHECK_EQ(commands, 0);
    CHECK_EQ(feed(p, "\n!S9E668C:4\n", akuma, &value), 1);
    CHECK_EQ(value, 4);
  }

  return testReport("serial_mutation");
}
