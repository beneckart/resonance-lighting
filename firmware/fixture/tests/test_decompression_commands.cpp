#include "test_util.h"
#include "../src/core/serial_mutation.h"
#include <string>

static int commands(const std::string &text, uint8_t type, uint16_t value) {
  SerialMutationParser parser;
  const uint8_t id[3] = {0x9f, 0x27, 0x38};
  int n = 0;
  for (uint8_t byte : text) {
    SerialMutationCommand out{};
    if (parser.feed(byte, id, out) == SERIAL_MUTATION_READY) {
      CHECK_EQ(out.type, type);
      CHECK_EQ(out.value, value);
      n++;
    }
  }
  return n;
}
int main() {
  CHECK_EQ(commands("!K9F2738:1\n", SERIAL_MUTATION_SIGNATURE, 1), 1);
  CHECK_EQ(commands("!K9f2738:0\r\n", SERIAL_MUTATION_SIGNATURE, 0), 1);
  CHECK_EQ(commands("!D9F2738:60\n", SERIAL_MUTATION_DECOMPRESSION, 60), 1);
  CHECK_EQ(commands("!D9F2738:120\n", SERIAL_MUTATION_DECOMPRESSION, 120), 1);
  CHECK_EQ(commands("!D9F2738:0\n", SERIAL_MUTATION_DECOMPRESSION, 0), 1);
  for (const char *bad : {"!K9F2738:1", "!KFFFFFF:1\n", "!K9F2738:2\n",
       "!K9F2738:01\n", "!D9F2738:121\n", "!D9F2738:-1\n",
       "!K9F2738:1 extra\n", "!!K9F2738:1\n", "K9F2738:1\n",
       "!D9F2738:00\n"})
    CHECK_EQ(commands(bad, 0, 0), 0);
  return testReport("decompression_commands");
}
