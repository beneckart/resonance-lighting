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

    // ADR 0084 pattern lines: READY for this lantern with the full line intact,
  // silent for another lantern, and still bounded.
  {
    const uint8_t me[3] = {0x9F, 0x27, 0x38};
    SerialMutationParser p;
    SerialMutationCommand cmd{};
    std::string line = "!P9F2738 SET 1a2b 2000000 16 0 0,1000000\n";
    int ready = 0;
    for (char ch : line) {
      if (p.feed((uint8_t)ch, me, cmd) == SERIAL_MUTATION_READY) {
        ready++;
        CHECK_EQ(cmd.type, SERIAL_MUTATION_PATTERN);
        CHECK(std::string(p.line()) == "!P9F2738 SET 1a2b 2000000 16 0 0,1000000");
      }
    }
    CHECK_EQ(ready, 1);
    std::string other = "!P9E668C SET 1a2b 2000000 16 0 0\n";
    ready = 0;
    for (char ch : other) ready += p.feed((uint8_t)ch, me, cmd) == SERIAL_MUTATION_READY;
    CHECK_EQ(ready, 0);
    // A short mutation still works right after a pattern line.
    std::string sig = "!K9F2738:1\n";
    ready = 0;
    for (char ch : sig)
      if (p.feed((uint8_t)ch, me, cmd) == SERIAL_MUTATION_READY) {
        ready++;
        CHECK_EQ(cmd.type, SERIAL_MUTATION_SIGNATURE);
      }
    CHECK_EQ(ready, 1);
    std::string huge = "!P9F2738 SET 1 2000000 16 0 " + std::string(400, '1') + "\n";
    ready = 0;
    for (char ch : huge) ready += p.feed((uint8_t)ch, me, cmd) == SERIAL_MUTATION_READY;
    CHECK_EQ(ready, 0);
  }

  // Range commands share framing but retain separate setters. Relational
  // validation (min < max < clear) belongs to the live presence state machine.
  {
    const uint8_t marill[3] = {0xF4, 0x03, 0x48};
    const char *lines[] = {"!LF40348:50\n", "!UF40348:400\n", "!VF40348:600\n",
                          "!VF40348:4000\n"};
    const uint16_t values[] = {50,400,600,4000};
    const SerialMutationType types[] = {SERIAL_MUTATION_TRIGGER_MIN,
      SERIAL_MUTATION_TRIGGER_MAX, SERIAL_MUTATION_TRIGGER_CLEAR, SERIAL_MUTATION_TRIGGER_CLEAR};
    for (unsigned i=0;i<4;++i) {
      SerialMutationParser p;
      SerialMutationCommand cmd{};
      unsigned ready=0;
      for (const char *s=lines[i];*s;++s)
        if (p.feed(*s,marill,cmd)==SERIAL_MUTATION_READY) {
          ++ready; CHECK_EQ(cmd.type,types[i]); CHECK_EQ(cmd.value,values[i]);
        }
      CHECK_EQ(ready,1u);
    }
    const char *bad[] = {"!UF40348:29\n", "!VF40348:4001\n", "!UF40349:400\n",
                        "!VF40348:0600\n", "!UF40348:400x\n"};
    for (const char *s:bad) {
      SerialMutationParser p;
      SerialMutationCommand cmd{};
      unsigned ready=0;
      for (;*s;++s) ready += p.feed(*s,marill,cmd)==SERIAL_MUTATION_READY;
      CHECK_EQ(ready,0u);
    }
  }
  return testReport("serial_mutation");
}
