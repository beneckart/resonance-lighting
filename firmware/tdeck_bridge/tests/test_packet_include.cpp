// Proves the wire contract is included from the fixture tree (never forked)
// and that this build path sees the same golden layout the fixture tests pin.
#include <cassert>
#include <cstdio>

#include "fixture/src/core/packet.h"

int main() {
  static_assert(sizeof(NbHeader) == 13, "wire header drifted");
  static_assert(sizeof(NbProgramSet) == 32, "NbProgramSet drifted");
  static_assert(sizeof(NbIdentify) == 20, "NbIdentify drifted");
  static_assert(sizeof(NbTargetU16) == 18, "NbTargetU16 drifted");
  static_assert(NB_DIRECT_MAX_ENTRIES == 18, "direct-frame capacity drifted");

  // Targeting semantics: 00:00:00 is "all" for nbTargetMatches consumers.
  const uint8_t all[3] = {0, 0, 0};
  const uint8_t me[3] = {0xF4, 0x02, 0x68};
  assert(nbTargetMatches(all, me));
  assert(nbTargetMatches(me, me));
  const uint8_t other[3] = {0xF4, 0x02, 0x69};
  assert(!nbTargetMatches(other, me));

  printf("packet include ok (ver=%d)\n", NB_PROTO_VER);
  return 0;
}
