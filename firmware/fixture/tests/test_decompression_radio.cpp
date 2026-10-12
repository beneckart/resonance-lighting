#include "test_util.h"
#include "../src/core/decompression_replay.h"
#include "../src/core/decompression_session.h"
#include "../../atom_decompression/src/core/button.h"
#include "../../atom_decompression/roster.h"

int main() {
  CHECK_EQ(SQUAD_COUNT,9u);
  NbDecompressionControl c = {};
  c.h.ver=1; c.h.type=NB_DECOMPRESSION_CONTROL;
  c.h.src_id[0]=0x54; c.h.src_id[1]=0xB0; c.h.src_id[2]=0xC8;
  c.controller_boot=42; c.h.seq=1; c.action=NB_DECOMP_SESSION; c.minutes=5;
  for (unsigned i=0;i<SQUAD_COUNT;++i) {
    memcpy(c.target_id,SQUAD[i].id,3);
    CHECK(nbDecompressionValid(c,SQUAD[i].id));
    for (unsigned j=0;j<SQUAD_COUNT;++j)
      CHECK_EQ(nbDecompressionValid(c,SQUAD[j].id),i==j);
  }
  const uint8_t other[3]={1,2,3};
  CHECK(!nbDecompressionValid(c,other));
  c.minutes=6; CHECK(!nbDecompressionValid(c,c.target_id));
  c.minutes=0; CHECK(nbDecompressionValid(c,c.target_id));
  c.action=NB_DECOMP_PLAY; CHECK(nbDecompressionValid(c,c.target_id));
  c.minutes=1; CHECK(!nbDecompressionValid(c,c.target_id)); c.minutes=0;
  c.controller_boot=0; CHECK(!nbDecompressionValid(c,c.target_id)); c.controller_boot=42;
  c.h.ver=2; CHECK(!nbDecompressionValid(c,c.target_id)); c.h.ver=1;
  memset(c.target_id,0,3); CHECK(!nbDecompressionValid(c,c.target_id));
  memcpy(c.target_id,SQUAD[0].id,3);
  DecompressionReplay replay; DecompressionReplay::Entry *entry;
  CHECK_EQ(replay.admit(c,100,entry),DecompressionReplay::NEW);
  entry->status=NB_DECOMP_REFUSED;
  CHECK_EQ(replay.admit(c,130,entry),DecompressionReplay::DUPLICATE);
  CHECK_EQ(entry->status,NB_DECOMP_REFUSED); // retry cannot turn refusal into play
  CHECK_EQ(replay.admit(c,10000,entry),DecompressionReplay::DUPLICATE);
  c.h.seq=3; c.action=NB_DECOMP_SESSION; c.minutes=5;
  CHECK_EQ(replay.admit(c,11000,entry),DecompressionReplay::NEW);
  c.h.seq=2; c.action=NB_DECOMP_PLAY; c.minutes=0;
  CHECK_EQ(replay.admit(c,11001,entry),DecompressionReplay::STALE);
  c.controller_boot=43; c.h.seq=1;
  CHECK_EQ(replay.admit(c,11002,entry),DecompressionReplay::NEW);
  c.controller_boot=42;
  CHECK_EQ(replay.admit(c,11003,entry),DecompressionReplay::STALE);
  c.controller_boot=44; c.h.seq=0xFFFFFFFE;
  CHECK_EQ(replay.admit(c,12000,entry),DecompressionReplay::NEW);
  c.h.seq=1;
  CHECK_EQ(replay.admit(c,12001,entry),DecompressionReplay::NEW);
  c.h.seq=0xFFFFFFFF;
  CHECK_EQ(replay.admit(c,12002,entry),DecompressionReplay::STALE);

  DecompressionSession lease;
  CHECK(lease.set(0,5)); CHECK(lease.active(299999999));
  CHECK(!lease.active(300000000)); // controller loss never becomes permanent hold
  CHECK(lease.set(299000000,5)); CHECK(lease.active(598999999));
  CHECK(!lease.active(599000000));

  DecompressionButton button;
  button.begin(false,0); // boot-held: release before any accepted press
  CHECK(!button.tick(false,10000));
  CHECK(!button.tick(true,10001)); CHECK(!button.tick(true,10036));
  CHECK(!button.tick(false,10040)); CHECK(button.tick(false,10075));
  CHECK(!button.tick(false,20000)); // held press cannot repeat
  CHECK(!button.tick(true,20001)); CHECK(!button.tick(true,20036));
  CHECK(!button.tick(false,20040)); CHECK(!button.tick(true,20050)); // bounce
  CHECK(!button.tick(false,20060)); CHECK(button.tick(false,20095));
  CHECK(!button.tick(true,20100)); CHECK(!button.tick(true,20135));
  CHECK(!button.tick(false,20140)); CHECK(!button.tick(false,20175)); // cooldown
  CHECK(!button.tick(false,40000)); // no delayed trigger at cooldown expiry
  button.begin(true,0xFFFFFFF0);
  CHECK(!button.tick(false,0xFFFFFFF5)); CHECK(button.tick(false,0x18));
  return testReport("decompression radio/roster/button");
}
