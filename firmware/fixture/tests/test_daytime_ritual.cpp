#include "test_util.h"

#include "../src/core/daytime_ritual.h"

static DaytimeRitualInputs baseInput() {
  DaytimeRitualInputs in = {};
  in.enabled = true;
  in.scheduledDay = true;
  in.energyReady = true;
  in.authorityFree = true;
  in.utcValid = true;
  in.utcS = 100UL * 3600UL;
  in.uncertaintyMs = 100;
  in.fixtureId[0] = 0x12;
  in.fixtureId[1] = 0x34;
  in.fixtureId[2] = 0x56;
  return in;
}

int main() {
  DaytimeRitualState state;
  daytimeRitualInit(state);
  DaytimeRitualInputs in = baseInput();

  // The receiver is held from T-20 s through a hard T+47 s boundary.
  in.utcS = 99UL * 3600UL + 3580UL;
  DaytimeRitualOutputs out = daytimeRitualTick(state, in);
  CHECK(out.keepAwake);
  CHECK(!out.strikeRequested);
  in.utcS = 100UL * 3600UL + 47UL;
  out = daytimeRitualTick(state, in);
  CHECK(out.keepAwake);
  in.utcS += 1;
  out = daytimeRitualTick(state, in);
  CHECK(!out.keepAwake);

  // High-quality UTC produces one unison attempt at T+5 s. Repeated loop
  // ticks and an in-window reset ledger cannot duplicate the event.
  in = baseInput();
  in.utcS += 5;
  out = daytimeRitualTick(state, in);
  CHECK(out.strikeRequested);
  CHECK_EQ(out.event, (uint8_t)DAYTIME_RITUAL_UNISON);
  out = daytimeRitualTick(state, in);
  CHECK(!out.strikeRequested);

  // RTC-grade uncertainty intentionally suppresses the falsely synchronized
  // opening, while keeping the organic portion eligible.
  daytimeRitualInit(state);
  in = baseInput();
  in.utcS += 5;
  in.uncertaintyMs = 2000;
  out = daytimeRitualTick(state, in);
  CHECK(!out.strikeRequested);
  CHECK(out.keepAwake);

  // Every fixture gets exactly one deterministic roll slot in 12.0..35.5 s.
  // Find this fixture's slot without exposing the implementation hash.
  bool sawRoll = false;
  for (uint32_t ms = 12000; ms <= 35500; ms += 500) {
    in.utcS = 100UL * 3600UL + ms / 1000UL;
    in.subMs = (uint16_t)(ms % 1000UL);
    out = daytimeRitualTick(state, in);
    if (out.strikeRequested) {
      CHECK_EQ(out.event, (uint8_t)DAYTIME_RITUAL_ROLL);
      sawRoll = true;
    }
  }
  CHECK(sawRoll);

  // A canary is locked to one exact UTC hour. It holds the named pre-roll and
  // may strike in that hour, but the identical wall-clock phase in either
  // adjacent hour is inert.
  daytimeRitualInit(state);
  in = baseInput();
  in.allowedHourKey = 100;
  in.utcS = 99UL * 3600UL + 3580UL;
  CHECK(daytimeRitualTick(state, in).keepAwake);
  in.utcS = 100UL * 3600UL + 5UL;
  CHECK(daytimeRitualTick(state, in).strikeRequested);
  in.utcS = 101UL * 3600UL + 5UL;
  out = daytimeRitualTick(state, in);
  CHECK(!out.keepAwake);
  CHECK(!out.strikeRequested);

  // Energy, schedule, authority, and time are all independent vetoes.
  DaytimeRitualInputs veto = baseInput();
  veto.utcS += 5;
  veto.energyReady = false;
  CHECK(!daytimeRitualTick(state, veto).keepAwake);
  veto = baseInput(); veto.utcS += 5; veto.scheduledDay = false;
  CHECK(!daytimeRitualTick(state, veto).strikeRequested);
  veto = baseInput(); veto.utcS += 5; veto.authorityFree = false;
  CHECK(!daytimeRitualTick(state, veto).strikeRequested);
  veto = baseInput(); veto.utcS += 5; veto.utcValid = false;
  CHECK(!daytimeRitualTick(state, veto).strikeRequested);

  // Ordinary five-minute sleep remains unchanged except for the final sleep,
  // which lands exactly at T-20 s. Fractional seconds round up, never early.
  CHECK_EQ(daytimeRitualSleepS(100UL * 3600UL + 3000UL, 0, 300), 300u);
  CHECK_EQ(daytimeRitualSleepS(100UL * 3600UL + 3570UL, 0, 300), 10u);
  CHECK_EQ(daytimeRitualSleepS(100UL * 3600UL + 3569UL, 500, 300), 11u);

  // A future canary hour uses the normal cadence until its final sleep, lands
  // on T-20, and never aligns to a second ritual after the named hour passes.
  CHECK_EQ(daytimeRitualSleepSForHour(100UL * 3600UL + 3000UL, 0, 300,
                                      102UL),
           300u);
  CHECK_EQ(daytimeRitualSleepSForHour(101UL * 3600UL + 3570UL, 0, 300,
                                      102UL),
           10u);
  CHECK_EQ(daytimeRitualSleepSForHour(102UL * 3600UL + 48UL, 0, 300,
                                      102UL),
           300u);

  DaytimeRitualInputs expectedInput = baseInput();
  uint8_t expected = daytimeRitualExpectedMask(expectedInput.fixtureId);
  CHECK((expected & DAYTIME_RITUAL_MASK_UNISON) != 0);
  CHECK((expected & DAYTIME_RITUAL_MASK_ROLL) != 0);
  CHECK_EQ(daytimeRitualEventMask(DAYTIME_RITUAL_UNISON),
           (uint8_t)DAYTIME_RITUAL_MASK_UNISON);
  CHECK_EQ(daytimeRitualEventMask(DAYTIME_RITUAL_ROLL),
           (uint8_t)DAYTIME_RITUAL_MASK_ROLL);
  CHECK_EQ(daytimeRitualEventMask(DAYTIME_RITUAL_AFTER),
           (uint8_t)DAYTIME_RITUAL_MASK_AFTER);

  return testReport("test_daytime_ritual");
}
