#include "test_util.h"
#include "../src/core/pattern_part.h"

#include <string>

int main() {
  const uint8_t me[3] = {0x9F, 0x27, 0x38};
  PatternCommand c;

  // SET: a valid two-hit part, echoed exactly.
  CHECK(patternParseLine("!P9F2738 SET 1a2b3c4d 2000000 16 12000 0,1000000", me, c));
  CHECK_EQ(c.type, PATTERN_CMD_SET);
  CHECK_EQ(c.part.patternId, 0x1A2B3C4DU);
  CHECK_EQ(c.part.loopUs, 2000000U);
  CHECK_EQ(c.part.pulseMs, 16);
  CHECK_EQ(c.part.leadUs, 12000U);
  CHECK_EQ(c.part.count, 2);
  CHECK_EQ(c.part.offsetsUs[1], 1000000U);
  CHECK(c.part.valid());

  // Silent member keeps time without hits.
  CHECK(patternParseLine("!P9f2738 SET 7 1000000 20 0 -", me, c));
  CHECK_EQ(c.part.count, 0);
  CHECK(c.part.valid());

  // CRC is stable and sensitive to every field.
  PatternPart a, b;
  CHECK(patternParseLine("!P9F2738 SET 1a2b3c4d 2000000 16 12000 0,1000000", me, c));
  a = c.part;
  b = a;
  CHECK_EQ(a.crc(), b.crc());
  b.offsetsUs[1] += 1;
  CHECK(a.crc() != b.crc());
  b = a; b.pulseMs = 17; CHECK(a.crc() != b.crc());
  b = a; b.leadUs = 0; CHECK(a.crc() != b.crc());
  // Pinned value: the tone lab recomputes this exact CRC32 in Python.
  CHECK_EQ(a.crc(), 0x2D1C4D4FU);

  // Another lantern's line is ignored without an error.
  CHECK(!patternParseLine("!P9E668C SET 1 2000000 16 0 0", me, c));
  CHECK(c.error == nullptr);

  // PLAY / STOP / CLEAR.
  CHECK(patternParseLine("!P9F2738 PLAY 1a2b3c4d 30", me, c));
  CHECK_EQ(c.type, PATTERN_CMD_PLAY);
  CHECK_EQ(c.patternId, 0x1A2B3C4DU);
  CHECK_EQ(c.minutes, 30);
  CHECK(patternParseLine("!P9F2738 STOP", me, c));
  CHECK_EQ(c.type, PATTERN_CMD_STOP);
  CHECK(patternParseLine("!P9F2738 CLEAR", me, c));
  CHECK_EQ(c.type, PATTERN_CMD_CLEAR);

  // Fail closed: each of these is rejected with a reason and nothing applied.
  const char *bad[] = {
      "!P9F2738 SET 0 2000000 16 0 0",               // zero pattern id
      "!P9F2738 SET 1 100000 16 0 0",                // loop too short
      "!P9F2738 SET 1 2000000 4 0 0",                // pulse too short
      "!P9F2738 SET 1 2000000 101 0 0",              // pulse too long
      "!P9F2738 SET 1 2000000 16 300000 0",          // lead too long
      "!P9F2738 SET 1 2000000 16 0 0,200000",        // hits 200 ms apart
      "!P9F2738 SET 1 2000000 16 0 1000000,500000",  // not increasing
      "!P9F2738 SET 1 2000000 16 0 0,1900000",       // 100 ms across the wrap
      "!P9F2738 SET 1 2000000 16 0 2000000",         // outside the loop
      "!P9F2738 SET 1 2000000 16 0 0,",              // trailing comma
      "!P9F2738 SET 1 2000000 16 0 0 extra",         // trailing text
      "!P9F2738 SET 1 2000000 16 0",                 // missing offsets
      "!P9F2738  SET 1 2000000 16 0 0",              // double space
      "!P9F2738 PLAY 1 0",                           // zero minutes
      "!P9F2738 PLAY 1 121",                         // too long
      "!P9F2738 PLAY 0 10",                          // zero id
      "!P9F2738 STOP now",                           // trailing text
      "!P9F2738 JUMP",                               // unknown command
      "!P9F273 SET 1 2000000 16 0 0",                // short id
      "!P9F2738 SET 123456789 2000000 16 0 0",       // nine hex digits
      "!P9F2738 SET 1 99999999999 16 0 0",           // overflow
  };
  for (const char *line : bad) {
    PatternCommand r;
    bool ok = patternParseLine(line, me, r);
    CHECK(!ok);
    CHECK(r.error != nullptr);
    if (ok || !r.error) std::printf("  accepted: %s\n", line);
  }

  // 32 hits fit; 33 do not.
  std::string many = "!P9F2738 SET 1 60000000 16 0 ";
  for (int i = 0; i < 33; ++i) {
    if (i) many += ",";
    many += std::to_string(i * 1000000);
  }
  CHECK(!patternParseLine(many.c_str(), me, c));
  std::string ok32 = many.substr(0, many.rfind(','));
  CHECK(patternParseLine(ok32.c_str(), me, c));
  CHECK_EQ(c.part.count, 32);
  CHECK(ok32.size() < PATTERN_LINE_MAX);

  return testReport("test_pattern_part");
}
