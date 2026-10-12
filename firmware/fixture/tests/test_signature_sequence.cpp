#include "test_util.h"
#include "../src/core/signature_sequence.h"
#include "../src/core/decompression_session.h"

int main() {
  SignatureSequence s;
  const uint64_t now = 0x100000000ULL; // no 32-bit clock rollover
  CHECK(s.start(now));
  CHECK(!s.start(now));
  CHECK_EQ(s.poll(now + 19999, false), SignatureSequence::WAIT);
  for (uint8_t i = 0; i < s.COUNT; ++i) {
    uint64_t due = s.startUs + s.beatUs(i);
    CHECK_EQ(s.poll(due + 1500, false), SignatureSequence::FIRE);
    CHECK_EQ(s.hits, i); // requested is not the same as physically started
    s.fired(due + 1505);
    CHECK_EQ(s.offsetsUs[i], s.beatUs(i) + 1505);
    CHECK_EQ(s.poll(due + 2000, true), SignatureSequence::WAIT);
  }
  CHECK_EQ(s.poll(now + 3061500, false), SignatureSequence::DONE);
  CHECK_EQ(s.maxLateUs, 1505U);
  CHECK(!s.start(now + 3061501));
  CHECK(s.start(s.readyUs));
  CHECK_EQ(s.poll(s.startUs + 20001, false), SignatureSequence::ABORT);
  CHECK_EQ(s.hits, 0);
  CHECK_EQ(s.result, SignatureSequence::LATE);
  CHECK_EQ(s.poll(s.startUs + 3000000, false), SignatureSequence::WAIT);
  CHECK(s.start(s.readyUs));
  CHECK_EQ(s.poll(s.startUs, true), SignatureSequence::ABORT);
  CHECK_EQ(s.result, SignatureSequence::MECHANISM);
  CHECK(s.start(s.readyUs));
  s.cancel(s.startUs);
  CHECK_EQ(s.poll(s.startUs, false), SignatureSequence::WAIT);
  CHECK_EQ(s.result, SignatureSequence::CANCELLED);
  CHECK(s.start(s.readyUs));
  CHECK_EQ(s.poll(s.startUs, false), SignatureSequence::FIRE);
  s.cancel(s.startUs, SignatureSequence::MECHANISM);
  CHECK_EQ(s.hits, 0); // failed hardware admission is not a phantom hit

  DecompressionSession session;
  CHECK(!session.active(now));
  CHECK(session.set(now, 60));
  CHECK_EQ(session.remainingSeconds(now), 3600U);
  CHECK(!session.active(now + 3600000000ULL));
  CHECK(!session.set(now, 121));
  CHECK(session.set(now, 0));
  CHECK(!session.active(now));
  return testReport("signature_sequence");
}
