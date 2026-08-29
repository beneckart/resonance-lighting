#include "test_util.h"

#include <cstring>
#include "../src/core/presence_wave.h"

static bool observe2(TmfPresenceGate &gate, uint32_t seq,
                     uint16_t zone0Mm, uint16_t zone0Confidence,
                     uint16_t zone1Mm, uint16_t zone1Confidence) {
  uint16_t mm[PRESENCE_ZONE_COUNT] = {};
  uint16_t confidence[PRESENCE_ZONE_COUNT] = {};
  mm[0] = zone0Mm;
  confidence[0] = zone0Confidence;
  mm[1] = zone1Mm;
  confidence[1] = zone1Confidence;
  return tmfPresenceObserve(gate, seq, mm, confidence);
}

int main() {
  // A stable close rig return in zone 0 warms up without firing. Movement in
  // another zone is compared with that zone's own background, so the rig does
  // not either auto-fire the detector or hide the person behind a scalar min.
  TmfPresenceGate gate;
  tmfPresenceInit(gate);
  uint32_t seq = 0;
  for (int i = 0; i < PRESENCE_WARMUP_READS; ++i)
    CHECK(!observe2(gate, ++seq, (i % 5) ? 300 : 1800, 50, 1800, 50));
  // Intermittently losing and reacquiring the close rig return is not a hit.
  CHECK(!observe2(gate, ++seq, 1800, 50, 1800, 50));
  CHECK(!observe2(gate, ++seq, 300, 50, 1800, 50));
  CHECK(!observe2(gate, ++seq, 300, 50, 1400, 50));
  CHECK(!observe2(gate, ++seq, 300, 50, 1390, 50));
  CHECK(observe2(gate, ++seq, 300, 50, 1380, 50));
  CHECK(!observe2(gate, seq, 300, 50, 1390, 50)); // same report ignored
  CHECK(!observe2(gate, ++seq, 300, 50, 1370, 50));
  for (int i = 0; i < PRESENCE_CLEAR_READS; ++i)
    CHECK(!observe2(gate, ++seq, 300, 50, 1800, 50));
  CHECK(!observe2(gate, ++seq, 300, 50, 1300, 50));
  CHECK(!observe2(gate, ++seq, 300, 50, 1290, 50));
  CHECK(observe2(gate, ++seq, 300, 50, 1280, 50));

  // An empty scene can warm up with no baseline. A confident return inside
  // the absolute demo range is then presence; confidence-zero never is.
  tmfPresenceInit(gate);
  seq = 0;
  for (int i = 0; i < PRESENCE_WARMUP_READS; ++i)
    CHECK(!observe2(gate, ++seq, 0, 0, 0, 0));
  CHECK(!observe2(gate, ++seq, 900, 0, 0, 0));
  CHECK(!observe2(gate, ++seq, 900, 50, 0, 0));
  CHECK(!observe2(gate, ++seq, 910, 50, 0, 0));
  CHECK(observe2(gate, ++seq, 920, 50, 0, 0));

  // Fifteen-foot canopy geometry: ground can be near the 5 m sensor limit,
  // while a person's head is typically around 3 m away. A fixture that warmed
  // up against a close bin wall must follow the new far background, then fire
  // on the persistent closer head return.
  tmfPresenceInit(gate);
  seq = 0;
  for (int i = 0; i < PRESENCE_WARMUP_READS; ++i)
    CHECK(!observe2(gate, ++seq, 200, 50, 0, 0));
  for (int i = 0; i < 48; ++i)
    CHECK(!observe2(gate, ++seq, 4600, 50, 0, 0));
  CHECK(!observe2(gate, ++seq, 3000, 50, 0, 0));
  CHECK(!observe2(gate, ++seq, 3010, 50, 0, 0));
  CHECK(observe2(gate, ++seq, 2990, 50, 0, 0));

  // If the 15 ft ground produces no confident return, clear the stale bin
  // baseline. A later head return is presence even without a ground baseline.
  tmfPresenceInit(gate);
  seq = 0;
  for (int i = 0; i < PRESENCE_WARMUP_READS; ++i)
    CHECK(!observe2(gate, ++seq, 200, 50, 0, 0));
  for (int i = 0; i < PRESENCE_EMPTY_REBASE_READS; ++i)
    CHECK(!observe2(gate, ++seq, 0, 0, 0, 0));
  CHECK(!observe2(gate, ++seq, 3100, 50, 0, 0));
  CHECK(!observe2(gate, ++seq, 3090, 50, 0, 0));
  CHECK(observe2(gate, ++seq, 3110, 50, 0, 0));

  // Two unrelated one-frame glitches in different zones do not combine into
  // presence; the same changed zone must persist for the second report.
  tmfPresenceInit(gate);
  seq = 0;
  for (int i = 0; i < PRESENCE_WARMUP_READS; ++i)
    CHECK(!observe2(gate, ++seq, 0, 0, 0, 0));
  CHECK(!observe2(gate, ++seq, 900, 50, 0, 0));
  CHECK(!observe2(gate, ++seq, 0, 0, 900, 50));
  CHECK(!observe2(gate, ++seq, 0, 0, 910, 50));
  CHECK(observe2(gate, ++seq, 0, 0, 920, 50));

  // Perimeter cover gesture: one close zone or one-frame spatial noise is not
  // enough. Two fresh frames with >=4 close zones fire once, a held hand does
  // not repeat, and four clear frames re-arm. Duplicate reports are ignored.
  Vl53CoverGate cover;
  vl53CoverInit(cover);
  CHECK(!vl53CoverObserve(cover, 1, 1));
  CHECK(!vl53CoverObserve(cover, 2, 4));
  CHECK(!vl53CoverObserve(cover, 3, 2));
  CHECK(!vl53CoverObserve(cover, 4, 5));
  CHECK(vl53CoverObserve(cover, 5, 6));
  CHECK(!vl53CoverObserve(cover, 5, 0)); // duplicate sequence
  CHECK(!vl53CoverObserve(cover, 6, 8));
  CHECK(!vl53CoverObserve(cover, 7, 0));
  CHECK(!vl53CoverObserve(cover, 8, 0));
  CHECK(!vl53CoverObserve(cover, 9, 0));
  CHECK(!vl53CoverObserve(cover, 10, 0));
  CHECK(!vl53CoverObserve(cover, 11, 4));
  CHECK(vl53CoverObserve(cover, 12, 4));

  uint8_t visited[4][3] = {{1, 2, 3}, {4, 5, 6}};
  uint8_t yes[3] = {4, 5, 6};
  uint8_t no[3] = {7, 8, 9};
  CHECK(waveIdSeen(visited, 2, yes));
  CHECK(!waveIdSeen(visited, 2, no));

  uint8_t r, g, b;
  waveHueToRgb(0, 96, r, g, b);
  CHECK_EQ(r, 96u);
  CHECK_EQ(g, 0u);
  CHECK_EQ(b, 0u);
  waveHueToRgb(86, 96, r, g, b);
  CHECK_EQ(r, 0u);
  CHECK_EQ(g, 96u);

  return testReport("test_presence_wave");
}
