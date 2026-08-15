// Golden wire-format pins. These numbers ARE the fleet protocol: the 26
// commissioned net_bench units and all host tooling parse these exact offsets.
// If an edit to core/packet.h moves one of them, this test failing is the only
// thing standing between you and a silent fleet-wide parse break.
#include "test_util.h"

#include <cstddef>
#include "../src/core/packet.h"

int main() {
  // Header: ver(1) type(1) src_id(3) seq(4) uptime(4).
  CHECK_EQ(sizeof(NbHeader), 13u);

  // Command shells (donor sizes).
  CHECK_EQ(sizeof(NbCmd), 14u);
  CHECK_EQ(sizeof(NbSetU16), 15u);
  CHECK_EQ(sizeof(NbTargetU16), 18u);
  CHECK_EQ(sizeof(NbTargetCmd), 17u);
  CHECK_EQ(sizeof(NbScanAp), 45u); // donor comment: "45 bytes"

  // ShowFrame: legacy 17 B + fixture tail -> 22 B. Old masters send 17.
  CHECK_EQ(offsetof(NbShowFrame, val), 17u);
  CHECK_EQ(sizeof(NbShowFrame), 22u);

  // Identify: legacy 17 B + color tail -> 19 B.
  CHECK_EQ(offsetof(NbIdentify, color), 17u);
  CHECK_EQ(sizeof(NbIdentify), 19u);

  // Heartbeat tails. Every offset here is load-bearing for the append-only
  // contract AND for send-side truncation (hb-short).
  CHECK_EQ(offsetof(NbHeartbeat, supply_mv), 24u);   // base block ends
  CHECK_EQ((unsigned)NB_HB_SHORT_LEN, 29u);          // hb-short = through tail 1
  CHECK_EQ(offsetof(NbHeartbeat, lux_x10), 29u);     // tail 2
  CHECK_EQ(offsetof(NbHeartbeat, ina_pv_mv), 42u);   // tail 3
  CHECK_EQ(offsetof(NbHeartbeat, cfg_cap_mah), 50u); // tail 4
  CHECK_EQ(offsetof(NbHeartbeat, drawdown_mah_x10), 54u); // tail 5
  CHECK_EQ(offsetof(NbHeartbeat, fw_rev), 59u);      // tail 6
  CHECK_EQ(offsetof(NbHeartbeat, maint_status), 83u); // tail 7
  CHECK_EQ(offsetof(NbHeartbeat, field_phase), 84u); // tail 8
  CHECK_EQ(offsetof(NbHeartbeat, bq_vindpm_mv), 98u); // tail 9
  CHECK_EQ(offsetof(NbHeartbeat, field_charge_wh_x10), 113u); // tail 10
  CHECK_EQ(offsetof(NbHeartbeat, mppt_status), 128u); // tail 11
  CHECK_EQ(offsetof(NbHeartbeat, field_load_dimmed), 140u); // tail 12
  // The net_bench-era struct ended here: a fixture hb-full truncated at
  // `profile` is byte-identical to a maxed-out bench heartbeat.
  CHECK_EQ(offsetof(NbHeartbeat, profile), 142u); // tail 13 (fixture era)
  CHECK_EQ(sizeof(NbHeartbeat), 148u);

  // Fixture-era payloads (era-18+ receivers only, still pinned).
  CHECK_EQ(sizeof(NbChoreoState), 22u);
  CHECK_EQ(sizeof(NbProgramSet), 32u);
  CHECK_EQ(sizeof(NbProfile), 18u);
  CHECK_EQ(sizeof(NbTimeQuality), 29u);
  CHECK_EQ(sizeof(NbNeighborSet), 13u + 3u + 1u + 1u + 24u);

  // Cambium-era payloads (25/26). The 15 B preamble + 7 B entry stride are
  // what the bridge's packetizer stands on (118 fixtures / 18 per frame =
  // 7 pkts per wave); valid wire length is 15 + 7*count.
  CHECK_EQ(sizeof(NbDirectEntry), 7u);
  CHECK_EQ(offsetof(NbDirectFrame, flags), 13u);
  CHECK_EQ(offsetof(NbDirectFrame, count), 14u);
  CHECK_EQ(offsetof(NbDirectFrame, entries), 15u);
  CHECK_EQ(sizeof(NbDirectFrame), 141u);
  CHECK_EQ(sizeof(NbForceLifecycle), 18u);

  // Receiver tail gate + truncation round-trip: an hb-short must satisfy the
  // gate for supply_good and fail it for lux_x10.
  CHECK(NB_HAS_HB_FIELD((int)NB_HB_SHORT_LEN, supply_good));
  CHECK(!NB_HAS_HB_FIELD((int)NB_HB_SHORT_LEN, lux_x10));
  CHECK(NB_HAS_HB_FIELD((int)sizeof(NbHeartbeat), night_min));
  // A legacy 142 B bench heartbeat fails the tail-13 gate.
  CHECK(!NB_HAS_HB_FIELD(142, profile));

  // Targeting convention.
  uint8_t me[3] = {0xF2, 0xBF, 0xA0};
  uint8_t all[3] = {0, 0, 0};
  uint8_t other[3] = {0xF4, 0x03, 0x1C};
  CHECK(nbTargetMatches(all, me));
  CHECK(nbTargetMatches(me, me));
  CHECK(!nbTargetMatches(other, me));

  return testReport("test_packet_layout");
}
