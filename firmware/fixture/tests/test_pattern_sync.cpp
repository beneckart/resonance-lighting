// Simulates several lanterns with drifting clocks, random radio delays and lost
// beats, and checks that the elected-conductor group clock keeps every
// lantern's hits aligned through start-up, conductor loss and re-merge.
#include "test_util.h"
#include "../src/core/pattern_sync.h"

#include <cmath>
#include <cstring>
#include <map>
#include <random>
#include <vector>

struct Sim {
  struct Node {
    uint8_t id[3];
    double ppm;        // clock rate error
    double bootUs;     // local clock = (t - bootUs) * (1 + ppm)
    bool alive = true;
    bool inRange = true;
    PatternSync sync;
    PatternPlayer player;
    PatternPart part;
    std::map<uint64_t, double> fired; // target group us -> true time fired
    double txPendingAt = -1;          // true time the queued beat leaves the radio
    NbPatternBeat txBeat{};
    uint64_t local(double t) const { return (uint64_t)llround((t - bootUs) * (1.0 + ppm * 1e-6)); }
  };
  std::vector<Node> nodes;
  std::mt19937 rng{1234};
  double lossRate = 0.1;

  void add(uint8_t a, uint8_t b, uint8_t c, double ppm, double bootUs) {
    Node n;
    n.id[0] = a; n.id[1] = b; n.id[2] = c;
    n.ppm = ppm; n.bootUs = bootUs;
    n.part.patternId = 0xABCD; n.part.loopUs = 2000000; n.part.pulseMs = 16;
    n.part.count = 4;
    for (int i = 0; i < 4; ++i) n.part.offsetsUs[i] = (uint32_t)(i * 500000);
    nodes.push_back(n);
  }
  void startAll(double t, double spreadUs) {
    for (size_t i = 0; i < nodes.size(); ++i) {
      double ts = t + spreadUs * (double)i / (double)nodes.size();
      nodes[i].sync.start(nodes[i].local(ts), nodes[i].id, 0xABCD, 2000000, 1000 + (uint32_t)i);
    }
  }
  void step(double t) {
    std::uniform_real_distribution<double> txDelay(200, 5000), rxJit(80, 130), u(0, 1);
    for (auto &n : nodes) {
      if (!n.alive) continue;
      uint64_t now = n.local(t);
      // A queued beat leaves the radio: tell the sender, deliver to the others.
      if (n.txPendingAt >= 0 && t >= n.txPendingAt) {
        n.sync.beatSent(n.txBeat.beat_seq, n.local(t));
        for (auto &r : nodes) {
          if (&r == &n || !r.alive || !r.inRange || !n.inRange || u(rng) < lossRate) continue;
          double rxT = t + rxJit(rng);
          r.sync.onBeat(n.txBeat, r.local(rxT), r.local(rxT));
        }
        n.txPendingAt = -1;
      }
      n.sync.tick(now);
      NbPatternBeat b{};
      if (n.txPendingAt < 0 && n.sync.beatDue(now, b)) {
        b.h.ver = NB_PROTO_VER; b.h.type = NB_PATTERN_BEAT;
        memcpy(b.h.src_id, n.id, 3);
        n.txBeat = b;
        n.txPendingAt = t + txDelay(rng); // variable queue/channel-access delay
      }
      uint64_t g = n.sync.groupNow(now);
      auto a = n.player.poll(n.part, g, n.sync.locked(now), false);
      if (a == PatternPlayer::FIRE) {
        n.fired[n.player.nextTargetUs] = t;
        n.player.fired(n.part, g);
      }
    }
  }
  int conductors() const {
    int c = 0;
    for (auto &n : nodes) if (n.alive && n.sync.role == PatternSync::CONDUCT) c++;
    return c;
  }
  // Worst true-time spread of the same target across nodes, for targets fired
  // by at least `minNodes` nodes in [t0, t1).
  double spread(double t0, double t1, size_t minNodes, int *targets = nullptr) const {
    std::map<uint64_t, std::vector<double>> by;
    for (auto &n : nodes)
      for (auto &kv : n.fired)
        if (kv.second >= t0 && kv.second < t1) by[kv.first].push_back(kv.second);
    double worst = 0;
    int count = 0;
    for (auto &kv : by) {
      if (kv.second.size() < minNodes) continue;
      count++;
      double lo = kv.second[0], hi = kv.second[0];
      for (double v : kv.second) { lo = std::min(lo, v); hi = std::max(hi, v); }
      worst = std::max(worst, hi - lo);
    }
    if (targets) *targets = count;
    return worst;
  }
};

