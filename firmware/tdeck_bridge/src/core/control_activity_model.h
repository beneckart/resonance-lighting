#pragma once

#include <stddef.h>
#include <stdint.h>

#include "rx_ring.h"

// Pure model behind the global Bridge OS activity ribbon. It distinguishes
// local program leases from passively observed publishers without changing the
// one fleet wire contract. Known one-off bridge IDs get human labels; an
// unfamiliar publisher remains visible by exact short ID as OTHER.

enum class ControlPublisherKind : uint8_t {
  OTHER = 0,
  TDECK,
  PUCA,
  CORES3,
};

enum class ForeignControlKind : uint8_t {
  NONE = 0,
  DIRECT,
  SHOW,
  PROGRAM,
  LIFECYCLE,
};

struct ForeignControlActivity {
  bool active;
  uint8_t id[3];
  ControlPublisherKind publisher;
  ForeignControlKind kind;
  uint8_t programId;
  bool expiring;
  uint32_t remainingMs;
  uint32_t ageMs;
};

ControlPublisherKind controlPublisherKind(const uint8_t id[3]);
const char *controlPublisherName(ControlPublisherKind kind);
const char *foreignControlKindName(ForeignControlKind kind);

class ForeignControlTracker {
 public:
  void observe(const RxItem &item, const uint8_t selfId[3]);
  bool latest(uint32_t nowMs, uint32_t liveFreshMs,
              ForeignControlActivity &out);

 private:
  static constexpr size_t kSlots = 8;
  struct Slot {
    bool used;
    uint8_t id[3];
    uint32_t lastObservedMs;
    uint32_t directMs;
    ForeignControlKind directKind;
    uint32_t lifecycleMs;
    bool programActive;
    uint8_t programId;
    uint32_t programUntilMs;
    uint32_t programObservedMs;
  };
  Slot mSlots[kSlots] = {};

  Slot *find(const uint8_t id[3], bool create, uint32_t nowMs);
};

struct ProgramLeaseActivity {
  bool active;
  bool fleetWide;
  uint8_t target[3];
  uint8_t programId;
  uint32_t remainingMs;
};

// Local program commands can have one fleet lease plus a later exact-target
// seed. Preserve both so seeding Contagion never hides the fleet-wide lease
// from the shell or makes its global Stop release only one fixture.
class ProgramLeaseTracker {
 public:
  void note(const uint8_t target[3], uint8_t programId, uint16_t leaseS,
            uint32_t nowMs);
  ProgramLeaseActivity snapshot(uint32_t nowMs);

 private:
  struct Lease {
    bool active;
    uint8_t target[3];
    uint8_t programId;
    uint32_t untilMs;
  };
  Lease mFleet = {};
  Lease mTarget = {};

  static bool isAll(const uint8_t target[3]);
  static void expire(Lease &lease, uint32_t nowMs);
  static ProgramLeaseActivity view(const Lease &lease, bool fleetWide,
                                   uint32_t nowMs);
};
