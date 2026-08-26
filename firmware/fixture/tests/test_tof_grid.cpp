#include "test_util.h"

#include <cstring>
#include "../src/core/tof_grid.h"

// Frames mirror the vendored ULD contract (vl53l5cx_api.cpp with the local
// VL53L5CX_NB_TARGET_PER_ZONE=2 edit): per-target arrays zone-major at
// zone*targetsPerZone+target, nb_target_detected per zone, entries past
// nb_target_detected stale from earlier frames, status 255 when no target.
enum { Z = 16, TPZ = 2 };

static int16_t gDist[Z * TPZ];
static uint8_t gStat[Z * TPZ];
static uint8_t gNb[Z];
static uint16_t gZoneMm[Z];
static uint16_t gClosest;

static void clearFrame() {
  std::memset(gDist, 0, sizeof(gDist));
  std::memset(gStat, 255, sizeof(gStat));
  std::memset(gNb, 0, sizeof(gNb));
  std::memset(gZoneMm, 0xAA, sizeof(gZoneMm)); // poison: outputs must be written
  gClosest = 0xAAAA;
}

static void setTarget(int z, int t, int16_t mm, uint8_t st) {
  gDist[z * TPZ + t] = mm;
  gStat[z * TPZ + t] = st;
  if (gNb[z] < t + 1) gNb[z] = (uint8_t)(t + 1);
}

static uint8_t run() {
  return l5cxSelectGround(gDist, gStat, gNb, Z, TPZ, 50, 4000, gZoneMm,
                          &gClosest);
}

int main() {
  // Full 4x4 grid, one target per zone, unique distances -- and every stale
  // second-target slot loaded with a valid-LOOKING decoy that only the
  // nb_target_detected gate rejects. Any indexing-arithmetic error (e.g. the
  // flat [zone] read this module replaces, which walked zones 0-7 interleaved
  // with their second targets) lands on a decoy or another zone's value.
  clearFrame();
  for (int z = 0; z < Z; z++) {
    setTarget(z, 0, (int16_t)(1000 + 10 * z), 5);
    gDist[z * TPZ + 1] = (int16_t)(3000 + z); // stale slot, plausible numbers
    gStat[z * TPZ + 1] = 5;
  }
  CHECK_EQ(run(), 16u);
  {
    bool exact = true;
    for (int z = 0; z < Z; z++)
      if (gZoneMm[z] != 1000 + 10 * z) exact = false;
    CHECK(exact);
  }
  CHECK_EQ(gClosest, 1000u);

  // Occluded zone, the reason for the 2-target vendored edit: bamboo splay
  // near + floor behind it far in the SAME zone. Plane pick = far (ground),
  // closest = near (presence).
  clearFrame();
  for (int z = 0; z < Z; z++) setTarget(z, 0, 2000, 5);
  setTarget(4, 0, 260, 5);
  setTarget(4, 1, 2100, 5);
  CHECK_EQ(run(), 16u);
  CHECK_EQ(gZoneMm[4], 2100u);
  CHECK_EQ(gClosest, 260u);

  // nb_target_detected can exceed the transferred slots (device counts up to
  // 4); the scan must clamp to targetsPerZone. Unclamped, zone 3 slot t=2 is
  // zone 4's t=0 cell (3*2+2 == 4*2+0) -- the 3500 there would win the
  // farthest pick.
  clearFrame();
  setTarget(3, 0, 1200, 5);
  setTarget(3, 1, 900, 5);
  gNb[3] = 4;
  setTarget(4, 0, 3500, 5);
  CHECK_EQ(run(), 2u);
  CHECK_EQ(gZoneMm[3], 1200u);
  CHECK_EQ(gZoneMm[4], 3500u);
  CHECK_EQ(gClosest, 900u);

  // Status gate: 5 and 9 are the only ranging-OK codes; 6/10/12/255 drop.
  // Range gate is strict on both ends (>50, <4000); negatives drop.
  clearFrame();
  setTarget(0, 0, 800, 6);
  setTarget(1, 0, 800, 10);
  setTarget(2, 0, 800, 12);
  setTarget(3, 0, 800, 255);
  setTarget(4, 0, 50, 5);    // == min -> out
  setTarget(5, 0, 51, 5);    // in
  setTarget(6, 0, 3999, 9);  // in (status 9 accepted)
  setTarget(7, 0, 4000, 5);  // == max -> out
  setTarget(8, 0, -30, 5);   // raw-format negative -> out
  setTarget(9, 0, 800, 6);   // bad t0...
  setTarget(9, 1, 1700, 5);  // ...valid t1 still keeps the zone
  CHECK_EQ(run(), 3u);
  CHECK_EQ(gZoneMm[0], 0u);
  CHECK_EQ(gZoneMm[3], 0u);
  CHECK_EQ(gZoneMm[4], 0u);
  CHECK_EQ(gZoneMm[5], 51u);
  CHECK_EQ(gZoneMm[6], 3999u);
  CHECK_EQ(gZoneMm[7], 0u);
  CHECK_EQ(gZoneMm[8], 0u);
  CHECK_EQ(gZoneMm[9], 1700u);
  CHECK_EQ(gClosest, 51u);

  // Empty frame: no zones kept, every output still written.
  clearFrame();
  CHECK_EQ(run(), 0u);
  CHECK_EQ(gClosest, 0u);
  {
    bool allZero = true;
    for (int z = 0; z < Z; z++)
      if (gZoneMm[z] != 0) allZero = false;
    CHECK(allZero);
  }

  // targetsPerZone=1 (stride 1): the stride must come from the parameter,
  // not a hardwired 2.
  {
    int16_t d1[4] = {200, 60, 4000, 51};
    uint8_t s1[4] = {5, 9, 5, 5};
    uint8_t n1[4] = {1, 1, 1, 1};
    uint16_t zm1[4];
    uint16_t c1 = 0xAAAA;
    CHECK_EQ(l5cxSelectGround(d1, s1, n1, 4, 1, 50, 4000, zm1, &c1), 3u);
    CHECK_EQ(zm1[0], 200u);
    CHECK_EQ(zm1[1], 60u);
    CHECK_EQ(zm1[2], 0u);
    CHECK_EQ(zm1[3], 51u);
    CHECK_EQ(c1, 51u);
  }

  // Near-cover count is spatial: multiple close targets in one zone count
  // once, invalid/no-return cells do not count, and stale target slots remain
  // gated by nb_target_detected.
  clearFrame();
  setTarget(0, 0, 100, 5);
  setTarget(0, 1, 80, 9);   // same zone, still one
  setTarget(3, 0, 200, 9);
  setTarget(7, 0, 349, 5);
  setTarget(8, 0, 350, 5);  // strict upper edge: out
  setTarget(9, 0, 90, 6);   // invalid status
  gDist[10 * TPZ] = 70;     // valid-looking stale slot, nb=0
  gStat[10 * TPZ] = 5;
  CHECK_EQ(l5cxCountNearZones(gDist, gStat, gNb, Z, TPZ, 30, 350), 3u);

  uint16_t nearest[Z];
  CHECK_EQ(l5cxSelectNearest(gDist, gStat, gNb, Z, TPZ, 30, 4000,
                            nearest), 4u);
  CHECK_EQ(nearest[0], 80u);
  CHECK_EQ(nearest[3], 200u);
  CHECK_EQ(nearest[7], 349u);
  CHECK_EQ(nearest[8], 350u);
  CHECK_EQ(nearest[9], 0u);
  CHECK_EQ(nearest[10], 0u);

  return testReport("tof_grid");
}
