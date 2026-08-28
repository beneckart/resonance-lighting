#include "control_activity_model.h"

#include <string.h>

#include "fixture/src/core/packet.h"

namespace {

static bool sameId(const uint8_t a[3], const uint8_t b[3]) {
  return memcmp(a, b, 3) == 0;
}

static bool beforeDeadline(uint32_t nowMs, uint32_t deadlineMs) {
  return (int32_t)(deadlineMs - nowMs) > 0;
}

static uint32_t remaining(uint32_t nowMs, uint32_t deadlineMs) {
  return beforeDeadline(nowMs, deadlineMs) ? deadlineMs - nowMs : 0;
}

}  // namespace

ControlPublisherKind controlPublisherKind(const uint8_t id[3]) {
  static const uint8_t kTDeck[][3] = {
      {0x8E, 0xB5, 0x08},  // primary T-Deck Plus
      {0x97, 0x96, 0x04},  // second T-Deck Plus
  };
  static const uint8_t kPuca[][3] = {
      {0xA4, 0xEB, 0x10},
  };
  static const uint8_t kCoreS3[][3] = {
      {0x4D, 0x5D, 0xB0},  // current CoreS3 Bridge OS
      {0xE3, 0x9F, 0x1C},  // historical Nevada City CoreS3
  };
  for (const auto &known : kTDeck)
    if (sameId(id, known)) return ControlPublisherKind::TDECK;
  for (const auto &known : kPuca)
    if (sameId(id, known)) return ControlPublisherKind::PUCA;
  for (const auto &known : kCoreS3)
    if (sameId(id, known)) return ControlPublisherKind::CORES3;
  return ControlPublisherKind::OTHER;
}

const char *controlPublisherName(ControlPublisherKind kind) {
  switch (kind) {
    case ControlPublisherKind::TDECK: return "T-DECK";
    case ControlPublisherKind::PUCA: return "PUCA";
    case ControlPublisherKind::CORES3: return "CORES3";
    case ControlPublisherKind::OTHER: return "OTHER";
  }
  return "OTHER";
}

const char *foreignControlKindName(ForeignControlKind kind) {
  switch (kind) {
    case ForeignControlKind::DIRECT: return "DIRECT";
    case ForeignControlKind::SHOW: return "SHOW";
    case ForeignControlKind::PROGRAM: return "PROGRAM";
    case ForeignControlKind::LIFECYCLE: return "LIFECYCLE";
    case ForeignControlKind::NONE: return "NONE";
  }
  return "NONE";
}

ForeignControlTracker::Slot *ForeignControlTracker::find(
    const uint8_t id[3], bool create, uint32_t nowMs) {
  Slot *freeSlot = nullptr;
  Slot *oldest = nullptr;
  for (Slot &slot : mSlots) {
    if (slot.used && sameId(slot.id, id)) return &slot;
    if (!slot.used && !freeSlot) freeSlot = &slot;
    if (slot.used && (!oldest ||
                      (int32_t)(slot.lastObservedMs - oldest->lastObservedMs) <
                          0))
      oldest = &slot;
  }
  if (!create) return nullptr;
  Slot *slot = freeSlot ? freeSlot : oldest;
  if (!slot) return nullptr;
  *slot = Slot{};
  slot->used = true;
  memcpy(slot->id, id, 3);
  slot->lastObservedMs = nowMs;
  return slot;
}

void ForeignControlTracker::observe(const RxItem &item,
                                    const uint8_t selfId[3]) {
  if (item.len < sizeof(NbHeader)) return;
  const NbHeader *header = (const NbHeader *)item.data;
  if (header->ver != NB_PROTO_VER || sameId(header->src_id, selfId)) return;

  bool relevant = header->type == NB_DIRECT_FRAME ||
                  header->type == NB_SHOWFRAME ||
                  header->type == NB_PROGRAM_SET ||
                  header->type == NB_FORCE_LIFECYCLE;
  if (!relevant) return;
  Slot *slot = find(header->src_id, true, item.ms);
  if (!slot) return;
  slot->lastObservedMs = item.ms;

  if (header->type == NB_DIRECT_FRAME || header->type == NB_SHOWFRAME) {
    slot->directMs = item.ms;
    slot->directKind = header->type == NB_DIRECT_FRAME
                           ? ForeignControlKind::DIRECT
                           : ForeignControlKind::SHOW;
    return;
  }
  if (header->type == NB_FORCE_LIFECYCLE) {
    if (item.len >= sizeof(NbForceLifecycle)) slot->lifecycleMs = item.ms;
    return;
  }
  if (item.len < sizeof(NbProgramSet)) return;
  const NbProgramSet *program = (const NbProgramSet *)item.data;
  if (program->program_id == 0 || program->lease_s == 0) {
    slot->programActive = false;
    return;
  }
  slot->programActive = true;
  slot->programId = program->program_id;
  slot->programUntilMs = item.ms + (uint32_t)program->lease_s * 1000UL;
  slot->programObservedMs = item.ms;
}