int main() {
  // ---- player unit behavior -------------------------------------------------
  {
    PatternPart p;
    p.patternId = 1; p.loopUs = 2000000; p.pulseMs = 16; p.leadUs = 10000; p.count = 2;
    p.offsetsUs[0] = 0; p.offsetsUs[1] = 1000000;
    CHECK_EQ(PatternPlayer::nextHit(p, 0), 1000000ULL);
    CHECK_EQ(PatternPlayer::nextHit(p, 1000000), 2000000ULL);
    CHECK_EQ(PatternPlayer::nextHit(p, 1999999), 2000000ULL);
    PatternPlayer pl;
    CHECK_EQ(pl.poll(p, 10000000, false, false), PatternPlayer::WAIT); // not locked
    CHECK_EQ(pl.poll(p, 10000000, true, false), PatternPlayer::WAIT);
    CHECK_EQ(pl.nextTargetUs, 11000000ULL);                             // first future hit
    CHECK_EQ(pl.poll(p, 10989999, true, false), PatternPlayer::WAIT);
    CHECK_EQ(pl.poll(p, 10990000, true, false), PatternPlayer::FIRE);   // lead 10 ms early
    pl.fired(p, 10990100);
    CHECK_EQ(pl.hits, 1U);
    CHECK_EQ(pl.maxLateUs, 100U);
    CHECK_EQ(pl.nextTargetUs, 12000000ULL);
    // A backward clock correction cannot replay the hit already played.
    CHECK_EQ(pl.poll(p, 10995000, true, false), PatternPlayer::WAIT);
    // A forward jump past the late limit skips instead of compressing.
    CHECK_EQ(pl.poll(p, 12050000, true, false), PatternPlayer::SKIP);
    CHECK_EQ(pl.lateSkips, 1U);
    CHECK_EQ(pl.nextTargetUs, 13000000ULL);
    // A still-driving gate skips the hit rather than queueing it.
    CHECK_EQ(pl.poll(p, 12990000, true, true), PatternPlayer::SKIP);
    CHECK_EQ(pl.blocked, 1U);
    CHECK_EQ(pl.nextTargetUs, 14000000ULL);
  }

  // ---- election basics -------------------------------------------------------
  {
    const uint8_t lo[3] = {0x11, 0, 0}, hi[3] = {0x22, 0, 0};
    PatternSync s;
    s.start(0, hi, 5, 2000000, 7);
    CHECK_EQ(s.role, PatternSync::LISTEN);
    CHECK(!s.locked(0));
    s.tick(PatternSync::LISTEN_US + PatternSync::STAGGER_US);
    CHECK_EQ(s.role, PatternSync::CONDUCT);
    NbPatternBeat b{};
    b.h.ver = NB_PROTO_VER; b.h.type = NB_PATTERN_BEAT; memcpy(b.h.src_id, lo, 3);
    b.pattern_id = 5; b.loop_us = 2000000; b.conductor_boot = 9; b.beat_seq = 1;
    NbPatternBeat other = b; other.pattern_id = 6;
    s.onBeat(other, 3000000, 3000000);
    CHECK_EQ(s.role, PatternSync::CONDUCT); // a different pattern is ignored
    s.onBeat(b, 3000000, 3000000);
    CHECK_EQ(s.role, PatternSync::FOLLOW);  // lower ID wins
    CHECK_EQ(s.yields, 1U);
    PatternSync l;
    l.start(0, lo, 5, 2000000, 8);
    l.tick(PatternSync::LISTEN_US + PatternSync::STAGGER_US);
    NbPatternBeat fromHi = b; memcpy(fromHi.h.src_id, hi, 3);
    l.onBeat(fromHi, 4000000, 4000000);
    CHECK_EQ(l.role, PatternSync::CONDUCT); // higher ID never displaces it
  }

  // ---- multi-lantern simulation ---------------------------------------------
  Sim sim;
  sim.add(0x9F, 0x26, 0xB0, +18.0, -3.0e9);
  sim.add(0x9F, 0x0E, 0x54, -22.0, -1.2e9);
  sim.add(0x9F, 0x27, 0x38, +7.0, -5.0e8);
  sim.add(0xF4, 0x02, 0x54, -9.0, -2.4e9);
  sim.add(0x9F, 0x26, 0xD8, +30.0, -7.7e8);
  const double MS = 1000.0, STEP = 250.0; // 250 us steps resolve sub-ms alignment
  sim.startAll(0, 40 * MS); // USB start commands arrive over ~40 ms
  double t = 0;
  for (; t < 20e6; t += STEP) sim.step(t);

  // Exactly one conductor (the lowest ID that was beating), and every other
  // lantern follows that one.
  CHECK_EQ(sim.conductors(), 1);
  int lowest = 0;
  for (size_t i = 0; i < sim.nodes.size(); ++i)
    if (sim.nodes[i].sync.role == PatternSync::CONDUCT) lowest = (int)i;
  for (auto &n : sim.nodes)
    if (n.sync.role == PatternSync::FOLLOW) CHECK(memcmp(n.sync.conductor, sim.nodes[lowest].id, 3) == 0);
  int targets = 0;
  double sp = sim.spread(8e6, 20e6, sim.nodes.size(), &targets);
  std::printf("  steady: %d shared targets, worst spread %.0f us\n", targets, sp);
  CHECK(targets >= 20);
  CHECK(sp < 1000); // radio jitter is tens of us; the step is 250 us

  // Conductor disappears: another takes over and the phase continues.
  sim.nodes[lowest].alive = false;
  for (; t < 40e6; t += STEP) sim.step(t);
  CHECK_EQ(sim.conductors(), 1);
  sp = sim.spread(26e6, 40e6, sim.nodes.size() - 1, &targets);
  std::printf("  after conductor loss: %d shared targets, worst spread %.0f us\n", targets, sp);
  CHECK(targets >= 20);
  CHECK(sp < 1000);

  // A lantern drifts out of range, conducts for itself, then re-merges.
  int roamer = (lowest + 1) % (int)sim.nodes.size();
  sim.nodes[roamer].inRange = false;
  for (; t < 70e6; t += STEP) sim.step(t);
  CHECK_EQ(sim.nodes[roamer].sync.role, PatternSync::CONDUCT);
  sim.nodes[roamer].inRange = true;
  for (; t < 90e6; t += STEP) sim.step(t);
  CHECK_EQ(sim.conductors(), 1);
  sp = sim.spread(80e6, 90e6, sim.nodes.size() - 1, &targets);
  std::printf("  after re-merge: %d shared targets, worst spread %.0f us\n", targets, sp);
  CHECK(targets >= 10);
  CHECK(sp < 1000);

  // No lantern ever fired the same target twice.
  for (auto &n : sim.nodes) {
    uint32_t fired = (uint32_t)n.fired.size();
    CHECK_EQ(fired, n.player.hits);
  }

  return testReport("test_pattern_sync");
}
