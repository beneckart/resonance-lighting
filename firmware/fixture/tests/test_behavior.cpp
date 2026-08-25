// Behavior-layer natives: GH CA wave propagation on a synthetic line graph,
// lease grant/expiry/fallback, lifecycle transitions incl. bounded night and
// the dev/prod postures, neighbor table modes.
#include "test_util.h"

#include <cstring>
#include "../src/core/choreo/program.h"
#include "../src/core/lifecycle.h"
#include "../src/core/neighbor_table.h"
#include "../src/core/packet.h"

// --- tiny 5-node line-graph GH simulation -----------------------------------
// Node i's neighbors are i-1 and i+1. We run 5 ChoreoRuntime-less GH programs
// directly through the Program interface, exchanging txState via views.
Program *newProgGhCa();

struct SimNode {
  ProgramOutputs out;
  uint8_t state = 0;
};

int main() {
  // GH: wave injected at node 0 must visit all 5 nodes; a removed node stops
  // the wave locally (organic decay, no crash/blank).
  {
    // Five independent GH instances is not possible with the function-local
    // singleton, so simulate the GH RULE directly through one instance per
    // step: instead, exercise the single instance's neighbor response.
    Program *gh = newProgGhCa();
    uint8_t params[8] = {1, 0, 3, 1, 100, 0, 0, 0}; // K=1, p=0 (no sparks), refr=3, tick=100ms
    gh->reset(42, params, FIXTURE_DOWNLIGHT, 1);

    // No neighbors, p=0: stays quiescent forever.
    ProgramInputs in = {};
    in.fixtureClass = FIXTURE_DOWNLIGHT;
    in.pixelCount = 1;
    ProgramOutputs out = {};
    uint32_t t = 1000;
    for (int i = 0; i < 50; i++) {
      in.nowMs = (t += 100);
      gh->tick(in, out);
    }
    CHECK_EQ(out.txState, 0u);

    // An excited fresh neighbor appears: next step excites us (K=1).
    NeighborView nv = {};
    nv.state = 1;
    nv.ageMs = 50;
    in.neighbors = &nv;
    in.neighborCount = 1;
    bool sawEdge = false;
    for (int i = 0; i < 3 && !sawEdge; i++) {
      in.nowMs = (t += 100);
      gh->tick(in, out);
      if (out.sendNow) sawEdge = true;
    }
    CHECK(sawEdge);          // excitation announced (edge-triggered send)
    CHECK_EQ(out.txState, 1u);

    // Then excited -> refractory countdown -> quiescent; refractory ignores
    // the still-excited neighbor (wavefronts annihilate, don't echo).
    uint8_t seen[8] = {};
    int idx = 0;
    for (int i = 0; i < 6 && idx < 8; i++) {
      in.nowMs = (t += 100);
      gh->tick(in, out);
      if (idx == 0 || out.txState != seen[idx - 1]) seen[idx++] = out.txState;
    }
    // Expect a descent through refractory states back to 0 or re-excite ONLY
    // after full recovery; with the neighbor still excited it may re-excite at
    // 0, but never from a refractory state.
    bool refractoryJump = false;
    for (int i = 1; i < idx; i++)
      if (seen[i] == 1 && seen[i - 1] >= 2) refractoryJump = true;
    CHECK(!refractoryJump);
  }

  // --- neighbor table: RSSI mode picks strongest fresh; pinned overrides ----
  {
    NeighborTable t;
    neighborTableInit(t);
    uint8_t a[3] = {1, 0, 0}, b[3] = {2, 0, 0}, c[3] = {3, 0, 0};
    neighborUpsert(t, a, 1000, -80)->choreoState = 1;
    neighborUpsert(t, b, 1000, -50)->choreoState = 2;
    neighborUpsert(t, c, 1000, -60)->choreoState = 3;
    NeighborView v[8];
    uint8_t n = neighborSnapshot(t, 1500, 3000, v, 2);
    CHECK_EQ(n, 2u);
    CHECK_EQ(v[0].state, 2u); // -50 strongest
    CHECK_EQ(v[1].state, 3u); // -60 second
    neighborUpsert(t, b, 1500, -50)->flags = 0x08;
    n = neighborSnapshot(t, 1501, 3000, v, 2);
    CHECK_EQ(v[0].flags, 0x08u);
    // Stale entries drop out.
    n = neighborSnapshot(t, 10000, 3000, v, 8);
    CHECK_EQ(n, 0u);
    // Pinned mode: only the pinned id, when fresh.
    uint8_t pins[1][3] = {{1, 0, 0}};
    neighborSetPinned(t, pins, 1);
    neighborUpsert(t, a, 20000, -80);
    neighborUpsert(t, b, 20000, -50);
    n = neighborSnapshot(t, 20100, 3000, v, 8);
    CHECK_EQ(n, 1u);
    CHECK_EQ(v[0].id[0], 1u);
    // A locate survey deliberately ignores the CA pin and sees the full heard
    // roster, still ordered strongest-first.
    n = neighborSurveySnapshot(t, 20100, 3000, v, 8);
    CHECK_EQ(n, 2u);
    CHECK_EQ(v[0].id[0], 2u);
    neighborClearPinned(t);
    n = neighborSnapshot(t, 20100, 3000, v, 8);
    CHECK_EQ(n, 2u); // back to RSSI mode
  }

  // --- neighbor eviction hysteresis ----------------------------------------
  {
    NeighborTable t;
    neighborTableInit(t);
    for (int i = 0; i < NEIGHBOR_TABLE_SIZE; i++) {
      uint8_t id[3] = {(uint8_t)(i + 1), 0, 0};
      neighborUpsert(t, id, 1000, -60);
    }
    uint8_t weakNew[3] = {0xFE, 1, 0};
    CHECK(neighborUpsert(t, weakNew, 1001, -58) == nullptr); // <6 dB stronger: rejected
    uint8_t strongNew[3] = {0xFE, 2, 0};
    CHECK(neighborUpsert(t, strongNew, 1002, -50) != nullptr); // >6 dB: evicts
  }

  // --- lifecycle: dusk/dawn, bounded night, energy gating, commissioning ----
  {
    LifeConfig prod = lifeConfigDefaults(false);
    LifeState_t st;
    lifeInit(st);
    LifeInputs in = {};
    in.rxHoldMs = 600000;
    in.awakeGraceUntilMs = 600000; // 10 min boot window
    in.forceNight = -1;
    uint32_t t = 1000;
    LifeOutputs o = {};

    // Day with supply: 30 min absence required for dusk; surplus -> ACTIVE.
    in.supplyGood = true;
    in.supplyMa = 400;
    in.battV = 3.35f;
    for (int i = 0; i < 61; i++) {
      in.nowMs = (t += 1000);
      o = lifeTick(st, in, prod);
    }
    CHECK_EQ(o.state, (uint8_t)LIFE_DAY_ACTIVE);
    CHECK(o.strikesAllowed); // tier FULL (0), surplus
    // Supply dies: 30 min sustained absence -> NIGHT.
    in.supplyGood = false;
    in.supplyMa = 0;
    in.battV = 3.28f;
    for (int i = 0; i < 301; i++) { in.nowMs = (t += 1000); o = lifeTick(st, in, prod); }
    CHECK_EQ(o.state, (uint8_t)LIFE_DAY_CHARGE); // fell back first (no surplus)
    for (int i = 0; i < 1500; i++) { in.nowMs = (t += 1000); o = lifeTick(st, in, prod); }
    CHECK_EQ(o.state, (uint8_t)LIFE_NIGHT_SHOW);
    CHECK(o.showActive);
    CHECK(!o.strikesAllowed); // never at night
    // Power veto: OFF tier suppresses the show but not the night state.
    in.tier = 2;
    in.nowMs = (t += 1000);
    o = lifeTick(st, in, prod);
    CHECK_EQ(o.state, (uint8_t)LIFE_NIGHT_SHOW);
    CHECK(!o.showActive);
    in.tier = 0;
    // Bounded night: force-exit at nightMaxMin and latch until day evidence.
    uint32_t nightMs = (uint32_t)prod.nightMaxMin * 60000UL;
    in.nowMs = (t += nightMs);
    o = lifeTick(st, in, prod);
    CHECK_EQ(o.state, (uint8_t)LIFE_DAY_CHARGE);
    // Still dark: must NOT re-enter night (latch).
    for (int i = 0; i < 2000; i++) { in.nowMs = (t += 1000); o = lifeTick(st, in, prod); }
    CHECK_EQ(o.state, (uint8_t)LIFE_DAY_CHARGE);
    // Prod day-charge with no rx and grace elapsed: wants the 300 s sleep.
    in.lastRxMs = 0;
    CHECK(o.wantSleep);
    CHECK_EQ(o.sleepS, 300u);
    // Heard the bridge 1 s ago: held awake.
    in.lastRxMs = in.nowMs - 1000;
    in.nowMs = (t += 1000);
    o = lifeTick(st, in, prod);
    CHECK(!o.wantSleep);
    // Morning: day evidence clears the latch and dawn logic resumes.
    in.supplyGood = true;
    in.supplyMa = 300;
    in.nowMs = (t += 1000);
    o = lifeTick(st, in, prod);
    CHECK_EQ(o.state, (uint8_t)LIFE_DAY_CHARGE);
  }

  {
    // Scheduled/explicit day suppresses dusk but must not freeze a field
    // fixture in DAY_CHARGE. Ordinary surplus confirmation still earns
    // DAY_ACTIVE and the existing strike gate.
    LifeConfig prod = lifeConfigDefaults(false);
    LifeState_t st;
    lifeInit(st);
    LifeInputs in = {};
    in.forceNight = 0;
    in.supplyGood = true;
    in.supplyMa = 300;
    in.battV = 3.35f;
    in.tier = 0;
    in.rxHoldMs = 600000;
    uint32_t t = 1000;
    LifeOutputs o = {};
    for (int i = 0; i < 61; ++i) {
      in.nowMs = (t += 1000);
      o = lifeTick(st, in, prod);
    }
    CHECK_EQ(o.state, (uint8_t)LIFE_DAY_ACTIVE);
    CHECK(o.strikesAllowed);

    in.forceNight = 1;
    in.nowMs = (t += 1000);
    o = lifeTick(st, in, prod);
    CHECK_EQ(o.state, (uint8_t)LIFE_NIGHT_SHOW);

    in.forceNight = 0;
    in.nowMs = (t += 1000);
    o = lifeTick(st, in, prod);
    CHECK_EQ(o.state, (uint8_t)LIFE_DAY_CHARGE);
    for (int i = 0; i < 61; ++i) {
      in.nowMs = (t += 1000);
      o = lifeTick(st, in, prod);
    }
    CHECK_EQ(o.state, (uint8_t)LIFE_DAY_ACTIVE);
    CHECK(o.strikesAllowed);
  }

  {
    // Dev's wire-stable value is COMMISSION: no inferred dusk/autonomy and no
    // sleep. The command runtime is available, with power tiers still vetoing.
    LifeConfig dev = lifeConfigDefaults(true);
    LifeState_t st;
    lifeInit(st);
    LifeInputs in = {};
    in.rxHoldMs = 600000;
    in.awakeGraceUntilMs = 0;
    in.forceNight = -1;
    in.supplyGood = false;
    in.supplyMa = 0;
    in.battV = 3.3f;
    uint32_t t = 1000;
    LifeOutputs o = {};
    for (int i = 0; i < 61; i++) { in.nowMs = (t += 1000); o = lifeTick(st, in, dev); }
    CHECK_EQ(o.state, (uint8_t)LIFE_COMMISSION);
    CHECK(o.showActive);
    CHECK(!o.strikesAllowed);
    CHECK(!o.wantSleep);
    // Solar/force-night controls cannot silently turn commissioning into an
    // autonomous lifecycle; bridge leases are the only artistic authority.
    in.forceNight = 0;
    in.nowMs = (t += 1000);
    o = lifeTick(st, in, dev);
    CHECK_EQ(o.state, (uint8_t)LIFE_COMMISSION);
    CHECK(!o.wantSleep);
    in.tier = 2;
    in.nowMs = (t += 1000);
    o = lifeTick(st, in, dev);
    CHECK(!o.showActive); // OFF/PROTECT power veto still wins
  }

  // --- runtime: lease grant/expiry -> smooth autonomous fallback -------------
  {
    ChoreoRuntime rt;
    rt.init(FIXTURE_DOWNLIGHT, 1, 7);
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_GH_CA);
    uint8_t params[8] = {};
    CHECK(rt.applyProgramSet(PROG_BRIDGE_SHOW, 30, 1, 0, params, 1000));
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_BRIDGE_SHOW);
    CHECK(rt.leaseActive());
    CHECK_EQ(rt.leaseRemainingS(11000), 20u);
    // Unknown program rejected, state unchanged (fails closed).
    CHECK(!rt.applyProgramSet(200, 30, 1, 0, params, 2000));
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_BRIDGE_SHOW);
    // Fresh frames keep it alive; lease expiry falls back to autonomous.
    ShowFrameState sf = {};
    sf.rxMs = 30500;
    sf.val = 200;
    ProgramInputs in = {};
    in.nowMs = 31500; // past the 30 s lease
    in.pixelCount = 1;
    in.fixtureClass = FIXTURE_DOWNLIGHT;
    in.showFrame = &sf;
    ProgramOutputs out = {};
    rt.tick(in, out);
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_GH_CA);
    CHECK(!rt.leaseActive());
    // Explicit release also returns to autonomous.
    CHECK(rt.applyProgramSet(PROG_BRIDGE_SHOW, 30, 1, 0, params, 40000));
    CHECK(rt.applyProgramSet(0, 0, 0, 0, params, 41000));
    CHECK(!rt.leaseActive());
    // ShowFrame staleness (>3 s) inside a valid lease -> autonomous.
    CHECK(rt.applyProgramSet(PROG_BRIDGE_SHOW, 300, 1, 1 /*hard cut*/, params, 50000));
    sf.rxMs = 50000;
    in.nowMs = 54000;
    rt.tick(in, out);
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_GH_CA);
    // Micro-lease: an extended frame with flags bit0 pulls in PROG_BRIDGE_SHOW.
    sf.rxMs = 60000;
    sf.flags = 0x01;
    rt.noteShowFrame(sf, 60000);
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_BRIDGE_SHOW);
  }

  // --- commissioning runtime: bridge lease -> electrically dark fallback ----
  {
    ChoreoRuntime rt;
    rt.init(FIXTURE_DOWNLIGHT, 1, 7, PROG_COMMISSION_DARK);
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_COMMISSION_DARK);
    ProgramInputs in = {};
    in.nowMs = 1000;
    in.fixtureClass = FIXTURE_DOWNLIGHT;
    in.pixelCount = 1;
    ProgramOutputs out = {};
    rt.tick(in, out);
    CHECK_EQ(out.frame.px[0][0], 0u);
    CHECK_EQ(out.frame.px[0][3], 0u);

    DirectFrameState df = {};
    df.rxMs = 2000;
    df.r = 100;
    df.flags = 0x03; // micro-lease + hard cut
    rt.noteDirectFrame(df, 2000);
    in.nowMs = 2000;
    rt.tick(in, out);
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_DIRECT);
    CHECK_EQ(out.frame.px[0][0], 100u);

    // Command silence never selects GH/CA in commissioning.
    in.nowMs = 5101;
    rt.tick(in, out);
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_COMMISSION_DARK);
    CHECK(!rt.leaseActive());
    CHECK_EQ(out.frame.px[0][0], 0u);

    // A live profile flip changes the fallback but does not invent a lease.
    CHECK(rt.setAutonomousProgram(PROG_GH_CA, 6000, true));
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_GH_CA);
  }

  // A bridge dark lease is distinguishable from the unleased commissioning
  // fallback so platform glue can cut the rail only for explicit blackout.
  {
    ChoreoRuntime rt;
    rt.init(FIXTURE_DOWNLIGHT, 1, 7, PROG_COMMISSION_DARK);
    CHECK(!rt.darkLeaseActive());
    uint8_t params[8] = {};
    CHECK(rt.applyProgramSet(PROG_COMMISSION_DARK, 30, 1,
                             1 /* hard cut */, params, 1000));
    CHECK(rt.leaseActive());
    CHECK(rt.darkLeaseActive());
    ProgramInputs in = {};
    in.nowMs = 31001;
    in.fixtureClass = FIXTURE_DOWNLIGHT;
    in.pixelCount = 1;
    ProgramOutputs out = {};
    rt.tick(in, out);
    CHECK(!rt.leaseActive());
    CHECK(!rt.darkLeaseActive());
  }

  // --- PROG_DIRECT: slew convergence, hard-cut, hold+half, stale fallback ---
  {
    ChoreoRuntime rt;
    rt.init(FIXTURE_DOWNLIGHT, 1, 7);
    // Micro-lease grant (flags bit0) pulls in PROG_DIRECT, like ShowFrame's.
    DirectFrameState df = {};
    df.rxMs = 1000;
    df.r = 255; df.g = 128; df.b = 0; df.w = 64;
    df.flags = 0x01;
    rt.noteDirectFrame(df, 1000);
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_DIRECT);
    // Stream at 10 Hz past the 2 s crossfade: slew (32/tick, <=0.8 s full
    // traverse) must converge on the commanded color exactly.
    ProgramInputs in = {};
    in.fixtureClass = FIXTURE_DOWNLIGHT;
    in.pixelCount = 1;
    ProgramOutputs out = {};
    uint32_t t = 1000;
    for (int i = 0; i < 40; i++) {
      df.rxMs = (t += 100);
      rt.noteDirectFrame(df, t);
      in.nowMs = t;
      rt.tick(in, out);
    }
    CHECK_EQ(out.frame.px[0][0], 255u);
    CHECK_EQ(out.frame.px[0][1], 128u);
    CHECK_EQ(out.frame.px[0][2], 0u);
    CHECK_EQ(out.frame.px[0][3], 64u);
    // Hard-cut (bit1): a new frame lands verbatim on the very next tick.
    df.r = 10; df.g = 20; df.b = 30; df.w = 40;
    df.flags = 0x03;
    df.rxMs = (t += 100);
    rt.noteDirectFrame(df, t);
    in.nowMs = t;
    rt.tick(in, out);
    CHECK_EQ(out.frame.px[0][0], 10u);
    CHECK_EQ(out.frame.px[0][3], 40u);
    // Silence 1-3 s: hold+half band -- still PROG_DIRECT, at half the value.
    in.nowMs = t + 1500;
    rt.tick(in, out);
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_DIRECT);
    CHECK_EQ(out.frame.px[0][0], 5u);
    // Silence >3 s: autonomous fallback, same ladder as SHOWFRAME staleness.
    in.nowMs = t + 3100;
    rt.tick(in, out);
    CHECK_EQ(rt.activeProgram(), (uint8_t)PROG_GH_CA);
  }
  {
    // PROG_DIRECT on PERIMETER: uniform 37-px wash, W written for wire truth
    // (the GRB hardware ignores it).
    ChoreoRuntime rt;
    rt.init(FIXTURE_PERIMETER, 37, 7);
    DirectFrameState df = {};
    df.r = 200; df.g = 100; df.b = 50; df.w = 25;
    df.flags = 0x01;
    ProgramInputs in = {};
    in.fixtureClass = FIXTURE_PERIMETER;
    in.pixelCount = 37;
    ProgramOutputs out = {};
    uint32_t t = 1000;
    for (int i = 0; i < 40; i++) {
      df.rxMs = (t += 100);
      rt.noteDirectFrame(df, t);
      in.nowMs = t;
      rt.tick(in, out);
    }
    CHECK_EQ(out.frame.count, 37u);
    bool washOk = true;
    for (int i = 0; i < 37; i++)
      if (out.frame.px[i][0] != 200 || out.frame.px[i][1] != 100 ||
          out.frame.px[i][2] != 50 || out.frame.px[i][3] != 25)
        washOk = false;
    CHECK(washOk);
  }

  // --- NbDirectFrame entry scan: a fixture ignores frames not naming it ------
  {
    NbDirectFrame f;
    memset(&f, 0, sizeof(f));
    uint8_t me[3] = {0xF2, 0xBF, 0xA0};
    uint8_t other[3] = {0xF4, 0x03, 0x1C};
    memcpy(f.entries[0].id, other, 3);
    memcpy(f.entries[1].id, me, 3);
    f.entries[1].r = 9;
    f.count = 2;
    int len = (int)offsetof(NbDirectFrame, entries) + 2 * (int)sizeof(NbDirectEntry);
    const NbDirectEntry *e = nbDirectFindEntry(&f, len, me);
    CHECK(e != nullptr && e->r == 9);
    // Not named anywhere: nullptr, the fixture ignores the frame entirely.
    uint8_t nobody[3] = {1, 2, 3};
    CHECK(nbDirectFindEntry(&f, len, nobody) == nullptr);
    // A lying `count` past the wire length is clamped: our entry, truncated
    // away on the air, must not be read out of the stale buffer tail.
    len = (int)offsetof(NbDirectFrame, entries) + 1 * (int)sizeof(NbDirectEntry);
    CHECK(nbDirectFindEntry(&f, len, me) == nullptr);
  }

  return testReport("test_behavior");
}
