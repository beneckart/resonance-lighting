#include "test_util.h"
#include "../src/core/squad_trigger.h"
#include "../src/core/serial_mutation.h"
#include "../src/core/strike_event.h"

int main() {
  SquadRadioRecovery repair;
  CHECK(repair.start(UINT32_MAX-99));
  CHECK(!repair.start(0)); CHECK(!repair.due(99)); CHECK(repair.due(100));
  repair.resumed(); CHECK(repair.start(300)); repair.resumed();
  CHECK(repair.start(600)); repair.resumed(); CHECK(!repair.start(900));
  CHECK_EQ(repair.attempts,3);
  SquadCycle c;
  CHECK_EQ(SQUAD_COUNT,9u);
  for (unsigned pass=0;pass<3;++pass) for (unsigned step=0;step<10;++step) {
    uint32_t now=(pass*10+step)*3000;
    CHECK_EQ(c.next,step);
    uint16_t expected=step==9 ? 511 : 1U<<step;
    CHECK_EQ(c.targets(),expected);
    CHECK(!c.canFire(now,0));
    CHECK(!c.canFire(now,expected & (expected-1)));
    CHECK(c.canFire(now,expected));
    c.sent(now);
    CHECK(!c.canFire(now+2499,511));
    CHECK(c.canFire(now+2500,511));
  }
  c.sent(UINT32_MAX-1000);
  CHECK(!c.canFire(1498,511)); CHECK(c.canFire(1499,511));
  for (unsigned i=0;i<9;++i) {
    CHECK_EQ(squadIndex(SQUAD[i].id),(int)i);
    CHECK(SQUAD[i].pulseMs>=5 && SQUAD[i].pulseMs<=100);
  }
  uint8_t foreign[3]={1,2,3}; CHECK_EQ(squadIndex(foreign),-1);

  SquadPresence p; uint16_t z[16]={}; z[5]=500;
  for (unsigned t=100;t<1000;t+=100) CHECK(!p.sample(t,true,z)); // boot occupied
  z[5]=0;
  for (unsigned t=1000;t<=1800;t+=100) CHECK(!p.sample(t,true,z));
  CHECK(p.armed);
  z[7]=600;
  CHECK(!p.sample(1900,true,z)); CHECK(p.sample(2000,true,z));
  for (unsigned t=2100;t<3100;t+=100) CHECK(!p.sample(t,true,z)); // held hand
  z[7]=750;
  for (unsigned t=3100;t<4200;t+=100) CHECK(!p.sample(t,true,z)); // hysteresis
  CHECK(!p.armed);
  z[7]=900;
  for (unsigned t=4200;t<=5000;t+=100) CHECK(!p.sample(t,true,z));
  CHECK(p.armed);
  CHECK(!p.sample(5100,false,z)); CHECK(!p.armed);
  for (unsigned t=5200;t<=6000;t+=100) CHECK(!p.sample(t,true,z));
  CHECK(p.armed);
  z[7]=300;
  CHECK(!p.sample(8000,true,z)); CHECK(!p.sample(8100,true,z)); // stale must rearm

  SquadPresence close;
  CHECK_EQ(close.minMm,50);
  memset(z,0,sizeof(z)); z[0]=49;
  for (unsigned t=100;t<=900;t+=100) CHECK(!close.sample(t,true,z));
  CHECK(close.armed); // below cutoff, including zero/no-return, is not a hit
  z[0]=50;
  CHECK(!close.sample(1000,true,z)); CHECK(close.sample(1100,true,z));
  CHECK(!close.sample(1200,true,z));
  CHECK(close.setMinMm(30)); CHECK(!close.armed);
  CHECK(!close.sample(1300,true,z)); CHECK(!close.sample(1400,true,z));
  CHECK(!close.setMinMm(29)); CHECK(!close.setMinMm(700));
  CHECK(!close.setMinMm(0)); CHECK_EQ(close.minMm,30);
  z[0]=29;
  for (unsigned t=1500;t<=2300;t+=100) CHECK(!close.sample(t,true,z));
  CHECK(close.armed); CHECK(close.setMinMm(30)); CHECK(close.armed); // no-op
  z[0]=30;
  CHECK(!close.sample(2400,true,z)); CHECK(close.sample(2500,true,z));
  CHECK(close.setMinMm(150)); CHECK_EQ(close.minMm,150); CHECK(!close.armed);

  const uint8_t id[3]={0xF4,3,0x48};
  const char *valid[]={"!TF40348:60\n","!TF40348:0\n","!TF40348:120\n","!JF40348:1\n",
    "!LF40348:30\n","!LF40348:50\n","!LF40348:699\n"};
  for (const char *s:valid) {
    SerialMutationParser parser; SerialMutationCommand cmd={};
    unsigned ready=0;
    for (;*s;++s) if (parser.feed(*s,id,cmd)==SERIAL_MUTATION_READY) ++ready;
    CHECK_EQ(ready,1u);
  }
  const char *invalid[]={"!TF40348:121\n","!TF40349:60\n","!JF40348:0\n","!JF40348:2\n","!TF40348:060\n","!JF40348:1!JF40348:1\n",
    "!LF40348:0\n","!LF40348:29\n","!LF40348:700\n","!LF40348:050\n","!LF40349:50\n"};
  for (const char *s:invalid) {
    SerialMutationParser parser; SerialMutationCommand cmd={};
    for (;*s;++s) CHECK(parser.feed(*s,id,cmd)!=SERIAL_MUTATION_READY);
  }
  // Existing receiver interprets repeated decremented delays as one deadline.
  StrikeEventState a,b; strikeEventInit(a); strikeEventInit(b);
  NbEvent e={}; e.kind=NB_EVENT_SOLENOID_STRIKE; e.event_id=123;
  e.params[0]=16; e.fire_in_ms=1000;
  CHECK_EQ(strikeEventAccept(a,e,100),STRIKE_EVENT_ARMED_FUTURE);
  e.fire_in_ms=900;
  CHECK_EQ(strikeEventAccept(b,e,200),STRIKE_EVENT_ARMED_FUTURE);
  CHECK_EQ(strikeEventAccept(a,e,200),STRIKE_EVENT_DUPLICATE);
  CHECK_EQ(a.fireAtMs,b.fireAtMs);
  uint16_t pulse=0;
  CHECK_EQ(strikeEventTick(a,1100,pulse),STRIKE_EVENT_FIRE); CHECK_EQ(pulse,16);
  CHECK_EQ(strikeEventTick(a,1101,pulse),STRIKE_EVENT_IDLE);
  return testReport("squad_trigger");
}
