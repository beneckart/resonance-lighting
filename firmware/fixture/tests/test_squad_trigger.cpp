#include "test_util.h"
#include "../src/core/squad_trigger.h"
#include "../src/core/serial_mutation.h"
#include "../src/core/strike_event.h"

// Validate the actual state machine during compilation as well as at runtime.
constexpr bool presenceBandContract() {
  SquadPresence p;
  uint16_t zones[16]={};
  if (p.minMm!=50 || p.nearMm!=400 || p.clearMm!=600) return false;
  if (p.sample(100,true,zones) || p.sample(899,true,zones) || p.armed) return false;
  if (p.sample(900,true,zones) || !p.armed) return false;
  zones[0]=401;
  if (p.sample(1000,true,zones) || p.sample(1100,true,zones)) return false;
  zones[0]=400;
  if (p.sample(1200,true,zones) || !p.sample(1300,true,zones)) return false;
  zones[0]=600;
  for (unsigned t=1400;t<=2200;t+=100)
    if (p.sample(t,true,zones) || p.armed) return false;
  zones[0]=601;
  if (p.sample(2300,true,zones) || p.sample(3099,true,zones) || p.armed) return false;
  if (p.sample(3100,true,zones) || !p.armed) return false;
  if (p.setMinMm(400) || p.setNearMm(50) || p.setNearMm(600) ||
      p.setClearMm(400) || p.setClearMm(4001) || !p.armed) return false;
  if (!p.setNearMm(400) || !p.setClearMm(600) || !p.armed) return false;
  zones[0]=300;
  if (p.sample(3200,true,zones) || p.nearFrames!=1) return false;
  if (!p.setNearMm(450) || p.armed || p.nearFrames!=0) return false;
  if (p.sample(3300,true,zones) || p.sample(3400,true,zones)) return false;
  if (!p.setClearMm(650) || p.setRange(50,700,600) || p.nearMm!=450) return false;
  return p.setRange(50,400,600) && !p.armed;
}
static_assert(presenceBandContract(), "presence band boundaries and safe retuning");

