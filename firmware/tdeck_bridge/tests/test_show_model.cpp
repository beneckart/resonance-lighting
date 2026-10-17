#include <stdio.h>
#include <string.h>
#include <string>

#include "core/click_model.h"
#include "core/show_model.h"

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                    \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static std::string seal(const std::string &body) {
  char end[24];
  snprintf(end, sizeof(end), "END|%08X\n", (unsigned)showCrc32((const uint8_t *)body.data(), body.size()));
  return body + end;
}

static Show gShow;  // large: keep off the stack

int main() {
  CHECK(showCrc32((const uint8_t *)"123456789", 9) == 0xCBF43926u);  // zlib.crc32

  const std::string body =
      "TLSHOW|1|2026-10-17T00:15:33.564Z|Brian bench\n"
      "CFG|11|250\n"
      "L|F40254|1|16|0|L|Leeloo\n"
      "L|F2BE0C|6|13|2|L|Sakura\n"
      "L|F401DC|11|40|0|P|Zubat\n"
      "P|pat-b|1|300|32|4|Freakout Mode\n"
      "H|pat-b|F40254|25,0,5\n"
      "H|pat-b|F2BE0C|5,10\n"
      "P|pat-a|0|50|32|8|Beat 1\n"
      "H|pat-a|F2BE0C|6\n"
      "P|pat-c|1|100|16|4|Cha Cha\n"
      "H|pat-c|F40254|0\n"
      "B|ix-1|1|any|single|performance|pat-b|2|Play performance\n"
      "B|ix-2|1|1B636C|double|next_active_pattern||2|Next active\n"
      "A|1B636C|Red button\n";
  const std::string text = seal(body);
  char err[96] = {};
  CHECK(showParse(text.data(), text.size(), gShow, err, sizeof(err)));
  CHECK(gShow.lanternCount == 3 && gShow.patternCount == 3 && gShow.ruleCount == 2);
  CHECK(gShow.channel == 11 && gShow.minSpacingMs == 250);
  CHECK(gShow.lanterns[2].perimeter && gShow.lanterns[1].pulseMs == 13);
  // Hits grouped per pattern and sorted by step.
  const ShowPattern &fm = gShow.patterns[0];
  CHECK(fm.hitCount == 5);
  CHECK(gShow.hits[fm.firstHit].step == 0 && gShow.hits[fm.firstHit + 4].step == 25);
  CHECK(gShow.hits[fm.firstHit + 1].step == 5 && gShow.hits[fm.firstHit + 2].step == 5);
  CHECK(showStepMs(fm) > 49.9f && showStepMs(fm) < 50.1f);   // 300 BPM, 4 per beat
  CHECK(showLoopMs(fm) > 1599.0f && showLoopMs(fm) < 1601.0f);
  // Rules resolve patterns and Atoms.
  CHECK(gShow.rules[0].action == ACT_PERFORMANCE && gShow.rules[0].pattern == 0 && gShow.rules[0].loops == 2);
  CHECK(gShow.rules[0].anyAtom && gShow.rules[0].click == CLICK_SINGLE);
  CHECK(!gShow.rules[1].anyAtom && gShow.rules[1].atom[0] == 0x1B && gShow.rules[1].click == CLICK_DOUBLE);
  // Active rotation in name order, skipping the inactive pattern: Cha Cha -> Freakout Mode -> Cha Cha.
  int a = showNextActive(gShow, -1);
  CHECK(a == 2);
  CHECK(showNextActive(gShow, 2) == 0);
  CHECK(showNextActive(gShow, 0) == 2);
  CHECK(showNextActive(gShow, 1) == 2);  // inactive current: first Active

  // Damage is caught.
  std::string bad = text;
  bad[bad.find("Leeloo")] = 'X';
  CHECK(!showParse(bad.data(), bad.size(), gShow, err, sizeof(err)));
  CHECK(strstr(err, "checksum") != nullptr);
  CHECK(!showParse(body.data(), body.size(), gShow, err, sizeof(err)));  // no END
  // Unknown pattern in a rule is refused, not silently dropped.
  const std::string badRule = seal("TLSHOW|1|x|y\nP|p1|1|60|4|1|A\nB|r|1|any|single|play_pattern|nope|1|R\n");
  CHECK(!showParse(badRule.data(), badRule.size(), gShow, err, sizeof(err)));
  // Spacing/timing limits.
  const std::string badBpm = seal("TLSHOW|1|x|y\nP|p1|1|900|4|1|A\n");
  CHECK(!showParse(badBpm.data(), badBpm.size(), gShow, err, sizeof(err)));

  // ---- Atom clicks ----
  AtomClicks c;
  ClickEvent ev = {};
  const uint8_t atom[3] = {0x1B, 0x63, 0x6C};
  // Single: one press (many copies), settles after the window.
  CHECK(c.observe(atom, 7, 10, 1000, &ev) && ev.clicks == 0);
  CHECK(!c.observe(atom, 7, 10, 1003, &ev));  // copy of the same press
  CHECK(!c.tick(1500, &ev));
  CHECK(c.tick(1601, &ev) && ev.clicks == 1);
  // Double: two presses 150 ms apart.
  CHECK(c.observe(atom, 7, 11, 3000, &ev));
  CHECK(c.observe(atom, 7, 12, 3150, &ev) && ev.clicks == 0);
  CHECK(!c.tick(3700, &ev));
  CHECK(c.tick(3751, &ev) && ev.clicks == 2);
  // Triple fires on the third press without waiting.
  CHECK(c.observe(atom, 7, 13, 5000, &ev));
  CHECK(c.observe(atom, 7, 14, 5120, &ev));
  CHECK(c.observe(atom, 7, 15, 5240, &ev) && ev.clicks == 3);
  CHECK(!c.tick(9000, &ev));
  // A new controller boot reusing an old seq is a new press.
  CHECK(c.observe(atom, 8, 10, 10000, &ev));
  CHECK(c.tick(10601, &ev) && ev.clicks == 1);
  // A press arriving after an unsettled window settles the old click first.
  CHECK(c.observe(atom, 8, 11, 20000, &ev));
  CHECK(c.observe(atom, 8, 12, 20900, &ev) && ev.clicks == 1);
  CHECK(c.tick(21501, &ev) && ev.clicks == 1);
  return 0;
}
