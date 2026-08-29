#include <cassert>
#include <cstdio>
#include <cstring>

#include "core/control_activity_model.h"
#include "fixture/src/core/packet.h"

template <typename T>
static RxItem frame(const T &packet, uint32_t atMs) {
  RxItem item = {};
  item.ms = atMs;
  item.len = sizeof(packet);
  std::memcpy(item.data, &packet, sizeof(packet));
  return item;
}

static NbHeader header(uint8_t type, uint32_t id) {
  NbHeader h = {};
  h.ver = NB_PROTO_VER;
  h.type = type;
  h.src_id[0] = (uint8_t)(id >> 16);
  h.src_id[1] = (uint8_t)(id >> 8);
  h.src_id[2] = (uint8_t)id;
  return h;
}

int main() {
  const uint8_t self[3] = {0x97, 0x96, 0x04};
  assert(controlPublisherKind(self) == ControlPublisherKind::TDECK);
  const uint8_t puca[3] = {0xA4, 0xEB, 0x10};
  assert(controlPublisherKind(puca) == ControlPublisherKind::PUCA);
  const uint8_t core[3] = {0x4D, 0x5D, 0xB0};
  assert(controlPublisherKind(core) == ControlPublisherKind::CORES3);

  ForeignControlTracker foreign;
  NbDirectFrame direct = {};
  direct.h = header(NB_DIRECT_FRAME, 0xA4EB10);
  foreign.observe(frame(direct, 1000), self);
  ForeignControlActivity activity = {};
  assert(foreign.latest(1200, 3000, activity));
  assert(activity.kind == ForeignControlKind::DIRECT);
  assert(activity.publisher == ControlPublisherKind::PUCA);
  assert(activity.ageMs == 200);
  assert(!foreign.latest(4001, 3000, activity));

  NbProgramSet program = {};
  program.h = header(NB_PROGRAM_SET, 0x8EB508);
  program.program_id = 5;
  program.lease_s = 600;
  foreign.observe(frame(program, 5000), self);
  assert(foreign.latest(6500, 3000, activity));
  assert(activity.kind == ForeignControlKind::PROGRAM);
  assert(activity.publisher == ControlPublisherKind::TDECK);
  assert(activity.programId == 5 && activity.expiring);
  assert(activity.remainingMs == 598500);
  program.program_id = 0;
  program.lease_s = 0;
  foreign.observe(frame(program, 7000), self);
  assert(!foreign.latest(7001, 3000, activity));

  // Self traffic is never reported as competition.
  direct.h = header(NB_DIRECT_FRAME, 0x979604);
  foreign.observe(frame(direct, 8000), self);
  assert(!foreign.latest(8001, 3000, activity));

  ProgramLeaseTracker local;
  const uint8_t all[3] = {};
  const uint8_t target[3] = {0x9F, 0x26, 0xD8};
  local.note(all, 5, 600, 1000);
  local.note(target, 5, 600, 2000);  // exact seed does not hide fleet lease
  ProgramLeaseActivity lease = local.snapshot(3000);
  assert(lease.active && lease.fleetWide && lease.programId == 5);
  assert(lease.remainingMs == 598000);
  local.note(all, 0, 0, 4000);
  assert(!local.snapshot(4001).active);
  local.note(target, 4, 10, 5000);
  lease = local.snapshot(6000);
  assert(lease.active && !lease.fleetWide);
  assert(std::memcmp(lease.target, target, 3) == 0);
  assert(!local.snapshot(15000).active);

  std::puts("control_activity_model ok");
  return 0;
}