int main() {
  SquadRadioRecovery repair;
  CHECK(repair.start(UINT32_MAX-99));
  CHECK(!repair.start(0)); CHECK(!repair.due(99)); CHECK(repair.due(100));
  repair.resumed(); CHECK(repair.start(300)); repair.resumed();
  CHECK(repair.start(600)); repair.resumed(); CHECK(!repair.start(900));
  CHECK_EQ(repair.attempts,3);
  SquadCycle c;
  CHECK_EQ(SQUAD_COUNT,9u);
  CHECK_EQ(SQUAD_STEPS,6u);
  // Rising pitch two at a time (roster is highest first: index 8 = lowest),
  // then the top lantern alone, then all nine (Brian, 2026-10-17).
  const uint16_t steps[6]={(1U<<8)|(1U<<7),(1U<<6)|(1U<<5),(1U<<4)|(1U<<3),(1U<<2)|(1U<<1),1U<<0,511};
  for (unsigned pass=0;pass<3;++pass) for (unsigned step=0;step<6;++step) {
    uint32_t now=(pass*6+step)*3000;
    CHECK_EQ(c.next,step);
    CHECK_EQ(c.targets(),steps[step]);
    CHECK(!c.canFire(now,0));
    CHECK(c.canFire(now,steps[step] & (uint16_t)(steps[step]-1) ? steps[step] & (uint16_t)(steps[step]-1) : steps[step]));  // part ready still fires
    CHECK(c.canFire(now,steps[step]));
    if (step<5) CHECK(!c.canFire(now,(uint16_t)(511 & ~steps[step])));  // only other lanterns ready
    c.sent(now);
    CHECK(!c.canFire(now+2499,511));
    CHECK(c.canFire(now+2500,511));
  }
  // The higher lantern of each pair sounds SQUAD_PAIR_GAP_MS after the lower.
  CHECK_EQ(squadStepOffsetMs(0,8),0u); CHECK_EQ(squadStepOffsetMs(0,7),SQUAD_PAIR_GAP_MS);
  CHECK_EQ(squadStepOffsetMs(3,2),0u); CHECK_EQ(squadStepOffsetMs(3,1),SQUAD_PAIR_GAP_MS);
  CHECK_EQ(squadStepOffsetMs(4,0),0u);
  for (unsigned i=0;i<9;++i) CHECK_EQ(squadStepOffsetMs(5,i),0u);  // the chord is together
  c.sent(UINT32_MAX-1000);
  CHECK(!c.canFire(1498,511)); CHECK(c.canFire(1499,511));
  for (unsigned i=0;i<9;++i) {  // lantern-dings partners point at another roster lantern
    CHECK(SQUAD[i].partner>=-1 && SQUAD[i].partner<(int)SQUAD_COUNT && SQUAD[i].partner!=(int)i);
  }
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
  z[7]=400;
  CHECK(!p.sample(1900,true,z)); CHECK(p.sample(2000,true,z));
  for (unsigned t=2100;t<3100;t+=100) CHECK(!p.sample(t,true,z)); // held hand
  z[7]=600;
  for (unsigned t=3100;t<4200;t+=100) CHECK(!p.sample(t,true,z)); // hysteresis
  CHECK(!p.armed);
  z[7]=601;
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

  SquadPresence band;
  CHECK_EQ(band.nearMm,400); CHECK_EQ(band.clearMm,600);
  memset(z,0,sizeof(z));
  CHECK(!band.sample(100,true,z)); CHECK(!band.sample(900,true,z)); CHECK(band.armed);
  z[0]=401;
  CHECK(!band.sample(1000,true,z)); CHECK(!band.sample(1100,true,z)); CHECK(band.armed);
  z[0]=400;
  CHECK(!band.sample(1200,true,z)); CHECK(band.sample(1300,true,z));
  z[0]=600;
  for (unsigned t=1400;t<=2200;t+=100) CHECK(!band.sample(t,true,z));
  CHECK(!band.armed);
  z[0]=601;
  for (unsigned t=2300;t<=3100;t+=100) CHECK(!band.sample(t,true,z));
  CHECK(band.armed);
  CHECK(!band.setNearMm(50)); CHECK(!band.setNearMm(600));
  CHECK(!band.setClearMm(400)); CHECK(!band.setClearMm(4001));
  CHECK(!band.setMinMm(400)); CHECK(band.armed); // refusals change nothing
  CHECK(band.setNearMm(400)); CHECK(band.setClearMm(600)); CHECK(band.armed);
  z[0]=300;
  CHECK(!band.sample(3200,true,z)); // first qualifying frame
  CHECK(band.setNearMm(450)); CHECK(!band.armed); CHECK_EQ(band.nearFrames,0);
  CHECK(!band.sample(3300,true,z)); CHECK(!band.sample(3400,true,z));
  CHECK(band.setClearMm(650)); CHECK_EQ(band.clearMm,650);
  CHECK(!band.setRange(50,700,600)); CHECK_EQ(band.nearMm,450);
  CHECK(band.setRange(50,400,600));

  const uint8_t id[3]={0xF4,3,0x48};
  const char *valid[]={"!TF40348:60\n","!TF40348:0\n","!TF40348:120\n","!JF40348:1\n",
    "!LF40348:30\n","!LF40348:50\n","!LF40348:699\n",
    "!UF40348:400\n","!VF40348:600\n","!VF40348:4000\n"};
  for (const char *s:valid) {
    SerialMutationParser parser; SerialMutationCommand cmd={};
    unsigned ready=0;
    for (;*s;++s) if (parser.feed(*s,id,cmd)==SERIAL_MUTATION_READY) ++ready;
    CHECK_EQ(ready,1u);
  }
  const char *invalid[]={"!TF40348:121\n","!TF40349:60\n","!JF40348:0\n","!JF40348:2\n","!TF40348:060\n","!JF40348:1!JF40348:1\n",
    "!LF40348:0\n","!LF40348:29\n","!LF40348:4001\n","!LF40348:050\n","!LF40349:50\n",
    "!UF40348:29\n","!VF40348:4001\n","!UF40349:400\n","!VF40348:0600\n"};
  for (const char *s:invalid) {
    SerialMutationParser parser; SerialMutationCommand cmd={};
    for (;*s;++s) CHECK(parser.feed(*s,id,cmd)!=SERIAL_MUTATION_READY);
  }
  const char *rangeCommands[]={"!LF40348:50\n","!UF40348:400\n","!VF40348:600\n"};
  const uint16_t rangeValues[]={50,400,600};
  for (unsigned i=0;i<3;++i) {
    SerialMutationParser parser; SerialMutationCommand cmd={};
    for (const char *s=rangeCommands[i];*s;++s) {
      if (parser.feed(*s,id,cmd)==SERIAL_MUTATION_READY) {
        CHECK_EQ(cmd.type,SERIAL_MUTATION_TRIGGER_MIN+i); CHECK_EQ(cmd.value,rangeValues[i]);
      }
    }
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
