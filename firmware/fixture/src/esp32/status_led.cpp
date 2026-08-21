#include "status_led.h"

#include <Arduino.h>

#include "board_power.h"
#include "boot_park.h"

// Locate pattern "..-" (dot dot dash + gap), unmistakable vs the battery blinks.
struct LedStep { uint16_t ms; bool on; };
static const LedStep IDENT_PAT[] = {
    {150, true}, {150, false}, {150, true}, {150, false}, {450, true}, {700, false}};
static const uint8_t IDENT_N = 6;

static uint32_t gIdentifyUntil = 0;

void statusLedInit() {
  pinMode(RES_STATUS_LED_PIN, OUTPUT);
  digitalWrite(RES_STATUS_LED_PIN, LOW);
}

void statusLedIdentify(uint8_t secs) {
  gIdentifyUntil = millis() + (uint32_t)secs * 1000UL;
}

// Battery-level display: >50% solid | 25-50% 1 Hz | 10-24% 2 Hz | <10% 4 Hz |
// no reading off. The donor's voltage cross-check floor was Li-ion-calibrated;
// this is the LFP table. Two LFP realities baked in (ADR 0023):
//   - the discharge curve is flat: voltage barely separates 30..90%, so the
//     floor only distinguishes healthy / low / critical bands;
//   - RepSOC parks at 1% from ~60% delivered onward, so a *low* gauge reading
//     must not be trusted below what the terminal voltage allows (the floor
//     RAISES a false-low gauge; a high gauge stands on its own).
static int lfpVoltageFloor(float v) {
  if (v >= 3.35f) return 60; // charged/plateau: never blink scary
  if (v >= 3.20f) return 30; // plateau: at least "mid"
  if (v >= 3.15f) return 12; // ADR 0046 dim: at least "low", not critical
  return 0;                  // below dim threshold: trust the panic blinks
}

void statusLedTick() {
  uint32_t now = millis();
  if (now < gIdentifyUntil) { // locate beacon overrides battery display
    static uint32_t t0 = 0;
    static uint8_t i = 0;
    if (now - t0 >= IDENT_PAT[i].ms) { i = (i + 1) % IDENT_N; t0 = now; }
    digitalWrite(RES_STATUS_LED_PIN, IDENT_PAT[i].on ? HIGH : LOW);
    return;
  }
  static uint32_t last = 0;
  static bool on = false;
  int soc = batterySocPct();
  float v = batteryVolts();
  if (soc < 0 && v < 2.5f) { digitalWrite(RES_STATUS_LED_PIN, LOW); return; }
  int vfloor = lfpVoltageFloor(v);
  int eff = (soc < 0) ? vfloor : (soc > vfloor ? soc : vfloor);
  if (eff > 50) { digitalWrite(RES_STATUS_LED_PIN, HIGH); return; }
  uint16_t interval = (eff >= 25) ? 500 : (eff >= 10) ? 250 : 125; // 1/2/4 Hz
  if (now - last >= interval) {
    last = now;
    on = !on;
    digitalWrite(RES_STATUS_LED_PIN, on ? HIGH : LOW);
  }
}