bool ForeignControlTracker::latest(uint32_t nowMs, uint32_t liveFreshMs,
                                   ForeignControlActivity &out) {
  out = ForeignControlActivity{};
  uint32_t bestObservedMs = 0;
  bool found = false;
  for (Slot &slot : mSlots) {
    if (!slot.used) continue;
    ForeignControlActivity candidate = {};
    uint32_t observedMs = 0;
    uint32_t directAge = nowMs - slot.directMs;
    uint32_t lifecycleAge = nowMs - slot.lifecycleMs;
    if (slot.directMs && directAge <= liveFreshMs) {
      candidate.active = true;
      candidate.kind = slot.directKind;
      candidate.ageMs = directAge;
      observedMs = slot.directMs;
    }
    if (slot.lifecycleMs && lifecycleAge <= liveFreshMs &&
        (!candidate.active ||
         (int32_t)(slot.lifecycleMs - observedMs) > 0)) {
      candidate.active = true;
      candidate.kind = ForeignControlKind::LIFECYCLE;
      candidate.ageMs = lifecycleAge;
      observedMs = slot.lifecycleMs;
    }
    if (slot.programActive &&
        !beforeDeadline(nowMs, slot.programUntilMs))
      slot.programActive = false;
    if (slot.programActive &&
        (!candidate.active ||
         (int32_t)(slot.programObservedMs - observedMs) > 0)) {
      candidate.active = true;
      candidate.kind = ForeignControlKind::PROGRAM;
      candidate.programId = slot.programId;
      candidate.expiring = true;
      candidate.remainingMs = remaining(nowMs, slot.programUntilMs);
      candidate.ageMs = nowMs - slot.programObservedMs;
      observedMs = slot.programObservedMs;
    }
    if (!candidate.active) continue;
    memcpy(candidate.id, slot.id, 3);
    candidate.publisher = controlPublisherKind(slot.id);
    if (!found || (int32_t)(observedMs - bestObservedMs) > 0) {
      out = candidate;
      bestObservedMs = observedMs;
      found = true;
    }
  }
  return found;
}

bool ProgramLeaseTracker::isAll(const uint8_t target[3]) {
  return target[0] == 0 && target[1] == 0 && target[2] == 0;
}

void ProgramLeaseTracker::expire(Lease &lease, uint32_t nowMs) {
  if (lease.active && !beforeDeadline(nowMs, lease.untilMs))
    lease.active = false;
}

ProgramLeaseActivity ProgramLeaseTracker::view(const Lease &lease,
                                               bool fleetWide,
                                               uint32_t nowMs) {
  ProgramLeaseActivity activity = {};
  if (!lease.active) return activity;
  activity.active = true;
  activity.fleetWide = fleetWide;
  memcpy(activity.target, lease.target, 3);
  activity.programId = lease.programId;
  activity.remainingMs = remaining(nowMs, lease.untilMs);
  return activity;
}

void ProgramLeaseTracker::note(const uint8_t target[3], uint8_t programId,
                               uint16_t leaseS, uint32_t nowMs) {
  bool all = isAll(target);
  if (programId == 0 || leaseS == 0) {
    if (all) {
      mFleet.active = false;
      mTarget.active = false;
    } else if (mTarget.active && sameId(mTarget.target, target)) {
      mTarget.active = false;
    }
    return;
  }
  Lease &lease = all ? mFleet : mTarget;
  lease = Lease{};
  lease.active = true;
  memcpy(lease.target, target, 3);
  lease.programId = programId;
  lease.untilMs = nowMs + (uint32_t)leaseS * 1000UL;
}

ProgramLeaseActivity ProgramLeaseTracker::snapshot(uint32_t nowMs) {
  expire(mFleet, nowMs);
  expire(mTarget, nowMs);
  if (mFleet.active) return view(mFleet, true, nowMs);
  return view(mTarget, false, nowMs);
}
